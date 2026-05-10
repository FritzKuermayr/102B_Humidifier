# Wiring — `esp32_humidity_control_switch`

Variant **with toggle / slide switch** for a manual on/off request; the
sensor still blocks the MOSFET above the upper threshold.

The switch position directly maps to `userWantsOn`:

- switch **closed** (pin → GND, LOW) → `userWantsOn = YES`
- switch **open**   (pin = HIGH)     → `userWantsOn = no`

For a momentary push-button the mister would only run while the button is
held — in that case use the Serial `t` command instead.

## Pin assignment

| Function          | Feather V2 pin     | Wire color        |
|-------------------|--------------------|-------------------|
| MOSFET SIG        | `27`               | purple            |
| MOSFET VCC        | `3V`               | blue (MOSFET)     |
| MOSFET GND        | `GND` (left)       | gray              |
| SHT40 VCC         | `3V`               | red               |
| SHT40 GND         | `GND` (right)      | blue (SHT40)      |
| SHT40 SDA         | `SDA`  (= GPIO22)  | white             |
| SHT40 SCL         | `SCL`  (= GPIO20)  | orange            |
| **Switch pin 1**  | **`33`**           | any               |
| **Switch pin 2**  | **`GND` (shared)** | any               |

`3V` is shared between MOSFET VCC and SHT40 VCC. The **switch GND** has to
share one of the two GND pins with either MOSFET GND or SHT40 GND because
the Feather V2 only has two GND pins on its headers. Easiest: twist the
switch GND with the MOSFET GND (gray) and stick both into `GND` (left).

Switch polarity does not matter — when the switch is closed, `33` is
pulled to `GND` through the switch; when open, the ESP32 internally pulls
the pin HIGH (`INPUT_PULLUP`).

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
   │   │       A3   ───┤                ├───  33   ●─┐│
   │   │       A4   ───┤                ├───  15     ││
   │   │       A5   ───┤                ├───  32     ││
   │   │       SCK  ───┤                ├───  14     ││
   │   │      MOSI  ───┤                ├───  SDA  ●─││──┐
   │   │      MISO  ───┤                ├───  SCL  ●─││─┐│
   │   │       RX   ───┤                ├───  38     ││ ││
   │   │       TX   ───┤  [STEMMA QT]   ├───  37     ││ ││
   │   │            └────────────────┘            ││ ││
   │   │                                           ││ ││
   │   │   MOSFET module (DFRobot Gravity)         ││ ││
   │   │   ────────────────────────────            ││ ││
   │   │   gray   (GND)  ──┐                       ││ ││
   │   │                   ├──► GND (left)         ││ ││
   │   │   Switch pin 2 ───┘    (shared)           ││ ││
   │   │                                           ││ ││
   ├───│── blue   (VCC)  ──────► 3V                ││ ││
   │   │   purple (SIG)  ─────────────────────────  │ ││
   │   │   Switch pin 1  ─────────────────────────── ┘ ││
   │   │                                                 ││
   │   │   SHT40                                         ││
   │   │   ─────                                         ││
   ├───│── red    (VCC)  ──────► 3V                      ││
   │   │   blue   (GND)  ──────► GND (right) ────────────┘│
   │   │   white  (SDA)  ──────► SDA                      │
   │   │   orange (SCL)  ───────────────────────────────── ┘
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

| Switch position             | RH                | MOSFET                    |
|-----------------------------|-------------------|---------------------------|
| open (`userWantsOn=no`)     | any               | OFF                       |
| closed (`userWantsOn=YES`)  | < 78.0 %          | **ON**                    |
| closed                      | ≥ 80.0 %          | OFF (sensor blocks)       |
| closed                      | 78 – 80 %         | hold current state        |
| any                         | sensor error      | OFF (fail-safe)           |

`userWantsOn` always follows the current switch position. If the switch
stays closed while RH rises above 80 %, the mister is forced off — once
RH drops below 78 % again, it resumes automatically.

## Testing without a hardware switch

In the Serial Monitor (115200 baud, line ending "Newline") send the
character `t` — same effect as flipping the switch. Use `s` for the
current status and `h` for help.
