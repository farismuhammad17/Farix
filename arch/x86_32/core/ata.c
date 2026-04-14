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

#include "arch/stubs.h"
#include "cpu/pci.h"
#include "drivers/terminal.h"

#include "drivers/ata.h"

// Registers
static int REG_DATA     = 0x1F0; // Data port: 16-bit I/O
static int REG_ERROR    = 0x1F1; // Error register (Read) / Features (Write)
static int REG_FEATURES = 0x1F1;
static int REG_SEC_CNT  = 0x1F2; // Sector count (0-255, 0 = 256)
static int REG_LBA_LO   = 0x1F3; // LBA bits 0-7
static int REG_LBA_MI   = 0x1F4; // LBA bits 8-15
static int REG_LBA_HI   = 0x1F5; // LBA bits 16-23
static int REG_DRV_SEL  = 0x1F6; // Drive select / LBA bits 24-27
static int REG_COMMAND  = 0x1F7; // Command register (Write)
static int REG_STATUS   = 0x1F7; // Status register (Read)
static int REG_CONTROL  = 0x3F6; // Used for software resets.
static int REG_ALT_STAT = 0x3F6; // Same as REG_CONTROL, but read-only

// Commands
static int CMD_READ     = 0x20;  // PIO Read with retry
static int CMD_WRITE    = 0x30;  // PIO Write with retry
static int CMD_IDENTIFY = 0xEC;  // Identify Drive
static int CMD_FLUSH    = 0xE7;  // Cache Flush

// Status register bits
static int SR_BSY       = 0x80;  // Busy: Drive is processing
static int SR_DRDY      = 0x40;  // Drive Ready
static int SR_DF        = 0x20;  // Drive Fault: Hardware failure
static int SR_DSC       = 0x10;  // Seek Complete
static int SR_DRQ       = 0x08;  // Data Request: Ready to move bytes
static int SR_CORR      = 0x04;  // Corrected data
static int SR_IDX       = 0x02;  // Index (Legacy)
static int SR_ERR       = 0x01;  // Error: Check Error Register

// Controls
static int LBA_MASTER   = 0xE0;  // LBA Mode, Master Drive
static int LBA_SLAVE    = 0xF0;  // LBA Mode, Slave Drive

