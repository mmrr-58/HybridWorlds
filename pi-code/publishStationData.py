import threading
import paho.mqtt.client as mqtt

BROKER = "localhost"
PORT = 1883

_connected = threading.Event()


def _on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        _connected.set()
    else:
        print(f"Connect failed: {reason_code}")


# paho-mqtt 2.x requires an explicit callback API version
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = _on_connect


def start():
    """Open the connection and wait until the broker confirms it."""
    client.connect(BROKER, PORT)
    client.loop_start()
    if not _connected.wait(timeout=5):
        raise RuntimeError("Timed out waiting for MQTT broker connection")


def publishStat(data):
    ids = []
    safety = []
    health = []
    cost = []
    pollution = []

    for city in data:
        ids.append(city['id'])
        safety.append(city['safety'])
        health.append(city['health'])
        cost.append(city['cost'])
        pollution.append(city['pollution'])
    
    info = client.publish("station/globe", str(ids))
    info.wait_for_publish()
    info = client.publish("station/safety", str(safety))
    info.wait_for_publish()
    info = client.publish("station/health", str(health))
    info.wait_for_publish()
    info = client.publish("station/cost", str(cost))
    info.wait_for_publish()
    info = client.publish("station/pollution", str(pollution))
    info.wait_for_publish()


def stop():
    client.loop_stop()
    client.disconnect()
