# 102B — Quadruped + Humidity Control

This repository combines two subsystems used in the ME 102B project:

- **Quadruped controller** (Raspberry Pi, Dynamixel RX-24F) — keyboard-driven
  gait control over SSH.
- **Humidifier controller** (ESP32 + SHT40 + MOSFET) — closed-loop humidity
  control firmware and wiring docs.

A small set of shared Dynamixel utilities lives at the top level under
`tools/`. Course documents (reports, presentations, CAD, renders) are under
[`docs/`](docs/) — see [`docs/README.md`](docs/README.md).

## Repository Layout

```text
.
├── README.md
├── docs/                            # Reports, presentations, CAD, renders
├── software/
│   ├── humidifier/                  # ESP32 humidity control firmware + wiring
│   │   ├── esp32_humidity_control/
│   │   ├── esp32_humidity_control_switch/
│   │   ├── esp32_mosfet_test/
│   │   ├── esp32_sht40_test/
│   │   ├── wiring_humidity_control.md
│   │   └── wiring_humidity_control_switch.md
│   └── quadruped/                   # Raspberry Pi gait control
│       ├── raspi_controller/
│       │   ├── main.py
│       │   ├── g_key_leg_test.py
│       │   ├── keyboard_interface.py
│       │   └── q8gait/
│       ├── dynamixel_tools/         # Optional C utilities (DynamixelSDK)
│       ├── requirements.txt
│       └── setup_pi.sh
└── tools/                           # Standalone Dynamixel test scripts
    ├── dynamixel_scan.py
    └── single_motor_test.py
```

---

## Humidifier (ESP32)

Arduino sketches for a humidity controller using an SHT40 sensor and a
MOSFET-switched humidifier output. Open the `.ino` files with the Arduino
IDE / arduino-cli.

- `software/humidifier/esp32_humidity_control/` — closed-loop control
- `software/humidifier/esp32_humidity_control_switch/` — switch-driven variant
- `software/humidifier/esp32_mosfet_test/` — MOSFET output bring-up
- `software/humidifier/esp32_sht40_test/` — SHT40 sensor bring-up

Wiring is documented in
[`software/humidifier/wiring_humidity_control.md`](software/humidifier/wiring_humidity_control.md)
and
[`software/humidifier/wiring_humidity_control_switch.md`](software/humidifier/wiring_humidity_control_switch.md).

---

## Quadruped (Raspberry Pi)

Keyboard-driven control software for the quadruped, run directly on the Pi
over SSH.

- Pi and laptop on the same Wi-Fi network (no hotspot).
- Code on the Pi in `~/102B_Humidifier`.
- Movement: `h/j/k/l` (arrow keys as fallback).
- Default Dynamixel port `/dev/ttyUSB0`, baudrate `1000000`.
- Motors: Dynamixel RX-24F, protocol `1.0`.

### Prepare the Raspberry Pi

1. Flash Raspberry Pi OS to the microSD card.
2. Enable SSH during flashing or afterwards.
3. Connect the Pi to the regular Wi-Fi network.
4. Find the Pi's IP address: `hostname -I` — or `ssh pi@raspberrypi.local`.

### Install on the Pi

```bash
cd ~
git clone https://github.com/FritzKuermayr/102B_Humidifier.git
cd ~/102B_Humidifier
chmod +x software/quadruped/setup_pi.sh
./software/quadruped/setup_pi.sh
```

Update an existing checkout:

```bash
cd ~/102B_Humidifier
git pull origin main
```

Plug in the Dynamixel USB adapter and check it:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
sudo chmod a+rw /dev/ttyUSB0   # if needed
```

### Python Dependencies

`software/quadruped/setup_pi.sh` handles installation. To reinstall:

```bash
cd ~/102B_Humidifier
source .venv/bin/activate
python -m pip install -r software/quadruped/requirements.txt
```

### Start Keyboard Control

Place the robot on a stand for the first test.

```bash
cd ~/102B_Humidifier
source .venv/bin/activate
python software/quadruped/raspi_controller/main.py \
    --port /dev/ttyUSB0 --gait TROT_LOW --torque-limit 400
```

Keys:

```text
k      walk forward
j      walk backward
h      turn left
l      turn right
Space  stop and hold neutral stance
s      stop and hold neutral stance
q      quit safely and disable torque
```

Arrow keys also work if the terminal passes them through:

```text
↑  forward    ↓  backward    ←  turn left    →  turn right
```

If the motors react too weakly on the stand, increase the torque limit:

```bash
python software/quadruped/raspi_controller/main.py \
    --port /dev/ttyUSB0 --gait TROT_LOW --torque-limit 500
```

Alternative gaits:

```bash
python software/quadruped/raspi_controller/main.py --port /dev/ttyUSB0 --gait WALK --torque-limit 500
python software/quadruped/raspi_controller/main.py --port /dev/ttyUSB0 --gait TROT --torque-limit 500
```

### Single-Leg Test

Moves only the front-left leg slightly away from neutral and back. For motor
and mapping checks only.

```bash
cd ~/102B_Humidifier
source .venv/bin/activate
python software/quadruped/raspi_controller/g_key_leg_test.py --port /dev/ttyUSB0
```

Keys: `g` to twitch the front-left leg, `q` to quit.

Mapping:

```text
FL_q1 = Motor ID 1
Neutral = 150 deg
Test movement = 165 deg for 0.35 s, then back to 150 deg
```

### Important Checks

- Motor IDs and ordering: [`software/quadruped/raspi_controller/q8gait/config_rx24f.py`](software/quadruped/raspi_controller/q8gait/config_rx24f.py).
- On startup, `main.py` moves all motors to neutral.
- On exit (`q` or `Ctrl+C`) torque is disabled.
- Always put the robot on a stand before the first motion test.
- Missing `dynamixel_sdk`: `python -m pip install -r software/quadruped/requirements.txt`.
- `/dev/ttyUSB0` not accessible: check the USB adapter, run
  `sudo chmod a+rw /dev/ttyUSB0`.

---

## Standalone Dynamixel Tools

Quick utilities under `tools/`:

- [`tools/dynamixel_scan.py`](tools/dynamixel_scan.py) — scan the bus for connected motors.
- [`tools/single_motor_test.py`](tools/single_motor_test.py) — gentle wobble test for a single motor.
