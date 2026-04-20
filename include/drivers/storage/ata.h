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

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#include "cpu/pci.h"

#define MAX_TIMEOUT_DURATION 1000000

// Registers
extern int REG_DATA;
extern int REG_ERROR;
extern int REG_FEATURES;
extern int REG_SEC_CNT;
extern int REG_LBA_LO;
extern int REG_LBA_MI;
extern int REG_LBA_HI;
extern int REG_DRV_SEL;
extern int REG_COMMAND;
extern int REG_STATUS;
extern int REG_CONTROL;
extern int REG_ALT_STAT;

// Commands
extern int CMD_READ;
extern int CMD_WRITE;
extern int CMD_IDENTIFY;
extern int CMD_FLUSH;

// Status register bits
extern int SR_BSY;
extern int SR_DRDY;
extern int SR_DF;
extern int SR_DSC;
extern int SR_DRQ;
extern int SR_CORR;
extern int SR_IDX;
extern int SR_ERR;

// Controls
extern int CTRL_RESET;
extern int CTRL_NORMAL;
extern int WAIT_INTERS;
extern int LBA_MASTER;
extern int LBA_SLAVE;

// Drives
extern int DRIVE_MASTER;
extern int DRIVE_SLAVE;
extern int LBA_MODE;

// Constants
extern int CTRL_LEGACY;

void RARE_FUNC init_ata(pci_device_t* pci_ata_device);

void ata_read_sector(uint32_t lba, uint8_t* buffer);
void ata_write_sector(uint32_t lba, uint8_t* buffer);

int  ata_wait_ready();

#endif
