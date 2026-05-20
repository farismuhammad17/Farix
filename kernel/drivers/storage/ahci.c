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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hal.h"

#include "cpu/irq.h"
#include "cpu/pci.h"
#include "cpu/timer.h"
#include "drivers/storage/bdl.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "drivers/storage/ahci.h"

#define MAX_TIMEOUT_DURATION 1000000

#define AHCI_HBA_PAGES 5
#define AHCI_BASE_VIRT 0xF0000000

// Reference: Serial ATA AHCI: Specification, Rev. 1.3.1, section 3.1.2
#define GHC_HR       (1 << 0)
#define GHC_IE       (1 << 1)
#define GHC_MRSM     (1 << 2)
#define GHC_AE       (1 << 31)

// Reference: Serial ATA AHCI: Specification, Rev. 1.3.1, section 3.3.6
#define PX_IE_TFES   (1 << 30)

// Reference: Serial ATA AHCI: Specification, Rev. 1.3.1, section 3.3.7
#define PX_CMD_ST    (1 << 0)
#define PX_CMD_FRE   (1 << 4)
#define PX_CMD_FR    (1 << 14)
#define PX_CMD_CR    (1 << 15)

// Reference: Serial ATA AHCI: Specification, Rev. 1.3.1, section 3.3.8
#define PX_TFD_DRQ   (1 << 3)
#define PX_TFD_BSY   (1 << 7)

// Reference: Serial ATA AHCI: Specification, Rev. 1.3.1, section 3.1.1
#define HBA_CAP_NCS(cap) (((cap) >> 8) & 0x1F)

// Reference: Serial ATA AHCI: Specification, Rev. 1.3.1, section 3.1.11
#define BOHC_BOS (1 << 0)
#define BOHC_OOS (1 << 1)
#define BOHC_BB  (1 << 4)

// Device Signatures (values for PxSIG, port signature)
#define AHCI_SIG_SATA   0x00000101  // Standard SATA Hard Disk / SSD
#define AHCI_SIG_SATAPI 0xEB140101  // CD/DVD-ROM (ATAPI)
#define AHCI_SIG_SEMB   0xC33C0101  // Enclosure Management Bridge
#define AHCI_SIG_PM     0x96690101  // Port Multiplier

// Reference: https://wiki.osdev.org/AHCI; SATA Basic; FIS types
#define FIS_TYPE_REG_H2D	0x27	// Register FIS - host to device
#define FIS_TYPE_REG_D2H	0x34	// Register FIS - device to host
#define FIS_TYPE_DMA_ACT	0x39	// DMA activate FIS - device to host
#define FIS_TYPE_DMA_SETUP	0x41	// DMA setup FIS - bidirectional
#define FIS_TYPE_DATA		0x46	// Data FIS - bidirectional
#define FIS_TYPE_BIST		0x58	// BIST activate FIS - bidirectional
#define FIS_TYPE_PIO_SETUP	0x5F	// PIO setup FIS - device to host
#define FIS_TYPE_DEV_BITS	0xA1	// Set device bits FIS - device to host

#define FIS_CMD_READ        0x25
#define FIS_CMD_WRITE       0x35
#define FIS_SATA_LBA_MODE   (1 << 6)

// Reference: https://wiki.osdev.org/AHCI; AHCI Registers and Memory Structures; HBA memory registers
typedef struct {
    uint32_t clb;		// 0x00, command list base address, 1K-byte aligned
	uint32_t clbu;		// 0x04, command list base address upper 32 bits
	uint32_t fb;		// 0x08, FIS base address, 256-byte aligned
	uint32_t fbu;		// 0x0C, FIS base address upper 32 bits
	uint32_t is;		// 0x10, interrupt status
	uint32_t ie;		// 0x14, interrupt enable
	uint32_t cmd;		// 0x18, command and status
	uint32_t rsv0;		// 0x1C, Reserved
	uint32_t tfd;		// 0x20, task file data
	uint32_t sig;		// 0x24, signature
	uint32_t ssts;		// 0x28, SATA status (SCR0:SStatus)
	uint32_t sctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32_t serr;		// 0x30, SATA error (SCR1:SError)
	uint32_t sact;		// 0x34, SATA active (SCR3:SActive)
	uint32_t ci;		// 0x38, command issue
	uint32_t sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fbs;		// 0x40, FIS-based switch control
	uint32_t rsv1[11];	// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	// 0x70 ~ 0x7F, vendor specific
} hba_port_t;

