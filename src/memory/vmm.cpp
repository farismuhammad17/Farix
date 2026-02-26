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

static uint32_t* kernel_directory = nullptr;

void init_vmm() {
    // Asks PMM for a 4KB page to hold the Directory
    kernel_directory = (uint32_t*) pmm_alloc_page();
    for (int i = 0; i < 1024; i++) {
        kernel_directory[i] = PAGE_RW;
    }

    // Create the first Page Table (to map the first 4MB)
    uint32_t* first_page_table = (uint32_t*) pmm_alloc_page();
    for (uint32_t i = 0; i < 1024; i++) {
        uint32_t address    = i * PAGE_SIZE;
        first_page_table[i] = address | PAGE_PRESENT | PAGE_RW;
    }

    kernel_directory[0] = (uint32_t) first_page_table | PAGE_PRESENT | PAGE_RW;

    vmm_switch_directory(kernel_directory);
}

void vmm_map_page(void* phys, void* virt, uint32_t flags) {
    uint32_t virt_addr = (uint32_t) virt;
    uint32_t pd_index  = virt_addr >> 22;
    uint32_t pt_index  = (virt_addr >> 12) & 0x3FF;

    if (!(kernel_directory[pd_index] & PAGE_PRESENT)) {
        uint32_t* new_table = (uint32_t*) pmm_alloc_page();
        for (int i = 0; i < 1024; i++) new_table[i] = 0;

        kernel_directory[pd_index] = (uint32_t) new_table | PAGE_PRESENT | PAGE_RW;
    }

    uint32_t* table = (uint32_t*) (kernel_directory[pd_index] & 0xFFFFF000);
    table[pt_index] = (uint32_t)  phys | flags | PAGE_PRESENT;

    // Tell the CPU to clear its cache for this address
    asm volatile("invlpg (%0)" : : "r"(virt));
}

void vmm_switch_directory(uint32_t* directory) {
    asm volatile("mov %0, %%cr3" : : "r"(directory));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // The Paging Bit
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}
