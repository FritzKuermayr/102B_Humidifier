// SHT40-Test fuer Adafruit ESP32 Feather V2
// Liest Temperatur und Luftfeuchte alle 2 s und gibt sie ueber Serial aus.
//
// I2C-Pins (Feather V2 Default):
//   SDA = 22, SCL = 20
// -> kollidiert NICHT mit dem MOSFET-Sketch (GPIO27).
//
// SHT40 -> Feather:
//   rot    (VCC) -> 3V
//   blau   (GND) -> GND
//   weiss  (SDA) -> SDA  (GPIO22)
//   orange (SCL) -> SCL  (GPIO20)
//
// Alternativ: Adafruit STEMMA QT Kabel direkt in die QT-Buchse
// auf dem Feather V2 stecken -> selbe Belegung, ohne Verkabelung.
//
// Library: "Adafruit SHT4x Library"
//   Arduino IDE -> Tools -> Manage Libraries -> "Adafruit SHT4x" -> Install
//   (zieht "Adafruit BusIO" und "Adafruit Unified Sensor" als Abhaengigkeit mit)

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
  // Feather V2: schaltet die I2C-/STEMMA-QT-Versorgung ein.
  pinMode(I2C_POWER, OUTPUT);
  digitalWrite(I2C_POWER, HIGH);
  delay(10);
#endif

  Wire.begin();

  if (!sht4.begin()) {
    Serial.println("SHT40 not found.");
    Serial.println("Check wiring: SDA=22 (weiss), SCL=20 (orange), VCC=3V (rot), GND (blau).");
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
