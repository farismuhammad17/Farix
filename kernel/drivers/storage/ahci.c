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

// NOTE: This file is WIP

#include <stdint.h>

#include "cpu/pci.h"
#include "drivers/storage/bdl.h"
#include "drivers/terminal.h"
#include "drivers/uart.h" // TODO REM
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "drivers/storage/ahci.h"

#define MAX_TIMEOUT_DURATION 1000000

#define AHCI_HBA_PAGES 5
#define AHCI_BASE_VIRT 0xF0000000

#define HBA_GHC_AE (1 << 31)
#define HBA_GHC_HR (1 << 1)

#define SATA_SIG_ATA 0x00000101

static volatile hba_mem_t* g_hba = NULL;

static BDLDevice bdl_ahci_device = {
    .read  = ahci_read_sector,
    .write = ahci_write_sector
};

static void ahci_wait(uint32_t* reg, uint32_t mask) {
    for (uint32_t i = 0; i < MAX_TIMEOUT_DURATION; i++) {
        if (!(*reg & mask)) return;
    }

    t_print("ahci_wait: timeout waiting for register");
}

void init_ahci(pci_device_t* dev) {
    t_print("init_ahci: Implementation is WIP"); // TODO REM

    // Enable PCI bus mastering + MMIO
    uint32_t cmd = pci_read(dev->bus, dev->device, dev->function, 0x04);
    pci_write(dev->bus, dev->device, dev->function, 0x04,
              cmd | (1 << 1) | (1 << 2));

    // Get BAR5 (AHCI MMIO base)
    uint32_t bar5 = pci_read(dev->bus, dev->device, dev->function, 0x24);
    uintptr_t phys = bar5 & 0xFFFFFFF0;

    if (bar5 & 1) {
        t_print("init_ahci: Invalid controller: BAR5 is IO space, not MMIO.");
        return;
    }

    uint32_t* pd = vmm_get_current_directory();

    // AHCI HBA is chunky and wants more space than 1 page
    for (int i = 0; i < AHCI_HBA_PAGES; i++) {
        vmm_map_page(pd,
            phys + (i << 12),
            AHCI_BASE_VIRT + (i << 12),
            PAGE_PRESENT | PAGE_RW | PAGE_PCD
        );
    }

    g_hba = (volatile hba_mem_t*) AHCI_BASE_VIRT;

    t_print("init_ahci: MMIO mapped"); // TODO REM

    // BIOS to OS ownership handoff
    if (g_hba->bohc & 1) {
        g_hba->bohc |= (1 << 1); // request ownership

        for (int i = 0; i < MAX_TIMEOUT_DURATION; i++) {
            if (!(g_hba->bohc & 1)) break;
        }
    }

    t_print("init_ahci: BIOS handoff complete"); // TODO REM

    // Enable init_ahci mode
    g_hba->ghc |= HBA_GHC_AE;

    // Controller reset (safe, bounded)
    g_hba->ghc |= HBA_GHC_HR;
    ahci_wait((uint32_t*) &g_hba->ghc, HBA_GHC_HR);

    t_print("init_ahci: controller reset complete"); // TODO REM

    // Detect ports
    uint32_t pi = g_hba->pi;
    t_printf("init_ahci: CAP=%x PI=%x VS=%x", g_hba->cap, g_hba->pi, g_hba->vs); // TODO REM

    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            hba_port_t* port = &g_hba->ports[i];

            uint8_t det = port->ssts & 0x0F;
            uint8_t ipm = (port->ssts >> 8) & 0x0F;

            t_printf("init_ahci: Port %d: DET=%x IPM=%x SIG=%x",
                     i, det, ipm, port->sig); // TODO REM

            if (det == 3 && port->sig == SATA_SIG_ATA) {
                ahci_probe_port(port, i);
            }
        }
    }

    t_print("init_ahci: init complete"); // TODO REM

    bdl_mount(&bdl_ahci_device);
}

void ahci_probe_port(volatile hba_port_t* port, int port_no) {
    // Stop engine safely
    port->cmd &= ~HBA_PxCMD_ST;
    port->cmd &= ~HBA_PxCMD_FRE;

    for (int i = 0; i < MAX_TIMEOUT_DURATION; i++) {
        if (!(port->cmd & (HBA_PxCMD_CR | HBA_PxCMD_FR))) break;
    }

    // Allocate structures
    uintptr_t clb_phys = (uintptr_t) pmm_alloc_page();
    uintptr_t fis_phys = (uintptr_t) pmm_alloc_page();

    uintptr_t clb_virt = AHCI_BASE_VIRT + 0x1000;
    uintptr_t fis_virt = AHCI_BASE_VIRT + 0x2000;

    uint32_t* pd = vmm_get_current_directory();

    vmm_map_page(pd, clb_phys, clb_virt,
        PAGE_PRESENT | PAGE_RW | PAGE_PCD);
    vmm_map_page(pd, fis_phys, fis_virt,
        PAGE_PRESENT | PAGE_RW | PAGE_PCD);

    kmemset((void*) clb_virt, 0, 1024);
    kmemset((void*) fis_virt, 0, 256);

    // Assign port registers
    port->clb  = (uint32_t) clb_phys;
    port->clbu = 0;

    port->fb   = (uint32_t) fis_phys;
    port->fbu  = 0;

    // Command list setup
    hba_cmd_header_t* cmd = (hba_cmd_header_t*) clb_phys;

    for (int i = 0; i < 32; i++) {
        uintptr_t ct_phys = (uintptr_t) pmm_alloc_page();

        cmd[i].ctba = ct_phys;
        cmd[i].ctbau = 0;
        cmd[i].prdtl = 0;
    }

    // Start engine
    port->is = (uint32_t) -1;

    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;

    t_printf("ahci_probe_port: port %d ready", port_no); // TODO REM
}

// TODO
void ahci_read_sector(uint32_t lba, uint8_t* buffer) {
    t_print("ahci_read_sector: Unimplemented");
}

// TODO
void ahci_write_sector(uint32_t lba, uint8_t* buffer) {
    t_print("ahci_write_sector: Unimplemented");
}
