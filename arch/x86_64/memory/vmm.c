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

#include <stddef.h>
#include <stdint.h>

#include "klib/string.h"

#include "cpu/multicore.h"
#include "memory/pmm.h"

#include "memory/vmm.h"

const uint64_t PAGE_PRESENT = 0x1;  // 10e0 in binary - If page is in RAM
const uint64_t PAGE_RW      = 0x2;  // 10e1 in binary - 0 = Read-only,   1 = Read/Write
const uint64_t PAGE_USER    = 0x4;  // 10e2 in binary - 0 = Kernel only, 1 = Everyone
const uint64_t PAGE_PWT     = 0x8;  // 10e3 in binary - Writes go to cache and memory immediately
const uint64_t PAGE_PCD     = 0x10; // 10e4 in binary - Completely disables CPU caching for that page
const uint64_t PAGE_CACHE   = 0x0;  // Not present in x86, does nothing, but required stub

#define PAGING_BIT  0x80000000
#define PAGE_WP_BIT 0x00010000

#define PAGE_MASK 0x000FFFFFFFFFF000ULL

uint64_t* kernel_directory = NULL;

static spinlock vmm_lock = 0;

// TODO: bit shifting 39, 30, etc. and & 0x1FF is common enough to have its own macro.

/*
We need both Identity Mapping (where Virtual Address 0x1000 equals Physical Address 0x1000)
as well as Higher Half Mapping (where the kernel lives up at PAGE_OFFSET), hence why we need
the two PDPTs (short for "Page Directory Pointer Table"):-
* identity_pdpt : Handles the bottom of memory (starting at virtual address 0x0).
* kernel_pdpt   : Handles the upper half of memory (starting at virtual address PAGE_OFFSET).
PDPTs are similar to dictionaries; it's a collection of pointers that the CPU can use to
know where is what. The Identity PDPT is only used by the kernel to survive the transition
of the paging from boot.s into the kernel's own paging setup. If we don't do this, we'll be
stuck with just 1 GB of RAM that was mapped back at boot.s. Once this function is completed,
we clean up identity PDPT to give us that precious RAM back.

The PML4 is the kernel_directory, which holds exactly 512 slots, starting from index 0. Each
slot holds a pointer to a PDPT. Currently, there are only two PDPTs, the two we just created.
The first slot (index 0) is set to the identity PDPT (flagged to exist and be read/write).
The kernel PDPT is set to the 510th slot ((PAGE_OFFSET >> 39) & 0x1FF), under the same flags.

In x86_64, memory is split off into four layers, each of which are sort of dictionaries that
point to a set of the next layer.
* The top most is the PML4 (Page Map Level 4), which has 512 slots, each for an individial PDPT
* The next level is the PDPT (Page Directory Pointer Table), which has 512 slots, each for an individual PD
* The next level is the PD (Page Directory), which has 512 slots, each for an individual PT
* The lowest level is the PT (Page Table), which has 512 slots, each for an individual page
This gives us a total of 512 * 512 * 512 * 512 = 16TB of addressable memory. The next for loop is to do just
that loop through the PD and PT to fill them out ito the kernel PDPT.

Currently, we only map out 8 MB at boot. This 8 MB of RAM is simply available to the kernel, but can be
used by another application if required. In terms of physical RAM used here, each dictionary calls for
another page in the PMM, each of which is 4 KB in size. We use exactly:-

1 PML4 + 2 PDPT + 1 PD + 4 PT
= (1 + 2 + 1 + 4) * 4 KB = 32 KB (physical RAM)

Note that the actual directories are set to the virtual addresses, because boot.s already enabled paging.

TODO: Consider what resursive mapping is, left out kernel PDPT slot 511 in case we'd need it.
*/
void init_vmm() {
    // Allocate PML4 Root
    uint64_t phys_kernel_directory = (uint64_t) pmm_alloc_page();
    kernel_directory = (uint64_t*) PHYSICAL_TO_VIRTUAL(phys_kernel_directory);
    memset(kernel_directory, 0, PAGE_SIZE);

    // Allocate distinct PDPTs
    uint64_t phys_identity_pdpt = (uint64_t) pmm_alloc_page();
    uint64_t* identity_pdpt = (uint64_t*) PHYSICAL_TO_VIRTUAL(phys_identity_pdpt);
    memset(identity_pdpt, 0, PAGE_SIZE);

    uint64_t phys_kernel_pdpt = (uint64_t) pmm_alloc_page();
    uint64_t* kernel_pdpt = (uint64_t*) PHYSICAL_TO_VIRTUAL(phys_kernel_pdpt);
    memset(kernel_pdpt, 0, PAGE_SIZE);

    kernel_directory[0] = phys_identity_pdpt | PAGE_PRESENT | PAGE_RW;
    kernel_directory[(PAGE_OFFSET >> 39) & 0x1FF] = phys_kernel_pdpt | PAGE_PRESENT | PAGE_RW;

    // Allocate separate Page Directories for Identity and Higher Half
    uint64_t phys_id_pd = (uint64_t) pmm_alloc_page();
    uint64_t* id_pd_ptr = (uint64_t*) PHYSICAL_TO_VIRTUAL(phys_id_pd);
    memset(id_pd_ptr, 0, PAGE_SIZE);
    identity_pdpt[0] = phys_id_pd | PAGE_PRESENT | PAGE_RW;

    uint64_t phys_k_pd = (uint64_t) pmm_alloc_page();
    uint64_t* k_pd_ptr = (uint64_t*) PHYSICAL_TO_VIRTUAL(phys_k_pd);
    memset(k_pd_ptr, 0, PAGE_SIZE);

    uint64_t kernel_pdpt_idx = (PAGE_OFFSET >> 30) & 0x1FF;
    kernel_pdpt[kernel_pdpt_idx] = phys_k_pd | PAGE_PRESENT | PAGE_RW;

    // Map the physical memory to both regions
    for (uint64_t j = 0; j < VMM_INIT_MAP_SIZE_MB / 2; j++) {
        uint64_t phys_pt = (uint64_t) pmm_alloc_page();
        uint64_t* pt_ptr = (uint64_t*) PHYSICAL_TO_VIRTUAL(phys_pt);
        memset(pt_ptr, 0, PAGE_SIZE);

        // Link the page table into both directories
        id_pd_ptr[j] = phys_pt | PAGE_PRESENT | PAGE_RW;
        k_pd_ptr[j]  = phys_pt | PAGE_PRESENT | PAGE_RW;

        for (uint64_t k = 0; k < 512; k++) {
            uint64_t physical_addr = (j * 512 * PAGE_SIZE) + (k * PAGE_SIZE);
            pt_ptr[k] = physical_addr | PAGE_PRESENT | PAGE_RW;
        }
    }

    vmm_switch_directory((uint64_t*) phys_kernel_directory);
}

