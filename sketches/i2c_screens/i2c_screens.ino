#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCDs
LiquidCrystal_I2C lcd1(0x23, 16, 2);
LiquidCrystal_I2C lcd2(0x24, 16, 2);
LiquidCrystal_I2C lcd3(0x25, 16, 2);
LiquidCrystal_I2C lcd4(0x26, 16, 2);
LiquidCrystal_I2C lcd5(0x27, 16, 2);

// Prices for levels 0-4
float rice[5]    = {5.00, 4.50, 4.00, 3.50, 3.00};
float oil[5]     = {8.00, 7.20, 6.40, 5.60, 4.80};
float chicken[5] = {12.0, 10.8, 9.60, 8.40, 7.20};
float milk[5]    = {4.00, 3.60, 3.20, 2.80, 2.40};
float carrot[5]  = {3.00, 2.70, 2.40, 2.10, 1.80};

void updateDisplays(int level)
{
  // Rice
  lcd1.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("Rice");
  lcd1.setCursor(0, 1);
  lcd1.print("EUR ");
  lcd1.print(rice[level], 2);

  // Oil
  lcd2.clear();
  lcd2.setCursor(0, 0);
  lcd2.print("Oil");
  lcd2.setCursor(0, 1);
  lcd2.print("EUR ");
  lcd2.print(oil[level], 2);

  // Chicken
  lcd3.clear();
  lcd3.setCursor(0, 0);
  lcd3.print("Chicken");
  lcd3.setCursor(0, 1);
  lcd3.print("EUR ");
  lcd3.print(chicken[level], 2);

  // Milk
  lcd4.clear();
  lcd4.setCursor(0, 0);
  lcd4.print("Milk");
  lcd4.setCursor(0, 1);
  lcd4.print("EUR ");
  lcd4.print(milk[level], 2);

  // Carrots
  lcd5.clear();
  lcd5.setCursor(0, 0);
  lcd5.print("Carrots");
  lcd5.setCursor(0, 1);
  lcd5.print("EUR ");
  lcd5.print(carrot[level], 2);
}

void setup()
{
  Serial.begin(115200);

  // ESP32 I2C pins
  Wire.begin(21, 22);

  // Initialize LCDs
  lcd1.init();
  lcd1.backlight();

  lcd2.init();
  lcd2.backlight();

  lcd3.init();
  lcd3.backlight();

  lcd4.init();
  lcd4.backlight();

  lcd5.init();
  lcd5.backlight();

  // Optional: I2C Scanner
  Serial.println("Scanning I2C bus...");

  for (byte address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0)
    {
      Serial.print("Found I2C device at 0x");
      Serial.println(address, HEX);
    }
  }

  Serial.println("Send a number 0-4");

  // Start at level 0
  updateDisplays(0);
}

void loop()
{
  if (Serial.available())
  {
    int level = Serial.parseInt();

    if (level >= 0 && level <= 4)
    {
      updateDisplays(level);

      Serial.print("Price level set to: ");
      Serial.println(level);
    }
    else
    {
      Serial.println("Invalid input. Enter a number from 0 to 4.");
    }

    while (Serial.available())
    {
      Serial.read();
    }
  }
}

// #include <Wire.h>

// void setup() {
//   Serial.begin(115200);

//   Wire.begin(21, 22);

//   Serial.println();
//   Serial.println("I2C Scanner");
//   Serial.println("Scanning...");
// }

// void loop() {

//   byte count = 0;

//   for (byte address = 1; address < 127; address++) {

//     Wire.beginTransmission(address);

//     if (Wire.endTransmission() == 0) {

//       Serial.print("Found device at 0x");

//       if (address < 16)
//         Serial.print("0");

//       Serial.println(address, HEX);

//       count++;
//     }
//   }

//   if (count == 0) {
//     Serial.println("No I2C devices found");
//   } else {
//     Serial.print("Found ");
//     Serial.print(count);
//     Serial.println(" device(s)");
//   }

//   Serial.println("----------------");

//   delay(5000);
// }