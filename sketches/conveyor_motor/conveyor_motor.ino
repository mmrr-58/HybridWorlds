Code for sensing blocks
const int sensePin1 = 34;
const int sensePin2 = 35;
const int sensePin3 = 36;
const int sensePin4 = 39;
const int sensePin5 = 32;
const int sensePin6 = 33;
const int sensePin7 = 25;
const int sensePin8 = 26;
const int sensePin9 = 27;
const int sensePin10 = 14;
const int sensePin11 = 13;
const int sensePin12 = 12;
const int sensePin13 = 15;
const int sensePin14 = 4;
const int sensePin15 = 2;
const int sensePin16 = 0;

float v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16;

void setup() {
  Serial.begin(115200);
}

void voltReadings() {
  v1 = analogRead(sensePin1) * 3.3 / 4095.0;
  v2 = analogRead(sensePin2) * 3.3 / 4095.0;
  v3 = analogRead(sensePin3) * 3.3 / 4095.0;
  v4 = analogRead(sensePin4) * 3.3 / 4095.0;
  v5 = analogRead(sensePin5) * 3.3 / 4095.0;
  v6 = analogRead(sensePin6) * 3.3 / 4095.0;
  v7 = analogRead(sensePin7) * 3.3 / 4095.0;
  v8 = analogRead(sensePin8) * 3.3 / 4095.0;
  v9 = analogRead(sensePin9) * 3.3 / 4095.0;
  v10 = analogRead(sensePin10) * 3.3 / 4095.0;
  v11 = analogRead(sensePin11) * 3.3 / 4095.0;
  v12 = analogRead(sensePin12) * 3.3 / 4095.0;
  v13 = analogRead(sensePin13) * 3.3 / 4095.0;
  v14 = analogRead(sensePin14) * 3.3 / 4095.0;
  v15 = analogRead(sensePin15) * 3.3 / 4095.0;
  v16 = analogRead(sensePin16) * 3.3 / 4095.0;
}

String identifyType(float voltage) {
  if (voltage > 0.45 && voltage < 0.90)
    return "Safety";

  if (voltage > 1.0 && voltage < 1.25)
    return "Cost";

  if (voltage > 1.75 && voltage < 2.30)
    return "Healthcare";

  if (voltage > 2.40 && voltage < 2.70)
    return "Pollution";

  return "Unknown";
}

void serialPrinter(){
  Serial.print("Pin 34: ");
  Serial.print(v1);
  Serial.print(" V -> ");
  Serial.println(identifyType(v1));

  Serial.print("Pin 35: ");
  Serial.print(v2);
  Serial.print(" V -> ");
  Serial.println(identifyType(v2));

  Serial.print("Pin 36: ");
  Serial.print(v3);
  Serial.print(" V -> ");
  Serial.println(identifyType(v3));

  Serial.print("Pin 39: ");
  Serial.print(v4);
  Serial.print(" V -> ");
  Serial.println(identifyType(v4));

  Serial.print("Pin 32: ");
  Serial.print(v5);
  Serial.print(" V -> ");
  Serial.println(identifyType(v5));

  Serial.print("Pin 33: ");
  Serial.print(v6);
  Serial.print(" V -> ");
  Serial.println(identifyType(v6));

  Serial.print("Pin 25: ");
  Serial.print(v7);
  Serial.print(" V -> ");
  Serial.println(identifyType(v7));

  Serial.print("Pin 26: ");
  Serial.print(v8);
  Serial.print(" V -> ");
  Serial.println(identifyType(v8));
  
  Serial.print("Pin 27: ");
  Serial.print(v9);
  Serial.print(" V -> ");
  Serial.println(identifyType(v9));

  Serial.print("Pin 14: ");
  Serial.print(v10);
  Serial.print(" V -> ");
  Serial.println(identifyType(v10));

  Serial.print("Pin 13: ");
  Serial.print(v11);
  Serial.print(" V -> ");
  Serial.println(identifyType(v11));

  Serial.print("Pin 12: ");
  Serial.print(v12);
  Serial.print(" V -> ");
  Serial.println(identifyType(v12));

  Serial.print("Pin 15: ");
  Serial.print(v13);
  Serial.print(" V -> ");
  Serial.println(identifyType(v13));

  Serial.print("Pin 5: ");
  Serial.print(v14);
  Serial.print(" V -> ");
  Serial.println(identifyType(v14));

  Serial.print("Pin 2: ");
  Serial.print(v15);
  Serial.print(" V -> ");
  Serial.println(identifyType(v15));

  Serial.print("Pin 0: "); 
  Serial.print(v16);
  Serial.print(" V -> ");
  Serial.println(identifyType(v16));


    // Serial.print("34 raw = ");
    // Serial.println(analogRead(34));
    // Serial.print("35 raw = ");
    // Serial.println(analogRead(35));
    // Serial.print("36 raw = ");
    // Serial.println(analogRead(36));
    // Serial.print("39 raw = ");
    // Serial.println(analogRead(39));

    Serial.println("----------------");
}

void loop() {
  voltReadings();
  serialPrinter();
  delay(500);
}