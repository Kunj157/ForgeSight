"""Helpers for scripts/load_test.py: synthetic device generation and
lightweight /proc-based CPU sampling, kept dependency-free (no psutil) so the
load test only needs the same paho-mqtt/PyYAML deps as the regular
simulator.
"""

from __future__ import annotations

from typing import Dict, List, Tuple

_SENSOR_TEMPLATE: List[Dict] = [
    {"name": "temperature", "unit": "°C", "min": 20.0, "max": 95.0, "normal_mean": 65.0, "normal_std": 3.0},
    {"name": "vibration", "unit": "mm/s", "min": 0.0, "max": 20.0, "normal_mean": 4.5, "normal_std": 1.0},
    {"name": "pressure", "unit": "bar", "min": 0.5, "max": 5.0, "normal_mean": 2.8, "normal_std": 0.2},
]


def build_device(index: int) -> Dict:
    """Build one synthetic load-test device, id `load-XXXX`.

    Device ids use a distinct 'load-' prefix from the seeded demo devices
    (pump-001, compressor-001, ...) so a load test run can share a broker
    and database with an already-running dev stack without colliding.
    """
    return {
        "id": f"load-{index:04d}",
        "name": f"Load Test Device {index}",
        "plant": "Load Test",
        "floor": f"Floor {index % 4 + 1}",
        "sensors": [dict(sensor) for sensor in _SENSOR_TEMPLATE],
    }


def build_config(
    n_devices: int,
    publish_interval_sec: float = 1.0,
    broker_host: str = "127.0.0.1",
    broker_port: int = 1883,
    topic_prefix: str = "factory/devices",
) -> Dict:
    """Build a simulators/config.yaml-shaped dict for `n_devices` synthetic
    devices, suitable for `yaml.safe_dump`-ing to a temp file and passing to
    `simulators.run`."""
    if n_devices < 1:
        raise ValueError("n_devices must be >= 1")

    return {
        "devices": [build_device(i) for i in range(1, n_devices + 1)],
        "publish_interval_sec": publish_interval_sec,
        "anomaly_injection_rate": 0.02,
        "broker": {
            "host": broker_host,
            "port": broker_port,
            "topic_prefix": topic_prefix,
        },
    }


def parse_proc_stat(line: str) -> Tuple[int, int]:
    """Parse (utime, stime) — fields 14 and 15 — out of a /proc/<pid>/stat
    line.

    The comm field (2nd field, in parens) may itself contain spaces or
    parens, so we anchor on the *last* ')' rather than naively splitting on
    whitespace from the start of the line (this is the same trick the
    kernel's own proc(5) docs recommend, and what psutil does internally).
    """
    close = line.rindex(")")
    fields = line[close + 1 :].split()
    # After the comm field, `fields[0]` is field #3 (state); utime is field
    # #14 -> fields[11], stime is field #15 -> fields[12].
    utime = int(fields[11])
    stime = int(fields[12])
    return utime, stime


def cpu_percent(
    prev: Tuple[int, int],
    curr: Tuple[int, int],
    elapsed_wall_s: float,
    clk_tck: int = 100,
) -> float:
    """CPU usage over [prev, curr] as a percentage of one core (100% == one
    fully busy core; can exceed 100% for a multi-threaded process)."""
    if elapsed_wall_s <= 0:
        return 0.0
    delta_ticks = (curr[0] + curr[1]) - (prev[0] + prev[1])
    return max(0.0, delta_ticks / clk_tck / elapsed_wall_s * 100.0)
