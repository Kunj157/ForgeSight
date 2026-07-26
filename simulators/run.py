#!/usr/bin/env python3
"""Run the device simulator: publish synthetic readings to MQTT in a loop."""

import argparse
import signal
import sys
import time

from simulators.config import load_config
from simulators.generator import ReadingGenerator
from simulators.publisher import MqttPublisher


def main() -> None:
    parser = argparse.ArgumentParser(description="ForgeSight device simulator")
    parser.add_argument(
        "-c", "--config", default="config.yaml", help="Path to config YAML"
    )
    args = parser.parse_args()

    cfg = load_config(args.config)
    gen = ReadingGenerator(cfg)
    pub = MqttPublisher(cfg, generator=gen)
    pub.connect()

    running = True

    def _stop(signum, _frame):
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, _stop)
    signal.signal(signal.SIGTERM, _stop)

    print(
        f"Simulating {len(cfg.devices)} device(s) "
        f"every {cfg.publish_interval_sec}s on "
        f"{cfg.broker_host}:{cfg.broker_port}"
    )

    try:
        while running:
            pub.run_once()
            time.sleep(cfg.publish_interval_sec)
    finally:
        pub.disconnect()
        print("Disconnected.")


if __name__ == "__main__":
    main()
