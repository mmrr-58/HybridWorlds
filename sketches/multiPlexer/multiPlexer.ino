#include <light_CD74HC4067.h>
#include <Adafruit_NeoPixel.h>

const int inS0 = 0;
const int inS1 = 1;
const int inS2 = 2;
const int inS3 = 10;
const int inSig = 4;
const int outS0 = 5;
const int outS1 = 6;
const int outS2 = 7;
const int outS3 = 8;
const int outSig = 9;

CD74HC4067 inMux(inS0, inS1, inS2, inS3);
CD74HC4067 outMux(outS0, outS1, outS2, outS3);

const int NUMPIXELS = 2;
Adafruit_NeoPixel pixels(NUMPIXELS, outSig, NEO_GRB + NEO_KHZ800);

String buildingSpots[16] = { "" };
float voltage;


void setup() {
  Serial.begin(9600);
  pixels.begin();
  pixels.clear();
  pixels.show();

  pinMode(inSig, INPUT);
  pinMode(outSig, OUTPUT);
}

void identifyWriteType() {
  for (int i = 0; i < 16; i++) {
    inMux.channel(i);
    outMux.channel(i);
    delay(1);
    voltage = analogRead(inSig) * 3.3 / 4095.0;

    if (voltage > 0.45 && voltage < 0.90) {
      buildingSpots[i] = "safety";
      pixels.setPixelColor(0, pixels.Color(150, 0, 150));
      pixels.setPixelColor(1, pixels.Color(150, 0, 150));
      pixels.show();

    } else if (voltage > 1.0 && voltage < 1.25) {
      buildingSpots[i] = "cost";
      pixels.setPixelColor(0, pixels.Color(0, 150, 0));
      pixels.setPixelColor(1, pixels.Color(0, 150, 0));
      pixels.show();
    } else if (voltage > 1.75 && voltage < 2.30) {
      buildingSpots[i] = "pollution";
      pixels.setPixelColor(0, pixels.Color(0, 0, 150));
      pixels.setPixelColor(1, pixels.Color(0, 0, 150));
      pixels.show();
    } else if (voltage > 2.40 && voltage < 2.70) {
      buildingSpots[i] = "healthcare";
      pixels.setPixelColor(0, pixels.Color(150, 0, 0));
      pixels.setPixelColor(1, pixels.Color(150, 0, 0));
      pixels.show();
    } else buildingSpots[i] = "unknown";

    Serial.print("Channel " + String(i) + ": " + String(voltage));
    Serial.print("\t");
    Serial.print(buildingSpots[i]);
    Serial.println();
    delay(500);
  }
}

//This function useless cuz you can just write values on outMux in function above

// void writeToType() {
//   for (int i = 0; i < 16; i++) {
//     outMux.channel(i);
//     delay(1);
//     if (buildingSpots[i] == "safety") {
//       //ledVal = message[0]

//     } else if (buildingSpots[i] == "cost")
//   }
// }



void loop() {
  identifyWriteType();
}
