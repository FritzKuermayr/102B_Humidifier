# ESP32 Feather V2 — Master Wiring Map

Single reference covering every component connected to the Feather V2 in
this project. Per-sketch details are kept in:

- [`wiring_humidity_control.md`](wiring_humidity_control.md) — sensor-only variant
- [`wiring_humidity_control_switch.md`](wiring_humidity_control_switch.md) — with manual switch

## Pin overview

```text
                        ┌──── USB-C ────┐
                        │    [Reset]    │
                RST  ───┤               ├───  VBUS  (5 V from USB)
       ★ 3V       ●─────┤               ├───  EN
                NC  ───┤   Adafruit     ├───  VBAT  (LiPo via JST-PH)
       ★ GND      ●─────┤   ESP32       ├───  GND   ★ (right)
                A0   ───┤   Feather V2  ├───  13    (red on-board LED)
                A1   ───┤               ├───  12
                A2   ───┤               ├───  27    ★  MOSFET SIG
                A3   ───┤               ├───  33    ★  Switch input
                A4   ───┤               ├───  15
                A5   ───┤               ├───  32
                SCK  ───┤               ├───  14
               MOSI  ───┤               ├───  SDA   ★  (= GPIO22) I2C data
               MISO  ───┤               ├───  SCL   ★  (= GPIO20) I2C clock
                RX   ───┤               ├───  38
                TX   ───┤  [STEMMA QT]  ├───  37
                        └───────────────┘

★ = pins used by this project.
```

## Component-by-component

### MOSFET module (DFRobot Gravity, active-low)

| Wire   | Color   | Goes to Feather pin |
|--------|---------|---------------------|
| SIG    | purple  | `27`                |
| VCC    | blue    | `3V` (shared)       |
| GND    | gray    | `GND` (left)        |

Notes:
- The module is **active-low**: `27` = LOW means MOSFET ON / status LED on.
- The Feather pin idles HIGH so the mister cannot accidentally turn on
  while the chip is booting.

### SHT40 humidity / temperature sensor (I2C)

| Wire   | Color   | Goes to Feather pin |
|--------|---------|---------------------|
| VCC    | red     | `3V` (shared)       |
| GND    | blue    | `GND` (right)       |
| SDA    | white   | `SDA`  (= GPIO22)   |
| SCL    | orange  | `SCL`  (= GPIO20)   |

Alternative: plug an Adafruit STEMMA QT cable straight into the QT
connector at the bottom of the Feather V2 — same wiring, no header
soldering.

### Manual switch (only used by `esp32_humidity_control_switch`)

| Wire        | Goes to Feather pin                      |
|-------------|------------------------------------------|
| Switch pin 1| `33`                                     |
| Switch pin 2| `GND` (shared, see note below)           |

Notes:
- The Feather V2 has only two GND pins on its headers and both are
  already used. Twist the switch GND with the MOSFET GND (gray) and put
  both leads into the same `GND` (left) pin.
- Polarity of the switch is irrelevant; the ESP32 holds `33` HIGH
  internally (`INPUT_PULLUP`), the switch pulls it to GND when closed.

## Shared rails

| Rail        | Devices sharing it                                     |
|-------------|--------------------------------------------------------|
| `3V`        | MOSFET VCC (blue) + SHT40 VCC (red)                    |
| `GND` (L)   | MOSFET GND (gray) + optional switch GND                |
| `GND` (R)   | SHT40 GND (blue)                                       |

To share `3V`: twist the two wires together and plug them into the same
header hole, or break it out on a small breadboard.

## External supply for the mister

The Feather **does not** power the mister. A separate supply (typically
5 V) feeds the mister through the MOSFET module.

```text
ext. supply +V   ────►  MOSFET VIN   (screw terminal)
ext. supply GND  ────►  MOSFET GND   (screw terminal)
ext. supply GND  ────►  Feather GND   ←  REQUIRED: common ground
MOSFET VOUT      ────►  Mister +
ext. supply GND  ────►  Mister −
```

Without the common ground between supply and Feather the MOSFET gate
driver has no stable reference and the FET will not switch reliably.

## Pin-to-purpose table (quick lookup)

| Feather V2 pin   | Used by                       | What             |
|------------------|-------------------------------|------------------|
| `3V`             | MOSFET, SHT40                 | 3.3 V logic supply |
| `GND` (left)     | MOSFET (+ switch, if used)    | ground           |
| `GND` (right)    | SHT40                         | ground           |
| `27`             | MOSFET                        | SIG (active-low) |
| `33`             | Switch (switch sketch only)   | input, pull-up   |
| `SDA` (= GPIO22) | SHT40                         | I2C data         |
| `SCL` (= GPIO20) | SHT40                         | I2C clock        |

## Per-sketch usage

| Sketch                            | Uses 27 | Uses 33 | Uses I2C |
|-----------------------------------|:-------:|:-------:|:--------:|
| `esp32_mosfet_test`               |   ✓     |         |          |
| `esp32_sht40_test`                |         |         |    ✓     |
| `esp32_humidity_control`          |   ✓     |         |    ✓     |
| `esp32_humidity_control_switch`   |   ✓     |    ✓    |    ✓     |

You can swap between any two sketches without re-wiring anything — the
switch sketch just adds the button on `33`, no other pins change.

## Powering the Feather itself

| Source          | Connector                  | Notes                                   |
|-----------------|----------------------------|-----------------------------------------|
| USB-C from PC   | USB-C port                 | Programming and 5 V supply              |
| USB-C charger   | USB-C port                 | Standalone, 5 V from any USB charger    |
| LiPo battery    | JST-PH on the underside    | Single-cell 3.7 V; on-board charger via USB-C |

The mister is always powered by its own external supply, regardless of
how the Feather is powered.
