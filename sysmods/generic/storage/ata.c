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

#include <stdint.h>

#include "hal.h"

#include "cpu/pci.h"

#include "sysmods/devices.h"
#include "sysmods/interface.h"

#include "drivers/storage.h"

// Commands
#define CMD_READ      0x20  // PIO Read with retry
#define CMD_WRITE     0x30  // PIO Write with retry
#define CMD_IDENTIFY  0xEC  // Identify Drive
#define CMD_FLUSH     0xE7  // Cache Flush

// Status register bits
#define SR_BSY        0x80  // Busy: Drive is processing
#define SR_DRDY       0x40  // Drive Ready
#define SR_DF         0x20  // Drive Fault: Hardware failure
#define SR_DSC        0x10  // Seek Complete
#define SR_DRQ        0x08  // Data Request: Ready to move bytes
#define SR_CORR       0x04  // Corrected data
#define SR_IDX        0x02  // Index (Legacy)
#define SR_ERR        0x01  // Error: Check Error Register

// Controls
#define CTRL_RESET    0x04  // SRST: Software Reset bit
#define CTRL_NORMAL   0x00  // Normal operation (Clear Reset)
#define LBA_MASTER    0xE0  // LBA Mode, Master Drive
#define LBA_SLAVE     0xF0  // LBA Mode, Slave Drive

// Drives
#define DRIVE_MASTER  0xA0  // Select Master Drive (CHS/Legacy)
#define DRIVE_SLAVE   0xB0  // Select Slave Drive (CHS/Legacy)
#define LBA_MODE      0x40  // Bit to enable Logical Block Addressing

// Constants
#define CTRL_LEGACY   0x3F4 // Legacy Base address for Control Block

#define ATA_LBA28_MAX 0x0FFFFFFFULL

#define MAX_TIMEOUT_DURATION 1000000

// Registers
static int REG_DATA      = 0x1F0; // Data port: 16-bit I/O
static int REG_ERROR     = 0x1F1; // Error register (Read) / Features (Write)
static int REG_FEATURES  = 0x1F1;
static int REG_SEC_CNT   = 0x1F2; // Sector count (0-255, 0 = 256)
static int REG_LBA_LO    = 0x1F3; // LBA bits 0-7
static int REG_LBA_MI    = 0x1F4; // LBA bits 8-15
static int REG_LBA_HI    = 0x1F5; // LBA bits 16-23
static int REG_DRV_SEL   = 0x1F6; // Drive select / LBA bits 24-27
static int REG_COMMAND   = 0x1F7; // Command register (Write)
static int REG_STATUS    = 0x1F7; // Status register (Read)
static int REG_CONTROL   = 0x3F6; // Used for software resets.
static int REG_ALT_STAT  = 0x3F6; // Same as REG_CONTROL, but read-only

static kernel_api_t* k_api = NULL;
static storage_dev_t* dev  = NULL;
static uint64_t base_addr  = 0;

static pci_device_t* pci_dev = NULL;

static bool ata_wait_ready() {
    // 400ns delay for status to stabilize
    for (int i = 0; i < 4; i++) inb(REG_STATUS);

    uint8_t status;
    uint32_t timeout = MAX_TIMEOUT_DURATION;

    while (((status = inb(REG_STATUS)) & SR_BSY) && --timeout > 0) {
        if (unlikely(status == 0xFF)) {
            k_api->err_print("ata_wait_ready: Bus floating/dead");
            return true;
        }
        system_pause(); // Prevents fast 64-bit CPUs from burning through the timeout
    }

    if (unlikely(timeout == 0)) {
        k_api->err_print("ata_wait_ready: Timeout waiting for BSY to clear");
        return true;
    }

    timeout = MAX_TIMEOUT_DURATION;
    while (!((status = inb(REG_STATUS)) & SR_DRQ) && --timeout > 0) {
        if (unlikely(status & SR_ERR)) {
            k_api->err_printf("ata_wait_ready: status: %x, error reg: %x",
                     status, inb(REG_ERROR));
            return true;
        }
        system_pause();
    }

    if (unlikely(timeout == 0)) {
        k_api->err_print("ata_wait_ready: Timeout waiting for DRQ");
        return true;
    }

    return false;
}

static void ata_read_sector(uint64_t lba, uint8_t* buffer) {
    if (unlikely(lba > ATA_LBA28_MAX)) {
        k_api->err_printf("ata_read_sector: LBA out of bounds for LBA28 (%x)", lba);
        return;
    }

    outb(REG_DRV_SEL, (uint8_t)(((lba >> 24) & 0x0F) | LBA_MASTER));
    for(int i = 0; i < 4; i++) inb(REG_STATUS);

    outb(REG_SEC_CNT, 1);
    outb(REG_LBA_LO, (uint8_t) lba);
    outb(REG_LBA_MI, (uint8_t)(lba >> 8));
    outb(REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(REG_COMMAND, CMD_READ);

    bool wait_stat = SYS_ICALL(ata_wait_ready); // Wait for DRQ before sending data
    if (unlikely(wait_stat)) {
        k_api->err_printf("ata_read_sector: Read aborted at %x", lba);
        return;
    }

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) ptr[i] = inw(REG_DATA);
}

