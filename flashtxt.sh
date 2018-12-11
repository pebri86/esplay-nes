#!/bin/bash
${IDF_PATH}/components/esptool_py/esptool/esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud 460800 write_flash -fs 4MB 0xa8000 "$1"
