#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// WiFi / MQTT config
const char* ssid       = "PiWiFi";
const char* password   = "1234509876";
const char* mqttServer = "10.42.0.1";

// BottomNeoPixel pin config
#define DATAPIN 17
#define LED_COUNT 40

// Siren light (PWM) pin config
#define SIREN_PIN  25
#define SIREN_FREQ 1000
#define SIREN_RES  8           // brightness 0..255

// RFID pin config
const int RST_PIN = 22;
const int SS_PIN  = 21;

// MQTT topics
const char* TOPIC_STATION = "station/safety";   // our station (Pi sends [val1,val2] + start1/start2)
const char* TOPIC_LEVEL   = "carrera/level";    // we re-publish the active level for the track

WiFiClient   esp;
PubSubClient client(esp);
MFRC522      mfrc522(SS_PIN, RST_PIN);
Adafruit_NeoPixel strip(LED_COUNT, DATAPIN, NEO_GRB + NEO_KHZ800);
Preferences  prefs;

unsigned long lastRFIDCheck = 0;
const unsigned long RFID_INTERVAL = 1000;  // ms between RFID polls

int activeCard = 0;  // 0 = none, 1 = Player1, 2 = Player2

// MSG logic
int val1 = 0;
int val2 = 0;

// Siren light + level
int           sirenBright[5] = {60, 110, 160, 210, 255};  // per-level brightness 0..255
int           speedLevel     = 0;                          // 0 = stop, 1..5 = active player's value
uint8_t       sirenPhase     = 0;
unsigned long lastFlash      = 0;

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

// Siren light (PWM, synced to the active level)

int sirenInterval() {            // higher level = faster flashing (ms)
  switch (speedLevel) {
    case 1: return 350;  case 2: return 260;  case 3: return 180;
    case 4: return 120;  case 5: return 70;   default: return 1000;
  }
}

void renderSirenLight() {
  if (speedLevel == 0) { ledcWrite(SIREN_PIN, 0); return; }   // stopped -> off
  bool on = (sirenPhase == 0 || sirenPhase == 2);            // strobe double-blink
  ledcWrite(SIREN_PIN, on ? sirenBright[speedLevel - 1] : 0);
}

// Level bridge (Pi -> us -> track)

int activeVal() {
  if (activeCard == 1) return val1;
  if (activeCard == 2) return val2;
  return 0;
}

void setSpeedLevel(int lvl) {
  lvl = constrain(lvl, 0, 5);
  if (lvl == speedLevel) return;
  speedLevel = lvl;
  if (client.connected()) client.publish(TOPIC_LEVEL, String(speedLevel).c_str(), true);  // retained
  Serial.printf("level %d -> carrera/level\n", speedLevel);
}

void updateLevel() {
  // While a card is active, the car runs at that player's safety value.
  // (Want "lower safety = faster"? use  setSpeedLevel(6 - activeVal());  instead.)
  if (activeCard == 0) return;     // idle: leave level alone so serial test still works
  setSpeedLevel(activeVal());
}

// Config (NVS) — siren light calibration

void saveConfig() {
  for (int i = 0; i < 5; i++) prefs.putInt(("b" + String(i)).c_str(), sirenBright[i]);
}

void loadConfig() {
  prefs.begin("safety", false);
  for (int i = 0; i < 5; i++) sirenBright[i] = prefs.getInt(("b" + String(i)).c_str(), sirenBright[i]);
}

void printConfig() {
  Serial.println("---- siren config ----");
  for (int i = 0; i < 5; i++)
    Serial.printf("  level %d -> brightness %d\n", i + 1, sirenBright[i]);
  Serial.printf("  current level : %d\n", speedLevel);
}

// Serial commands (debug / calibrate)

void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (isDigit(line[0]) && line.indexOf(' ') == -1) {   // force a level (bench test, no card needed)
    int lvl = line.toInt();
    if (lvl >= 0 && lvl <= 5) setSpeedLevel(lvl);
    else Serial.println("level must be 0..5");
    return;
  }

  int sp = line.indexOf(' ');
  String cmd  = line.substring(0, sp);
  String rest = (sp == -1) ? "" : line.substring(sp + 1);
  rest.trim();

  if (cmd == "set") {
    int s = rest.indexOf(' ');
    int L = rest.substring(0, s).toInt();
    int B = rest.substring(s + 1).toInt();
    if (L >= 1 && L <= 5 && B >= 0 && B <= 255) {
      sirenBright[L - 1] = B; saveConfig();
      Serial.printf("level %d -> brightness %d\n", L, B);
    } else Serial.println("usage: set <1-5> <0-255>");
  }
  else if (cmd == "show") printConfig();
  else if (cmd == "help") Serial.println("cmds: 0..5 | set <L> <bright> | show | help");
  else Serial.println("? type 'help'");
}

// MQTT

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
    if (client.connect("safetyStation")) {
      Serial.println("connected");
      client.subscribe(TOPIC_STATION); // change topic name for correct station!!
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
      client.publish(TOPIC_STATION, msg.c_str()); // change topic name for correct station!!
      strip.clear();
      strip.show();
      setSpeedLevel(0);            // car + siren stop when the card leaves
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
    blockLights(1);               // show player 1's blocks once on tap
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
    blockLights(2);               // show player 2's blocks once on tap
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

  ledcAttach(SIREN_PIN, SIREN_FREQ, SIREN_RES);
  ledcWrite(SIREN_PIN, 0);

  loadConfig();

  setupWiFi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Safety station ready. Type 'help'.");
  printConfig();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (Serial.available()) handleLine(Serial.readStringUntil('\n'));

  unsigned long now = millis();
  if (now - lastRFIDCheck >= RFID_INTERVAL) {
    lastRFIDCheck = now;
    rfidAction();
  }

  updateLevel();   // keep the published level in step with the active player

  if (now - lastFlash >= (unsigned long)sirenInterval()) {
    lastFlash = now;
    sirenPhase = (sirenPhase + 1) % 8;
    renderSirenLight();
  }
}

/*
  CORE 2.x: swap the ledc calls for the channel API:
    const int CH = 0; ledcSetup(CH, SIREN_FREQ, SIREN_RES); ledcAttachPin(SIREN_PIN, CH);
    then ledcWrite(CH, brightness);
*/
