PICP
====

This folder contains `picp`, a PIC16F145x based PIC16F145x programmer.

To program a PIC16F145x with this, all you need is a PIC16F145x programmed
with the software in `pic/` connected with USB to a Linux computer with the
software in `pc/`. (So you still need to get the first PIC programmed
somehow, but after that, you have a very simple and cheap programmer.)

No additional circuit is required, the USB wires are directly connected to the
programmer PIC, and the pins of that PIC are directly connected to the PIC
that's to be programmed.
The reset, data and clock pins are set to high impedance while not programming,
so the programmer can be left connected while running your application.

It's probably pretty easy to port the PC software to other operating systems.

Usage
-----

1. Program a PIC16F145x with the software in `pic/`.
2. Compile the software in `pc/` and put the resulting `picp` executable somewhere in your `PATH`. (e.g. `/usr/local/bin`)
3. Copy `udev/40-picp.rules` to `/etc/udev/rules.d/`.
4. Connect the ICSP pins (RC0 and RC1) of the programmer and the
   to-be-programmed chip, and connect RC2 of the programmer to the reset pin
   (RA3) of the to-be-programmed chip.
5. Connect the programmer with USB, and use `picp program < file` to
   flash a program.

Protocol
--------

The programmer is a USB ACM device, which basically means it acts like a
simplified serial port. (Linux will recognize it and make a `/dev/ttyACMx` for it,
which is symlinked from `/dev/picp0` if you use the provided udev rule.)

The protocol that's used to talk with the programmer is very simple.
Commands are just single bytes (ASCII characters),
and parameters (which are always 14-bit) are encoded as two bytes with the most
significant bit set to 1, to prevent any confusion with command bytes.

The commands are:

| Command | Description | Reply
|---------|-------------|-------
| `'V'`   | Get the programmer version | A newline (`\n`) terminated version string (in ASCII).
| `'T'`   | No operation (for testing) | One byte: `'Y'`
| `'B'`   | Begin program mode: Pull the reset pin low, and clock in the magic number 0x4D434850 to get the chip in low voltage programming mode.
| `'E'`   | End program mode: Set the reset, data and clock pins back to high impedance mode.

Furthermore, the following commmands map directly to the commands specified in the
[PIC16(L)F145X Programming Specification](http://ww1.microchip.com/downloads/en/DeviceDoc/41620C.pdf):

| Command | Parameter | Reply | ICSP Command
|---------|-----------|-------|--------------
| `'C'`   | Data      |       | Load Configuration
| `'L'`   | Data      |       | Load Data For Program Memory
| `'R'`   |           | Data  | Read Data From Program Memory
| `'I'`   |           |       | Increment Address
| `'A'`   |           |       | Reset Address
| `'P'`   |           |       | Begin Internally Timed Programming
| `'Q'`   |           |       | Begin Externally Timed Programming
| `'S'`   |           |       | End Externally Timed Programming
| `'X'`   |           |       | Bulk Erase Program Memory
| `'Y'`   |           |       | Row Erase Program Memory

Parameters and replies are 14 bits, encoded as two bytes with the most significant bit set:
The least significant 7 of the first byte contain the most significant 7 bits of the data,
the least significant 7 bits of the second byte contain the least significant 7 bits of the data.

Schematic
---------

This is how you should connect the programmer and the target, for the 14-pin packages:

          .-------------.
          |    +------+  '-------[5V]---
          +----|1   14|-----+----[GND]--    USB
          |   -|2   13|-----|----[D+]---  (to PC)
          |   -|3   12|-----|----[D-]---
          |   -|4   11|-    |
          |   -|5   10|-----|--.
          |   -|6    9|-----|-----.
          |   -|7    8|-.   |  |  |
          |    +------+ |   |  |  |
          |   Programmer|   |  |  |
          |             |   |  |  |
     .----|-------------'   |  |  |
     |    |                 |  |  |
     |    |    +------+     |  |  |
     |    '----|1   14|-----'  |  |
     |        -|2   13|-       |  |
     |        -|3   12|-       |  |
     '-[Reset]-|4   11|-       |  |
              -|5   10|-[Data]-'  |
              -|6    9|-[Clock]---'
              -|7    8|-
               +------+
                Target

Contact
-------

If you have any questions, feel free to contact me: https://m-ou.se/contact
