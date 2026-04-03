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

#include "memory/vmm.h"

/*
TODO: The 4 KB and 1 KB mismatch causes a ton (75%) of memory wastage.

init_vmm, vmm_copy_kernel_directory:
    ARM L1 Table (16KB) wastes 3 pages if PMM only tracks 4KB frames.
    Consider a dedicated 16KB-aligned allocator for Page Directories.

vmm_map_page:
    ARM L2 Page Tables (256 entries * 4B = 1KB) are currently
    allocated via pmm_alloc_page() (4KB). This wastes 3KB per allocation.
    Optimize by implementing a 1KB-slab sub-allocator or pooling 4 tables
    per 4KB physical frame to improve memory efficiency.
     Currently, this causes no errors, it's just memory inefficient, so I
    will fix this on a later day; as of now, we are just implementing the
    basic functions to get through with the HAL.
*/

const uintptr_t PAGE_PRESENT = 0x02; // Small Page type
const uintptr_t PAGE_RW      = 0x30; // AP[1:0] bits set to 11 (Full Access)
const uintptr_t PAGE_USER    = 0x20; // AP[1] bit (Simplified User access)
const uintptr_t PAGE_CACHE   = 0x0C; // Sets C (bit 3) and B (bit 2) for Write-Back caching.

const uintptr_t VMM_TYPE_TABLE = 0x01; // For Directory entries

uint32_t* kernel_directory = NULL;

static void vmm_enable_paging(uint32_t pd_phys) {
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(pd_phys));
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(0x55555555));

    uint32_t sctlr;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));

    sctlr |= 0x1;       // Bit 0: MMU Enable (Like PAGING_BIT)
    sctlr |= (1 << 2);  // Bit 2: Data Cache (Essential for ARM stability/speed)
    sctlr |= (1 << 12); // Bit 12: Instruction Cache

    asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(sctlr));
}

void init_vmm() {
    // 16KB required for ARM L1 table. Since pmm_alloc_page gives 4KB,
    // We have dummy allocs to reserve the 16KB block
    uint32_t phys_pd  = (uint32_t) pmm_alloc_page();
    for (size_t i = 0; i < 3; i++) pmm_alloc_page();

    kernel_directory = (uint32_t*) phys_pd;
    uint32_t* pd_ptr = (uint32_t*) phys_pd;

    // ARM Directory has 4096 entries (covers 4GB, 1MB per entry)
    for (int i = 0; i < 4096; i++) pd_ptr[i] = 0;

    for (uint32_t i = 0; i < VMM_INIT_MAP_SIZE; i++) {
        uint32_t phys_pt = (uint32_t) pmm_alloc_page();
        uint32_t* pt_ptr = (uint32_t*) phys_pt;

        for (uint32_t j = 0; j < 256; j++) {
            uint32_t physical_addr = (i << 20) + (j << 12);
            pt_ptr[j] = physical_addr | PAGE_PRESENT | PAGE_RW| PAGE_CACHE;
        }

        // Identity map
        pd_ptr[i] = phys_pt | VMM_TYPE_TABLE;

        // Higher half map: 0xC0000000 / 1MB = index 3072
        pd_ptr[3072 + i] = phys_pt | VMM_TYPE_TABLE;
    }

    vmm_enable_paging(phys_pd);
}

