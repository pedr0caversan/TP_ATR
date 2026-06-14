# Na pasta em que esse arquivo está ficarão os publishers e subscribers (sensores, atuadores, etc)
# Só criei esse arquivo para possibilitar subir a pasta pro remoto

import paho.mqtt.client as mqtt
import json

class MQTTHandler:
    def __init__(self, client_id, broker="localhost", port=1883):
        self.client = mqtt.Client(client_id)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.broker = broker
        self.port = port
        self.callbacks = {} # Guarda as funções que vão tratar cada tópico

    def connect_and_start(self):
        self.client.connect(self.broker, self.port, 60)
        self.client.loop_start() # Roda em background, não trava a interface

    def on_connect(self, client, userdata, flags, rc):
        print(f"[{client._client_id.decode()}] Conectado ao MQTT. Status: {rc}")
        client.subscribe("atr/telemetria/#")
        client.subscribe("atr/sim/#")

    def on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode()
        if topic in self.callbacks:
            self.callbacks[topic](payload)

    def register_callback(self, topic, func):
        self.callbacks[topic] = func

    def publish_data(self, topic, payload):
        self.client.publish(topic, str(payload))

    def stop(self):
        self.client.loop_stop()
        self.client.disconnect()