/* Map physical page to virtual */
void vmm_map_page(uint64_t* pd_phys, void* phys, void* virt, uint64_t flags) {
    uint64_t virt_addr = (uint64_t) virt;

    uint64_t pml4_idx  = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx  = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx    = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx    = (virt_addr >> 12) & 0x1FF;

    // Extract table-level permission flags (Present, R/W, User) from the target flags
    uint64_t table_flags = flags & (PAGE_PRESENT | PAGE_RW | PAGE_USER);

    spin_lock(&vmm_lock);

    uint64_t* pml4_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pd_phys);

    // PML4 -> PDPT
    if (unlikely(!(pml4_virt[pml4_idx] & PAGE_PRESENT))) {
        uint64_t phys_pdpt = (uint64_t) pmm_alloc_page();
        pml4_virt[pml4_idx] = phys_pdpt | table_flags;
        memset((uint64_t*) PHYSICAL_TO_VIRTUAL(phys_pdpt), 0, PAGE_SIZE);
    } else {
        // Inherit flags upward if a subsequent mapping requires broader permissions (e.g., User access)
        pml4_virt[pml4_idx] |= table_flags;
    }
    uint64_t* pdpt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pml4_virt[pml4_idx] & PAGE_MASK);

    // PDPT -> PD
    if (unlikely(!(pdpt_virt[pdpt_idx] & PAGE_PRESENT))) {
        uint64_t phys_pd = (uint64_t) pmm_alloc_page();
        pdpt_virt[pdpt_idx] = phys_pd | table_flags;
        memset((uint64_t*) PHYSICAL_TO_VIRTUAL(phys_pd), 0, PAGE_SIZE);
    } else {
        pdpt_virt[pdpt_idx] |= table_flags;
    }
    uint64_t* pd_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pdpt_virt[pdpt_idx] & PAGE_MASK);

    // PD -> PT
    if (unlikely(!(pd_virt[pd_idx] & PAGE_PRESENT))) {
        uint64_t phys_pt = (uint64_t) pmm_alloc_page();
        pd_virt[pd_idx] = phys_pt | table_flags;
        memset((uint64_t*) PHYSICAL_TO_VIRTUAL(phys_pt), 0, PAGE_SIZE);
    } else {
        pd_virt[pd_idx] |= table_flags;
    }
    uint64_t* pt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pd_virt[pd_idx] & PAGE_MASK);

    // Map the leaf physical page layout inside the Page Table
    pt_virt[pt_idx] = (uint64_t) phys | flags;

    // Flush the TLB for this specific virtual address modification
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

    spin_unlock(&vmm_lock);
}

