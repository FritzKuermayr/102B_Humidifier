# Anschlussbild — `esp32_humidity_control_switch`

Variante **mit Kipp-/Schiebe-Schalter** als manuelle An/Aus-Anforderung; Sensor sperrt zusaetzlich nach oben.

Die Schalter-Position bestimmt direkt `userWantsOn`:

- Schalter **geschlossen** (Pin → GND, LOW) → `userWantsOn = YES`
- Schalter **offen**       (Pin = HIGH)     → `userWantsOn = no`

Bei einem reinen Push-Button (Momentary) wuerde der Mistifyer nur waehrend
der Druckdauer laufen — dafuer lieber das Serial-Kommando `t` benutzen.

## Pin-Zuordnung

| Funktion        | Feather V2 Pin     | Kabelfarbe        |
|-----------------|--------------------|-------------------|
| MOSFET SIG      | `27`               | lila              |
| MOSFET VCC      | `3V`               | blau (MOSFET)     |
| MOSFET GND      | `GND` (links)      | grau              |
| SHT40 VCC       | `3V`               | rot               |
| SHT40 GND       | `GND` (rechts)     | blau (SHT40)      |
| SHT40 SDA       | `SDA`  (= GPIO22)  | weiss             |
| SHT40 SCL       | `SCL`  (= GPIO20)  | orange            |
| **Taster Pin 1**| **`33`**           | beliebig          |
| **Taster Pin 2**| **`GND` (geteilt)**| beliebig          |

`3V` teilen sich MOSFET-VCC und SHT40-VCC. Der **Taster-GND** muss sich einen
der beiden GND-Pins mit MOSFET-GND oder SHT40-GND teilen, weil der Feather V2
nur zwei GND-Pins auf den Stiftleisten hat. Praktisch: Taster-GND mit
MOSFET-GND (grau) verzwirbeln und gemeinsam in `GND` (links) stecken.

Polung des Tasters egal — beim Druecken wird `33` ueber den Taster auf `GND`
gezogen, der ESP32 zieht den Pin intern auf HIGH (`INPUT_PULLUP`).

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
   │   │   MOSFET-Modul (DFRobot Gravity)          ││ ││
   │   │   ──────────────────────────              ││ ││
   │   │   grau   (GND)  ──┐                       ││ ││
   │   │                   ├──► GND (links)        ││ ││
   │   │   Taster Pin 2 ───┘   (gemeinsam)         ││ ││
   │   │                                           ││ ││
   ├───│── blau   (VCC)  ──────► 3V                ││ ││
   │   │   lila   (SIG)  ─────────────────────────  │ ││
   │   │   Taster Pin 1  ─────────────────────────── ┘ ││
   │   │                                                 ││
   │   │   SHT40                                         ││
   │   │   ─────                                         ││
   ├───│── rot    (VCC)  ──────► 3V                      ││
   │   │   blau   (GND)  ──────► GND (rechts) ───────────┘│
   │   │   weiss  (SDA)  ──────► SDA                      │
   │   │   orange (SCL)  ───────────────────────────────── ┘
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

| Schalter-Position    | RH                | MOSFET                |
|----------------------|-------------------|-----------------------|
| offen (`userWantsOn=no`)   | egal        | aus                   |
| geschlossen (`userWantsOn=YES`) | < 78,0 % | **AN**              |
| geschlossen          | ≥ 80,0 %          | aus (Sensor sperrt)   |
| geschlossen          | 78 – 80 %         | aktueller Zustand     |
| egal                 | Sensor-Fehler     | aus (fail-safe)       |

`userWantsOn` folgt immer der aktuellen Schalter-Position. Bleibt der
Schalter geschlossen waehrend RH ueber 80 % steigt, wird der Mistifyer
zwangsweise abgeschaltet — sobald RH wieder unter 78 % faellt, laeuft er
von selbst weiter.

## Test ohne Hardware-Taster

Im Serial Monitor (115200, Newline) das Zeichen `t` schicken — wirkt wie ein
Tastendruck. Mit `s` den aktuellen Status abrufen, mit `h` die Hilfe.
