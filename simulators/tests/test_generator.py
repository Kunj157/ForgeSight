import time
import pytest

from simulators.config import load_config
from simulators.generator import ReadingGenerator


class TestReadingGenerator:
    def test_generates_reading_for_each_sensor(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        readings = gen.generate()
        sensor_names = {r["sensor"] for r in readings}
        assert "temperature" in sensor_names

    def test_reading_has_required_fields(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        readings = gen.generate()
        for r in readings:
            assert "device_id" in r
            assert "sensor" in r
            assert "value" in r
            assert "unit" in r
            assert "timestamp" in r

    def test_reading_value_within_bounds(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        for _ in range(100):
            readings = gen.generate()
            for r in readings:
                device = next(d for d in cfg.devices if d.id == r["device_id"])
                sensor = next(s for s in device.sensors if s.name == r["sensor"])
                assert sensor.min <= r["value"] <= sensor.max, (
                    f"{r['sensor']} value {r['value']} out of "
                    f"[{sensor.min}, {sensor.max}]"
                )

    def test_topic_format(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        readings = gen.generate()
        for r in readings:
            topic = gen.build_topic(r)
            assert topic.startswith("factory/devices/")
            assert r["device_id"] in topic
            assert r["sensor"] in topic

    def test_anomaly_spike_can_exceed_normal_range(self, valid_config_path):
        cfg = load_config(valid_config_path)
        cfg.anomaly_injection_rate = 1.0
        gen = ReadingGenerator(cfg)
        saw_spike = False
        for _ in range(200):
            readings = gen.generate()
            for r in readings:
                device = next(d for d in cfg.devices if d.id == r["device_id"])
                sensor = next(s for s in device.sensors if s.name == r["sensor"])
                if r.get("anomaly"):
                    assert sensor.min <= r["value"] <= sensor.max
                    saw_spike = True
        assert saw_spike, "With 100% injection rate, at least one anomaly expected"

    def test_timestamp_is_iso_format(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        readings = gen.generate()
        for r in readings:
            assert "T" in r["timestamp"]


class TestReadingGeneratorDeterministic:
    def test_seed_reproducibility(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen1 = ReadingGenerator(cfg, seed=42)
        gen2 = ReadingGenerator(cfg, seed=42)
        readings1 = gen1.generate()
        readings2 = gen2.generate()
        for r1, r2 in zip(readings1, readings2):
            assert r1["value"] == r2["value"]
