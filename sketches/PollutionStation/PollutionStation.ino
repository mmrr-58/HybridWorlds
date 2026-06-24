#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>

// WiFi / MQTT config
const char* ssid       = "PiWiFi";
const char* password   = "1234509876";
const char* mqttServer = "10.42.0.1";

// NeoPizel pin config
#define DATAPIN 17
#define LED_COUNT 40

// RFID pin config
const int RST_PIN = 22;
const int SS_PIN  = 21;

WiFiClient   esp;
PubSubClient client(esp);
MFRC522      mfrc522(SS_PIN, RST_PIN);
Adafruit_NeoPixel strip(LED_COUNT, DATAPIN, NEO_GRB + NEO_KHZ800);

unsigned long lastRFIDCheck = 0;
const unsigned long RFID_INTERVAL = 1000;  // ms between RFID polls

int activeCard = 0;  // 0 = none, 1 = Player1, 2 = Player2

// MSG logic
int val1 = 0;
int val2 = 0;

// WiFi 

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

// NeoPixel

void startUpLights(int player) {
  if (player == 1) {
    strip.fill(strip.Color(140, 10, 0));  // orange
    strip.show();
  } else if (player == 2) {
    strip.fill(strip.Color(100, 0, 140));  // purple
    strip.show();
  }
}

void blockLights(int player) {
  strip.clear();
  strip.show();

  if (player == 1 && val1 > 1) {
    int groups = val1 - 1;
    for (int g = 0; g < groups; g++) {
      int start = g * 5;
      strip.setPixelColor(start,     strip.Color(140, 10, 0));
      strip.setPixelColor(start + 1, strip.Color(140, 10, 0));
      strip.setPixelColor(start + 2, strip.Color(140, 10, 0));
      strip.show();
      delay(1000);
    }
  }

  if (player == 2 && val2 > 1) {
    int groups = val2 - 1;
    for (int g = 0; g < groups; g++) {
      int start = (LED_COUNT - 1) - g * 5;
      strip.setPixelColor(start,     strip.Color(100, 0, 140));
      strip.setPixelColor(start - 1, strip.Color(100, 0, 140));
      strip.setPixelColor(start - 2, strip.Color(100, 0, 140));
      strip.show();
      delay(1000);
    }
  }
}

// MQTT 

void stationAction() {
  if (val1) {
    if (activeCard == 1) { 
      blockLights(1);     
    }
  }
  if (val2) {
    if (activeCard == 2) {
      blockLights(2);
    }
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
   
  if (message == "start1") {
    startUpLights(1);
    return;
  }
  if (message == "start2") {
    startUpLights(2);
    return;
  }

  int commaIndex = message.indexOf(',');
  if (commaIndex == -1) return;

  val1 = message.substring(1, commaIndex).toInt();
  val2 = message.substring(commaIndex + 1, message.length() - 1).toInt();

  Serial.println(val1);
  Serial.println(val2);

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("pollutionStation")) {
      Serial.println("connected");
      client.subscribe("station/pollution"); // change topic name for correct station!!
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// RFID

// WUPA reaches halted cards too, unlike REQA used by PICC_IsNewCardPresent()
bool isCardStillPresent() {
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  MFRC522::StatusCode result = mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);
  return (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION);
}

void rfidAction() {
  // A light is on — just check if the same card is still there
  if (activeCard != 0) {
    if (!isCardStillPresent()) {
      String msg = "Removed: " + String(activeCard);
      client.publish("station/pollution", msg.c_str()); // change topic name for correct station!!
      strip.clear();
      strip.show();
      activeCard = 0;
    } else {
      mfrc522.PICC_HaltA();  // put it back to halt so next WUPA can find it
    }
    return;
  }

  // No card was active — look for a new one
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Tag 1: 4A 5B 8E 32
  if (mfrc522.uid.uidByte[0] == 0x4A &&
      mfrc522.uid.uidByte[1] == 0x5B &&
      mfrc522.uid.uidByte[2] == 0x8E &&
      mfrc522.uid.uidByte[3] == 0x32 && 
      val1) {

    Serial.println("Tag 1 detected! val received");
    activeCard = 1;
  } 
  else if (mfrc522.uid.uidByte[0] == 0x4A &&
      mfrc522.uid.uidByte[1] == 0x5B &&
      mfrc522.uid.uidByte[2] == 0x8E &&
      mfrc522.uid.uidByte[3] == 0x32 && 
      !val1) {

    Serial.println("Tag 1 detected! No val received");
  } 
  // Tag 2: 87 64 97 31
  else if (mfrc522.uid.uidByte[0] == 0x87 &&
           mfrc522.uid.uidByte[1] == 0x64 &&
           mfrc522.uid.uidByte[2] == 0x97 &&
           mfrc522.uid.uidByte[3] == 0x31 &&
           val2) {

    Serial.println("Tag 2 detected! val received");
    activeCard = 2;
  }
  else if (mfrc522.uid.uidByte[0] == 0x87 &&
           mfrc522.uid.uidByte[1] == 0x64 &&
           mfrc522.uid.uidByte[2] == 0x97 &&
           mfrc522.uid.uidByte[3] == 0x31 &&
           !val2) {

    Serial.println("Tag 2 detected! No val received");
  }
  else {
    Serial.print("Unknown tag:");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
  }

  mfrc522.PICC_HaltA();  // halt after reading so WUPA can detect it next poll
}

// Setup / Loop 

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.show();  // start with all pixels off

  setupWiFi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Ready");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastRFIDCheck >= RFID_INTERVAL) {
    lastRFIDCheck = now;
    rfidAction();
  }

  stationAction();
}
