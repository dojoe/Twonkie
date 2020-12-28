#!/usr/bin/env python3
import usb1
from sys import argv, stdin, stdout
from threading import Thread

endpoints = {
    "SHELL": 0,
    "SNIFFER": 1,
    "COMMANDS": 2,
}

iface = 0 if len(argv) < 2 else endpoints(argv[1].upper())


class Printer(Thread):
    def __init__(self, handle):
        super().__init__(daemon=True)
        self.handle = handle

    def run(self):
        while True:
            data = handle.bulkRead(0x81 + iface, 64)
            stdout.write(bytes(data).decode())
            stdout.flush()


context = usb1.USBContext()
handle = context.openByVendorIDAndProductID(0x18D1, 0x500A)
with handle.claimInterface(iface):
    printer = Printer(handle)
    printer.start()
    for line in stdin:
        handle.bulkWrite(0x1 + iface, line.encode())
