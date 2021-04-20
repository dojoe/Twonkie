# Twinkie / Twonkie firmware sources

These sources are the original sources from the Chromium OS Embedded Controller
project, stripped down to the minimum required to build the Twinkie/Twonkie
firmware.

# Building

## Prerequisites

Beyond what your distro should provide anyway, you'll need `arm-none-eabi-gcc`
to build the firmware and not much else.

## Building the Twonkie firmware

`make -j`

The completed firmware will end up in `build/twonkie/ec.bin` and can be flashed
as is.

## Building the Twinkie firmware

`make -j BOARD=twinkie`

The completed firmware will end up in `build/twinkie/ec.bin` and can be flashed
as is.

## Pre-built firmware

Check out the GitHub [Releases](https://github.com/dojoe/Twonkie/releases) page
for pre-built firmware images.

# Flashing

For flashing you'll need `dfu-util`. On Linux your package manager should offer
a package for it, for Windows you can find builds on their [SourceForge site](http://dfu-util.sourceforge.net/).

## Enter the bootloader

There are several ways to put Twinkie or Twonkie into bootloader mode:

 1. Twonkie only: Via the bootloader button. Press and hold the button while
    plugging in the USB cable.
 2. Via a cursed OTG cable. Plug an A-to-A USB cable into an USB-OTG adapter and
    use that concoction to connect Twonkie to your computer.
    The OTG adapter pulls the ID pin on the Micro-B connector high, a circuit on
    the Twonkie board detects this and asserts the STM32's bootloader signal.
 3. Via the `dfu` command at the USB console. Obviously this only works once
    there's a working firmware on the chip.

## Flash the firmware

`dfu-util -a 0 -s 0x08000000 -D ec.bin`

## Exit the bootloader

If you entered DFU mode via the button or a cursed cable, just unplug Twonkie
and plug it in again using a regular cable.

If you entered DFU mode via the `dfu` command at the console, you have to
switch back using this command:

`dfu-util -a 0 -s 0x08000000:force:unprotect`

This will not actually flash anything, it will only exit the bootloader.

# USB console

## Linux

Twonkie has a simple console mode using a pair of bulk USB endpoints. In Linux,
the USB serial driver should support this mode and make the console available as
`/dev/ttyUSBx`. If that device doesn't show up immediately you can try

    sudo modprobe usbserial
    echo '18d1 500A' | sudo tee /sys/bus/usb-serial/drivers/generic/new_id

## Windows

For Windows there's a simple USB console program in [util/shell.py](util/shell.py).
It requires the `libusb1` package from PyPI.
