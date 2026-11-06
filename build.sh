#!/bin/bash
set -e
cd "$(dirname "$0")"
export PATH="$HOME/workspace/toolchain/usr/bin:$PATH"
arm-none-eabi-gcc -mcpu=arm7tdmi -marm -O2 -ffreestanding -nostdlib -Wall -T gba.ld -o musicbox.elf src/crt0.s src/main.c -lgcc
arm-none-eabi-objcopy -O binary musicbox.elf music-box-house.gba
python3 fix_header.py music-box-house.gba
python3 -c "import sys; d=open('music-box-house.gba','rb').read(); open('music-box-house.gba','wb').write(d + b'\x00' * (32768 - len(d)) if len(d) < 32768 else d)"
mkdir -p releases
cp music-box-house.gba releases/music-box-house.gba
ls -l releases/music-box-house.gba
