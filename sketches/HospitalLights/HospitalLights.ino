#include <Adafruit_NeoPixel.h>

#define PIN 5
#define NUMPIXELS 21

Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int currentAnimation = 0;

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();

  randomSeed(analogRead(0));
}

void loop() {

  // Receive animation number (0-4)
  if (Serial.available()) {
    int received = Serial.parseInt();

    if (received >= 0 && received <= 4) {
      currentAnimation = received;
      strip.clear();
      strip.show();

      Serial.print("Animation: ");
      Serial.println(currentAnimation);
    }
  }

  switch (currentAnimation) {
    case 0:
      breathingYellow();
      break;

    case 1:
      nightShift();
      break;

    case 2:
      busyHospital();
      break;

    case 3:
      emergencyMode();
      break;

    case 4:
      powerSweep();
      break;
  }
}

// ------------------------
// Animation 0
// Calm breathing hospital
// ------------------------
void breathingYellow() {

  static int brightness = 20;
  static int direction = 2;

  brightness += direction;

  if (brightness >= 150 || brightness <= 20) {
    direction = -direction;
  }

  strip.fill(strip.Color(brightness, brightness, 0));
  strip.show();

  delay(20);
}

// ------------------------
// Animation 1
// Night shift
// ------------------------
void nightShift() {

  int pixel = random(NUMPIXELS);

  if (random(100) < 50)
    strip.setPixelColor(pixel, strip.Color(60, 50, 0));
  else
    strip.setPixelColor(pixel, 0);

  strip.show();

  delay(300);
}

// ------------------------
// Animation 2
// Busy hospital
// ------------------------
void busyHospital() {

  int pixel = random(NUMPIXELS);

  int b = random(80, 255);

  strip.setPixelColor(pixel, strip.Color(b, b * 0.8, 0));

  strip.show();

  delay(80);
}

// ------------------------
// Animation 3
// Emergency entrance
// ------------------------
void emergencyMode() {

  static bool flash = false;

  for (int i = 0; i < NUMPIXELS; i++) {

    if (i < 5) {
      if (flash)
        strip.setPixelColor(i, strip.Color(255, 0, 0));
      else
        strip.setPixelColor(i, strip.Color(255, 180, 0));
    } else {
      strip.setPixelColor(i, strip.Color(50, 50, 0));
    }
  }

  strip.show();

  flash = !flash;

  delay(150);
}

// ------------------------
// Animation 4
// Power sweep
// ------------------------
void powerSweep() {

  static int pos = 0;

  strip.clear();

  strip.setPixelColor(pos, strip.Color(255, 255, 0));

  if (pos > 0)
    strip.setPixelColor(pos - 1, strip.Color(100, 100, 0));

  if (pos > 1)
    strip.setPixelColor(pos - 2, strip.Color(30, 30, 0));

  strip.show();

  pos++;

  if (pos >= NUMPIXELS)
    pos = 0;

  delay(50);
}