#!/usr/bin/env python3
"""Wobble a single Dynamixel motor a few degrees around its current
position and return to it.

Reads the current position first and only moves the motor by a small
amount around that point, so the motor will not slam into a hard stop
even if the mechanical setup is unknown.

Defaults match the existing Quadruped wiring (RX-24F, protocol 1.0,
1_000_000 baud, /dev/ttyUSB0, ID 1).

Run on the Pi:
    python3 tools/single_motor_test.py
With a different motor ID (run dynamixel_scan.py first to find it):
    python3 tools/single_motor_test.py --id 5
"""

import argparse
import sys
import time

from dynamixel_sdk import PortHandler, PacketHandler, COMM_SUCCESS


# Protocol 1.0 register addresses
ADDR_TORQUE_ENABLE   = 24
ADDR_GOAL_POSITION   = 30  # 2 bytes
ADDR_MOVING_SPEED    = 32  # 2 bytes
ADDR_PRESENT_POS     = 36  # 2 bytes

# RX-24F position range: 0..1023 = 0..300 degrees (joint mode).
POS_MIN_SAFE = 50
POS_MAX_SAFE = 973
WOBBLE_TICKS = 30  # ~9 degrees on RX-24F


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=1_000_000)
    parser.add_argument('--protocol', type=float, default=1.0, choices=(1.0, 2.0))
    parser.add_argument('--id', type=int, default=1, help='motor ID (default 1)')
    parser.add_argument('--speed', type=int, default=100,
                        help='moving speed 0-1023 (default 100, gentle)')
    return parser.parse_args()


def check(comm_result: int, error: int, what: str) -> None:
    if comm_result != COMM_SUCCESS or error != 0:
        print(f'  WARNING during {what}: comm={comm_result} error={error}',
              file=sys.stderr)


def main() -> int:
    args = parse_args()

    port = PortHandler(args.port)
    pkt = PacketHandler(args.protocol)

    if not port.openPort():
        print(f'Could not open {args.port}', file=sys.stderr)
        return 1
    if not port.setBaudRate(args.baud):
        print(f'Could not set baud rate {args.baud}', file=sys.stderr)
        return 1

    motor = args.id

    # Ping first so we fail fast if the motor is not responding.
    model, comm, err = pkt.ping(port, motor)
    if comm != COMM_SUCCESS or err != 0:
        print(f'Motor ID {motor} did not respond.', file=sys.stderr)
        print('Run `python3 tools/dynamixel_scan.py` to find the right ID.',
              file=sys.stderr)
        return 2
    print(f'Motor ID {motor} responded, model number {model}.')

    # Read current position so we wobble around it instead of jumping somewhere.
    present, comm, err = pkt.read2ByteTxRx(port, motor, ADDR_PRESENT_POS)
    check(comm, err, 'read present position')
    print(f'  current position: {present}')

    start = max(POS_MIN_SAFE, min(POS_MAX_SAFE, present))

    # Park goal at the current position before enabling torque.
    # Otherwise torque-enable can snap the motor to a stale goal position.
    pkt.write2ByteTxRx(port, motor, ADDR_GOAL_POSITION, start)

    # Gentle speed.
    pkt.write2ByteTxRx(port, motor, ADDR_MOVING_SPEED, args.speed)

    # Torque on.
    pkt.write1ByteTxRx(port, motor, ADDR_TORQUE_ENABLE, 1)

    try:
        sequence = [start + WOBBLE_TICKS, start - WOBBLE_TICKS, start]
        for goal in sequence:
            goal = max(POS_MIN_SAFE, min(POS_MAX_SAFE, goal))
            print(f'  goal -> {goal}')
            res, err = pkt.write2ByteTxRx(port, motor, ADDR_GOAL_POSITION, goal)
            check(res, err, f'goal position {goal}')
            time.sleep(1.5)
    finally:
        # Always release torque so the motor does not heat up if the script
        # exits unexpectedly.
        pkt.write1ByteTxRx(port, motor, ADDR_TORQUE_ENABLE, 0)
        port.closePort()

    print('Done.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
