from __future__ import annotations

import json
from typing import Any, Dict, List, Optional

import paho.mqtt.client as mqtt

from simulators.config import SimulatorConfig
from simulators.generator import ReadingGenerator


class MqttPublisher:
    def __init__(
        self,
        config: SimulatorConfig,
        generator: Optional[ReadingGenerator] = None,
        client: Optional[mqtt.Client] = None,
    ):
        self._config = config
        self._generator = generator or ReadingGenerator(config)
        self._client = client or mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    def connect(self) -> None:
        self._client.connect(self._config.broker_host, self._config.broker_port)
        self._client.loop_start()

    def disconnect(self) -> None:
        self._client.loop_stop()
        self._client.disconnect()

    def publish_batch(self, readings: List[Dict[str, Any]]) -> None:
        for r in readings:
            topic = self._generator.build_topic(r)
            payload = json.dumps(r)
            self._client.publish(topic, payload)

    def run_once(self) -> None:
        readings = self._generator.generate()
        self.publish_batch(readings)