// Reference: https://wiki.osdev.org/AHCI; AHCI Registers and Memory Structures; HBA memory registers
typedef struct {
    // 0x00 - 0x2B, Generic Host Control
	uint32_t cap;		// 0x00, Host capability
	uint32_t ghc;		// 0x04, Global host control
	uint32_t is;		// 0x08, Interrupt status
	uint32_t pi;		// 0x0C, Port implemented
	uint32_t vs;		// 0x10, Version
	uint32_t ccc_ctl;	// 0x14, Command completion coalescing control
	uint32_t ccc_pts;	// 0x18, Command completion coalescing ports
	uint32_t em_loc;	// 0x1C, Enclosure management location
	uint32_t em_ctl;	// 0x20, Enclosure management control
	uint32_t cap2;		// 0x24, Host capabilities extended
	uint32_t bohc;		// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	uint8_t  rsv[0xA0-0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t  vendor[0x100-0xA0];

	// 0x100 - 0x10FF, Port control registers
	hba_port_t	ports[32];	// 1 ~ 32
} hba_mem_t;

// Reference: https://wiki.osdev.org/AHCI; AHCI Registers and Memory Structures; Command Table and Physical Region Descriptor Table
typedef struct {
	uint32_t dba;		// Data base address
	uint32_t dbau;		// Data base address upper 32 bits
	uint32_t rsv0;		// Reserved

	// DW3
	uint32_t dbc:22;		// Byte count, 4M max
	uint32_t rsv1:9;		// Reserved
	uint32_t i:1;		// Interrupt on completion
} hba_prdt_entry_t;

// Reference: https://wiki.osdev.org/AHCI; AHCI Registers and Memory Structures; Command List
typedef struct {
	// DW0
	uint8_t  cfl:5;     // Command FIS length in DWORDS, 2 ~ 16
	uint8_t  a:1;       // ATAPI
	uint8_t  w:1;       // Write, 1: H2D, 0: D2H
	uint8_t  p:1;       // Prefetchable

	uint8_t  r:1;       // Reset
	uint8_t  b:1;       // BIST
	uint8_t  c:1;       // Clear busy upon R_OK
	uint8_t  rsv0:1;    // Reserved
	uint8_t  pmp:4;     // Port multiplier port

	uint16_t prdtl;     // Physical region descriptor table length in entries

	// DW1
	volatile uint32_t prdbc; // Physical region descriptor byte count transferred

	// DW2, 3
	uint32_t ctba;  // Command table descriptor base address
	uint32_t ctbau; // Command table descriptor base address upper 32 bits

	// DW4 - 7
	uint32_t rsv1[4];   // Reserved
} __attribute__((packed)) hba_cmd_header_t;

// Reference: https://wiki.osdev.org/AHCI; AHCI Registers and Memory Structures; Command Table and Physical Region Descriptor Table
typedef struct {
	// 0x00
	uint8_t  cfis[64];	// Command FIS

	// 0x40
	uint8_t  acmd[16];	// ATAPI command, 12 or 16 bytes

	// 0x50
	uint8_t  rsv[48];	// Reserved

	// 0x80
	hba_prdt_entry_t prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} __attribute__((packed)) hba_cmd_table_t;

// Reference: https://wiki.osdev.org/AHCI; SATA Basic; Register FIS – Host to Device
typedef struct {
	// DWORD 0
	uint8_t  fis_type;

	uint8_t  pmport:4;	// Port multiplier
	uint8_t  rsv0:3;	// Reserved
	uint8_t  c:1;		// 1: Command, 0: Control

	uint8_t  command;	// Command register
	uint8_t  featurel;	// Feature register, 7:0

	// DWORD 1
	uint8_t  lba0;		// LBA low register, 7:0
	uint8_t  lba1;		// LBA mid register, 15:8
	uint8_t  lba2;		// LBA high register, 23:16
	uint8_t  device;	// Device register

	// DWORD 2
	uint8_t  lba3;		// LBA register, 31:24
	uint8_t  lba4;		// LBA register, 39:32
	uint8_t  lba5;		// LBA register, 47:40
	uint8_t  featureh;	// Feature register, 15:8

	// DWORD 3
	uint8_t  countl;	// Count register, 7:0
	uint8_t  counth;	// Count register, 15:8
	uint8_t  icc;		// Isochronous command completion
	uint8_t  control;	// Control register

	// DWORD 4
	uint8_t  rsv1[4];	// Reserved
} fis_reg_h2d_t;

static volatile hba_mem_t* g_hba = NULL;
static hba_port_t* active_drives[32] = {NULL};
static int drives_found = 0;

