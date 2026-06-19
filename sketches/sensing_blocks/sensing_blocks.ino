#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  lcd.init();      // or lcd.begin(16, 2) depending on library
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Enter 1-4");

  // I2C Scanner
  Serial.println("Scanning...");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(address, HEX);
    }
  }
}

void loop() {
  if (Serial.available()) {
    int number = Serial.parseInt();

    if (number >= 1 && number <= 4) {
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("Number:");

      lcd.setCursor(0, 1);
      lcd.print(number);

      Serial.print("Displayed: ");
      Serial.println(number);
    }
    else {
      Serial.println("Invalid! Enter 1, 2, 3, or 4.");
    }

    while (Serial.available()) {
      Serial.read();
    }
  }
}