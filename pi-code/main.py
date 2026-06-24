import time
from createPlayers import *
from pi_camera import scan_year
from publishStationData import start, publishStat, stop, startStation, startInitialStations, STATIONS
import paho.mqtt.client as mqtt

# Module-level so on_message always references the same dict
state = {"1": 0, "2": 3}

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe("station/+")

def on_message(client, userdata, msg):
    msg = msg.payload.decode()
    print(msg)
    startStation(msg, state)

subscriber = mqtt.Client()
subscriber.on_connect = on_connect
subscriber.on_message = on_message
subscriber.connect("10.42.0.1", 1883)
subscriber.loop_start()

players = createPlayers()
orangeCity = assign_city(players[0])
purpleCity = assign_city(players[1])
cities = [orangeCity, purpleCity]

start()
try:
    publishStat(cities)
    startInitialStations(state)

    # Wait until player 1 has been removed from the last station
    while state["1"] < len(STATIONS):
        time.sleep(0.1)

    # Players are now at the camera — scan year as many times as they like
    while True:
        year = scan_year()
        setCityParameters(orangeCity, year)
        setCityParameters(purpleCity, year)
        cities = [orangeCity, purpleCity]
        print(f"Year {year} | Orange: {orangeCity} | Purple: {purpleCity}")
finally:
    stop()
