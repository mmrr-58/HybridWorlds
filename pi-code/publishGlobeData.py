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

def publishID(data):
    info = client.publish("players", str(data))
    info.wait_for_publish()
    print(f"IDs: {data} publised")

def stop():
    client.loop_stop()
    client.disconnect()
