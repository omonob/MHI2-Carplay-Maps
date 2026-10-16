"""Reproduce the 791 IDR5 update from the exact public v1.1.0 core.

This is a hash-gated parameter-only transformation, not a generic ELF patcher.
It changes the modulo divisor and the corresponding diagnostic argument from
20 to 5. The encoded 800x480/maxFPS=40 display declaration stays unchanged.
"""
import argparse
import hashlib
from pathlib import Path

BASE_SHA256 = '00b48a8fe876e6ff405124b87ed9eace1bcb079220ff7f02b866f42676477a4a'
BASE_SIZE = 75099
FPS_INSTRUCTION_OFFSET = 0x2A70
INTERVAL_OFFSETS = (0x4370, 0x452C)
OLD_INSTRUCTION = bytes.fromhex('1420a0e3')  # ARM: mov r2, #20
NEW_INSTRUCTION = bytes.fromhex('0520a0e3')  # ARM: mov r2, #5


def transform(data):
    if len(data) != BASE_SIZE or hashlib.sha256(data).hexdigest() != BASE_SHA256:
        raise ValueError('Not the exact published 791 baseline; no output produced')
    if data[:6] != b'\x7fELF\x01\x01' or data[18:20] != b'\x28\x00':
        raise ValueError('Expected little-endian ELF32 ARM')
    if data[FPS_INSTRUCTION_OFFSET:FPS_INSTRUCTION_OFFSET + 4] != bytes.fromhex('2820a0e3'):
        raise ValueError('The maxFPS=40 declaration instruction does not match')
    out = bytearray(data)
    for offset in INTERVAL_OFFSETS:
        if data[offset:offset + 4] != OLD_INSTRUCTION:
            raise ValueError('IDR interval instruction does not match')
        out[offset:offset + 4] = NEW_INSTRUCTION
    changed = tuple(i for i, (old, new) in enumerate(zip(data, out)) if old != new)
    if changed != INTERVAL_OFFSETS:
        raise ValueError('Unexpected parameter transformation scope')
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = transform(args.baseline.read_bytes())
    if args.output.exists():
        raise SystemExit('Output exists; refusing to overwrite it')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    print('791_40FPS_IDR5_CORE_OK', hashlib.sha256(result).hexdigest())


if __name__ == '__main__':
    main()
