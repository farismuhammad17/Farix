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

#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>

#include "cpu/pci.h"

extern int HBA_PxCMD_ST;
extern int HBA_PxCMD_FRE;
extern int HBA_PxCMD_FR;
extern int HBA_PxCMD_CR;

void RARE_FUNC init_ahci(pci_device_t* pci_ahci_device);

void ahci_read_sector(uint32_t lba, uint8_t* buffer);
void ahci_write_sector(uint32_t lba, uint8_t* buffer);

#endif