void init_ata() {
    pci_device_t* ata_dev = NULL;

    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == 0x01) {
            ata_dev = &pci_devices[i];
            break;
        }
    }

    if (ata_dev == NULL) {
        t_print("init_ata (ATA): PCI Storage Controller not found");
        // Some very old machines have an IDE controller that is "invisible" to PCI but still exists at 0x1F0
    }

    // Enable I/O Space and Bus Mastering in PCI Command Register
    // Offset 0x04 is the Command Register. Bit 0: I/O Space, Bit 2: Bus Master.
    uint32_t pci_cmd = pci_read(ata_dev->bus, ata_dev->device, ata_dev->function, 0x04);
    pci_write(ata_dev->bus, ata_dev->device, ata_dev->function, 0x04, pci_cmd | 0x05);

    // Retrieve Base Address from BAR0 (Base Address Register 0)
    uint32_t bar0 = pci_read(ata_dev->bus, ata_dev->device, ata_dev->function, 0x10);
    uint32_t bar1 = pci_read(ata_dev->bus, ata_dev->device, ata_dev->function, 0x14);

    // If bit 0 is set, it's I/O space. If not, it's Memory Mapped.
    uint16_t base = (bar0 & 1) ? (uint16_t)(bar0 & 0xFFFC) : 0x1F0;
    uint16_t ctrl = (bar1 & 1) ? (uint16_t)(bar1 & 0xFFFC) : 0x3F4;

    // If ctrl is 0, we MUST fallback to the legacy control port.
    if (ctrl == 0) ctrl = 0x3F4;

    // Revalue the global register variables
    REG_DATA     = base;
    REG_ERROR    = base + 1;
    REG_FEATURES = base + 1;
    REG_SEC_CNT  = base + 2;
    REG_LBA_LO   = base + 3;
    REG_LBA_MI   = base + 4;
    REG_LBA_HI   = base + 5;
    REG_DRV_SEL  = base + 6;
    REG_COMMAND  = base + 7;
    REG_STATUS   = base + 7;
    REG_CONTROL  = ctrl + 2;

    outb(REG_CONTROL, 0x04);                     // Set SRST bit (Software Reset)
    for(int i = 0; i < 20; i++) inb(REG_STATUS); // Wait for the hardware to react
    outb(REG_CONTROL, 0x00);                     // Clear SRST (Back to normal operation)
    for(int i = 0; i < 20; i++) inb(REG_STATUS);

    outb(REG_DRV_SEL, 0xA0); // Master
    for(int i = 0; i < 4; i++) inb(REG_STATUS); // 400ns "Select" delay

    // Clear the counts
    outb(REG_SEC_CNT, 0);
    outb(REG_LBA_LO, 0);
    outb(REG_LBA_MI, 0);
    outb(REG_LBA_HI, 0);

    outb(REG_COMMAND, CMD_IDENTIFY);

    uint8_t status = inb(REG_STATUS);
    if (status == 0 || status == 0xFF) {
        t_print("init_ata (ATA): Floating bus, unresponsive hardware at port");
        return;
    }
    for (int i = 0; i < 3; i++) inb(REG_STATUS); // 400ns "Command" delay (1 from earlier 'status')

    ata_wait_ready();

    for (int i = 0; i < 256; i++) inw(REG_DATA);
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    outb(REG_DRV_SEL, (uint8_t)((lba >> 24) | LBA_MASTER));
    for(int i = 0; i < 4; i++) inb(REG_STATUS);

    outb(REG_SEC_CNT, 1);
    outb(REG_LBA_LO, (uint8_t) lba);
    outb(REG_LBA_MI, (uint8_t)(lba >> 8));
    outb(REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(REG_COMMAND, CMD_READ);

    ata_wait_ready();

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) ptr[i] = inw(REG_DATA);
}

void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    outb(REG_DRV_SEL, (uint8_t)((lba >> 24) | LBA_MASTER));
    for(int i = 0; i < 4; i++) inb(REG_STATUS);

    outb(REG_SEC_CNT, 1);
    outb(REG_LBA_LO, (uint8_t) lba);
    outb(REG_LBA_MI, (uint8_t)(lba >> 8));
    outb(REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(REG_COMMAND, CMD_WRITE);

    int wait_stat = ata_wait_ready(); // Wait for DRQ before sending data
    if (wait_stat) {
        t_printf("ata_write_sector (ATA): Write aborted at %x", lba);
        return;
    }

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) outw(REG_DATA, ptr[i]);

    outb(REG_COMMAND, CMD_FLUSH);
    while (inb(REG_STATUS) & SR_BSY);
}

int ata_wait_ready() {
    // 400ns delay for status to stabilize
    for(int i = 0; i < 4; i++) inb(REG_STATUS);

    uint8_t status;
    uint32_t timeout = 1000000;

    while (((status = inb(REG_STATUS)) & SR_BSY) && --timeout > 0) {
        if (status == 0xFF) {
            t_print("ata_wait_ready (ATA): Bus floating/dead");
            return 1;
        }
    }

    if (timeout == 0) {
        t_print("ata_wait_ready (ATA): Timeout waiting for BSY to clear");
        return 1;
    }

    timeout = 1000000;
    while (!((status = inb(REG_STATUS)) & SR_DRQ) && --timeout > 0) {
        if (status & SR_ERR) {
            t_printf("ata_wait_ready (ATA): status: %x, error reg: %x",
                     status, inb(REG_ERROR));
            return 1;
        }
    }

    if (timeout == 0) {
        t_print("ata_wait_ready (ATA): Timeout waiting for DRQ");
        return 1;
    }

    return 0;
}
