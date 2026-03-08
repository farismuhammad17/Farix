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

uint32_t* kernel_directory = nullptr;

void init_vmm() {
    uint32_t phys_addr = (uint32_t) pmm_alloc_page();

    kernel_directory = (uint32_t*) phys_addr;
    for (int i = 0; i < PAGE_SIZE / 4; i++) {
        kernel_directory[i] = 0;
    }

    // Create the first Page Table (to map the first 4MB)
    uint32_t* first_page_table = (uint32_t*) pmm_alloc_page();
    for (uint32_t i = 0; i < PAGE_SIZE / 4; i++) {
        uint32_t address    = i * PAGE_SIZE;
        first_page_table[i] = address | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    kernel_directory[0] = (uint32_t) first_page_table | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    kernel_directory[1023] = (uint32_t) phys_addr | PAGE_PRESENT | PAGE_RW | PAGE_USER;

    vmm_switch_directory(kernel_directory);
}

void vmm_map_page(uint32_t* dir, void* phys, void* virt, uint32_t flags) {
    uint32_t virt_addr = (uint32_t) virt;
    uint32_t pd_index  = virt_addr >> 22;
    uint32_t pt_index  = (virt_addr >> 12) & 0x3FF;

    uint32_t* pd_window = (uint32_t*) 0xFFFFF000;

    if (!(pd_window[pd_index] & PAGE_PRESENT)) {
        uint32_t phys_table = (uint32_t) pmm_alloc_page();
        pd_window[pd_index] = phys_table | PAGE_PRESENT | PAGE_RW | PAGE_USER;

        asm volatile("invlpg (%0)" : : "r"((0xFFC00000 + (pd_index * 4096))));

        uint32_t* new_table_virt = (uint32_t*)(0xFFC00000 + (pd_index * 4096));
        for (int i = 0; i < 1024; i++) new_table_virt[i] = 0;
    }

    uint32_t* table = (uint32_t*)(0xFFC00000 + (pd_index * 4096));
    table[pt_index] = (uint32_t) phys | flags;

    asm volatile("invlpg (%0)" : : "r"(virt));
}
void vmm_switch_directory(uint32_t* directory) {
    asm volatile("mov %0, %%cr3" : : "r"(directory));
}

void vmm_unmap_page(uint32_t* dir, void* virt) {
    uint32_t virt_addr = (uint32_t)virt;
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    uint32_t* pd_window = (uint32_t*)0xFFFFF000;
    if (pd_window[pd_index] & PAGE_PRESENT) {
        uint32_t* table = (uint32_t*)(0xFFC00000 + (pd_index * 4096));
        table[pt_index] = 0; // Unset the page table entry.
        asm volatile("invlpg (%0)" : : "r"(virt));
    }
}

uint32_t* vmm_get_current_directory() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (uint32_t*)cr3;
}
