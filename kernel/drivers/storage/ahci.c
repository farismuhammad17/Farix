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

#include "cpu/pci.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "drivers/storage/ahci.h"

void init_ahci(pci_device_t* pci_ahci_device) {
    // Get the MMIO Base Address from BAR5
    // PCI BAR5 is the address where the HBA registers are mapped in memory.
    uint32_t bar5 = pci_read(pci_ahci_device->bus, pci_ahci_device->device, pci_ahci_device->function, 0x24);

    uintptr_t phys_addr = (uintptr_t)(bar5 & 0xFFFFFFF0);
    vmm_map_page(vmm_get_current_directory(), (void*) phys_addr, (void*) phys_addr, PAGE_PRESENT | PAGE_RW | (1 << 4)); // TODO: 1 << 4

    // We mask the lower bits because BARs contain alignment flags.
    volatile hba_mem_t* hba = (volatile hba_mem_t*) phys_addr;

    // Take control of the HBA (Global Host Control)
    // Bit 31 (AE) must be 1 to tell the controller we are using AHCI mode, not legacy IDE.
    hba->ghc |= (1 << 31);

    // Reset the controller
    // Bit 0 (HR) resets the HBA. We set it and wait for the hardware to clear it to 0.
    hba->ghc |= 0x01;
    while (hba->ghc & 0x01) {}

    // Re-enable AHCI mode after the reset
    hba->ghc |= (1 << 31);

    // This register tells us which of the 32 possible SATA ports actually exist on this chip.
    uint32_t pi = hba->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            // Clear any pending interrupts on the port
            hba->ports[i].is = 0xFFFFFFFF;
            // Clear error register
            hba->ports[i].serr = 0xFFFFFFFF;

            uint32_t timeout = 1000000;
            while ((hba->ports[i].ssts & 0x0F) != 0x03 && timeout > 0) timeout--;
            if ((hba->ports[i].ssts & 0x0F) != 0x03) {
                t_print("init_ahci: Timeout waiting for SSTS");
                return;
            }

            uint32_t ssts = hba->ports[i].ssts;
            ahci_probe_port(&hba->ports[i], i);
        }
    }
}

void ahci_probe_port(volatile hba_port_t* port, int port_no) {
    port->cmd &= ~HBA_PxCMD_ST;
    port->cmd &= ~HBA_PxCMD_FRE;
    while (port->cmd & HBA_PxCMD_CR || port->cmd & HBA_PxCMD_FR);

    uintptr_t clb_phys = (uintptr_t) pmm_alloc_page();
    uintptr_t fis_phys = (uintptr_t) pmm_alloc_page();

    if (!clb_phys || !fis_phys) {
        t_print("ahci_probe_port: PMM allocation failed");
        uart_print("ahci_probe_port: PMM allocation failed");
        return;
    }

    port->clb = (uint32_t) clb_phys;
    port->fb  = (uint32_t) fis_phys;

    port->clbu = 0;
    port->fbu  = 0;

    // Start the port
    port->is = 0xFFFFFFFF; // Clear interrupts
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;

    t_printf("AHCI: Port %d initialized", port_no);
}
