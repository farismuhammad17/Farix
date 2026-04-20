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

#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>

#include "cpu/pci.h"

// Reference: https://wiki.osdev.org/AHCI
typedef struct {
    uint32_t clb;       // 0x00: Command List Base Address (Lower 32)
    uint32_t clbu;      // 0x04: Command List Base Address (Upper 32)
    uint32_t fb;        // 0x08: FIS Base Address (Lower 32)
    uint32_t fbu;       // 0x0C: FIS Base Address (Upper 32)
    uint32_t is;        // 0x10: Interrupt Status
    uint32_t ie;        // 0x14: Interrupt Enable
    uint32_t cmd;       // 0x18: Command and Status
    uint32_t rsv0;      // 0x1C: Reserved
    uint32_t tfd;       // 0x20: Task File Data (Status/Error bits)
    uint32_t sig;       // 0x24: Signature (Tells you if it's SATA, ATAPI, etc)
    uint32_t ssts;      // 0x28: SATA Status (Presence, Speed)
    uint32_t sctl;      // 0x2C: SATA Control
    uint32_t serr;      // 0x30: SATA Error
    uint32_t sact;      // 0x34: SATA Active
    uint32_t ci;        // 0x38: Command Issue (The "Go" button)
    uint32_t sntf;      // 0x3C: SATA Notification
    uint32_t fbs;       // 0x40: FIS-based Switching Control
    uint32_t rsv1[11];  // 0x44 - 0x6F: Reserved
    uint32_t vendor[4]; // 0x70 - 0x7F: Vendor specific
} hba_port_t;

typedef struct {
    uint32_t cap;      // 0x00: Host Capabilities
    uint32_t ghc;      // 0x04: Global Host Control
    uint32_t is;       // 0x08: Interrupt Status
    uint32_t pi;       // 0x0C: Ports Implemented (Bitmask)
    uint32_t vs;       // 0x10: AHCI Version
    uint32_t ccc_ctl;  // 0x14: Command Completion Coalescing Control
    uint32_t ccc_pts;  // 0x18: Command Completion Coalescing Ports
    uint32_t em_loc;   // 0x1C: Enclosure Management Location
    uint32_t em_ctl;   // 0x20: Enclosure Management Control
    uint32_t cap2;     // 0x24: Host Capabilities Extended
    uint32_t bohc;     // 0x28: BIOS/OS Handoff Control and Status

    uint8_t  rsv[0xA0];    // 0x2C - 0x9F: Reserved
    uint8_t  vendor[0x60]; // 0xA0 - 0xFF: Vendor Specific registers

    hba_port_t ports[32]; // 0x100: Port registers start here
} hba_mem_t;

extern int HBA_PxCMD_ST;
extern int HBA_PxCMD_FRE;
extern int HBA_PxCMD_FR;
extern int HBA_PxCMD_CR;

void RARE_FUNC init_ahci(pci_device_t* pci_ahci_device);

void ahci_read_sector(uint32_t lba, uint8_t* buffer);
void ahci_write_sector(uint32_t lba, uint8_t* buffer);

void ahci_probe_port(volatile hba_port_t* port, int port_no);

#endif
