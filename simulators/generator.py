from __future__ import annotations

import random
from datetime import datetime, timezone
from typing import Any, Dict, List, Optional

from simulators.config import SimulatorConfig


class ReadingGenerator:
    def __init__(self, config: SimulatorConfig, seed: Optional[int] = None):
        self._config = config
        self._rng = random.Random(seed)

    def generate(self) -> List[Dict[str, Any]]:
        now = datetime.now(timezone.utc).isoformat()
        readings: List[Dict[str, Any]] = []
        for device in self._config.devices:
            for sensor in device.sensors:
                value = self._rng.gauss(sensor.normal_mean, sensor.normal_std)
                anomaly = False

                if self._rng.random() < self._config.anomaly_injection_rate:
                    anomaly = True
                    spike = self._rng.uniform(0.3, 0.8) * (sensor.max - sensor.normal_mean)
                    value = sensor.normal_mean + spike

                value = max(sensor.min, min(sensor.max, round(value, 2)))

                readings.append(
                    {
                        "device_id": device.id,
                        "sensor": sensor.name,
                        "value": value,
                        "unit": sensor.unit,
                        "timestamp": now,
                        "anomaly": anomaly,
                    }
                )
        return readings

    def build_topic(self, reading: Dict[str, Any]) -> str:
        return f"{self._config.topic_prefix}/{reading['device_id']}/{reading['sensor']}"
