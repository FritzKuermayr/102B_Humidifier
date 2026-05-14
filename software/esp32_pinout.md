# ESP32 Feather V2 — Pin Connections

Compact reference: which Feather pin connects to what. No wire colors —
those are in [`wiring_map_esp32.md`](wiring_map_esp32.md) and the
per-sketch wiring docs.

## Connections

| Feather V2 pin    | Connected to              |
|-------------------|---------------------------|
| `3V`              | MOSFET VCC, SHT40 VCC     |
| `GND` (left)      | MOSFET GND, Switch GND*   |
| `GND` (right)     | SHT40 GND                 |
| `27`              | MOSFET SIG                |
| `33`              | Switch input*             |
| `SDA` (= GPIO22)  | SHT40 SDA                 |
| `SCL` (= GPIO20)  | SHT40 SCL                 |

\* Only used by `esp32_humidity_control_switch.ino`. Switch GND shares
the same physical `GND (left)` pin as the MOSFET GND.

## External supply for the mister

Independent from the Feather; only the MOSFET output drives the mister.

| External rail    | Connected to                              |
|------------------|-------------------------------------------|
| Supply +V        | MOSFET VIN (screw terminal)               |
| Supply GND       | MOSFET GND (screw terminal), Feather GND  |
| MOSFET VOUT      | Mister +                                  |
| Supply GND       | Mister −                                  |

The external supply GND **must** be tied to the Feather GND or the
MOSFET gate driver will not have a stable reference.
