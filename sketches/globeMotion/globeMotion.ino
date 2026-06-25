#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

//----------------------------------------BASE GLOBE CODE INITIALIZERS------------------------------------------
const int PIN = 12;
const int NUMPIXELS = 16;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int DELAYVAL = 10;  // Time (in milliseconds) to pause between pixels

Adafruit_MPU6050 mpu;
bool startLoop = false;

//Acceleration sensor
float accTotal;
float accThreshold = 12;
bool onlyTouched = false;
bool userTouched = false;
//Spin stuff
int spinAmount = 0;
bool userSpin = false;

//Showing leds stuff
bool showAllCities1 = false;
bool showAllCities2 = false;
bool occupied = false;

//Idle breathing
int x = 0;
bool xIncrease = true;
bool xDecrease = false;

//Timer stuff
unsigned long timer;
unsigned long citiesTimer;
unsigned long citiesRuntimer;
unsigned long sampleTimer;
int sampleDelay = 500;
int citiesDelay = 1000;
int citiesRuntime = 6000;
int runTime = 5000;
bool citiesInit = false;
bool citiesRunInit = false;
bool initialized = false;
bool reInit = false;
bool sampleInit = false;

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

int orangeCity = 0;
int purpleCity = 0;
bool end = false;

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
  //Serial.print(message);
  stationAction(message, topic);

  int commaIndex = message.indexOf(',');
  if (commaIndex == -1) return;

  orangeCity = message.substring(1, commaIndex).toInt();
  purpleCity = message.substring(commaIndex + 1, message.length() - 1).toInt();

  Serial.print(orangeCity);
  Serial.println(purpleCity);
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
    sqrt(a.acceleration.z * a.acceleration.z);
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
  if (!sampleInit) {
    sampleTimer = millis();
  }

  if (!userSpin && spinAmount == 0) {
    ledIdle();
  }

  if (accTotal >= accThreshold && spinAmount < 2) {
    userTouched = true;
    sampleInit = true;
  }
  //User touched globe
  if (userTouched) {
    Serial.print("user touched");
    Serial.print("\t");

    //User only touched the globe, no spin
    if (millis() - sampleTimer >= sampleDelay) {
      onlyTouched = true;
      Serial.print("user touched once, no spin");
      Serial.print("\t");

      //User actually spun the globe
      if (accTotal >= accThreshold) {
        userSpin = true;
        initialized = true;
        onlyTouched = false;
        Serial.print("user spun the globe");
        Serial.print("\t");
      }
    }
  }

  if ((millis() - sampleTimer >= sampleDelay + 500) && onlyTouched) {
    userTouched = false;
    sampleInit = false;
    showAllCities1 = false;
    showAllCities2 = false;
  }


  //If user spun do this
  if (userSpin && spinAmount < 2 && initialized) {
    if (spinAmount == 0) {
      ledFlicker();
      if (millis() - timer >= runTime && initialized) {
        DELAYVAL = 10;
        initialized = false;
        userSpin = false;
        spinAmount++;
        showAllCities1 = true;
      }
    }
    if (spinAmount == 1) {
      ledFlicker();
      if (millis() - timer >= runTime && initialized) {
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

void endCities() {
  pixels.setPixelColor(orangeCity, pixels.Color(140, 10, 0));
  pixels.setPixelColor(purpleCity, pixels.Color(100, 0, 140));
  // if (xIncrease) {
  //   x++;
  // }
  // if (xDecrease) {
  //   x--;
  // }
  // if (x >= 150) {
  //   xIncrease = false;
  //   xDecrease = true;
  // }
  // if (x <= 0) {
  //   xIncrease = true;
  //   xDecrease = false;
  // }
  pixels.show();
}

void serialPrinter() {
  Serial.print("Acc: ");
  Serial.print(accTotal);
  Serial.print("\t");
  Serial.print(spinAmount);
  Serial.print("\t");
  Serial.print(millis() - sampleTimer);
  Serial.print("\t");
  Serial.print(userSpin);
  Serial.print("\t");
  Serial.println();
}

void stationAction(String message, String topic) {
  Serial.print(message);
  if (message == "start") {
    startLoop = true;
    end = false;
    pixels.clear();
    Serial.println("starting loop");
  }
  else if (message == "reset") {
    Serial.println("resetting globe");
    ESP.restart();
    // spinAmount = 0;
    // startLoop = false;
    // citiesInit = false;
    // citiesRunInit = false;
    // initialized = false;
    // reInit = false;
    // x = 0;
    // xIncrease = true;
    // xDecrease = false;
    // userSpin = false;
    // //Showing leds stuff
    // showAllCities1 = false;
    // showAllCities2 = false;
    // occupied = false;
  } else {
    startLoop = false;
    end = true;
    pixels.clear();
    Serial.println("end cities shown");
  }
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
  if (end) {
    Serial.println("show end cities");
    endCities();
  }
}