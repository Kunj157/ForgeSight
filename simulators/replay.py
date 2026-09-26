"""Replay NASA C-MAPSS / UCI SECOM traces as ForgeSight MQTT readings.

C-MAPSS rows are 26 space-separated columns (unit, cycle, 3 settings, 21
sensors), no header. SECOM is space-separated features, optionally paired
with a labels file whose first token is 1 (fail) or -1 (pass).

A documented subset of columns is mapped onto the configured sensor names
and min-max scaled into each sensor's [min, max]. Datasets themselves are
fetched into gitignored data/ — this module only parses whatever path it
is given.
"""

from __future__ import annotations

from datetime import datetime, timezone
from typing import Any, Dict, List, Optional

from simulators.config import DeviceConfig, SimulatorConfig

# 0-based column indices in a 26-col C-MAPSS row.
# Names match the simulator's configured sensors so a device only receives
# columns it actually has.
CMAPSS_COLUMNS: Dict[str, int] = {
    "temperature": 6,  # T24 LPC outlet temperature
    "pressure": 11,  # P30 HPC outlet pressure
    "vibration": 8,  # T50 as a wear-correlated proxy
    "flow": 16,  # phi (fuel flow equivalent)
    "current": 20,  # W31 coolant bleed — mapped to electrical load
}

# SECOM has 590 anonymous process features; we take the first N columns in
# a stable name order so a 5-col fixture and the real file both work.
SECOM_COLUMNS: Dict[str, int] = {
    "temperature": 0,
    "pressure": 1,
    "vibration": 2,
    "flow": 3,
    "current": 4,
}

CMAPSS_WIDTH = 26


def scale_series(values: List[float], dest_min: float, dest_max: float) -> List[float]:
    """Min-max scale `values` onto [dest_min, dest_max].

    A constant series has no range to stretch, so every point lands on the
    midpoint of the destination interval rather than collapsing to dest_min.
    """
    if not values:
        return []
    dest_span = dest_max - dest_min
    mid = dest_min + dest_span / 2.0
    # SECOM ships literal "NaN" tokens; treat them as missing so a single
    # bad cell doesn't collapse the whole series.
    finite = [v for v in values if v == v]
    if not finite:
        return [mid for _ in values]
    src_min = min(finite)
    src_max = max(finite)
    if src_max == src_min:
        return [mid for _ in values]
    src_span = src_max - src_min
    return [
        mid if v != v else dest_min + (v - src_min) / src_span * dest_span for v in values
    ]


def parse_cmapss(path: str) -> Dict[int, List[List[float]]]:
    """Group C-MAPSS rows by unit id. Each row is 26 floats."""
    units: Dict[int, List[List[float]]] = {}
    with open(path) as fh:
        for lineno, raw in enumerate(fh, start=1):
            line = raw.strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) != CMAPSS_WIDTH:
                raise ValueError(
                    f"{path}:{lineno}: expected {CMAPSS_WIDTH} columns, got {len(parts)}"
                )
            row = [float(p) for p in parts]
            unit = int(row[0])
            units.setdefault(unit, []).append(row)
    if not units:
        raise ValueError(f"{path}: no C-MAPSS rows")
    return units


def parse_secom(
    path: str, labels_path: Optional[str] = None
) -> List[Dict[str, Any]]:
    """Parse SECOM feature rows; optional labels file marks fail (1) as anomaly."""
    labels: List[bool] = []
    if labels_path:
        with open(labels_path) as fh:
            for raw in fh:
                line = raw.strip()
                if not line:
                    continue
                token = line.split()[0]
                labels.append(token == "1")

    rows: List[Dict[str, Any]] = []
    with open(path) as fh:
        for raw in fh:
            line = raw.strip()
            if not line:
                continue
            features = [float(p) for p in line.split()]
            idx = len(rows)
            anomaly = labels[idx] if idx < len(labels) else False
            rows.append({"features": features, "anomaly": anomaly})
    if not rows:
        raise ValueError(f"{path}: no SECOM rows")
    return rows


