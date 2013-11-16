This folder contains 'picp', a PIC16F145x based PIC16F145x programmer.

To program a PIC16F145x with this, all you need is a PIC16F145x programmed
with the software in /pic/ connected with USB to a Linux computer with the
software in /pc/. (So you still need to get the first PIC programmed
somehow, but after that, you have a very simple and cheap programmer.)

No additional circuit is required, the USB wires are directly connected to the
programmer PIC, and the pins of that PIC are directly connected to the PIC
that's to be programmed.

== Usage ==

1. Program a PIC16F154x with the software in /pic/.
2. Compile the software in /pc/ and put the 'picp' executale in your PATH.
   (e.g. /usr/local/bin)
3. Copy udev/40-picp.rules to /etc/udev/rules.d/.
4. Connect the ICSP pins (RC0 and RC1) of the programmer and the
   to-be-programmed chip, and connect RC2 of the programmer to the reset pin
   (RA3) of the to-be-programmed chip.
5. Connect the programmer with USB, and use 'picp /dev/picp0 upload < file' to
   flash a program.

