import os
import pytest

from simulators.config import load_config, SimulatorConfig


class TestLoadConfig:
    def test_loads_valid_config(self, valid_config_path):
        cfg = load_config(valid_config_path)
        assert isinstance(cfg, SimulatorConfig)
        assert cfg.publish_interval_sec == 1.0
        assert cfg.anomaly_injection_rate == 0.02

    def testLoadsMinimalConfig(self, minimal_config_path):
        cfg = load_config(minimal_config_path)
        assert len(cfg.devices) == 1
        assert cfg.devices[0].id == "sensor-a"

    def test_has_two_devices(self, valid_config_path):
        cfg = load_config(valid_config_path)
        assert len(cfg.devices) == 2

    def test_raises_on_missing_file(self):
        with pytest.raises(FileNotFoundError):
            load_config("/nonexistent/config.yaml")

    def test_raises_on_empty_devices(self):
        path = os.path.join(
            os.path.dirname(__file__), "fixtures", "empty_devices_config.yaml"
        )
        with pytest.raises(ValueError, match="no devices"):
            load_config(path)

    def test_broker_settings(self, valid_config_path):
        cfg = load_config(valid_config_path)
        assert cfg.broker_host == "localhost"
        assert cfg.broker_port == 1883
        assert cfg.topic_prefix == "factory/devices"


class TestDeviceConfig:
    def test_device_has_required_fields(self, valid_config_path):
        cfg = load_config(valid_config_path)
        dev = cfg.devices[0]
        assert dev.id == "pump-001"
        assert dev.name == "Main Coolant Pump"
        assert len(dev.sensors) == 1

    def test_sensor_bounds(self, valid_config_path):
        cfg = load_config(valid_config_path)
        sensor = cfg.devices[0].sensors[0]
        assert sensor.min == 20.0
        assert sensor.max == 95.0
        assert sensor.normal_mean == 65.0
        assert sensor.normal_std == 3.0
