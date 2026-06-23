from createPlayers import *
from publishStationData import start, publishStat, stop

# Create the players with their statistics
players = createPlayers()
orangeCity = assign_city(players[0])
purpleCity = assign_city(players[1])
cities = [orangeCity, purpleCity]

# # Publish statistic per topic
start()
try:
    publishStat(cities)
finally:
    stop()