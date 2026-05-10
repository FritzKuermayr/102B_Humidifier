# Wiring — `esp32_humidity_control`

Variant **without switch**, fully automatic based on the sensor reading.

## Pin assignment

| Function       | Feather V2 pin     | Wire color        |
|----------------|--------------------|-------------------|
| MOSFET SIG     | `27`               | purple            |
| MOSFET VCC     | `3V`               | blue (MOSFET)     |
| MOSFET GND     | `GND` (left)       | gray              |
| SHT40 VCC      | `3V`               | red               |
| SHT40 GND      | `GND` (right)      | blue (SHT40)      |
| SHT40 SDA      | `SDA`  (= GPIO22)  | white             |
| SHT40 SCL      | `SCL`  (= GPIO20)  | orange            |

`3V` is shared between MOSFET VCC and SHT40 VCC — twist the two leads
together and stick them into the same `3V` pin (or use a small breadboard).

## ASCII diagram

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
   │   │   MOSFET module (DFRobot Gravity)          │ ││
   │   │   ────────────────────────────             │ ││
   │   │   gray   (GND)  ─┐                          │ ││
   │   │                  └─────────► GND (left)     │ ││
   ├───│── blue   (VCC)  ─────────► 3V               │ ││
   │   │   purple (SIG)  ─────────────────────────── │─┘│
   │   │                                             │  │
   │   │   SHT40                                     │  │
   │   │   ─────                                     │  │
   ├───│── red    (VCC)  ─────────► 3V               │  │
   │   │   blue   (GND)  ─────────► GND (right) ─────┘  │
   │   │   white  (SDA)  ─────────► SDA                 │
   │   │   orange (SCL)  ────────────────────────────── ┘
```

## Power

- **Feather:** USB-C, or a single-cell 3.7 V LiPo on the JST-PH connector.
- **Mister:** external supply on the MOSFET screw terminals `VIN` / `GND`.
- **Required:** external-supply `GND`  ↔  Feather `GND`  (common ground).

```text
ext. supply +V   ────►  MOSFET VIN   (screw terminal)
ext. supply GND  ────►  MOSFET GND   (screw terminal)
ext. supply GND  ────►  Feather GND  (common ground - mandatory)
MOSFET VOUT      ────►  Mister +
ext. supply GND  ────►  Mister −
```

## Logic summary

| RH                     | MOSFET                         |
|------------------------|--------------------------------|
| < 78.0 %               | **ON** (mister runs)           |
| ≥ 80.0 %               | OFF (sensor blocks)            |
| 78 – 80 %              | hold current state             |
| sensor unreachable     | OFF (fail-safe)                |
