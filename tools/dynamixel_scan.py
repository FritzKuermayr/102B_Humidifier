#!/usr/bin/env python3
"""Scan the U2D2 bus and list all detected Dynamixel motors.

Run this on the Pi after wiring exactly one motor to the U2D2 to confirm
that the motor responds, uses the ID you expect, and that no other motor
is on the bus by accident. Useful for sanity-checking a fresh setup
before running anything more ambitious.

Defaults match the existing Quadruped setup:
  - port      /dev/ttyUSB0
  - baud      1_000_000
  - protocol  1.0  (RX-24F, AX series)

Override on the command line if needed:
  python3 dynamixel_scan.py --port /dev/ttyUSB1 --baud 57600 --protocol 2.0
"""

import argparse
import sys

from dynamixel_sdk import PortHandler, PacketHandler, COMM_SUCCESS


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--port', default='/dev/ttyUSB0',
                        help='serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=1_000_000,
                        help='baud rate (default: 1000000)')
    parser.add_argument('--protocol', type=float, default=1.0,
                        choices=(1.0, 2.0),
                        help='Dynamixel protocol version (default: 1.0)')
    parser.add_argument('--id-min', type=int, default=1,
                        help='lowest ID to scan (default: 1)')
    parser.add_argument('--id-max', type=int, default=253,
                        help='highest ID to scan (default: 253)')
    parser.add_argument('--expected', type=int, default=1,
                        help='expected number of motors (default: 1)')
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    port = PortHandler(args.port)
    pkt = PacketHandler(args.protocol)

    if not port.openPort():
        print(f'Could not open port {args.port}', file=sys.stderr)
        return 1
    if not port.setBaudRate(args.baud):
        print(f'Could not set baud rate {args.baud}', file=sys.stderr)
        return 1

    print(f'Scanning {args.port} @ {args.baud} baud, '
          f'protocol {args.protocol}, IDs {args.id_min}-{args.id_max}...')

    found = []
    for dxl_id in range(args.id_min, args.id_max + 1):
        model, comm_result, error = pkt.ping(port, dxl_id)
        if comm_result == COMM_SUCCESS and error == 0:
            print(f'  ID {dxl_id:>3}: model number {model}')
            found.append((dxl_id, model))

    port.closePort()

    print()
    print(f'Detected {len(found)} motor(s).')

    if len(found) == args.expected:
        print('OK')
        return 0

    if len(found) == 0:
        print('No motor responded. Check:')
        print('  - 12 V power on the motor')
        print('  - U2D2 plugged in, /dev/ttyUSB0 visible (ls /dev/ttyUSB*)')
        print('  - Cable from U2D2 to motor (4-pin RS-485 for RX-24F)')
        print('  - Baud rate matches motor (default RX-24F = 1_000_000)')
        print('  - Protocol matches motor (RX-24F = 1.0)')
    else:
        print(f'Expected exactly {args.expected} motor(s) on the bus.')

    return 2


if __name__ == '__main__':
    sys.exit(main())