static uint8_t ahci_bounce[512] __attribute__((aligned(512)));

static BDLDevice bdl_ahci_device = {
    .read  = ahci_read_sector,
    .write = ahci_write_sector
};

static bool ahci_stop_port(hba_port_t *port) {
    // Clear ST (Start)
    port->cmd &= ~PX_CMD_ST;

    // Wait for CR to clear
    int timeout = MAX_TIMEOUT_DURATION;
    while (timeout--) {
        if (unlikely(!(port->cmd & PX_CMD_CR))) break;
        system_pause();
    }
    if (unlikely(timeout <= 0)) return false; // Failed to stop command engine

    // Clear FRE
    port->cmd &= ~PX_CMD_FRE;

    // Wait for FR to clear
    timeout = MAX_TIMEOUT_DURATION;
    while (timeout--) {
        if (unlikely(!(port->cmd & PX_CMD_FR))) break;
        system_pause();
    }

    return (timeout > 0);
}

static hba_port_t* ahci_find_free_port(size_t max_retry_attempts) {
    for (size_t attempt_i = 0; attempt_i < max_retry_attempts; attempt_i++) {
        for (size_t port_i = 0; port_i < drives_found; port_i++) {
            hba_port_t* port = active_drives[port_i];

            // We want only SATA
            if (port->sig == AHCI_SIG_SATA && !(port->tfd & (PX_TFD_BSY | PX_TFD_DRQ))) {
                return port;
            }
        }

        timer_stall(1000); // 1 ms
    }

    return NULL;
}

static int ahci_find_free_slot(hba_port_t* port) {
    // If a bit is set in either register, the slot is occupied
    uint32_t slots = (port->ci | port->sact);

    for (int i = 0; i < 32; i++) {
        if (!((slots >> i) & 1)) {
            return i;
        }
    }

    return -1; // All 32 slots are currently full
}

