/*
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include "arch/stubs.h"

#include "arch/x86/pci.h"

#include <stdint.h>

pci_device_t pci_devices[32];
int pci_device_count = 0;

static uint32_t read_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    // Port 0xCF8: CONFIG_ADDRESS
    // Port 0xCFC: CONFIG_DATA

    uint32_t address = (uint32_t)((bus << 16) | (device << 11) |
                       (func << 8) | (offset & 0xFC) | ((uint32_t) 0x80000000));

    // Write the address we want to read
    outl(0xCF8, address);

    // Read the data from the data port
    return inl(0xCFC);
}

void init_pci() {
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            uint32_t data = read_config(bus, dev, 0, 0);

            if ((data & 0xFFFF) != 0xFFFF) { // Found some device
                if (pci_device_count < 32) {
                    pci_device_t* d = &pci_devices[pci_device_count++];
                    d->vendor_id = data & 0xFFFF;
                    d->device_id = (data >> 16) & 0xFFFF;
                    d->bus       = bus;
                    d->device    = dev;
                    d->function  = 0;

                    // Read class code (at offset 0x08) to know what it is
                    uint32_t class_data = read_config(bus, dev, 0, 0x08);
                    d->class_code = (class_data >> 24) & 0xFF;
                    d->subclass   = (class_data >> 16) & 0xFF;
                }
            }
        }
    }
}
