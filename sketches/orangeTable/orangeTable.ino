#include <light_CD74HC4067.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <WiFi.h>

// WiFi / MQTT config
const char* ssid = "PiWiFi";
const char* password = "1234509876";
const char* mqttServer = "10.42.0.1";


const int inS0 = 21;
const int inS1 = 19;
const int inS2 = 18;
const int inS3 = 17;
const int inSig = 34;
const int outS0 = 12;
const int outS1 = 13;
const int outS2 = 25;
const int outS3 = 26;
const int outSig = 27;

int rS;
int gS;
int bS;

int rH;
int gH;
int bH;

int rC;
int gC;
int bC;

int rP;
int gP;
int bP;

int indexSafety;
int indexHealth;
int indexCost;
int indexPol;
float scaleSafety;
float scaleHealth;
float scaleCost;
float scalePol;

WiFiClient esp;
PubSubClient client(esp);

CD74HC4067 inMux(inS0, inS1, inS2, inS3);
CD74HC4067 outMux(outS0, outS1, outS2, outS3);

const int NUMPIXELS = 2;
Adafruit_NeoPixel pixels(NUMPIXELS, 27, NEO_GRB + NEO_KHZ800);

String buildingSpots[16] = { "" };
float voltage;

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi FAILED. Scanning for visible networks...");
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) Serial.println(WiFi.SSID(i));
    return;
  }

  Serial.print("\nConnected. IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
}

void stationAction() {
  if (indexSafety == 1) {
    rS = 255;
    gS = 0;
    bS = 0;
  } else if (indexSafety == 2) {
    rS = 255;
    gS = 127;
    bS = 0;
  } else if (indexSafety == 3) {
    rS = 255;
    gS = 255;
    bS = 0;
  } else if (indexSafety == 4) {
    rS = 144;
    gS = 220;
    bS = 144;
  } else if (indexSafety == 5) {
    rS = 0;
    gS = 255;
    bS = 0;
  }

  if (indexHealth == 1) {
    rH = 255;
    gH = 0;
    bH = 0;
  } else if (indexHealth == 2) {
    rH = 255;
    gH = 127;
    bH = 0;
  } else if (indexHealth == 3) {
    rH = 255;
    gH = 255;
    bH = 0;
  } else if (indexHealth == 4) {
    rH = 144;
    gH = 220;
    bH = 144;
  } else if (indexHealth == 5) {
    rH = 0;
    gH = 255;
    bH = 0;
  }

  if (indexCost == 1) {
    rC = 255;
    gC = 0;
    bC = 0;
  } else if (indexCost == 2) {
    rC = 255;
    gC = 127;
    bC = 0;
  } else if (indexCost == 3) {
    rC = 255;
    gC = 255;
    bC = 0;
  } else if (indexCost == 4) {
    rC = 144;
    gC = 220;
    bC = 144;
  } else if (indexCost == 5) {
    rC = 0;
    gC = 255;
    bC = 0;
  }

  if (indexPol == 1) {
    rP = 255;
    gP = 0;
    bP = 0;
  } else if (indexPol == 2) {
    rP = 255;
    gP = 127;
    bP = 0;
  } else if (indexPol == 3) {
    rP = 255;
    gP = 255;
    bP = 0;
  } else if (indexPol == 4) {
    rP = 144;
    gP = 220;
    bP = 144;
  } else if (indexPol == 5) {
    rP = 0;
    gP = 255;
    bP = 0;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Remove brackets
  message.replace("[", "");
  message.replace("]", "");

  int c1 = message.indexOf(',');
  int c2 = message.indexOf(',', c1 + 1);
  int c3 = message.indexOf(',', c2 + 1);
  int c4 = message.indexOf(',', c3 + 1);
  int c5 = message.indexOf(',', c4 + 1);
  int c6 = message.indexOf(',', c5 + 1);
  int c7 = message.indexOf(',', c6 + 1);

  indexSafety = message.substring(0, c1).toFloat();
  indexHealth = message.substring(c1 + 1, c2).toFloat();
  indexCost = message.substring(c2 + 1, c3).toFloat();
  indexPol = message.substring(c3 + 1, c4).toFloat();
  scaleSafety = message.substring(c4 + 1, c5).toFloat();
  scaleHealth = message.substring(c5 + 1, c6).toFloat();
  scaleCost = message.substring(c6 + 1, c7).toFloat();
  scalePol = message.substring(c7 + 1).toFloat();

  Serial.println(indexSafety);
  Serial.println(indexHealth);
  Serial.println(indexCost);
  Serial.println(indexPol);
  Serial.println(scaleSafety);
  Serial.println(scaleHealth);
  Serial.println(scaleCost);
  Serial.println(scalePol);
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("tableOrange")) {
      Serial.println("connected");
      client.subscribe("table/orange");  // change topic name for correct station!!
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  pixels.begin();

  pinMode(outSig, OUTPUT);

  setupWiFi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}

void identifyWriteType() {

  for (int i = 0; i < 16; i++) {
    inMux.channel(i);
    outMux.channel(i);
    delay(1);
    delayMicroseconds(50);
    analogRead(inSig);  // throw away first reading
    int raw = analogRead(inSig);
    voltage = raw * 3.3 / 4095.0;

    if (voltage > 0.45 && voltage < 0.95) {
      buildingSpots[i] = "safety";


    } else if (voltage > 1.0 && voltage < 1.68) {
      buildingSpots[i] = "cost";

    } else if (voltage > 1.75 && voltage < 2.38) {
      buildingSpots[i] = "pollution";

    } else if (voltage > 2.40 && voltage < 3.35) {
      buildingSpots[i] = "healthcare";

    } else buildingSpots[i] = "unknown";

    // Serial.print("Channel " + String(i) + ": " + String(voltage));
    // Serial.print("\t");
    // Serial.print(buildingSpots[i]);
    // Serial.println();
    delay(10);
  }
}

//This function useless cuz you can just write values on outMux in function above

void writeToType() {
  for (int i = 0; i < 16; i++) {
    outMux.channel(i);
    delayMicroseconds(100);
    if (buildingSpots[i] == "safety") {
      pixels.setPixelColor(0, pixels.Color(rS * scaleSafety, gS * scaleSafety, bS * scaleSafety));
      pixels.setPixelColor(1, pixels.Color(rS * scaleSafety, gS * scaleSafety, bS * scaleSafety));

    } else if (buildingSpots[i] == "cost") {
      pixels.setPixelColor(0, pixels.Color(rC * scaleCost, gC * scaleCost, bC * scaleCost));
      pixels.setPixelColor(1, pixels.Color(rC * scaleCost, gC * scaleCost, bC * scaleCost));

    } else if (buildingSpots[i] == "pollution") {
      pixels.setPixelColor(0, pixels.Color(rP * scalePol, gP * scalePol, bP * scalePol));
      pixels.setPixelColor(1, pixels.Color(rP * scalePol, gP * scalePol, bP * scalePol));

    } else if (buildingSpots[i] == "healthcare") {
      pixels.setPixelColor(0, pixels.Color(rH * scaleHealth, gH * scaleHealth, bH * scaleHealth));
      pixels.setPixelColor(1, pixels.Color(rH * scaleHealth, gH * scaleHealth, bH * scaleHealth));
    }
    pixels.show();
    delay(10);
  }
}



void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  stationAction();
  identifyWriteType();
  writeToType();
}
