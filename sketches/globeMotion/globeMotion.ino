#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

//----------------------------------------BASE GLOBE CODE INITIALIZERS------------------------------------------
#include <Adafruit_NeoPixel.h>
const int PIN = 12;
const int NUMPIXELS = 16;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int DELAYVAL = 10;  // Time (in milliseconds) to pause between pixels

Adafruit_MPU6050 mpu;
bool startLoop = false;

//Acceleration sensor
float accTotal;
float accThreshold = 15;

//Spin stuff
int spinAmount = 0;
bool userSpin = false;

//Showing leds stuff
bool showAllCities1 = false;
bool showAllCities2 = false;
bool occupied = false;

int selectedCity1;
int selectedCity2;

//Idle breathing
int x = 0;
bool xIncrease = true;
bool xDecrease = false;

//Timer stuff
unsigned long timer;
unsigned long citiesTimer;
unsigned long citiesRuntimer;
int citiesDelay = 1000;
int citiesRuntime = 6000;
int runTime = 5000;
bool citiesInit = false;
bool citiesRunInit = false;
bool initialized = false;
bool reInit = false;

//Random flickering when spun stuff
int turnOn;
int turnOn2;
int randGreen;
int randBlue;
int randRed;

//Player colors for showing cities
int r;
int g;
int b;

//------------------------------------------------MQTT MESSAGING INITIALIZERS--------------------------------------------------
const char* ssid = "PiWiFi";
const char* password = "1234509876";
const char* mqttServer = "10.42.0.1";
const int LEDPin = 2;  //Pin for the Seeed Studio ESP

WiFiClient esp;
PubSubClient client(esp);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print(message);

  stationAction(message, topic);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("globe")) {  //Change client name depending on function
      Serial.println("connected");
      client.subscribe("station/globe");  //Subscribe to the important topic
      client.subscribe("reset");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//-------------------------------------------------------------SETUP--------------------------------------------------
void setup() {
  Serial.begin(9600);

  //PIXELS
  randomSeed(analogRead(15));
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  pixels.show();

  //IMU
  while (!Serial)
    delay(10);  // will pause Zero, Leonardo, etc until serial console opens
  Serial.println("Adafruit MPU6050 test!");
  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  //setup motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);  // Keep it latched.  Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);
  Serial.println("");
  delay(100);

  //MQTT
  setupWiFi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}


void accSensor() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  accTotal =
    sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z);
}

void ledIdle() {
  if (!occupied) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, x, 0));
    }
    if (xIncrease) {
      x++;
    }
    if (xDecrease) {
      x--;
    }
    if (x >= 150) {
      xIncrease = false;
      xDecrease = true;
    }
    if (x <= 0) {
      xIncrease = true;
      xDecrease = false;
    }
  } else if (occupied) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(120, 0, 0));
    }
  }
  pixels.show();
}

void ledFlicker() {
  pixels.clear();

  turnOn = random(0, 16);
  turnOn2 = random(0, 16);
  randGreen = random(150, 255);
  randBlue = random(150, 255);
  randRed = random(0, 100);
  pixels.setPixelColor(turnOn, pixels.Color(randRed, randGreen, randBlue));
  pixels.setPixelColor(turnOn2, pixels.Color(randRed, randGreen, randBlue));

  pixels.show();
  DELAYVAL++;
  delay(DELAYVAL);
}

void ledAllCities() {
  if (!citiesRunInit) {
    citiesRuntimer = millis();
    citiesRunInit = true;
  }
  if (!citiesInit) {
    citiesTimer = millis();
  }

  if (millis() - citiesRuntimer <= citiesRuntime) {

    if (millis() - citiesTimer <= citiesDelay) {
      citiesInit = true;
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(r, g, b));
      }
      pixels.show();
    } else if (millis() - citiesTimer <= citiesDelay * 2) {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
      pixels.show();
    } else {
      citiesInit = false;
    }
  } else {
    if (spinAmount == 2) {
      occupied = true;
    }
    ledIdle();
  }
}


void userSpun() {
  if (!initialized) {
    timer = millis();
  }

  if (!userSpin && spinAmount == 0) {
    ledIdle();
  }

  if (accTotal >= accThreshold && spinAmount < 2) {
    userSpin = true;
    initialized = true;
    showAllCities1 = false;
    showAllCities2 = false;
  }
  if (userSpin && spinAmount < 2 && initialized) {

    if (spinAmount == 0) {
      ledFlicker();
      if (millis() - timer >= runTime && initialized) {
        DELAYVAL = 10;
        selectedCity1 = random(1, 16);
        initialized = false;
        userSpin = false;
        spinAmount++;
        showAllCities1 = true;
      }
    }
    if (spinAmount == 1) {
      ledFlicker();
      if (millis() - timer >= runTime && initialized) {
        selectedCity2 = random(1, 16);
        initialized = false;
        userSpin = false;
        spinAmount++;
        showAllCities2 = true;
      }
    }
  }

  if (showAllCities1) {
    r = 140;
    g = 10;
    b = 0;
    ledAllCities();
    //orange
  }
  if (showAllCities2) {
    r = 100;
    g = 0;
    b = 140;
    if (!reInit) {
      citiesInit = false;
      citiesRunInit = false;
      reInit = true;
    }
    ledAllCities();
    //purple
  }
  pixels.show();
}

void serialPrinter() {
  // Serial.print("Acc: ");
  // Serial.print(accTotal);
  // Serial.print("\t");
  // Serial.print(millis());
  // Serial.print("\t");
  // Serial.print(millis() - timer);
  // Serial.print("\t");
  Serial.print(spinAmount);
  Serial.print("\t");
  Serial.println();
}

void stationAction(String message, String topic) {
  if (message == "start") {
    startLoop = true;
  }
  if (message == "reset") {
    spinAmount = 0;
    startLoop = false;
    citiesInit = false;
    citiesRunInit = false;
    initialized = false;
    reInit = false;
    x = 0;
    xIncrease = true;
    xDecrease = false;
    userSpin = false;
    //Showing leds stuff
    showAllCities1 = false;
    howAllCities2 = false;
    occupied = false;
  }
  // else {

  // }
  // if (topic == )
  //   if (message == "") {
  //     digitalWrite(LEDPin, HIGH);
  //   } else {
  //     digitalWrite(LEDPin, LOW);
  //   }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (startLoop) {
    accSensor();
    userSpun();
    serialPrinter();
  }
}