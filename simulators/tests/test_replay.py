import os

import pytest

from simulators.config import load_config
from simulators.generator import ReadingGenerator
from simulators.replay import (
    ReplayGenerator,
    parse_cmapss,
    parse_secom,
    scale_series,
)
from simulators.run import make_generator

FIXTURES = os.path.join(os.path.dirname(__file__), "fixtures")
CMAPSS_TINY = os.path.join(FIXTURES, "cmapss_tiny.txt")
SECOM_DATA = os.path.join(FIXTURES, "secom_tiny.data")
SECOM_LABELS = os.path.join(FIXTURES, "secom_tiny.labels")


class TestScaleSeries:
    def test_minmax_maps_endpoints(self):
        out = scale_series([10.0, 20.0], dest_min=0.0, dest_max=100.0)
        assert out == [0.0, 100.0]

    def test_constant_series_maps_to_midpoint(self):
        out = scale_series([5.0, 5.0, 5.0], dest_min=0.0, dest_max=10.0)
        assert out == [5.0, 5.0, 5.0]

    def test_empty_returns_empty(self):
        assert scale_series([], dest_min=0.0, dest_max=1.0) == []

    def test_nan_maps_to_midpoint(self):
        out = scale_series([0.0, float("nan"), 10.0], dest_min=0.0, dest_max=100.0)
        assert out[0] == 0.0
        assert out[1] == 50.0
        assert out[2] == 100.0


class TestParseCmapss:
    def test_groups_rows_by_unit(self):
        units = parse_cmapss(CMAPSS_TINY)
        assert set(units) == {1, 2}
        assert len(units[1]) == 3
        assert len(units[2]) == 2

    def test_row_has_26_columns(self):
        units = parse_cmapss(CMAPSS_TINY)
        for rows in units.values():
            for row in rows:
                assert len(row) == 26

    def test_cycle_column_is_monotonic_per_unit(self):
        units = parse_cmapss(CMAPSS_TINY)
        for rows in units.values():
            cycles = [row[1] for row in rows]
            assert cycles == sorted(cycles)

    def test_raises_on_short_row(self, tmp_path):
        bad = tmp_path / "bad.txt"
        bad.write_text("1 1 0 0 100\n")
        with pytest.raises(ValueError, match="26"):
            parse_cmapss(str(bad))


class TestParseSecom:
    def test_row_count_matches_file(self):
        rows = parse_secom(SECOM_DATA, SECOM_LABELS)
        assert len(rows) == 3

    def test_features_are_floats(self):
        rows = parse_secom(SECOM_DATA, SECOM_LABELS)
        assert rows[0]["features"][0] == 10.0
        assert rows[1]["features"][1] == 2.0

    def test_fail_label_is_anomaly(self):
        rows = parse_secom(SECOM_DATA, SECOM_LABELS)
        assert rows[0]["anomaly"] is False
        assert rows[1]["anomaly"] is False
        assert rows[2]["anomaly"] is True

    def test_without_labels_nothing_is_anomaly(self):
        rows = parse_secom(SECOM_DATA)
        assert all(not r["anomaly"] for r in rows)


class TestReplayGeneratorCmapss:
    def test_emits_schema_matching_synthetic_generator(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReplayGenerator.from_cmapss(cfg, CMAPSS_TINY)
        readings = gen.generate()
        assert readings
        for r in readings:
            assert {"device_id", "sensor", "value", "unit", "timestamp", "anomaly"} <= set(r)

    def test_values_stay_inside_configured_bounds(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReplayGenerator.from_cmapss(cfg, CMAPSS_TINY)
        for _ in range(20):
            for r in gen.generate():
                device = next(d for d in cfg.devices if d.id == r["device_id"])
                sensor = next(s for s in device.sensors if s.name == r["sensor"])
                assert sensor.min <= r["value"] <= sensor.max

    def test_only_emits_sensors_the_device_has(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReplayGenerator.from_cmapss(cfg, CMAPSS_TINY)
        readings = gen.generate()
        by_device = {}
        for r in readings:
            by_device.setdefault(r["device_id"], set()).add(r["sensor"])
        for device in cfg.devices:
            if device.id not in by_device:
                continue
            configured = {s.name for s in device.sensors}
            assert by_device[device.id] <= configured

    def test_late_life_cycles_are_anomalies(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReplayGenerator.from_cmapss(cfg, CMAPSS_TINY, anomaly_tail=0.4)
        # unit 1 (first device) has 3 cycles; last 40% → last cycle is anomaly.
        # Check only that device — unit 2 is shorter, so its "late life" lands
        # on a different tick.
        first_dev = cfg.devices[0].id
        first = [r for r in gen.generate() if r["device_id"] == first_dev]
        second = [r for r in gen.generate() if r["device_id"] == first_dev]
        third = [r for r in gen.generate() if r["device_id"] == first_dev]
        assert all(not r["anomaly"] for r in first)
        assert all(not r["anomaly"] for r in second)
        assert any(r["anomaly"] for r in third)

    def test_loops_when_exhausted(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReplayGenerator.from_cmapss(cfg, CMAPSS_TINY, loop=True)
        first = [(r["device_id"], r["sensor"], r["value"]) for r in gen.generate()]
        # unit 1 has 3 cycles, unit 2 has 2 — after 3 ticks unit 1 wraps
        gen.generate()
        gen.generate()
        wrapped = [(r["device_id"], r["sensor"], r["value"]) for r in gen.generate()]
        # first device (unit 1) should repeat its first cycle
        first_dev = cfg.devices[0].id
        first_vals = [v for d, s, v in first if d == first_dev]
        wrap_vals = [v for d, s, v in wrapped if d == first_dev]
        assert first_vals == wrap_vals


class TestReplayGeneratorSecom:
    def test_fail_rows_flagged_anomaly(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReplayGenerator.from_secom(cfg, SECOM_DATA, SECOM_LABELS)
        gen.generate()
        gen.generate()
        fail = gen.generate()
        assert any(r["anomaly"] for r in fail)

    def test_values_scaled_into_bounds(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReplayGenerator.from_secom(cfg, SECOM_DATA, SECOM_LABELS)
        for _ in range(10):
            for r in gen.generate():
                device = next(d for d in cfg.devices if d.id == r["device_id"])
                sensor = next(s for s in device.sensors if s.name == r["sensor"])
                assert sensor.min <= r["value"] <= sensor.max


class TestMakeGenerator:
    def test_default_is_synthetic(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = make_generator(cfg)
        assert isinstance(gen, ReadingGenerator)

    def test_cmapss_returns_replay_generator(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = make_generator(cfg, replay="cmapss", replay_file=CMAPSS_TINY)
        assert isinstance(gen, ReplayGenerator)
        assert gen.generate()

    def test_replay_without_file_raises(self, valid_config_path):
        cfg = load_config(valid_config_path)
        with pytest.raises(ValueError, match="replay-file"):
            make_generator(cfg, replay="cmapss")
