# Robot Assembly

The robot is built around a four-legged mechanical frame that carries
eight Dynamixel RX-24F servomotors — two per leg, one driving the hip
joint and one driving the knee — held in place by aluminium brackets
and the leg segments themselves. With the motors bolted into the
frame and the leg linkages assembled, all eight servos are
daisy-chained with the standard four-pin Dynamixel cables and brought
back to a single U2D2 USB-to-Dynamixel adapter, which connects to the
Raspberry Pi over USB and to a 12 V supply that powers the motors
through a small power hub. The Pi sits above the motors on the
chassis, talks to the motor bus at one megabaud over RS-485, and runs
the gait controller from `software/raspi_controller/main.py`. A second,
independent electronics block sits next to it for humidity control: an
Adafruit ESP32 Feather V2 is wired to an SHT40 temperature/humidity
sensor over I²C (`white → SDA`, `orange → SCL`, plus `3V` and `GND`)
and to a DFRobot Gravity MOSFET module on `GPIO27`, which in turn
switches a 5 V ultrasonic mister fed from its own external supply. All
three grounds — the 12 V motor supply, the 5 V mister supply, and the
Feather itself — are tied together at one common point so the MOSFET
gate driver has a stable reference; an optional toggle switch between
`GPIO33` and `GND` gives a hardware on/off override. Once each
subsystem has been verified individually (Dynamixel bus scan, MOSFET
switching test, SHT40 readout), the three blocks are mounted side by
side on the chassis, the harnesses are routed along the frame, and
powering up the Pi and the Feather is enough to start the whole robot
— the Pi handles locomotion while the Feather runs closed-loop
humidity regulation independently in parallel.
