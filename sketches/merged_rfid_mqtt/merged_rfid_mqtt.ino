#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// WiFi / MQTT config
const char* ssid       = "PiWiFi";
const char* password   = "1234509876";
const char* mqttServer = "10.42.0.1";

// Pin config
const int LEDPin  = 2;   // Seeed Studio ESP built-in LED
const int LIGHT_1 = 16;
const int LIGHT_2 = 17;

// RFID pin config
const int RST_PIN = 22;
const int SS_PIN  = 21;

WiFiClient   esp;
PubSubClient client(esp);
MFRC522      mfrc522(SS_PIN, RST_PIN);

unsigned long lastRFIDCheck = 0;
const unsigned long RFID_INTERVAL = 1000;  // ms between RFID polls

int activeLight = 0;  // 0 = none, 1 = LIGHT_1, 2 = LIGHT_2

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

// MQTT 

void stationAction(String message) {
  if (message == "4") {
    digitalWrite(LEDPin, HIGH);
  } else {
    digitalWrite(LEDPin, LOW);
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

  stationAction(message);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("healthStation")) {
      Serial.println("connected");
      client.subscribe("health");
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
  if (activeLight != 0) {
    if (!isCardStillPresent()) {
      Serial.println("Card removed. Turning off lights.");
      digitalWrite(LIGHT_1, LOW);
      digitalWrite(LIGHT_2, LOW);
      activeLight = 0;
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
      mfrc522.uid.uidByte[3] == 0x32) {

    Serial.println("Tag 1 detected! Turning on Light 1.");
    digitalWrite(LIGHT_1, HIGH);
    digitalWrite(LIGHT_2, LOW);
    activeLight = 1;
  }
  // Tag 2: 87 64 97 31
  else if (mfrc522.uid.uidByte[0] == 0x87 &&
           mfrc522.uid.uidByte[1] == 0x64 &&
           mfrc522.uid.uidByte[2] == 0x97 &&
           mfrc522.uid.uidByte[3] == 0x31) {

    Serial.println("Tag 2 detected! Turning on Light 2.");
    digitalWrite(LIGHT_1, LOW);
    digitalWrite(LIGHT_2, HIGH);
    activeLight = 2;
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

  pinMode(LEDPin,  OUTPUT);
  pinMode(LIGHT_1, OUTPUT);
  pinMode(LIGHT_2, OUTPUT);
  digitalWrite(LEDPin,  LOW);
  digitalWrite(LIGHT_1, LOW);
  digitalWrite(LIGHT_2, LOW);

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
}
