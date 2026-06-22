
#include <Adafruit_NeoPixel.h>
#define PIN 32
#define NUMPIXELS 3
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 20  // Time (in milliseconds) to pause between pixels

int turnOn;
int turnOn2;
int turnOn3;
int turnOn4;
int turnOn5;
int randGreen;
int randBlue;
int randRed;

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(15));
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
}

void ledFlicker() {
  pixels.clear();
  
  turnOn = random(0, NUMPIXELS);
  // turnOn2 = random(0, NUMPIXELS);
  // turnOn3 = random(0, NUMPIXELS);
  // turnOn4 = random(0, NUMPIXELS);
  // turnOn5 = random(0, NUMPIXELS);
  randGreen = random(150, 255);
  randBlue = random(150, 255);
  randRed = random(0, 100);
  pixels.setPixelColor(turnOn, pixels.Color(randRed, randGreen, randBlue));
  // pixels.setPixelColor(turnOn2, pixels.Color(randRed, randGreen, randBlue));
  // pixels.setPixelColor(turnOn3, pixels.Color(randRed, randGreen, randBlue));
  // pixels.setPixelColor(turnOn4, pixels.Color(randRed, randGreen, randBlue));
  // pixels.setPixelColor(turnOn5, pixels.Color(randRed, randGreen, randBlue));

  Serial.print(randRed);
  Serial.print("  ");
  Serial.print(randGreen);
  Serial.print("  ");
  Serial.println(randBlue);

  pixels.show();
  delay(DELAYVAL);
}

void loop() {
  ledFlicker();
  // pixels.clear();  // Set all pixel colors to 'off'

  // // The first NeoPixel in a strand is #0, second is 1, all the way up
  // // to the count of pixels minus one.
  // for (int i = 0; i < NUMPIXELS; i++) {  // For each pixel...

  //   // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
  //   // Here we're using a moderately bright green color:
  //   pixels.setPixelColor(i, pixels.Color(0, 150, 0));

  //   pixels.show();  // Send the updated pixel colors to the hardware.

  //   delay(DELAYVAL);  // Pause before next pass through loop
  // }
}
