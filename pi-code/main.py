from createPlayers import *
from pi_camera import scan_year
from publishStationData import start, publishStat, stop, startStation, startInitialStations
import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe("station/+")

def on_message(client, userdata, msg):
    msg = msg.payload.decode()
    print(f"[{msg.topic}] {msg}")
    startStation(msg, state)

subscriber = mqtt.Client()
subscriber.on_connect = on_connect
subscriber.on_message = on_message
subscriber.connect("10.42.0.1", 1883)
subscriber.loop_start()

# Create the players with their statistics
players = createPlayers()
orangeCity = assign_city(players[0])
purpleCity = assign_city(players[1])
cities = [orangeCity, purpleCity]

# player "1" ascends 0→3, player "2" descends 3→0
state = {"1": 0, "2": 3}

# # Publish statistic per topic
year = scan_year()
setCityParameters(orangeCity, year)
setCityParameters(purpleCity, year)

start()
try:
    publishStat(cities)
    startInitialStations(state)
finally:
    stop()

