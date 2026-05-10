// Humidity controller for Adafruit ESP32 Feather V2
// SHT40 + DFRobot Gravity MOSFET (active-low), fully automatic.
//
// Behaviour:
//   RH <  78.0 %  -> MOSFET ON  (mister runs)
//   RH >= 80.0 %  -> MOSFET OFF
//   between 78 and 80 -> hold current state (hysteresis prevents flicker)
//   sensor error / sensor missing -> MOSFET OFF (fail-safe)
//
// Pins (Feather V2):
//   MOSFET SIG (purple) -> 27
//   MOSFET VCC (blue)   -> 3V
//   MOSFET GND (gray)   -> GND
//   SHT40  VCC (red)    -> 3V
//   SHT40  GND (blue)   -> GND
//   SHT40  SDA (white)  -> SDA  (GPIO22)
//   SHT40  SCL (orange) -> SCL  (GPIO20)
//
// Library: Adafruit SHT4x (Library Manager)
//
// Serial: 115200 baud, line ending "Newline".
//   s -> show status
//   h -> help

#include <Wire.h>
#include "Adafruit_SHT4x.h"

// ---------- Configuration ----------
const uint8_t MOSFET_PIN = 27;
const bool    MOSFET_ACTIVE_LOW = true;

const float HUMIDITY_LIMIT_HIGH = 80.0;  // >= -> OFF
const float HUMIDITY_LIMIT_LOW  = 78.0;  // <   -> ON allowed

const uint32_t SAMPLE_INTERVAL_MS = 2000;

// ---------- Runtime state ----------
Adafruit_SHT4x sht4;
bool sensorOk = false;

// true = blocked (too humid or sensor failure). Boot default: blocked.
bool humidityBlocks = true;
bool currentMosfetOn = false;

uint32_t lastSampleMs = 0;
float lastRh = NAN;
float lastTemp = NAN;

// ---------- Helpers ----------
void writeMosfet(bool on) {
  uint8_t level;
  if (MOSFET_ACTIVE_LOW) {
    level = on ? LOW : HIGH;
  } else {
    level = on ? HIGH : LOW;
  }
  digitalWrite(MOSFET_PIN, level);
}

void applyMosfetIfChanged(bool desiredOn) {
  if (desiredOn == currentMosfetOn) return;
  currentMosfetOn = desiredOn;
  writeMosfet(currentMosfetOn);
  Serial.print(">> MOSFET ");
  Serial.println(currentMosfetOn ? "ON" : "OFF");
}

void printStatus() {
  Serial.print("[status] RH=");
  if (isnan(lastRh)) {
    Serial.print("--.--");
  } else {
    Serial.print(lastRh, 2);
  }
  Serial.print(" %  T=");
  if (isnan(lastTemp)) {
    Serial.print("--.--");
  } else {
    Serial.print(lastTemp, 2);
  }
  Serial.print(" C  block=");
  Serial.print(humidityBlocks ? "YES" : "no ");
  Serial.print("  MOSFET=");
  Serial.println(currentMosfetOn ? "ON" : "OFF");
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  s -> status");
  Serial.println("  h -> help");
}

// ---------- Setup ----------
void setup() {
  // Force MOSFET pin to OFF level before pinMode (active-low: HIGH = off).
  uint8_t idleLevel = MOSFET_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(MOSFET_PIN, idleLevel);
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, idleLevel);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Humidity controller starting...");

#if defined(I2C_POWER)
  // Feather V2: enable the I2C / STEMMA QT power rail.
  pinMode(I2C_POWER, OUTPUT);
  digitalWrite(I2C_POWER, HIGH);
  delay(10);
#endif

  Wire.begin();

  if (sht4.begin()) {
    sensorOk = true;
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    Serial.print("SHT4x found, serial 0x");
    Serial.println(sht4.readSerial(), HEX);
  } else {
    sensorOk = false;
    Serial.println("SHT40 NOT found - MOSFET stays OFF (fail-safe).");
  }

  Serial.print("Thresholds: block at RH >= ");
  Serial.print(HUMIDITY_LIMIT_HIGH, 1);
  Serial.print(" %, release at RH < ");
  Serial.print(HUMIDITY_LIMIT_LOW, 1);
  Serial.println(" %");

  printHelp();
  printStatus();
}

// ---------- Sensor / hysteresis ----------
void readSensorIfDue() {
  uint32_t now = millis();
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  if (!sensorOk) {
    humidityBlocks = true;
    return;
  }

  sensors_event_t humidity, temp;
  if (!sht4.getEvent(&humidity, &temp)) {
    Serial.println("Sensor read failed - blocking MOSFET.");
    humidityBlocks = true;
    return;
  }

  lastTemp = temp.temperature;
  lastRh = humidity.relative_humidity;

  Serial.print("T = ");
  Serial.print(lastTemp, 2);
  Serial.print(" C   RH = ");
  Serial.print(lastRh, 2);
  Serial.println(" %");

  // Hysteresis: block above HIGH, release below LOW.
  if (lastRh >= HUMIDITY_LIMIT_HIGH) {
    if (!humidityBlocks) {
      Serial.println(">> Humidity limit reached - blocking MOSFET.");
    }
    humidityBlocks = true;
  } else if (lastRh < HUMIDITY_LIMIT_LOW) {
    if (humidityBlocks) {
      Serial.println(">> Humidity dropped below release threshold - allowed again.");
    }
    humidityBlocks = false;
  }
  // Between LOW and HIGH: keep current block state.
}

// ---------- Serial commands ----------
void readSerial() {
  while (Serial.available()) {
    int c = Serial.read();
    switch (c) {
      case 's':
      case 'S':
        printStatus();
        break;
      case 'h':
      case 'H':
        printHelp();
        break;
      case '\r':
      case '\n':
      case ' ':
        break;
      default:
        Serial.print("Unknown command: ");
        Serial.println((char)c);
        break;
    }
  }
}

// ---------- Loop ----------
void loop() {
  readSensorIfDue();
  readSerial();

  bool desired = !humidityBlocks;
  applyMosfetIfChanged(desired);
}