class ReplayGenerator:
    """Drop-in stand-in for ReadingGenerator that walks a precomputed series."""

    def __init__(
        self,
        config: SimulatorConfig,
        series: Dict[str, List[Dict[str, Any]]],
        loop: bool = True,
    ):
        self._config = config
        self._series = series
        self._loop = loop
        self._cursors = {device_id: 0 for device_id in series}

    def generate(self) -> List[Dict[str, Any]]:
        now = datetime.now(timezone.utc).isoformat()
        readings: List[Dict[str, Any]] = []
        for device in self._config.devices:
            steps = self._series.get(device.id)
            if not steps:
                continue
            idx = self._cursors[device.id]
            if idx >= len(steps):
                if not self._loop:
                    idx = len(steps) - 1
                else:
                    idx = 0
                    self._cursors[device.id] = 0
            step = steps[idx]
            self._cursors[device.id] = idx + 1
            for sensor in device.sensors:
                sample = step.get(sensor.name)
                if sample is None:
                    continue
                readings.append(
                    {
                        "device_id": device.id,
                        "sensor": sensor.name,
                        "value": round(sample["value"], 2),
                        "unit": sensor.unit,
                        "timestamp": now,
                        "anomaly": bool(sample["anomaly"]),
                    }
                )
        return readings

    def build_topic(self, reading: Dict[str, Any]) -> str:
        return f"{self._config.topic_prefix}/{reading['device_id']}/{reading['sensor']}"

    @classmethod
    def from_cmapss(
        cls,
        config: SimulatorConfig,
        path: str,
        loop: bool = True,
        anomaly_tail: float = 0.2,
    ) -> "ReplayGenerator":
        units = parse_cmapss(path)
        unit_ids = sorted(units)
        series: Dict[str, List[Dict[str, Any]]] = {}
        for device, unit_id in zip(config.devices, unit_ids):
            series[device.id] = _scale_cmapss_unit(
                device, units[unit_id], anomaly_tail
            )
        return cls(config, series, loop=loop)

    @classmethod
    def from_secom(
        cls,
        config: SimulatorConfig,
        path: str,
        labels_path: Optional[str] = None,
        loop: bool = True,
    ) -> "ReplayGenerator":
        rows = parse_secom(path, labels_path)
        # Every configured device walks the same SECOM wafer/trace; the
        # process file is not per-unit the way C-MAPSS is.
        built = _scale_secom_rows(config.devices, rows)
        series = {device.id: built[device.id] for device in config.devices}
        return cls(config, series, loop=loop)


def _scale_cmapss_unit(
    device: DeviceConfig, rows: List[List[float]], anomaly_tail: float
) -> List[Dict[str, Any]]:
    n = len(rows)
    # Round so a 3-cycle unit with tail=0.4 marks only the last cycle, not
    # the last two (int(3*0.6) would have started the tail one cycle early).
    tail_count = max(1, int(round(n * anomaly_tail))) if n and anomaly_tail > 0 else 0
    tail_start = n - tail_count
    raw: Dict[str, List[float]] = {}
    for sensor in device.sensors:
        col = CMAPSS_COLUMNS.get(sensor.name)
        if col is None:
            continue
        raw[sensor.name] = [row[col] for row in rows]

    scaled = {
        name: scale_series(
            values,
            dest_min=next(s.min for s in device.sensors if s.name == name),
            dest_max=next(s.max for s in device.sensors if s.name == name),
        )
        for name, values in raw.items()
    }

    steps: List[Dict[str, Any]] = []
    for i in range(n):
        step: Dict[str, Any] = {}
        anomaly = i >= tail_start
        for name, values in scaled.items():
            step[name] = {"value": values[i], "anomaly": anomaly}
        steps.append(step)
    return steps


def _scale_secom_rows(
    devices: List[DeviceConfig], rows: List[Dict[str, Any]]
) -> Dict[str, List[Dict[str, Any]]]:
    out: Dict[str, List[Dict[str, Any]]] = {}
    for device in devices:
        raw: Dict[str, List[float]] = {}
        for sensor in device.sensors:
            col = SECOM_COLUMNS.get(sensor.name)
            if col is None:
                continue
            values = []
            for row in rows:
                features = row["features"]
                if col >= len(features):
                    continue
                values.append(features[col])
            if values:
                raw[sensor.name] = values

        scaled = {
            name: scale_series(
                values,
                dest_min=next(s.min for s in device.sensors if s.name == name),
                dest_max=next(s.max for s in device.sensors if s.name == name),
            )
            for name, values in raw.items()
        }
        steps: List[Dict[str, Any]] = []
        for i, row in enumerate(rows):
            step: Dict[str, Any] = {}
            for name, values in scaled.items():
                if i < len(values):
                    step[name] = {"value": values[i], "anomaly": row["anomaly"]}
            steps.append(step)
        out[device.id] = steps
    return out
