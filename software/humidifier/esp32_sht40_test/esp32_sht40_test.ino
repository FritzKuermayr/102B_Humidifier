// SHT40 test for Adafruit ESP32 Feather V2
// Reads temperature and relative humidity every 2 s and prints to Serial.
//
// I2C pins (Feather V2 default):
//   SDA = 22, SCL = 20
// -> does NOT collide with the MOSFET sketch (GPIO27).
//
// SHT40 -> Feather:
//   red    (VCC) -> 3V
//   blue   (GND) -> GND
//   white  (SDA) -> SDA  (GPIO22)
//   orange (SCL) -> SCL  (GPIO20)
//
// Alternative: plug an Adafruit STEMMA QT cable directly into the QT
// connector on the Feather V2 -> same wiring, no soldering.
//
// Library: "Adafruit SHT4x Library"
//   Arduino IDE -> Tools -> Manage Libraries -> "Adafruit SHT4x" -> Install
//   (pulls in "Adafruit BusIO" and "Adafruit Unified Sensor" as deps)

#include <Wire.h>
#include "Adafruit_SHT4x.h"

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

const uint32_t SAMPLE_INTERVAL_MS = 2000;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("SHT40 test starting...");

#if defined(I2C_POWER)
  // Feather V2: enable the I2C / STEMMA QT power rail.
  pinMode(I2C_POWER, OUTPUT);
  digitalWrite(I2C_POWER, HIGH);
  delay(10);
#endif

  Wire.begin();

  if (!sht4.begin()) {
    Serial.println("SHT40 not found.");
    Serial.println("Check wiring: SDA=22 (white), SCL=20 (orange), VCC=3V (red), GND (blue).");
    while (true) {
      delay(1000);
    }
  }

  Serial.print("Found SHT4x, serial 0x");
  Serial.println(sht4.readSerial(), HEX);

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  Serial.println("Streaming readings...");
}

void loop() {
  sensors_event_t humidity, temp;

  uint32_t t0 = millis();
  bool ok = sht4.getEvent(&humidity, &temp);
  uint32_t dt = millis() - t0;

  if (!ok) {
    Serial.println("Read failed");
  } else {
    Serial.print("T = ");
    Serial.print(temp.temperature, 2);
    Serial.print(" C    RH = ");
    Serial.print(humidity.relative_humidity, 2);
    Serial.print(" %    (");
    Serial.print(dt);
    Serial.println(" ms)");
  }

  delay(SAMPLE_INTERVAL_MS);
}