/* Unmap virtual address from physical */
uint64_t vmm_unmap_page(void* virt) {
    uint64_t virt_addr = (uint64_t) virt;

    uint64_t pml4_idx  = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx  = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx    = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx    = (virt_addr >> 12) & 0x1FF;

    spin_lock(&vmm_lock);

    uint64_t* pml4_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(vmm_get_current_directory());

    if (unlikely(!(pml4_virt[pml4_idx] & PAGE_PRESENT))) {
        spin_unlock(&vmm_lock);
        return 0;
    }
    uint64_t* pdpt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pml4_virt[pml4_idx] & PAGE_MASK);

    if (unlikely(!(pdpt_virt[pdpt_idx] & PAGE_PRESENT))) {
        spin_unlock(&vmm_lock);
        return 0;
    }
    uint64_t* pd_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pdpt_virt[pdpt_idx] & PAGE_MASK);

    if (unlikely(!(pd_virt[pd_idx] & PAGE_PRESENT))) {
        spin_unlock(&vmm_lock);
        return 0;
    }
    uint64_t* pt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pd_virt[pd_idx] & PAGE_MASK);

    // Hardening check: Ensure the leaf page table entry itself is actually present
    if (unlikely(!(pt_virt[pt_idx] & PAGE_PRESENT))) {
        spin_unlock(&vmm_lock);
        return 0;
    }

    // Safely extract the physical address using the proper mask
    uint64_t phys_to_return = pt_virt[pt_idx] & PAGE_MASK;

    // Clear the entry completely
    pt_virt[pt_idx] = 0;

    // Invalidate the TLB for this specific address
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

    spin_unlock(&vmm_lock);

    return phys_to_return;
}

/* Copy kernel directory into a new page */
uint64_t* vmm_copy_kernel_directory() {
    uint64_t phys_pml4 = (uint64_t) pmm_alloc_page();
    uint64_t* pml4_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(phys_pml4);
    for (int i = 0; i < 512; i++) pml4_virt[i] = 0;

    spin_lock(&vmm_lock);

    // Copy Higher-Half fields (Upper 256 entries in PML4 table)
    for (int i = 256; i < 512; i++) {
        pml4_virt[i] = kernel_directory[i];
    }
    // Maintain low-half shared initialization segments mapping
    pml4_virt[0] = kernel_directory[0];

    spin_unlock(&vmm_lock);

    return (uint64_t*) phys_pml4;
}

/* Switch a new directory given the physical address */
void vmm_switch_directory(uint64_t* page_directory) {
    if (likely(vmm_get_current_directory() != page_directory)) {
        asm volatile("mov %0, %%cr3" : : "r"(page_directory) : "memory");
    }
}

/* Get the current page directory as a physical address */
uint64_t* vmm_get_current_directory() {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (uint64_t*) cr3;
}

/* Get the physical address of a virtual address */
uint64_t vmm_get_phys(uint64_t* pd_phys, void* virt_addr) {
    uint64_t v = (uint64_t) virt_addr;

    uint64_t pml4_idx = (v >> 39) & 0x1FF;
    uint64_t pdpt_idx = (v >> 30) & 0x1FF;
    uint64_t pd_idx   = (v >> 21) & 0x1FF;
    uint64_t pt_idx   = (v >> 12) & 0x1FF;

    spin_lock(&vmm_lock);

    uint64_t* pml4_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pd_phys);
    if (unlikely(!(pml4_virt[pml4_idx] & PAGE_PRESENT))) goto fail;

    uint64_t* pdpt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pml4_virt[pml4_idx] & PAGE_MASK);
    if (unlikely(!(pdpt_virt[pdpt_idx] & PAGE_PRESENT))) goto fail;

    uint64_t* pd_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pdpt_virt[pdpt_idx] & PAGE_MASK);
    if (unlikely(!(pd_virt[pd_idx] & PAGE_PRESENT))) goto fail;

    uint64_t* pt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pd_virt[pd_idx] & PAGE_MASK);
    if (unlikely(!(pt_virt[pt_idx] & PAGE_PRESENT))) goto fail;

    uint64_t phys_addr = (pt_virt[pt_idx] & PAGE_MASK) | (v & 0xFFF);
    spin_unlock(&vmm_lock);

    return phys_addr;

fail:
    spin_unlock(&vmm_lock);
    return 0;
}

/* Check if a virtual address is mapped or not */
int vmm_is_mapped(uint64_t* pd_phys, void* virt) {
    uint64_t virt_addr = (uint64_t) virt;

    uint64_t pml4_idx  = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx  = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx    = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx    = (virt_addr >> 12) & 0x1FF;

    spin_lock(&vmm_lock);

    uint64_t* pml4_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pd_phys);
    if (unlikely(!(pml4_virt[pml4_idx] & PAGE_PRESENT))) goto fail;

    uint64_t* pdpt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pml4_virt[pml4_idx] & PAGE_MASK);
    if (unlikely(!(pdpt_virt[pdpt_idx] & PAGE_PRESENT))) goto fail;

    uint64_t* pd_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pdpt_virt[pdpt_idx] & PAGE_MASK);
    if (unlikely(!(pd_virt[pd_idx] & PAGE_PRESENT))) goto fail;

    uint64_t* pt_virt = (uint64_t*) PHYSICAL_TO_VIRTUAL(pd_virt[pd_idx] & PAGE_MASK);
    if (unlikely(!(pt_virt[pt_idx] & PAGE_PRESENT))) goto fail;

    spin_unlock(&vmm_lock);
    return 1;

fail:
    spin_unlock(&vmm_lock);
    return 0;
}
