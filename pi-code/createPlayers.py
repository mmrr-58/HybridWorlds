import sqlite3
import random

con = sqlite3.connect("pi-code/cityData.db")
cursor = con.cursor()

def createPlayers():
    orange = random.randint(1,16)
    purple = random.randint(1,16)
    while orange == purple:
        purple = random.randint(1,16)
    return [orange, purple]
    


def assign_city(cityID):
    city = cursor.execute("""
            SELECT id, cost_of_living_classification, safety_classification, health_care_classification, pollution_classification
            FROM quality_of_life
            WHERE id = ?
                AND year = 2026;
            """, (cityID,)).fetchone()
    city_stats = {"id": city[0], "safety": city[1], "health": city[2], "cost": city[3], "pollution": city[4],}
    return city_stats
    
