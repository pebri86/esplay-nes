#!/bin/bash
. ${IDF_PATH}/add_path.sh
${IDF_PATH}/components/esptool_py/esptool/esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud $((460800)) write_flash -fs 4MB 0x69000 ROMS/1942.nes 0x82000 ROMS/contra.nes 0xc3000 ROMS/duck_tales2.nes 0x124000 ROMS/duck_tales.nes 0x145000 ROMS/metal_gear.nes 0x186000 ROMS/dr_mario.nes 0x19f000 ROMS/tetris.nes 0x1b8000 ROMS/mario.nes 0x1d1000 ROMS/double_dragon_3.nes
