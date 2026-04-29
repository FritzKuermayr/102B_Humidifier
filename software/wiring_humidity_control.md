# Anschlussbild — `esp32_humidity_control`

Variante **ohne Taster**, vollautomatisch nach Sensorwert.

## Pin-Zuordnung

| Funktion       | Feather V2 Pin     | Kabelfarbe        |
|----------------|--------------------|-------------------|
| MOSFET SIG     | `27`               | lila              |
| MOSFET VCC     | `3V`               | blau (MOSFET)     |
| MOSFET GND     | `GND` (links)      | grau              |
| SHT40 VCC      | `3V`               | rot               |
| SHT40 GND      | `GND` (rechts)     | blau (SHT40)      |
| SHT40 SDA      | `SDA`  (= GPIO22)  | weiss             |
| SHT40 SCL      | `SCL`  (= GPIO20)  | orange            |

`3V` wird von MOSFET-VCC und SHT40-VCC gemeinsam genutzt — beide Drähte in denselben `3V`-Pin stecken (verzwirbeln oder Mini-Steckbrett).

## ASCII-Diagramm

```text
                       ┌──── USB-C ────┐
                       │    [Reset]    │
               RST  ───┤               ├───  VBUS
   ┌──── 3V        ●───┤   Adafruit    ├───  EN
   │           NC  ───┤   ESP32        ├───  VBAT
   │   ┌── GND  ●───┤   Feather V2   ├───  GND  ●──┐
   │   │       A0   ───┤                ├───  13 (LED)
   │   │       A1   ───┤                ├───  12
   │   │       A2   ───┤                ├───  27   ●──┐
   │   │       A3   ───┤                ├───  33      │
   │   │       A4   ───┤                ├───  15      │
   │   │       A5   ───┤                ├───  32      │
   │   │       SCK  ───┤                ├───  14      │
   │   │      MOSI  ───┤                ├───  SDA  ●──│──┐
   │   │      MISO  ───┤                ├───  SCL  ●──│─┐│
   │   │       RX   ───┤                ├───  38      │ ││
   │   │       TX   ───┤  [STEMMA QT]   ├───  37      │ ││
   │   │            └────────────────┘             │ ││
   │   │                                            │ ││
   │   │   MOSFET-Modul (DFRobot Gravity)           │ ││
   │   │   ──────────────────────────                │ ││
   │   │   grau   (GND)  ─┐                          │ ││
   │   │                  └─────────► GND (links)    │ ││
   ├───│── blau   (VCC)  ─────────► 3V               │ ││
   │   │   lila   (SIG)  ─────────────────────────── │─┘│
   │   │                                             │  │
   │   │   SHT40                                     │  │
   │   │   ─────                                     │  │
   ├───│── rot    (VCC)  ─────────► 3V               │  │
   │   │   blau   (GND)  ─────────► GND (rechts) ────┘  │
   │   │   weiss  (SDA)  ─────────► SDA                 │
   │   │   orange (SCL)  ────────────────────────────── ┘
```

## Stromversorgung

- **Feather:** USB-C oder LiPo am JST-PH-Stecker (3.7 V einzellig)
- **Mistifyer:** externe Versorgung an MOSFET-Schraubklemmen `VIN` / `GND`
- **Pflicht:** externe-Supply-`GND`  ↔  Feather-`GND` (gemeinsame Masse)

```text
ext. Supply +V   ────►  MOSFET VIN  (Schraubklemme)
ext. Supply GND  ────►  MOSFET GND  (Schraubklemme)
ext. Supply GND  ────►  Feather GND   (gemeinsame Masse, Pflicht!)
MOSFET VOUT      ────►  Mistifyer +
ext. Supply GND  ────►  Mistifyer −
```

## Logik-Zusammenfassung

| RH                     | MOSFET                       |
|------------------------|------------------------------|
| < 78,0 %               | **AN** (Mistifyer laeuft)    |
| ≥ 80,0 %               | aus (Sensor sperrt)          |
| 78 – 80 %              | aktueller Zustand bleibt     |
| Sensor nicht erreicht  | aus (fail-safe)              |
