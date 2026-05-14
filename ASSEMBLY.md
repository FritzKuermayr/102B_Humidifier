# Robot Assembly

The robot is split into a **Lower Part** (chassis + locomotion + on-board
electronics) and an **Upper Part** (water tank + humidity-control
subsystem) that are bolted together with standoffs and tied into a
common electrical ground. Every load-bearing structural piece is
3D-printed; all fasteners and bearings are off-the-shelf.

## Bill of materials

### Lower Part

| Part                                        | Qty |
|---------------------------------------------|----:|
| Chassis (3D-printed)                        |   1 |
| Upper chassis (3-part print)                |   1 |
| Upper legs (3D-printed)                     |   8 |
| Lower legs (3D-printed)                     |   8 |
| Ball bearings                               |  16 |
| Dynamixel RX-24F actuators                  |   8 |
| Roboteck K4P motor brackets                 |   9 |
| Motor controller holder (3D-printed)        |   1 |
| Battery holder (3D-printed)                 |   1 |
| Distance holder M2.5 (20 mm)                |   6 |
| Distance holder M2.5 (10 mm)                |   6 |
| Screws & Nuts (M2.5 / M3)                   |  20 |
| Raspberry Pi 4                              |   1 |
| U2D2 USB-to-Serial adapter                  |   1 |
| Diatone Mamba Micro 2 A BEC                 |   1 |
| 12 V 5000 mAh battery                       |   2 |
| USB-C to USB-B cable                        |   1 |
| Arduino USB dongle                          |   1 |

### Upper Part

| Part                                        | Qty |
|---------------------------------------------|----:|
| 3D-printed tank + platform                  |   1 |
| 3D-printed LED housing                      |   1 |
| ESP32 Feather V2 microcontroller            |   1 |
| SHT40 humidity / temperature sensor         |   1 |
| DFRobot Gravity MOSFET module               |   1 |
| Mister (pump + nozzle)                      |   1 |
| Washers                                     |  10 |
| Fasteners                                   |  10 |
| Gaskets                                     |  10 |
| Cables (assorted lengths, dupont/silicone)  |  30 |

## Lower Part assembly

### Mechanical chassis and legs

The base chassis is the single largest 3D-printed piece and forms the
flat platform that everything else mounts onto. Each of the four legs
is built from one **upper leg** and one **lower leg**, both 3D-printed,
connected at the knee by a single **ball bearing** pair that allows the
lower leg to swing freely on the knee shaft (two bearings per knee × 4
knees = 8 bearings). The remaining 8 ball bearings sit at the hip
joints, supporting the upper-leg shaft against the chassis so the
Dynamixel does not carry the radial load itself. With both bearing
pairs installed the legs spin smoothly under their own weight, which is
the first sanity-check before any motor is wired.

### Servomotors and brackets

Eight **Dynamixel RX-24F** actuators are screwed into **Roboteck K4P
motor brackets** — one bracket carries each motor; the ninth bracket
sits centrally on the chassis as a cable-routing anchor. Each leg gets
two motors: one driving the hip joint (rotates the upper leg about its
mounting point on the chassis) and one driving the knee (rotates the
lower leg relative to the upper leg). The K4P brackets keep the motor
output shaft concentric with the bearing axis so no side load reaches
the gear train.

### On-board electronics shelf

A separate 3D-printed **motor controller holder** mounts on top of the
chassis and carries the **Raspberry Pi 4**, the **U2D2 adapter**, and
the **Diatone Mamba Micro 2 A BEC**. The four M2.5 standoffs that come
with the Pi (and the 10 mm distance holders) raise the Pi above the
holder so the GPIO header and USB ports stay accessible. The two
**12 V 5000 mAh batteries** sit on the dedicated **battery holder**,
low and central, for a low centre of gravity; they are wired in
parallel through a small power-distribution junction so total capacity
roughly doubles while voltage stays at 12 V.

### Lower-Part wiring

The motor bus is a daisy chain of all eight Dynamixels with the
standard four-pin Dynamixel cables; the chain terminates at the
**U2D2** which plugs into the Pi via the **USB-B → USB-C cable**. The
12 V battery rail feeds the Dynamixels directly (via a small power hub
that is part of the daisy chain) and also feeds the **BEC**, which
steps the rail down to 5 V for the Pi. The Pi can therefore be powered
from the battery alone — no separate Pi supply is needed in the field.

