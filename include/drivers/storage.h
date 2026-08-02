/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

#include "sysmods/devices.h"

#define PCI_CLASS_CODE_STORAGE 0x01

#define PCI_ATA_SUBCLASS       0x01
#define PCI_AHCI_SUBCLASS      0x06

typedef struct {
    uint8_t id;
    dev_type_t type;

    void (*read_sector)(uint64_t lba, uint8_t* buffer);
    void (*write_sector)(uint64_t lba, uint8_t* buffer);
} storage_dev_t;

extern storage_dev_t* storage_dev;

#endif
