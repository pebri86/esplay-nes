ESP32-NESEMU, a Nintendo Entertainment System emulator for the ESP32
====================================================================

This is a quick and dirty port of Nofrendo, a Nintendo Entertainment System emulator. It lacks sound, but can emulate a NES at close
to full speed, albeit with some framedrop due to the way the display is driven.

Warning
-------

This is a proof-of-concept and not an official application note. As such, this code is entirely unsupported by Espressif.


Compiling
---------

This code is an esp-idf project. You will need esp-idf to compile it. Newer versions of esp-idf may introduce incompatibilities with this code;
for your reference, the code was tested against release/v3.1 branch of esp-idf.


Display
-------

To display the NES output, please connect a 320x240 ili9341-based SPI display to the ESP32 in this way:

    =====  =======================
    Pin    GPIO
    =====  =======================
    MOSI/SDA   23
    CLK        19
    CS         22
    DC/A0       5
    RST        18
    BCKL       17
    =====  =======================

(BCKL = backlight enable)

Also connect the power supply and ground. For now, the LCD is controlled using a SPI peripheral, fed using the 2nd CPU. This is less than ideal; feeding
the SPI controller using DMA is better, but was left out due to this being a proof of concept.


Controller
----------

To control the NES, connect GPIO ppins to a common ground pcb gamepad:

    =======  =====
    Key      GPIO
    =======  =====
    A        0
    B        4
    START    12
    SELECT   14
    RIGHT    27
    DOWN     25
    UP       33
    LEFT     32
    =======  =====

Also connect the ground line.

ROM
---
This NES emulator does not come with a ROM. Please supply your own and flash to address 0x00100000. You can use the flashrom.sh script as a template for doing so.

Copyright
---------

Code in this repository is Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE. Code in the
components/nofrendo is Copyright (c) 1998-2000 Matthew Conte (matt@conte.com) and licensed under the GPLv2.
