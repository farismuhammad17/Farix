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

#include <stddef.h>
#include <stdint.h>

#include "memory/pmm.h"

#include "memory/vmm.h"

uint32_t* kernel_directory = NULL;

static void vmm_enable_paging(uint32_t pd_phys) {
    asm volatile("mov %0, %%cr3" : : "r"(pd_phys));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 |= PAGING_BIT;
    cr0 |= PAGE_WP_BIT;

    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

void init_vmm() {
    uint32_t phys_pd = (uint32_t) pmm_alloc_page();
    kernel_directory = (uint32_t*) phys_pd;

    // Before paging is on, physical == virtual
    uint32_t* pd_ptr = (uint32_t*) phys_pd;
    for (int i = 0; i < 1024; i++) pd_ptr[i] = 0;

    for (uint32_t i = 0; i < (VMM_INIT_MAP_SIZE >> 2); i++) {
        uint32_t phys_pt = (uint32_t) pmm_alloc_page();
        uint32_t* pt_ptr = (uint32_t*) phys_pt;

        for (uint32_t j = 0; j < 1024; j++) {
            uint32_t physical_addr = (i << 22) + (j << 12);
            pt_ptr[j] = physical_addr | PAGE_PRESENT | PAGE_RW;
        }

        pd_ptr[i] = phys_pt | PAGE_PRESENT | PAGE_RW;
        pd_ptr[768 + i] = phys_pt | PAGE_PRESENT | PAGE_RW;
    }

    vmm_enable_paging(phys_pd);
}

void vmm_map_page(uint32_t* pd_phys, void* phys, void* virt, uint32_t flags) {
    uint32_t virt_addr = (uint32_t) virt;
    uint32_t pd_index  = virt_addr >> 22;
    uint32_t pt_index  = (virt_addr >> 12) & 0x3FF;
    uint32_t* pd_virt  = (uint32_t*) PHYSICAL_TO_VIRTUAL(pd_phys);

    // Check if the Page Table exists
    if (!(pd_virt[pd_index] & PAGE_PRESENT)) {
        uint32_t phys_table = (uint32_t) pmm_alloc_page();

        // Link it in the PD
        pd_virt[pd_index] = phys_table | PAGE_PRESENT | PAGE_RW | PAGE_USER;

        // Zero out the new table
        uint32_t* table_ptr = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_table);
        for (int i = 0; i < 1024; i++) {
            table_ptr[i] = 0;
        }
    }

    // Get the virtual address of the Page Table
    uint32_t phys_table_addr = pd_virt[pd_index] & ~0xFFF; // masks out the bottom 12 bits (the offset/flags)
    uint32_t* table = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_table_addr);

    // Map the physical page into the Page Table
    table[pt_index] = (uint32_t) phys | flags;

    // Flush the TLB for the virtual address we just mapped
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap_page(void* virt) {
    uint32_t virt_addr = (uint32_t)virt;
    uint32_t pd_index  = virt_addr >> 22;
    uint32_t pt_index  = (virt_addr >> 12) & 0x3FF;

    // Get the PD using your new direct map logic
    uint32_t phys_pd  = (uint32_t)  vmm_get_current_directory();
    uint32_t* pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_pd);

    if (pd_virt[pd_index] & PAGE_PRESENT) {
        // Find the Page Table
        uint32_t pt_phys = pd_virt[pd_index] & ~0xFFF;
        uint32_t* pt_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(pt_phys);

        // Clear the entry
        pt_virt[pt_index] = 0;

        // Flush the TLB so the CPU realizes the page is gone
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    }
}

uint32_t* vmm_copy_kernel_directory() {
    uint32_t  phys_pd = (uint32_t)  pmm_alloc_page();
    uint32_t* pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_pd);
    for (int i = 0; i < 1024; i++) pd_virt[i] = 0;

    uint32_t* k_pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(kernel_directory);

    for (int i = 768; i < 1024; i++)
        pd_virt[i] = k_pd_virt[i];
    for (int i = 0; i < (VMM_INIT_MAP_SIZE >> 2); i++)
        pd_virt[i] = k_pd_virt[i];

    return (uint32_t*) phys_pd;
}

// Uses physical addresses
void vmm_switch_directory(uint32_t* page_directory) {
    asm volatile("mov %0, %%cr3" : : "r"(page_directory) : "memory");
}

uint32_t* vmm_get_current_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (uint32_t*) cr3;
}