```text
12 V battery (×2 parallel) ──┬───►  U2D2 / Dynamixel bus  ───►  8 × RX-24F
                              │
                              └───►  BEC  ────►  5 V  ────►  Raspberry Pi 4

Raspberry Pi 4  ──── USB-C to USB-B ────►  U2D2  ──── 4-pin RS-485 ────►  motor chain
```

The **Arduino USB dongle** is used as the USB-C programming/charging
cable for the ESP32 Feather V2 in the Upper Part; during normal
operation it can be unplugged because the Feather runs from its own
LiPo or from the BEC rail if a tap is fitted.

## Upper Part assembly

### Tank and platform

The **3D-printed tank + platform** is a single combined piece: the
lower half holds the water for the mister, the upper half is a flat
mounting surface for the electronics. The tank seam and the lid
opening are sealed with the **gaskets**; **washers** distribute the
clamping load from the **fasteners** so the print does not crack.
The **mister (pump + nozzle)** sits inside the tank with its nozzle
poking through a sealed opening on the top so the spray plume is
released upward.

### Humidity electronics

The **ESP32 Feather V2** is mounted on the platform with two M2.5 ×
10 mm standoffs, and the **SHT40 sensor** is placed next to it (or
plugged into the Feather's STEMMA QT connector). The **DFRobot Gravity
MOSFET module** sits between the platform and the mister: its
screw-terminal input is fed by an external 5 V supply, its output
switches the mister's positive lead. The **3D-printed LED housing**
holds an indicator LED that lights when the mister is active — wired
in parallel with the MOSFET signal LED or directly across the mister
output (a current-limiting resistor in series is required).

### Upper-Part wiring

```text
ESP32 Feather V2 GPIO27    ───►  MOSFET SIG (purple)
ESP32 Feather V2 3V        ───►  MOSFET VCC (blue)  +  SHT40 VCC (red)
ESP32 Feather V2 GND (L)   ───►  MOSFET GND (gray)
ESP32 Feather V2 GND (R)   ───►  SHT40 GND  (blue)
ESP32 Feather V2 SDA / SCL ───►  SHT40 SDA / SCL  (white / orange)

5 V external supply  ───►  MOSFET VIN  ───►  MOSFET VOUT  ───►  Mister +
5 V external supply GND  ─────────────────────────────────────►  Mister −
```

The full pin map for the Feather, including the optional override
switch on `GPIO33`, lives in
[`software/wiring_map_esp32.md`](software/wiring_map_esp32.md).

## Joining Upper and Lower parts

The Upper Part sits on top of the **upper chassis** (the 3-part print
that closes off the locomotion deck) and is held by the **six M2.5 ×
20 mm distance holders**, which raise the tank a few centimetres above
the chassis so the mister plume can escape sideways without hitting
the Pi. The six 10 mm holders stay below in the Lower Part for the Pi
and motor-controller mounts. **Screws and nuts** clamp the standoffs
through both the upper chassis and the Upper-Part platform.

A single short cable runs from the Upper-Part `GND` rail down to a
chassis-side `GND` point so the **two power supplies** (the 12 V motor
battery and the 5 V mister supply) share a reference with the Feather.
Without this single ground bond the MOSFET gate driver loses its
reference and the mister will not switch reliably.

## Bring-up order

The three subsystems are verified individually before any combined
test:

1. **Dynamixel chain.** Power the motor rail, plug in the U2D2, and
   run `python3 tools/dynamixel_scan.py` on the Pi. Confirm all eight
   motors appear with the expected IDs.
2. **MOSFET + mister.** With the ESP32 powered by USB only, run
   `esp32_mosfet_test.ino` and toggle the MOSFET manually via Serial.
   Verify the indicator LED switches and that the mister runs when
   the external 5 V supply is connected.
3. **Humidity sensor.** Flash `esp32_sht40_test.ino` and confirm
   sensible temperature and RH readings (breath test: humidity should
   spike and recover within seconds).

Once each of the three checks is green, flash
`esp32_humidity_control.ino` (or `..._switch.ino` if the toggle switch
is fitted), mount the Upper Part onto the Lower Part, connect the
common ground wire, and power up the batteries. The Pi handles
locomotion; the Feather runs closed-loop humidity regulation
independently in parallel.
