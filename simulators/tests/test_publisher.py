import json
import pytest
from unittest.mock import MagicMock, patch

from simulators.config import load_config
from simulators.generator import ReadingGenerator
from simulators.publisher import MqttPublisher


class TestMqttPublisher:
    def test_publishes_json_payload(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        readings = gen.generate()

        mock_client = MagicMock()
        pub = MqttPublisher(cfg, client=mock_client)
        pub.publish_batch(readings)

        assert mock_client.publish.call_count == len(readings)

    def test_payload_is_valid_json(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        readings = gen.generate()

        mock_client = MagicMock()
        pub = MqttPublisher(cfg, client=mock_client)
        pub.publish_batch(readings)

        for call in mock_client.publish.call_args_list:
            topic, payload = call[0]
            parsed = json.loads(payload)
            assert "device_id" in parsed
            assert "sensor" in parsed
            assert "value" in parsed
            assert "timestamp" in parsed

    def test_topic_matches_config_prefix(self, valid_config_path):
        cfg = load_config(valid_config_path)
        gen = ReadingGenerator(cfg)
        readings = gen.generate()

        mock_client = MagicMock()
        pub = MqttPublisher(cfg, client=mock_client)
        pub.publish_batch(readings)

        for call in mock_client.publish.call_args_list:
            topic, _ = call[0]
            assert topic.startswith("factory/devices/")

    def test_connects_to_broker(self, valid_config_path):
        cfg = load_config(valid_config_path)
        mock_client = MagicMock()
        pub = MqttPublisher(cfg, client=mock_client)
        pub.connect()
        mock_client.connect.assert_called_once_with("localhost", 1883)

    def test_disconnects_cleanly(self, valid_config_path):
        cfg = load_config(valid_config_path)
        mock_client = MagicMock()
        pub = MqttPublisher(cfg, client=mock_client)
        pub.disconnect()
        mock_client.disconnect.assert_called_once()
