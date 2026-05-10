// MOSFET test for Adafruit ESP32 Feather V2
// Manual control of GPIO27 (-> Gravity MOSFET SIG, purple wire) via Serial.
// Purpose: verify that the MOSFET cleanly switches the external supply.
//
// MOSFET module -> Feather:
//   purple (SIG) -> 27
//   blue   (VCC) -> 3V
//   gray   (GND) -> GND
//
// NOTE: This module is active-low.
//   SIG LOW  -> MOSFET ON  (status LED on)
//   SIG HIGH -> MOSFET OFF (status LED off)
// The code below inverts the polarity so "1" still means MOSFET ON.
//
// Serial commands (115200 baud, line ending "Newline"):
//   1  -> MOSFET ON  (GPIO27 LOW)
//   0  -> MOSFET OFF (GPIO27 HIGH)
//   t  -> toggle
//   s  -> print current status
//   h  -> help
//
// For resistance measurement:
//   * Disconnect the external supply BEFORE measuring with an ohmmeter.
//     Ohms mode injects a small test voltage and only gives sensible
//     readings when no foreign voltage is present on the rail.
//   * Probe between MOSFET VOUT (+) and the load-side VOUT (-) / GND.
//   * Expected values:
//       ON  -> very low (typ. < 1 ohm, the FET's RDS_on)
//       OFF -> very high / OL (multimeter shows "open")

const uint8_t MOSFET_PIN = 27;
const bool MOSFET_ACTIVE_LOW = true;  // see header note

bool mosfetState = false;

void applyState() {
  // Active-low: ON -> pin LOW, OFF -> pin HIGH.
  bool pinHigh = MOSFET_ACTIVE_LOW ? !mosfetState : mosfetState;
  digitalWrite(MOSFET_PIN, pinHigh ? HIGH : LOW);
  Serial.print("MOSFET ");
  Serial.print(mosfetState ? "ON " : "OFF");
  Serial.print("  (GPIO27 ");
  Serial.print(pinHigh ? "HIGH" : "LOW");
  Serial.println(")");
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  1 -> MOSFET ON");
  Serial.println("  0 -> MOSFET OFF");
  Serial.println("  t -> toggle");
  Serial.println("  s -> status");
  Serial.println("  h -> help");
}

void setup() {
  // Idle level = MOSFET OFF. For active-low this is HIGH.
  uint8_t idleLevel = MOSFET_ACTIVE_LOW ? HIGH : LOW;

  // Write idle level first, then set to OUTPUT -> no boot glitch.
  digitalWrite(MOSFET_PIN, idleLevel);
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, idleLevel);

  Serial.begin(115200);
  delay(200);
  Serial.println("MOSFET manual test ready.");
  printHelp();
  applyState();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  int c = Serial.read();
  switch (c) {
    case '1':
      mosfetState = true;
      applyState();
      break;
    case '0':
      mosfetState = false;
      applyState();
      break;
    case 't':
    case 'T':
      mosfetState = !mosfetState;
      applyState();
      break;
    case 's':
    case 'S':
      applyState();
      break;
    case 'h':
    case 'H':
      printHelp();
      break;
    case '\r':
    case '\n':
    case ' ':
      // Ignore newlines, CR, spaces.
      break;
    default:
      Serial.print("Unknown command: ");
      Serial.println((char)c);
      printHelp();
      break;
  }
}
