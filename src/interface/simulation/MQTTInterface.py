import logging
from typing import Any, Callable, Optional

import paho.mqtt.client as mqtt
from paho.mqtt.client import CallbackAPIVersion


logger = logging.getLogger(__name__)


class MQTTInterface:
    """Interface simples para enviar e receber dados via MQTT na simulação."""

    def __init__(
        self,
        broker_host: str = "localhost",
        broker_port: int = 1883,
        actuator_topic: str = "atr/sim/esforco_controle",
        lidar_topic: str = "atr/sim/lidar",
        encoder_topic: str = "atr/sim/encoder",
        actuator_callback: Optional[Callable[[dict], Any]] = None,
    ) -> None:
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.actuator_topic = actuator_topic
        self.lidar_topic = lidar_topic
        self.encoder_topic = encoder_topic
        self.actuator_callback = actuator_callback or self.default_actuator_callback

        self.client = mqtt.Client(CallbackAPIVersion.VERSION1)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

        self.last_actuator_message: Optional[dict] = None

    def _on_connect(
        self, client: mqtt.Client, userdata: Any, flags: dict, rc: int
    ) -> None:
        if rc == 0:
            logger.info(
                "Conectado ao broker MQTT %s:%s", self.broker_host, self.broker_port
            )
            self.client.subscribe(self.actuator_topic)
            logger.info("Inscrito em %s", self.actuator_topic)
        else:
            logger.error("Falha ao conectar no broker MQTT: código %s", rc)

    def _on_message(
        self, client: mqtt.Client, userdata: Any, msg: mqtt.MQTTMessage
    ) -> None:
        try:
            payload = msg.payload.decode("utf-8")
            data = float(payload)
        except Exception as exc:
            logger.error("Erro ao decodificar mensagem MQTT em %s: %s", msg.topic, exc)
            return

        if msg.topic == self.actuator_topic:
            logger.debug("Recebido comando do atuador: %s", data)
            self.last_actuator_message = data
            self.actuator_callback(data)
        else:
            logger.warning("Mensagem recebida em tópico inesperado: %s", msg.topic)

    def default_actuator_callback(self, data: dict) -> None:
        logger.info(
            "Comando de atuador recebido, mas nenhum callback foi configurado: %s", data
        )

    def connect(self) -> None:
        self.client.connect(self.broker_host, self.broker_port)
        self.client.loop_start()

    def disconnect(self) -> None:
        self.client.loop_stop()
        self.client.disconnect()

    def publish_lidar(self, value: float) -> None:
        payload = str(value)
        self.client.publish(self.lidar_topic, payload)
        logger.debug("Publicado dado lidar em %s: %s", self.lidar_topic, payload)

    def publish_encoder(self, value: bool) -> None:
        payload = "1" if value else "0"
        self.client.publish(self.encoder_topic, payload)
        logger.debug("Publicado dado encoder em %s: %s", self.encoder_topic, payload)

    def publish_sensor_data(self, lidar_value: float, encoder_value: bool) -> None:
        self.publish_lidar(lidar_value)
        self.publish_encoder(encoder_value)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    def actuador_handler(message: dict) -> None:
        print("Atuador recebeu:", message)

    mqtt_interface = MQTTInterface(actuator_callback=atuador_handler)
    mqtt_interface.connect()

    try:
        while True:
            pass
    except KeyboardInterrupt:
        mqtt_interface.disconnect()