void init_ahci(pci_device_t* dev) {
    static int timeout;

    // Enable PCI Bus Mastering and Memory Space
    uint32_t pci_cmd = pci_read(dev->bus, dev->device, dev->function, 0x04);
    pci_write(dev->bus, dev->device, dev->function, 0x04, pci_cmd | (1 << 1) | (1 << 2));

    uint8_t irq_line = (uint8_t) pci_read(dev->bus, dev->device, dev->function, 0x3C);

    // Map BAR5 (ABAR)
    uint32_t  bar5 = pci_read(dev->bus, dev->device, dev->function, 0x24);
    uintptr_t phys = bar5 & 0xFFFFFFF0;

    // Check for 64-bit BAR
    if (unlikely(((bar5 >> 1) & 0x03) == 0x02)) {
        uint32_t upper = pci_read(dev->bus, dev->device, dev->function, 0x28);
        if (unlikely(upper > 0)) {
            err_print("AHCI: BAR5 above 4GB. 32-bit kernel cannot map this");
            return;
        }
    }

    uint32_t* pd = vmm_get_current_directory();
    for (int i = 0; i < AHCI_HBA_PAGES; i++) {
        vmm_map_page(pd, phys + (i * PAGE_SIZE), AHCI_BASE_VIRT + (i * PAGE_SIZE),
                     PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
    }

    g_hba = (volatile hba_mem_t*) AHCI_BASE_VIRT;

    // BIOS-OS Handoff
    if (g_hba->cap2 & 0x01) {
        g_hba->bohc |= BOHC_OOS; // Set OS Owned Semaphore

        timeout = MAX_TIMEOUT_DURATION;
        while ((g_hba->bohc & BOHC_BOS) && --timeout) { // Wait for BIOS Owned Semaphore to clear
            system_pause();
        }

        if (unlikely(timeout == 0)) {
            err_print("init_ahci: BIOS Handoff timed out");
        }

        timeout = MAX_TIMEOUT_DURATION;
        while ((g_hba->bohc & BOHC_BB) && --timeout) {
            system_pause();
        }

        if (unlikely(timeout == 0)) {
            err_print("init_ahci: BIOS busy bit timed out");
        }
    }

    // Global Reset
    g_hba->ghc |= GHC_AE;    // Ensure AE (AHCI Enable) is set
    timer_stall(1000);       // 1ms
    g_hba->ghc |= GHC_HR;    // Set HR

    timeout = MAX_TIMEOUT_DURATION;
    while ((g_hba->ghc & GHC_HR) && --timeout) {
        system_pause();
    }

    if (unlikely(timeout == 0)) {
        err_print("init_ahci: HBA Reset timed out");
    }

    g_hba->ghc |= GHC_AE;    // Re-enable AHCI mode after reset
    timer_stall(1000);       // 1ms
    g_hba->ghc |= GHC_IE;    // Enable Interrupts

    if (unlikely(!(g_hba->ghc & GHC_AE))) {
        err_print("init_ahci: GHC.AE bit failed to persist");
    }

    irq_unmask(irq_line, 46);

    uint32_t ncs = HBA_CAP_NCS(g_hba->cap);
    int slot_count = ncs + 1;

    uint32_t pi = g_hba->pi;

    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            hba_port_t* port = &g_hba->ports[i];

            // If port is not IDLE, make it IDLE
            if (unlikely((port->cmd & (PX_CMD_ST | PX_CMD_CR | PX_CMD_FRE | PX_CMD_FR)) != 0)) {
                bool res = ahci_stop_port(port);

                if (unlikely(!res)) {
                    err_printf("init_ahci: Port %d failed to stop (PxCMD: 0x%x)", i, port->cmd);
                    continue;
                }
            }

            uint32_t port_phys = (uint32_t) pmm_alloc_page();
            uint32_t port_virt = PHYSICAL_TO_VIRTUAL(port_phys);

            if (unlikely(!port_phys)) {
                err_print("init_ahci: PMM out of memory");
                continue;
            }

            vmm_map_page(kernel_directory, (void*) port_phys, (void*) port_virt,
                         PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
            memset((void*) port_virt, 0, PAGE_SIZE);

            port->clb  = (uint32_t)(port_phys & 0xFFFFFFFF);
            port->clbu = (uint32_t)(port_phys >> 32);

            // Each page can only fit 16 ports, we have 32, thus 2 pages
            uint32_t cmd_tables_phys = (uint32_t) pmm_alloc_pages(2);
            uint32_t cmd_tables_virt = PHYSICAL_TO_VIRTUAL(cmd_tables_phys);

            if (unlikely(!cmd_tables_phys)) {
                err_print("init_ahci: 2 consecutive pages not found for cmd_table");
                continue;
            }

            // Map the 2 pages
            for (int j = 0; j < 2; j++) {
                vmm_map_page(kernel_directory,
                    (void*) cmd_tables_phys + (PAGE_SIZE * j),
                    (void*) cmd_tables_virt + (PAGE_SIZE * j),
                    PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
            }

            memset((void*) cmd_tables_virt, 0, PAGE_SIZE * 2);

            hba_cmd_header_t* headers = (hba_cmd_header_t*) port_virt;
            for (int j = 0; j < 32; j++) {
                // Each table is 256 bytes (0x100)
                headers[j].ctba = cmd_tables_phys + (j * 256);
            }

            uint64_t fis_addr = port_phys + 1024;
            port->fb  = (uint32_t)(fis_addr & 0xFFFFFFFF);
            port->fbu = (uint32_t)(fis_addr >> 32);

            port->cmd |= PX_CMD_FRE;

            port->serr = 0xFFFFFFFF;

            port->is  = port->is;
            g_hba->is = (1 << i);
            port->ie = 0xF; // Trigger interrupts for DHRE, PSE, DSE, and SDBE

            port->cmd |= PX_CMD_ST;

            timer_stall(1000); // 1ms

            // Check if a device is present (0x3 means present and communication established)
            if ((port->ssts & 0x0F) == 0x03) {
                active_drives[drives_found++] = port;
            }
        }
    }

    g_hba->ghc |= GHC_IE;

    if (unlikely(drives_found == 0)) {
        err_print("init_ahci: No drives found on any port");
    }

    bdl_mount(&bdl_ahci_device);
}

void ahci_read_sector(uint32_t lba, uint8_t* buffer) {
    if (unlikely(!buffer)) {
        err_print("ahci_read_sector: Void buffer");
        return;
    }

    hba_port_t* port = ahci_find_free_port(5);

    if (unlikely(!port)) {
        err_print("ahci_read_sector: No free port found");
        return;
    }

    // If SERR isn't 0, the HBA might refuse to process new commands
    if (port->serr) {
        port->serr = port->serr;
    }

    int port_slot = ahci_find_free_slot(port);

    if (unlikely(port_slot == -1)) {
        err_print("ahci_read_sector: No free slot found on port");
        return;
    }

    hba_cmd_header_t* cmd = (hba_cmd_header_t*) PHYSICAL_TO_VIRTUAL(port->clb);
    hba_cmd_header_t* header = &cmd[port_slot];

    header->cfl = 5;
    header->w = 0;
    header->prdtl = 1;
    header->prdbc = 0;

    hba_cmd_table_t* table = (hba_cmd_table_t*) PHYSICAL_TO_VIRTUAL(header->ctba);
    memset(table, 0, sizeof(hba_cmd_table_t));

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&table->cfis);

    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = FIS_CMD_READ;
    fis->device  = FIS_SATA_LBA_MODE;

    fis->lba0 = (uint8_t) lba;
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = 0;
    fis->lba5 = 0;

    // TODO: This could be utilised to make everything faster
    // but requires ATA to comply with that too
    fis->countl = 1; // We want 1 sector

    table->prdt_entry[0].dba = ahci_bounce; // Destination
    table->prdt_entry[0].dbc = 511;           // 512 bytes - 1
    table->prdt_entry[0].i   = 1;

    header->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); // FIS length in DWORDS (usually 5)
    header->prdtl = 1;                                      // We used 1 PRDT entry
    header->w = 0;                                          // 0 = Read, 1 = Write

    port->ci = (1 << port_slot);

    int timeout = MAX_TIMEOUT_DURATION;
    while (--timeout) {
        uint32_t ci_val = port->ci;

        // Check if our CI bit is gone
        if (!(ci_val & (1 << port_slot))) {
            break;
        }

        if (unlikely(port->is & PX_IE_TFES)) {
            err_print("ahci_read_sector: Task File Error Status detected");
            return;
        }

        system_pause();
    }

    if (unlikely(timeout == 0)) {
        err_print("ahci_read_sector: Timeout waiting for CI to clear");
    }

    memcpy(buffer, ahci_bounce, 512);
}

