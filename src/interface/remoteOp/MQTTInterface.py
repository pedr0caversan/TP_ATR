import logging
import json
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
        data_colector_topic: str = "atr/telemetria/log",
        speed_topic: str = "atr/telemetria/velocidade",
        inspection_topic: str = "atr/telemetria/inspecao",
        setpoint_topic: str = "atr/cmd/velocidade",
        actuator_callback: Optional[Callable[[dict], Any]] = None,
    ) -> None:
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.data_colector_topic = data_colector_topic
        self.speed_topic = speed_topic
        self.inspection_topic = inspection_topic
        self.setpoint_topic = setpoint_topic
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
            self.client.subscribe(self.data_colector_topic)
            self.client.subscribe(self.speed_topic)
            self.client.subscribe(self.inspection_topic)
            logger.info("Inscrito em %s, %s, %s", self.data_colector_topic, self.speed_topic, self.inspection_topic)
        else:
            logger.error("Falha ao conectar no broker MQTT: código %s", rc)

    def _on_message(
        self, client: mqtt.Client, userdata: Any, msg: mqtt.MQTTMessage
    ) -> None:
        try:
            payload = msg.payload.decode("utf-8")
        except Exception as exc:
            logger.error("Erro ao decodificar payload MQTT em %s: %s", msg.topic, exc)
            return

        if msg.topic == self.data_colector_topic:
            try:
                payload_data = json.loads(payload)
            except Exception:
                try:
                    payload_data = float(payload)
                except Exception as exc:
                    logger.error("Erro ao decodificar mensagem MQTT em %s: %s", msg.topic, exc)
                    return

            message = {
                "topic": msg.topic,
                "type": "telemetria",
                "payload": payload_data,
            }
            logger.debug("Recebido dado do data colector: %s", message)
            self.last_actuator_message = message
            self.actuator_callback(message)

        elif msg.topic == self.speed_topic:
            try:
                payload_data = float(payload)
            except Exception as exc:
                logger.error("Erro ao decodificar mensagem MQTT em %s: %s", msg.topic, exc)
                return

            message = {
                "topic": msg.topic,
                "type": "velocidade",
                "payload": payload_data,
            }
            self.last_actuator_message = message
            self.actuator_callback(message)

        elif msg.topic == self.inspection_topic:
            try:
                payload_data = int(payload)
            except Exception as exc:
                logger.error("Erro ao decodificar mensagem MQTT em %s: %s", msg.topic, exc)
                return

            message = {
                "topic": msg.topic,
                "type": "inspecao",
                "payload": payload_data,
            }
            self.last_actuator_message = message
            self.actuator_callback(message)

        else:
            logger.warning("Mensagem recebida em tópico inesperado: %s", msg.topic)

    def default_data_colector_callback(self, data: dict) -> None:
        logger.info(
            "Comando de data colector recebido, mas nenhum callback foi configurado: %s", data
        )

    def connect(self) -> None:
        self.client.connect(self.broker_host, self.broker_port)
        self.client.loop_start()

    def disconnect(self) -> None:
        self.client.loop_stop()
        self.client.disconnect()

    def publish_setpoint(self, value: float) -> None:
        payload = str(value)
        self.client.publish(self.setpoint_topic, payload)
        logger.debug("Publicado dado lidar em %s: %s", self.setpoint_topic, payload)

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
