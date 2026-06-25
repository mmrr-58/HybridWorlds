#include <light_CD74HC4067.h>

const int inSig = 34;
// const int inS0 = 25;
// const int inS1 = 26;
// const int inS2 = 27;
// const int inS3 = 33;
const int outSig = 13;
const int outS0 = 25;
const int outS1 = 26;
const int outS2 = 27;
const int outS3 = 33;

//CD74HC4067 inMux(inS0, inS1, inS2, inS3);
CD74HC4067 outMux(outS0, outS1, outS2, outS3);

String buildingSpots[16] = { "" };
float voltage;


void setup() {
  Serial.begin(9600);

  pinMode(inSig, INPUT);
  pinMode(outSig, OUTPUT);
  pinMode(12, OUTPUT);
}

void identifyWriteType() {
  digitalWrite(12, HIGH);
  for (int i = 0; i < 16; i++) {
    //inMux.channel(i);
    outMux.channel(i);
    delay(1);
    //voltage = analogRead(inSig) * 3.3 / 4095.0;
    voltage = 0.5;
    if (voltage > 0.45 && voltage < 0.90){
      buildingSpots[i] = "safety";
      digitalWrite(outSig, HIGH);
  }
    else if (voltage > 1.0 && voltage < 1.25)
      buildingSpots[i] = "cost";

    else if (voltage > 1.75 && voltage < 2.30)
      buildingSpots[i] = "pollution";

    else if (voltage > 2.40 && voltage < 2.70)
      buildingSpots[i] = "healthcare";

    else buildingSpots[i] = "unknown";

    Serial.print("Channel " + String(i) + ": " + String(voltage));
    Serial.print("\t");
    Serial.print(buildingSpots[i]);
    Serial.print("\t");
    Serial.print(analogRead(inSig));
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
