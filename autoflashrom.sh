#!/bin/bash
. ${IDF_PATH}/add_path.sh
${IDF_PATH}/components/esptool_py/esptool/esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud $((460800)) write_flash -fs 4MB 0xa9000 ROMS/1942.nes 0xc2000 ROMS/contra.nes 0x103000 ROMS/duck_tales2.nes 0x164000 ROMS/duck_tales.nes 0x185000 ROMS/metal_gear.nes 0x1c6000 ROMS/dr_mario.nes 0x1df000 ROMS/tetris.nes 0x1f8000 ROMS/mario.nes 0x211000 ROMS/double_dragon_3.nes
