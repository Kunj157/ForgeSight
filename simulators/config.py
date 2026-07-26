from __future__ import annotations

import os
from dataclasses import dataclass, field
from typing import List

import yaml


@dataclass
class SensorConfig:
    name: str
    unit: str
    min: float
    max: float
    normal_mean: float
    normal_std: float


@dataclass
class DeviceConfig:
    id: str
    name: str
    sensors: List[SensorConfig] = field(default_factory=list)


@dataclass
class SimulatorConfig:
    devices: List[DeviceConfig] = field(default_factory=list)
    publish_interval_sec: float = 1.0
    anomaly_injection_rate: float = 0.02
    broker_host: str = "localhost"
    broker_port: int = 1883
    topic_prefix: str = "factory/devices"


def load_config(path: str) -> SimulatorConfig:
    if not os.path.isfile(path):
        raise FileNotFoundError(f"Config not found: {path}")

    with open(path) as f:
        raw = yaml.safe_load(f)

    devices: List[DeviceConfig] = []
    for d in raw.get("devices", []):
        sensors = [
            SensorConfig(
                name=s["name"],
                unit=s["unit"],
                min=s["min"],
                max=s["max"],
                normal_mean=s["normal_mean"],
                normal_std=s["normal_std"],
            )
            for s in d.get("sensors", [])
        ]
        devices.append(DeviceConfig(id=d["id"], name=d["name"], sensors=sensors))

    if not devices:
        raise ValueError("Config has no devices")

    broker = raw.get("broker", {})

    return SimulatorConfig(
        devices=devices,
        publish_interval_sec=raw.get("publish_interval_sec", 1.0),
        anomaly_injection_rate=raw.get("anomaly_injection_rate", 0.02),
        broker_host=broker.get("host", "localhost"),
        broker_port=broker.get("port", 1883),
        topic_prefix=broker.get("topic_prefix", "factory/devices"),
    )