static void ata_write_sector(uint64_t lba, uint8_t* buffer) {
    if (unlikely(lba > ATA_LBA28_MAX)) {
        k_api->err_printf("ata_write_sector: LBA out of bounds for LBA28 (%x)", lba);
        return;
    }

    outb(REG_DRV_SEL, (uint8_t)(((lba >> 24) & 0x0F) | LBA_MASTER));
    for(int i = 0; i < 4; i++) inb(REG_STATUS);

    outb(REG_SEC_CNT, 1);
    outb(REG_LBA_LO, (uint8_t) lba);
    outb(REG_LBA_MI, (uint8_t)(lba >> 8));
    outb(REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(REG_COMMAND, CMD_WRITE);

    bool wait_stat = SYS_ICALL(ata_wait_ready); // Wait for DRQ before sending data
    if (unlikely(wait_stat)) {
        k_api->err_printf("ata_write_sector: Write aborted at %x", lba);
        return;
    }

    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) outw(REG_DATA, ptr[i]);

    outb(REG_COMMAND, CMD_FLUSH);

    // Safely spin for flush completion
    while (inb(REG_STATUS) & SR_BSY) {
        system_pause();
    }
}

static int init_ata(kernel_api_t* api, uint64_t b_addr) {
    k_api = api;
    base_addr = b_addr;

    for (int i = 0; i < PCI_MAX_DEVICES; i++) {
        if (k_api->pci_devices[i].subclass == PCI_ATA_SUBCLASS) {
            pci_dev = &k_api->pci_devices[i];
            break;
        }
    }

    if (unlikely(!pci_dev)) {
        k_api->err_print("init_ata: ATA device not found");
        return 1;
    }

    // Enable I/O Space and Bus Mastering in PCI Command Register
    // Offset 0x04 is the Command Register. Bit 0: I/O Space, Bit 2: Bus Master.
    uint32_t pci_cmd = k_api->pci_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x04);
    k_api->pci_write(pci_dev->bus, pci_dev->device, pci_dev->function, 0x04, pci_cmd | 0x05);

    // Retrieve Base Address from BAR0 (Base Address Register 0)
    uint32_t bar0 = k_api->pci_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x10);
    uint32_t bar1 = k_api->pci_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x14);

    // If bit 0 is set, it's I/O space. If not, it's Memory Mapped.
    uint16_t base = (bar0 & 1) ? (uint16_t)(bar0 & 0xFFFC) : REG_DATA;
    uint16_t ctrl = (bar1 & 1) ? (uint16_t)(bar1 & 0xFFFC) : CTRL_LEGACY;

    // If ctrl is 0, we MUST fallback to the legacy control port.
    if (unlikely(ctrl == 0)) ctrl = CTRL_LEGACY;

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

    outb(REG_CONTROL, CTRL_RESET);               // Set SRST bit (Software Reset)
    for(int i = 0; i < 20; i++) inb(REG_STATUS); // Wait for the hardware to react
    outb(REG_CONTROL, CTRL_NORMAL);              // Clear SRST (Back to normal operation)
    for(int i = 0; i < 20; i++) inb(REG_STATUS);

    outb(REG_DRV_SEL, DRIVE_MASTER); // Master
    for(int i = 0; i < 4; i++) inb(REG_STATUS); // 400ns "Select" delay

    // Clear the counts
    outb(REG_SEC_CNT, 0);
    outb(REG_LBA_LO, 0);
    outb(REG_LBA_MI, 0);
    outb(REG_LBA_HI, 0);

    outb(REG_COMMAND, CMD_IDENTIFY);

    uint8_t status = inb(REG_STATUS);
    if (unlikely(status == 0 || status == 0xFF)) {
        k_api->err_print("init_ata: Floating bus, unresponsive hardware at port");
        return 2;
    }
    for (int i = 0; i < 3; i++) inb(REG_STATUS); // 400ns "Command" delay (1 from earlier 'status')

    SYS_ICALL(ata_wait_ready);

    for (int i = 0; i < 256; i++) inw(REG_DATA);

    dev = k_api->kmalloc(sizeof(storage_dev_t));
    dev->id = ATA_DEV_ID;
    dev->type = DEV_STORAGE;

    dev->read_sector = (void*) SYSMOD_TO_KERNEL(ata_read_sector);
    dev->write_sector = (void*) SYSMOD_TO_KERNEL(ata_write_sector);

    k_api->register_device(DEV_STORAGE, (void*) dev);

    return 0;
}

static int exit_ata() {
    uint32_t pci_cmd = k_api->pci_read(pci_dev->bus, pci_dev->device, pci_dev->function, 0x04);
    k_api->pci_write(pci_dev->bus, pci_dev->device, pci_dev->function, 0x04, pci_cmd & ~0x05);

    k_api->unregister_device(DEV_STORAGE, (void*) dev);

    k_api->kfree(dev);

    return 0;
}

SYSMOD_HEADER sysmod_t module_entry = {
    .name = "ATA",
    .init = init_ata,
    .exit = exit_ata
};