void ahci_write_sector(uint32_t lba, uint8_t* buffer) {
    if (unlikely(!buffer)) {
        err_print("ahci_read_sector: Void buffer");
        return;
    }

    hba_port_t* port = ahci_find_free_port(5);

    if (unlikely(!port)) {
        err_print("ahci_write_sector: No free port found");
        return;
    }

    // If SERR isn't 0, the HBA might refuse to process new commands
    if (port->serr) {
        port->serr = port->serr;
    }

    int port_slot = ahci_find_free_slot(port);

    if (unlikely(port_slot == -1)) {
        err_print("ahci_write_sector: No free slot found on port");
        return;
    }

    memcpy((void*) PHYSICAL_TO_VIRTUAL(ahci_bounce), buffer, 512);

    hba_cmd_header_t* cmd = (hba_cmd_header_t*) PHYSICAL_TO_VIRTUAL(port->clb);
    hba_cmd_header_t* header = &cmd[port_slot];

    header->cfl = 5;
    header->w = 1;
    header->prdtl = 1;
    header->prdbc = 0;

    hba_cmd_table_t* table = (hba_cmd_table_t*) PHYSICAL_TO_VIRTUAL(header->ctba);
    memset(table, 0, sizeof(hba_cmd_table_t));

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&table->cfis);

    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = FIS_CMD_WRITE;
    fis->device  = FIS_SATA_LBA_MODE;

    fis->lba0 = (uint8_t) lba;
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = 0;
    fis->lba5 = 0;

    fis->countl = 1; // We want 1 sector

    table->prdt_entry[0].dba = ahci_bounce; // Destination
    table->prdt_entry[0].dbc = 511;         // 512 bytes - 1
    table->prdt_entry[0].i   = 1;

    header->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); // FIS length in DWORDS (usually 5)
    header->prdtl = 1;                                      // We used 1 PRDT entry
    header->w = 1;                                          // Write (1)

    port->ci = (1 << port_slot);

    int timeout = MAX_TIMEOUT_DURATION;
    while (--timeout) {
        uint32_t ci_val = port->ci;

        // Check if our CI bit is gone
        if (!(ci_val & (1 << port_slot))) {
            break;
        }

        if (unlikely(port->is & PX_IE_TFES)) {
            err_print("ahci_write_sector: Task File Error Status detected");
            return;
        }

        system_pause();
    }

    if (unlikely(timeout == 0)) {
        err_print("ahci_write_sector: Timeout waiting for CI to clear");
    }
}

// Defined in idt.asm
void ahci_interrupt_handler() {
    uint32_t is = g_hba->is;
    if (is == 0) return;

    // Clear the bits for all ports that fired
    for (int i = 0; i < 32; i++) {
        if (is & (1 << i)) {
            g_hba->ports[i].is = g_hba->ports[i].is; // Clear port-specific bits
        }
    }

    g_hba->is = is; // Clear global status
    irq_send_eoi();
}