void vmm_map_page(uint32_t* pd_phys, void* phys, void* virt, uint32_t flags) {
    uint32_t virt_addr = (uint32_t) virt;
    uint32_t pd_index  = virt_addr >> 20;
    uint32_t pt_index  = (virt_addr >> 12) & 0xFF;
    uint32_t* pd_virt  = (uint32_t*) PHYSICAL_TO_VIRTUAL(pd_phys);

    // On ARM, we check for Bit 0 (0x01) for a valid Page Table link.
    if (!(pd_virt[pd_index] & 0x01)) {
        uint32_t phys_table = (uint32_t) pmm_alloc_page();

        // Link it in the PD.
        pd_virt[pd_index] = phys_table | VMM_TYPE_TABLE;

        // Zero out the new table.
        uint32_t* table_ptr = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_table);
        for (int i = 0; i < 1024; i++) {
            table_ptr[i] = 0;
        }
    }

    uint32_t  phys_table_addr = pd_virt[pd_index] & ~0x3FF;
    uint32_t* table = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_table_addr);

    table[pt_index] = (uint32_t) phys | flags;

    asm volatile("mcr p15, 0, %0, c8, c7, 1" : : "r"(virt_addr) : "memory");
    asm volatile("dsb" ::: "memory");
    asm volatile("isb" ::: "memory");
}

uint32_t vmm_unmap_page(void* virt) {
    uint32_t virt_addr = (uint32_t) virt;
    uint32_t pd_index  = virt_addr >> 20;
    uint32_t pt_index  = (virt_addr >> 12) & 0xFF;
    uint32_t* pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(vmm_get_current_directory());

    // Check if the Page Table exists (Bit 0 on ARM)
    if (!(pd_virt[pd_index] & 0x01)) return 0;

    // Mask bottom 10 bits to get L2 Table address
    uint32_t pt_phys = pd_virt[pd_index] & ~0x3FF;
    uint32_t* pt_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(pt_phys);

    // Grab the physical address (Mask bottom 12 bits for 4KB page)
    uint32_t phys_to_return = pt_virt[pt_index] & ~0xFFF;

    // Wipe the entry
    pt_virt[pt_index] = 0;

    asm volatile("mcr p15, 0, %0, c8, c7, 1" : : "r"(virt_addr) : "memory");
    asm volatile("dsb" ::: "memory");
    asm volatile("isb" ::: "memory");

    return phys_to_return;
}

uint32_t* vmm_copy_kernel_directory() {
    uint32_t phys_pd = (uint32_t) pmm_alloc_page();
    for (int i = 0; i < 3; i++) pmm_alloc_page();

    uint32_t* pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_pd);

    // ARM Directory has 4096 entries (1MB per entry)
    for (int i = 0; i < 4096; i++) pd_virt[i] = 0;

    uint32_t* k_pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(kernel_directory);

    // Copy Higher-Half (0xC0000000 to 0xFFFFFFFF)
    for (int i = 3072; i < 4096; i++) {
        pd_virt[i] = k_pd_virt[i];
    }

    // Copy Identity Map (Low memory)
    for (int i = 0; i < VMM_INIT_MAP_SIZE; i++) {
        pd_virt[i] = k_pd_virt[i];
    }

    return (uint32_t*) phys_pd;
}

void vmm_switch_directory(uint32_t* page_directory) {
    if (vmm_get_current_directory() != page_directory) {
        // Write to TTBR0 (Translation Table Base Register 0)
        asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(page_directory) : "memory");

        // Invalidate the entire unified TLB to ensure the new mapping takes effect
        asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0) : "memory");

        // Barriers to ensure the pipe is flushed before next instruction
        asm volatile("dsb" ::: "memory");
        asm volatile("isb" ::: "memory");
    }
}

uint32_t vmm_get_phys(uint32_t* pd_phys, void* virt_addr) {
    uint32_t v = (uint32_t) virt_addr;
    uint32_t pd_idx = v >> 20;
    uint32_t pt_idx = (v >> 12) & 0xFF;

    uint32_t* pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(pd_phys);

    // Check if the Page Table exists
    if (!(pd_virt[pd_idx] & 0x01)) return 0;

    // Get the Page Table
    uint32_t pt_phys = pd_virt[pd_idx] & ~0x3FF;
    uint32_t* pt_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(pt_phys);

    // Check if the Page is present
    if (!(pt_virt[pt_idx] & 0x02)) return 0;

    // Return physical address (mask 12-bit flags) + offset
    return (pt_virt[pt_idx] & ~0xFFF) | (v & 0xFFF);
}
