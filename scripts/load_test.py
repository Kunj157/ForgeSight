#!/usr/bin/env python3
"""Load-test the ForgeSight ingestion pipeline.

Scales the device simulator up to N synthetic devices, runs it against an
already-running backend stack (Postgres + Mosquitto + ingestion +
alarm-engine + api — see scripts/dev-up.sh), then reports:

  - end-to-end ingestion latency (device reading timestamp -> `readings` row
    landing in Postgres), via Postgres's own clock so there's no
    cross-process clock-skew to worry about
  - CPU usage of the ingestion/alarm-engine/api processes during the run

Synthetic devices use a `load-XXXX` id prefix so they never collide with the
demo devices (pump-001, compressor-001, ...) a normal dev-up.sh run seeds,
and are cleaned out of the database when the run finishes.

Usage:
    PYTHONPATH=. python3 scripts/load_test.py --devices 50 --duration 30
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from simulators.loadgen import build_config, cpu_percent, parse_proc_stat  # noqa: E402

BACKEND_PROCESSES = ["ingestion", "alarm-engine", "api"]
# Ingestion flushes its DB write buffer every 500ms (see ingestion/main.cpp);
# give it a couple of cycles to drain after the simulator stops publishing.
FLUSH_GRACE_SEC = 2.0


@dataclass
class Sample:
    stats: Dict[str, Optional[tuple]]
    at: float


def die(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def find_pid(process_name: str) -> Optional[int]:
    result = subprocess.run(["pgrep", "-x", process_name], capture_output=True, text=True)
    if result.returncode != 0 or not result.stdout.strip():
        return None
    return int(result.stdout.strip().splitlines()[0])


def read_proc_stat(pid: int) -> Optional[tuple]:
    try:
        line = Path(f"/proc/{pid}/stat").read_text()
    except (FileNotFoundError, ProcessLookupError):
        return None
    return parse_proc_stat(line)


def check_prereqs(api_url: str, db_conn: str) -> None:
    if shutil.which("psql") is None:
        die("psql not found on PATH — needed to query latency stats")

    result = subprocess.run(["psql", db_conn, "-c", "select 1"], capture_output=True, text=True)
    if result.returncode != 0:
        die(
            f"cannot connect to Postgres with {db_conn!r}:\n{result.stderr}\n"
            "Is the backend stack running? See scripts/dev-up.sh."
        )

    import urllib.request

    try:
        urllib.request.urlopen(f"{api_url}/api/devices", timeout=3)
    except Exception as exc:  # noqa: BLE001 - want to report any failure reason
        die(f"API not reachable at {api_url}: {exc}\nIs scripts/dev-up.sh running?")

    missing = [name for name in BACKEND_PROCESSES if find_pid(name) is None]
    if missing:
        die(f"backend process(es) not found: {', '.join(missing)}. Run scripts/dev-up.sh first.")


def run_simulator(config_path: Path, duration_sec: float) -> None:
    repo_root = Path(__file__).resolve().parent.parent
    proc = subprocess.Popen(
        [sys.executable, "-m", "simulators.run", "-c", str(config_path)],
        cwd=repo_root,
        env={"PYTHONPATH": str(repo_root)},
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )

    pids = {name: find_pid(name) for name in BACKEND_PROCESSES}
    samples: List[Sample] = []
    start = time.monotonic()
    samples.append(Sample({n: read_proc_stat(p) if p else None for n, p in pids.items()}, start))

    while time.monotonic() - start < duration_sec:
        time.sleep(1.0)
        now = time.monotonic()
        samples.append(Sample({n: read_proc_stat(p) if p else None for n, p in pids.items()}, now))

    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()

    print_cpu_report(samples)


def print_cpu_report(samples: List[Sample]) -> None:
    print("\nBackend CPU usage (%% of one core) during the run:")
    print(f"  {'process':<14}{'avg':>8}{'peak':>8}")
    for name in BACKEND_PROCESSES:
        pcts = []
        for prev, curr in zip(samples, samples[1:]):
            p, c = prev.stats.get(name), curr.stats.get(name)
            if p is None or c is None:
                continue
            pcts.append(cpu_percent(p, c, curr.at - prev.at))
        if not pcts:
            print(f"  {name:<14}{'n/a':>8}{'n/a':>8}")
            continue
        avg = sum(pcts) / len(pcts)
        print(f"  {name:<14}{avg:>7.1f}%{max(pcts):>7.1f}%")


def report_latency(db_conn: str, since_iso: str) -> None:
    query = (
        "SELECT count(*), "
        "coalesce(avg(extract(epoch from (created_at - timestamp))), 0), "
        "coalesce(percentile_cont(0.5) within group (order by extract(epoch from (created_at - timestamp))), 0), "
        "coalesce(percentile_cont(0.95) within group (order by extract(epoch from (created_at - timestamp))), 0), "
        "coalesce(max(extract(epoch from (created_at - timestamp))), 0) "
        "FROM readings WHERE device_id LIKE 'load-%' AND created_at >= "
        f"'{since_iso}'::timestamptz"
    )
    result = subprocess.run(
        ["psql", db_conn, "-t", "-A", "-F,", "-c", query], capture_output=True, text=True
    )
    if result.returncode != 0:
        die(f"latency query failed:\n{result.stderr}")

    count, avg_s, p50_s, p95_s, max_s = result.stdout.strip().split(",")
    print("\nEnd-to-end ingestion latency (reading timestamp -> DB row):")
    print(f"  readings ingested: {count}")
    print(f"  avg:  {float(avg_s) * 1000:.1f} ms")
    print(f"  p50:  {float(p50_s) * 1000:.1f} ms")
    print(f"  p95:  {float(p95_s) * 1000:.1f} ms")
    print(f"  max:  {float(max_s) * 1000:.1f} ms")


def cleanup(db_conn: str) -> None:
    subprocess.run(
        ["psql", db_conn, "-c", "DELETE FROM readings WHERE device_id LIKE 'load-%'"],
        capture_output=True,
        text=True,
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--devices", type=int, default=50, help="number of synthetic devices (default: 50)")
    parser.add_argument("--duration", type=float, default=30.0, help="test duration in seconds (default: 30)")
    parser.add_argument(
        "--interval", type=float, default=1.0, help="publish interval per device in seconds (default: 1.0)"
    )
    parser.add_argument("--broker-host", default="127.0.0.1")
    parser.add_argument("--broker-port", type=int, default=1883)
    parser.add_argument("--api-url", default="http://127.0.0.1:8080")
    parser.add_argument("--db", default="dbname=forgesight", help="libpq connection string")
    parser.add_argument("--keep-data", action="store_true", help="skip deleting load-* rows when done")
    args = parser.parse_args()

    check_prereqs(args.api_url, args.db)

    sensors_per_device = 3
    print(
        f"Simulating {args.devices} device(s) x {sensors_per_device} sensors "
        f"every {args.interval}s for {args.duration}s "
        f"(~{args.devices * sensors_per_device / args.interval:.0f} msg/s)..."
    )

    cfg = build_config(
        args.devices,
        publish_interval_sec=args.interval,
        broker_host=args.broker_host,
        broker_port=args.broker_port,
    )

    start_iso = datetime.now(timezone.utc).isoformat()

    with tempfile.NamedTemporaryFile("w", suffix=".yaml", delete=False) as f:
        yaml.safe_dump(cfg, f)
        config_path = Path(f.name)

    try:
        run_simulator(config_path, args.duration)
        time.sleep(FLUSH_GRACE_SEC)
        report_latency(args.db, start_iso)
    finally:
        config_path.unlink(missing_ok=True)
        if not args.keep_data:
            cleanup(args.db)


if __name__ == "__main__":
    main()
