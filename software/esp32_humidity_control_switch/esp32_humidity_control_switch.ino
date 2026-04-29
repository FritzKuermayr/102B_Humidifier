// Humidity controller WITH switch, for Adafruit ESP32 Feather V2.
// SHT40 + DFRobot Gravity MOSFET (active-low) + toggle/slide switch.
//
// Behaviour:
//   The switch position directly maps to "user request":
//     switch closed (pin LOW)  -> userWantsOn = YES
//     switch open   (pin HIGH) -> userWantsOn = no
//   MOSFET is ON exactly when:
//     userWantsOn = true  AND  humidityBlocks = false  (hysteresis allows it)
//
//   This works for both a toggle (latching) and a slide switch: every
//   physical position change immediately changes the state. (For a
//   momentary push-button the mister would only run while the button is
//   held - use the Serial 't' command instead in that case.)
//
//   Hysteresis:
//     RH >= 80.0 %  -> block
//     RH <  78.0 %  -> release
//     between -> hold current block state
//
//   Sensor error / sensor missing -> MOSFET forced OFF (fail-safe).
//
// Pins (Feather V2):
//   MOSFET SIG (purple) -> 27
//   MOSFET VCC (blue)   -> 3V
//   MOSFET GND (gray)   -> GND  (left side)
//   SHT40  VCC (red)    -> 3V
//   SHT40  GND (blue)   -> GND  (right side)
//   SHT40  SDA (white)  -> SDA  (GPIO22)
//   SHT40  SCL (orange) -> SCL  (GPIO20)
//   Switch              -> GPIO33  and  GND  (INPUT_PULLUP, closed = LOW)
//
// Note about the switch GND:
//   The Feather V2 only has 2 GND pins on its headers, both already in
//   use. Twist the switch GND together with the MOSFET GND or the SHT40
//   GND and plug both leads into the same GND pin - electrically
//   identical.
//
// Library: Adafruit SHT4x (Library Manager).
//
// Serial: 115200 baud, line ending "Newline".
//   t -> toggle userWantsOn (works without a hardware switch)
//   s -> show status
//   h -> help

#include <Wire.h>
#include "Adafruit_SHT4x.h"

// ---------- Configuration ----------
const uint8_t MOSFET_PIN = 27;
const uint8_t BUTTON_PIN = 33;
const bool    MOSFET_ACTIVE_LOW = true;

const float HUMIDITY_LIMIT_HIGH = 80.0;  // >= -> block
const float HUMIDITY_LIMIT_LOW  = 78.0;  // <   -> release

const uint32_t SAMPLE_INTERVAL_MS = 2000;
const uint32_t DEBOUNCE_MS = 40;

// ---------- Runtime state ----------
Adafruit_SHT4x sht4;
bool sensorOk = false;

bool userWantsOn = false;
bool humidityBlocks = true;     // boot default: blocked until first valid reading
bool currentMosfetOn = false;

uint32_t lastSampleMs = 0;
float lastRh = NAN;
float lastTemp = NAN;

int lastButtonReading = HIGH;
int stableButtonState = HIGH;
uint32_t lastButtonChangeMs = 0;

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
  Serial.print(" C  userWantsOn=");
  Serial.print(userWantsOn ? "YES" : "no ");
  Serial.print("  block=");
  Serial.print(humidityBlocks ? "YES" : "no ");
  Serial.print("  MOSFET=");
  Serial.println(currentMosfetOn ? "ON" : "OFF");
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  t -> toggle userWantsOn");
  Serial.println("  s -> status");
  Serial.println("  h -> help");
}

// ---------- Setup ----------
void setup() {
  // Force MOSFET OFF before pinMode takes effect (active-low: HIGH = off).
  uint8_t idleLevel = MOSFET_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(MOSFET_PIN, idleLevel);
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, idleLevel);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Humidity controller (with switch) starting...");

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
}

// ---------- Switch ----------
// After debounce the switch position is mapped directly onto userWantsOn.
// This makes both toggle/latching and slide switches usable: each
// position change leads to exactly one state change.
void readButton() {
  int reading = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonChangeMs = now;
    return;
  }

  if ((now - lastButtonChangeMs) < DEBOUNCE_MS) return;

  if (reading != stableButtonState) {
    stableButtonState = reading;
    bool desired = (stableButtonState == LOW);  // LOW = closed = ON
    if (desired != userWantsOn) {
      userWantsOn = desired;
      Serial.print("Switch -> userWantsOn=");
      Serial.println(userWantsOn ? "YES" : "no");
    }
  }
}

// ---------- Serial commands ----------
void readSerial() {
  while (Serial.available()) {
    int c = Serial.read();
    switch (c) {
      case 't':
      case 'T':
        userWantsOn = !userWantsOn;
        Serial.print("Serial toggle -> userWantsOn=");
        Serial.println(userWantsOn ? "YES" : "no");
        break;
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
  readButton();
  readSerial();

  bool desired = userWantsOn && !humidityBlocks;
  applyMosfetIfChanged(desired);
}
