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

#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#define PCI_CLASS_CODE_STORAGE 0x01
#define PCI_ATA_SUBCLASS       0x01
#define PCI_AHCI_SUBCLASS      0x06

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint8_t  class_code;
    uint8_t  subclass;
} pci_device_t;

extern pci_device_t pci_devices[32];
extern int pci_device_count;

void RARE_FUNC init_pci();

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);

#endif
