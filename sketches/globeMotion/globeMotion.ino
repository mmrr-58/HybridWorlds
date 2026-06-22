#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include <Adafruit_NeoPixel.h>
const int PIN = 32;
const int NUMPIXELS = 16;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int DELAYVAL = 10;  // Time (in milliseconds) to pause between pixels

Adafruit_MPU6050 mpu;

float accTotal;
float accThreshold = 15;
int spinAmount = 0;
bool userSpin = false;

int selectedCity1;
int selectedCity2;


unsigned long timer;
int runTime = 5000;
bool initialized = false;

int turnOn;
int turnOn2;
int randGreen;
int randBlue;
int randRed;


void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(15));
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  pixels.show();

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
}

void accSensor() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  accTotal =
    sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z);
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

void userSpun() {
  if (!initialized) {
    timer = millis();
  }

  if (accTotal >= accThreshold) {
    userSpin = true;
    initialized = true;
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
      }
    }
    if (spinAmount == 1) {
      ledFlicker();
      if (millis() - timer >= runTime && initialized) {
        selectedCity2 = random(1, 16);
        initialized = false;
        userSpin = false;
        spinAmount++;
      }
    }
    pixels.clear();
    pixels.show();

    // if (spinAmount == 2){
    //   send message with values
    // }

    //if message received to reset installation{
    //spinAmount = 0;
    //}
  }
}


void serialPrinter() {
  Serial.print("Acc: ");
  Serial.print(accTotal);
  Serial.print("  ");
  Serial.print(millis());
  Serial.print("  ");
  Serial.print(millis() - timer);
  Serial.print("\t");
  Serial.print(spinAmount);
  Serial.print("  ");
  Serial.print(selectedCity1);
  Serial.print("  ");
  Serial.print(selectedCity2);
  Serial.println();
}

void loop() {
  accSensor();
  userSpun();
  serialPrinter();
}