// Humidity-Controller MIT Schalter, fuer Adafruit ESP32 Feather V2.
// SHT40 + DFRobot Gravity MOSFET (active-low) + Taster.
//
// Verhalten:
//   Schalter-Position bestimmt direkt die "User-Anforderung":
//     Schalter geschlossen (Pin LOW)  -> userWantsOn = YES
//     Schalter offen       (Pin HIGH) -> userWantsOn = no
//   MOSFET ist genau dann AN, wenn:
//     userWantsOn  = true   UND
//     humidityBlocks = false  (Hysterese erlaubt es)
//
//   Damit funktioniert sowohl ein klassischer Kippschalter als auch ein
//   Schiebe-Schalter: jede Schalter-Bewegung loest sofort eine Zustands-
//   aenderung aus. (Bei einem reinen Push-Button wuerde der Mistifyer nur
//   waehrend der Druckdauer laufen - dafuer das Serial-Kommando 't' nutzen.)
//
//   Hysterese:
//     RH >= 80.0 %  -> sperren
//     RH <  78.0 %  -> wieder freigeben
//     dazwischen    -> aktuellen Sperr-Zustand halten
//
//   Sensor-Fehler / -Ausfall -> MOSFET zwangsweise AUS (fail-safe).
//
// Pins (Feather V2):
//   MOSFET SIG (lila)   -> 27
//   MOSFET VCC (blau)   -> 3V
//   MOSFET GND (grau)   -> GND  (links)
//   SHT40 VCC  (rot)    -> 3V
//   SHT40 GND  (blau)   -> GND  (rechts)
//   SHT40 SDA  (weiss)  -> SDA  (GPIO22)
//   SHT40 SCL  (orange) -> SCL  (GPIO20)
//   Taster              -> GPIO33  und  GND  (Pulldown gegen GND, Pin gezogen via INPUT_PULLUP)
//
// Hinweis zum Taster-GND:
//   Der Feather V2 hat nur 2 GND-Pins. Beide sind oben schon belegt.
//   Den Taster-GND einfach mit dem MOSFET-GND oder SHT40-GND verzwirbeln und
//   gemeinsam in den jeweiligen GND-Pin stecken - elektrisch identisch.
//
// Library: Adafruit SHT4x (Library Manager).
//
// Serial: 115200, Newline.
//   t -> userWantsOn umschalten (auch ohne Taster nutzbar)
//   s -> Status anzeigen
//   h -> Hilfe

#include <Wire.h>
#include "Adafruit_SHT4x.h"

// ---------- Konfiguration ----------
const uint8_t MOSFET_PIN = 27;
const uint8_t BUTTON_PIN = 33;
const bool    MOSFET_ACTIVE_LOW = true;

const float HUMIDITY_LIMIT_HIGH = 80.0;  // >= -> sperren
const float HUMIDITY_LIMIT_LOW  = 78.0;  // <   -> wieder erlauben

const uint32_t SAMPLE_INTERVAL_MS = 2000;
const uint32_t DEBOUNCE_MS = 40;

// ---------- Laufzeit-Status ----------
Adafruit_SHT4x sht4;
bool sensorOk = false;

bool userWantsOn = false;
bool humidityBlocks = true;     // Boot-Default: gesperrt bis erste gueltige Messung
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
  // MOSFET sicher AUS bevor pinMode greift (active-low: HIGH = aus).
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
  // Feather V2: I2C-/STEMMA-QT-Versorgung einschalten.
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

// ---------- Sensor / Hysterese ----------
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

// ---------- Schalter ----------
// Nach Debounce wird die Schalter-Position direkt auf userWantsOn abgebildet.
// Das macht sowohl Toggle- als auch Schiebe-Schalter brauchbar: jede
// Positions-Aenderung fuehrt zu genau einer Zustandsaenderung.
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
    bool desired = (stableButtonState == LOW);  // LOW = Schalter geschlossen = AN
    if (desired != userWantsOn) {
      userWantsOn = desired;
      Serial.print("Switch -> userWantsOn=");
      Serial.println(userWantsOn ? "YES" : "no");
    }
  }
}

// ---------- Serial-Befehle ----------
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
