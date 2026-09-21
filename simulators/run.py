#!/usr/bin/env python3
"""Run the device simulator: publish synthetic readings to MQTT in a loop."""

import argparse
import signal
import sys
import time

from simulators.config import SimulatorConfig, load_config
from simulators.generator import ReadingGenerator
from simulators.publisher import MqttPublisher
from simulators.replay import ReplayGenerator


def make_generator(
    cfg: SimulatorConfig,
    replay: str | None = None,
    replay_file: str | None = None,
    replay_labels: str | None = None,
    loop: bool = True,
):
    """Synthetic generator, or a C-MAPSS/SECOM replay when `replay` is set."""
    if not replay:
        return ReadingGenerator(cfg)
    if not replay_file:
        raise ValueError("--replay requires --replay-file")
    if replay == "cmapss":
        return ReplayGenerator.from_cmapss(cfg, replay_file, loop=loop)
    if replay == "secom":
        return ReplayGenerator.from_secom(cfg, replay_file, replay_labels, loop=loop)
    raise ValueError(f"unknown replay format: {replay}")


def main() -> None:
    parser = argparse.ArgumentParser(description="ForgeSight device simulator")
    parser.add_argument(
        "-c", "--config", default="config.yaml", help="Path to config YAML"
    )
    parser.add_argument(
        "--replay",
        choices=("cmapss", "secom"),
        help="Replay a C-MAPSS or SECOM trace instead of synthetic noise",
    )
    parser.add_argument(
        "--replay-file",
        help="Path to the trace file (train_FD001.txt or secom.data)",
    )
    parser.add_argument(
        "--replay-labels",
        help="Optional SECOM labels file (fail=1 is marked anomaly)",
    )
    parser.add_argument(
        "--no-replay-loop",
        action="store_true",
        help="Hold the last sample when the trace ends (default: loop)",
    )
    args = parser.parse_args()

    cfg = load_config(args.config)
    try:
        gen = make_generator(
            cfg,
            replay=args.replay,
            replay_file=args.replay_file,
            replay_labels=args.replay_labels,
            loop=not args.no_replay_loop,
        )
    except ValueError as exc:
        parser.error(str(exc))
    pub = MqttPublisher(cfg, generator=gen)

    running = True

    def _stop(signum, _frame):
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, _stop)
    signal.signal(signal.SIGTERM, _stop)

    pub.connect()

    mode = f"replay={args.replay} file={args.replay_file}" if args.replay else "synthetic"
    print(
        f"Simulating {len(cfg.devices)} device(s) ({mode}) "
        f"every {cfg.publish_interval_sec}s on "
        f"{cfg.broker_host}:{cfg.broker_port}",
        flush=True,
    )

    try:
        while running:
            pub.run_once()
            time.sleep(cfg.publish_interval_sec)
    finally:
        pub.disconnect()
        print("Disconnected.", flush=True)


if __name__ == "__main__":
    main()
