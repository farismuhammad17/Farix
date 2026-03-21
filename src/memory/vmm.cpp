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

#include "memory/pmm.h"

#include "memory/vmm.h"

#include "test.h" // TODO REMOVE

int test_value_int;
uint32_t test_pt_virt;
uint32_t test_pt_first_entry;

uint32_t* kernel_directory = nullptr;

void init_vmm() {
    // Allocate the Page Directory
    uint32_t phys_pd = (uint32_t)  pmm_alloc_page();
    kernel_directory = (uint32_t*) phys_pd; // Physical anchor for CR3

    // Clear the Directory using its physical address
    uint32_t* pd_ptr = (uint32_t*) phys_pd;
    for (int i = 0; i < 1024; i++) pd_ptr[i] = 0;

    // Allocate one Page Table to identity map the first 4MB
    uint32_t phys_pt = (uint32_t)  pmm_alloc_page();
    uint32_t* pt_ptr = (uint32_t*) phys_pt;
    for (uint32_t i = 0; i < 1024; i++) {
        pt_ptr[i] = (i << 12) | PAGE_PRESENT | PAGE_RW;
    }

    // Index 0: Identity Map (0x0 - 0x3FFFFF)
    pd_ptr[0] = phys_pt | PAGE_PRESENT | PAGE_RW;

    // Index 768: Direct Map (0xC0000000 - 0xC03FFFFF)
    // 22 = log_2(PAGE_SIZE) + log_2(PAGE_TABLES) = 12 + 10 = 22
    pd_ptr[PAGE_OFFSET >> 22] = phys_pt | PAGE_PRESENT | PAGE_RW;

    vmm_switch_directory(kernel_directory);
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

        // Zero out the new table using Direct Mapping
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

void vmm_switch_directory(uint32_t* directory) {
    asm volatile("mov %0, %%cr3" : : "r"(directory) : "memory");
}

uint32_t* vmm_get_current_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (uint32_t*) cr3;
}
