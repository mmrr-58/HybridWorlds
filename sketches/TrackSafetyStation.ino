#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// WiFi / MQTT config
const char* ssid       = "PiWiFi";
const char* password   = "1234509876";
const char* mqttServer = "10.42.0.1";

// Motor (IRL540N low-side) pin config
#define PWM_PIN  25
#define PWM_FREQ 1000
#define PWM_RES  10            // duty 0..1023

// NeoPixel (police siren) pin config
#define DATAPIN  4
#define MAX_LEDS 300           // buffer cap; real count set at runtime

// MQTT topics
const char* TOPIC_LEVEL = "carrera/level";   // shared level (Pi or track serial sets it; both boards follow)
const char* TOPIC_CMD   = "carrera/cmd";     // remote calibration (set / leds / bright / show)

WiFiClient   esp;
PubSubClient client(esp);
Adafruit_NeoPixel strip(MAX_LEDS, DATAPIN, NEO_RGB + NEO_KHZ800);
Preferences  prefs;

// Speed / calibration
int  levelDuty[5] = {375, 405, 425, 455, 475};  // out of 1023
int speedLevel   = 0;          // 0 = stop, 1..5 = speeds
int numLeds      = 30;
int brightness   = 250;

// Siren animation
uint8_t       sirenPhase = 0;
unsigned long lastFlash  = 0;

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
}

// NeoPixel

void initStrip() {
  strip.updateLength(numLeds);
  strip.setBrightness(brightness);
  strip.begin();
  strip.clear();
  strip.show();
}

int sirenInterval() {            // higher speed = faster flashing (ms)
  switch (speedLevel) {
    case 1: return 350;  case 2: return 260;  case 3: return 180;
    case 4: return 120;  case 5: return 70;   default: return 1000;
  }
}

void renderSiren() {
  strip.clear();
  if (speedLevel == 0) { strip.show(); return; }   // stopped -> siren off
  int half = numLeds / 2;
  bool leftOn  = (sirenPhase == 0 || sirenPhase == 2);
  bool rightOn = (sirenPhase == 4 || sirenPhase == 6);
  if (numLeds == 1) {
    strip.setPixelColor(0, (sirenPhase < 4) ? strip.Color(255, 0, 0) : strip.Color(0, 0, 255));
  } else {
    if (leftOn)  for (int i = 0; i < half; i++)       strip.setPixelColor(i, strip.Color(255, 0, 0));
    if (rightOn) for (int i = half; i < numLeds; i++) strip.setPixelColor(i, strip.Color(0, 0, 255));
  }
  strip.show();
}

// Motor

void applySpeed() {
  int duty = (speedLevel == 0) ? 0 : levelDuty[speedLevel - 1];
  ledcWrite(PWM_PIN, duty);
}

// Level helpers

void applyLevel(int lvl) {                 // set locally only (used by incoming MQTT)
  speedLevel = constrain(lvl, 0, 5);
  applySpeed();
}

void setLevelAndPublish(int lvl) {         // local change from serial -> tell everyone
  applyLevel(lvl);
  if (client.connected()) client.publish(TOPIC_LEVEL, String(speedLevel).c_str(), true);  // retained
  Serial.printf("speed level %d\n", speedLevel);
}

// Config (NVS)

void saveConfig() {
  for (int i = 0; i < 5; i++) prefs.putInt(("d" + String(i)).c_str(), levelDuty[i]);
  prefs.putInt("nleds", numLeds);
  prefs.putInt("bright", brightness);
}

void loadConfig() {
  prefs.begin("carrera", false);
  for (int i = 0; i < 5; i++) levelDuty[i] = prefs.getInt(("d" + String(i)).c_str(), levelDuty[i]);
  numLeds    = prefs.getInt("nleds", numLeds);
  brightness = prefs.getInt("bright", brightness);
}

void printConfig() {
  Serial.println("---- config ----");
  for (int i = 0; i < 5; i++)
    Serial.printf("  level %d -> duty %d (%d%%)\n", i + 1, levelDuty[i], levelDuty[i] * 100 / 1023);
  Serial.printf("  speed level : %d\n", speedLevel);
  Serial.printf("  neopixels   : %d\n", numLeds);
  Serial.printf("  brightness  : %d\n", brightness);
}

// Commands (serial + carrera/cmd)

void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  // bare number -> set speed level (and broadcast it)
  if (isDigit(line[0]) && line.indexOf(' ') == -1) {
    int lvl = line.toInt();
    if (lvl >= 0 && lvl <= 5) setLevelAndPublish(lvl);
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
    int D = rest.substring(s + 1).toInt();
    if (L >= 1 && L <= 5 && D >= 0 && D <= 1023) {
      levelDuty[L - 1] = D; saveConfig();
      if (speedLevel == L) applySpeed();
      Serial.printf("level %d -> duty %d\n", L, D);
    } else Serial.println("usage: set <1-5> <0-1023>");
  }
  else if (cmd == "leds") {
    int n = rest.toInt();
    if (n >= 1 && n <= MAX_LEDS) { numLeds = n; initStrip(); saveConfig();
      Serial.printf("neopixels = %d\n", numLeds); }
    else Serial.printf("leds must be 1..%d\n", MAX_LEDS);
  }
  else if (cmd == "bright") {
    int b = rest.toInt();
    if (b >= 0 && b <= 255) { brightness = b; strip.setBrightness(brightness); saveConfig();
      Serial.printf("brightness = %d\n", brightness); }
    else Serial.println("bright must be 0..255");
  }
  else if (cmd == "show") printConfig();
  else if (cmd == "help") Serial.println("cmds: 0..5 | set <L> <duty> | leds <n> | bright <v> | show | help");
  else Serial.println("? type 'help'");
}

// MQTT

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];

  // shared level from the Pi (or another board) -> apply locally, do NOT re-publish
  if (String(topic) == TOPIC_LEVEL) {
    applyLevel(message.toInt());
    Serial.printf("level <- mqtt: %d\n", speedLevel);
    return;
  }

  // remote calibration / admin
  Serial.printf("cmd <- mqtt: %s\n", message.c_str());
  handleLine(message);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("carreraTrack")) {
      Serial.println("connected");
      client.subscribe(TOPIC_LEVEL);   // follow shared level (Pi or track serial)
      client.subscribe(TOPIC_CMD);     // remote calibration
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Setup / Loop

void setup() {
  Serial.begin(115200);

  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);
  ledcWrite(PWM_PIN, 0);

  loadConfig();
  initStrip();

  setupWiFi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  Serial.println("Track controller ready. Type 'help'.");
  printConfig();
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (Serial.available()) handleLine(Serial.readStringUntil('\n'));

  if (millis() - lastFlash >= (unsigned long)sirenInterval()) {
    lastFlash = millis();
    sirenPhase = (sirenPhase + 1) % 8;
    renderSiren();
  }
}

/*
  CORE 2.x: swap the ledc calls for the channel API:
    const int CH = 0; ledcSetup(CH, PWM_FREQ, PWM_RES); ledcAttachPin(PWM_PIN, CH);
    then ledcWrite(CH, duty);
*/
