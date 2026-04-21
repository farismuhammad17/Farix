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

#include <stdint.h>

#include "hal.h"

#include "cpu/pci.h"

pci_device_t pci_devices[32];
int pci_device_count;

void init_pci() {
    pci_device_count = 0;

    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            for (int func = 0; func < 8; func++) { // Check all 8 functions per device
                uint32_t data = pci_read(bus, dev, func, 0);

                // 0xFFFFFFFF means no device is present
                if ((data & 0xFFFF) == 0xFFFF) {
                    // If function 0 is missing, don't bother checking 1-7
                    if (func == 0) break;
                    continue;
                }

                if (pci_device_count >= 32) {
                    t_print("init_pci: pci_devices limit reached");
                    return;
                }

                pci_device_t* d = &pci_devices[pci_device_count++];
                d->vendor_id = data & 0xFFFF;
                d->device_id = (data >> 16) & 0xFFFF;
                d->bus       = bus;
                d->device    = dev;
                d->function  = func;

                uint32_t class_data = pci_read(bus, dev, func, 0x08);
                d->class_code = (class_data >> 24) & 0xFF;
                d->subclass   = (class_data >> 16) & 0xFF;
            }
        }
    }

    if (pci_device_count == 0) t_print("init_pci: No devices found on bus");
}

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    uint32_t addr = (1U << 31) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val) {
    uint32_t addr = (1U << 31) | (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);
    outl(0xCF8, addr);
    outl(0xCFC, val);
}
