#!/usr/bin/env python3
"""Patch GBA header fields (title, game code, maker, fixed byte, version, checksum)."""
import sys

def main(path):
    rom = bytearray(open(path, "rb").read())
    assert len(rom) >= 0xC0, "ROM too small"
    title = b"MUSICBOX\x00\x00\x00\x00"  # 12 bytes: header title field is 0xA0..0xAB; a short title shrinks the ROM and shifts all code (Python slice assignment deletes the missing bytes)
    rom[0xA0:0xAC] = title
    rom[0xAC:0xB0] = b"MBOX"
    rom[0xB0:0xB2] = b"01"
    rom[0xB2] = 0x96
    rom[0xB3] = 0x00
    rom[0xB4] = 0x00
    rom[0xBC] = 0x00  # version
    chk = 0
    for i in range(0xA0, 0xBD):
        chk -= rom[i]
    chk -= 0x19
    rom[0xBD] = chk & 0xFF
    open(path, "wb").write(rom)
    print(f"header ok: {path} ({len(rom)} bytes), checksum 0x{rom[0xBD]:02X}")

if __name__ == "__main__":
    main(sys.argv[1])
