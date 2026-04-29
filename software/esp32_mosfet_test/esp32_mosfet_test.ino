// MOSFET-Test fuer Adafruit ESP32 Feather V2
// Manuelle Steuerung von GPIO27 (-> Gravity MOSFET SIG, lila) ueber Serial.
// Zweck: Schalt-/Spannungstest am MOSFET-Ausgang.
//
// MOSFET-Modul -> Feather:
//   lila (SIG) -> 27
//   blau (VCC) -> 3V
//   grau (GND) -> GND
//
// HINWEIS: Dieses Modul ist active-low.
//   SIG LOW  -> MOSFET AN  (Status-LED leuchtet)
//   SIG HIGH -> MOSFET AUS (Status-LED aus)
// Die Logik im Code dreht das so, dass "1" = MOSFET AN bleibt.
//
// Serial-Kommandos (115200 Baud, "Newline" als Zeilenendung):
//   1  -> MOSFET AN  (GPIO27 HIGH)
//   0  -> MOSFET AUS (GPIO27 LOW)
//   t  -> umschalten
//   s  -> aktuellen Status ausgeben
//   h  -> Hilfe
//
// WICHTIG fuer die Widerstandsmessung:
//   * Externe Versorgungsspannung VOR der Messung abklemmen.
//     Multimeter im Ohm-Modus speist eine kleine Pruefspannung ein und
//     liefert nur dann sinnvolle Werte, wenn keine Fremdspannung anliegt.
//   * Multimeter zwischen MOSFET VOUT (+) und VOUT (-)/GND der Lastseite.
//   * Erwartung:
//       AN  -> sehr niedrig (typ. < 1 Ohm, RDS_on des FET)
//       AUS -> sehr hoch / OL (Multimeter zeigt offen an)

const uint8_t MOSFET_PIN = 27;
const bool MOSFET_ACTIVE_LOW = true;  // siehe Header-Kommentar

bool mosfetState = false;

void applyState() {
  // Bei active-low: ON -> Pin LOW, OFF -> Pin HIGH
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
  // Idle-Pegel = MOSFET AUS. Bei active-low ist das HIGH.
  uint8_t idleLevel = MOSFET_ACTIVE_LOW ? HIGH : LOW;

  // Erst Idle-Level schreiben, dann auf OUTPUT -> kein Glitch beim Boot.
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
      // Ignorieren - Newlines, CR, Leerzeichen.
      break;
    default:
      Serial.print("Unknown command: ");
      Serial.println((char)c);
      printHelp();
      break;
  }
}
