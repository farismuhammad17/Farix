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

/*

Allocation method

In x86, a 4 KB page is actually perfect, but unfortunately, that is not the case
here in ARM. Somewhy, it only needs 1 KB, but at the same time, it is heavily
discouraged to change PAGE_SIZE from 4096, as x86 might be sad. The solution for
this problem is what I thought up over here.

Because we use only 1 KB of every 4 KB page in ARM, we could track the partial
pages to keep making more use of it, till we fill out the 4 KB with 4 seperate
things inside the same page. Now, this is slow, and I actually asked Gemini what
it thought. It suggested a boring linked list, but that is absolute trash: we're
inside the functions that probably get called the most; linked lists suffer from
cache misses, making everything slower. It works, in theory, but terribly.

Instead, I chose to store a last_page, that will be NULL if the actual last page
is 4 KB full, but if it's 1 KB, 2 KB, or 3 KB, then it is stored in last_page.
When we allocate a "page", we just slap it on the last_page directly. If it filled,
just set it to NULL, if it's NULL, make a new 4 KB page the normal way.

There is an obvious loss here: fragmentation. Imagine we have a page, we fill it out:
(1, 2, 3, 4), then we make a new page, which just stores 1 KB, (5). Suppose we free
the page at 2, now the first page is 3 KB, but the kernel is blind to that, and as we
keep moving in runtime, these pages fill out, and we never go back and fill them, thus
losing memory with literally every new 1 KB made. The solution is to store partial tasks
if we ever free a page.

The problem lies in the fact that storing a flat array of "partial tasks" looks bad,
is ineffeicient, and is O(n). The better way is to still store an array of 64 partial
tasks, but with a bit mask uint64_t, where 0 is empty, 1 is occupied, which we can
use to quickly look up empty slots, and zoom past the empty slots. Unfortunately, arrays
have a fixed size. If you have 64 pages already in the array, and a new 65th page got
freed, where does it go? We have to prioritize cleaning this array as fast as possible,
thus, while allocating new pages, see that we first look inside this array. Only if the
bit mask is 0, do we use last_page, else, use an appropriate page from the array.

But if we were allocating a new page, and we had two choices, one with 1 KB, other with
3 KB, what is the better choice? Obviously the 3 KB, because once we fill it, we can
remove it from the array. Thus, while allocating pages, prioritize bigger pages, but
while freeing pages, prioritize smaller pages.

Unfortunately, this does mean we have to O(n) through our bit mask, and I don't like that,
so here is the better plan:-

Same 64 sized array, except index 0 is last_page, and we don't store it separetely, but we
treat it as unique. From index [1-21] is every page 1 KB full, [22-42] is 2 KB, [43-63] is
3 KB. Thus, we have a clean array of 20 pages per filled size, and when we are using any of
these subsets of the array, we can just use the first one we get, because it doesn't matter.

TODO: The array is, of course, finite, but because it is rare, I decided to store all the pages
that didn't find their place in the array into a linked list, and we can just pop them
into the array whenever we want.

*/

#define PARTIAL_PAGES_ARRAY_LEN 64

#define MASK_1KB_RANGE 0x00000000003FFFFEULL
#define MASK_2KB_RANGE 0x000007FFFFC00000ULL
#define MASK_3KB_RANGE 0xFFFFFF0000000000ULL

#define last_page partial_pages[0]

static void* partial_pages[PARTIAL_PAGES_ARRAY_LEN] = {NULL};
static uint64_t partial_pages_mask = 0;
static uint8_t last_page_usage = 0; // 1 = 1 KB used, 2 = 2 KB used, 3 = 3 KB used

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

static void vmm_map_page_new(uint32_t* pd_phys, void* phys, void* virt, uint32_t flags) {
    uint32_t virt_addr = (uint32_t) virt;
    uint32_t pd_index  = virt_addr >> 20;
    uint32_t pt_index  = (virt_addr >> 12) & 0xFF;
    uint32_t* pd_virt  = (uint32_t*) PHYSICAL_TO_VIRTUAL(pd_phys);

    // We don't need to check, since we only get to this function if
    // last_page is full and there are no partial pages anyway.
    uint32_t phys_table = (uint32_t) pmm_alloc_page();

    // Update Registry
    last_page = (void*) phys_table;
    last_page_usage = 1;
    partial_pages_mask |= (1ULL << 0);

    // Zero the 4KB so partial slots are ready later
    uint32_t* whole_page_ptr = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_table);
    for (int i = 0; i < 1024; i++) whole_page_ptr[i] = 0;

    // Link the first 1KB into the PD
    pd_virt[pd_index] = phys_table | VMM_TYPE_TABLE;

    // Map the actual page
    uint32_t* table = (uint32_t*) PHYSICAL_TO_VIRTUAL(phys_table);
    table[pt_index] = (uint32_t) phys | flags;

    // Flush
    asm volatile("mcr p15, 0, %0, c8, c7, 1" : : "r"(virt_addr) : "memory");
    asm volatile("dsb" ::: "memory");
    asm volatile("isb" ::: "memory");
}

static void vmm_map_existing_l2(uint32_t* pd_phys, void* l2_table_phys, void* phys, void* virt, uint32_t flags) {
    uint32_t virt_addr = (uint32_t) virt;
    uint32_t pd_index  = virt_addr >> 20;
    uint32_t pt_index  = (virt_addr >> 12) & 0xFF;
    uint32_t* pd_virt  = (uint32_t*) PHYSICAL_TO_VIRTUAL(pd_phys);

    // Link the 1KB table to the Page Directory if not already there
    if (!(pd_virt[pd_index] & 0x01)) {
        pd_virt[pd_index] = (uint32_t) l2_table_phys | VMM_TYPE_TABLE; // 0x01 = Page Table descriptor
    }

    // Get the virtual address of our 1KB table
    uint32_t* table = (uint32_t*) PHYSICAL_TO_VIRTUAL((uint32_t)l2_table_phys & ~0x3FF);

    // Set the entry
    table[pt_index] = (uint32_t) phys | flags | PAGE_PRESENT;

    asm volatile("mcr p15, 0, %0, c8, c7, 1" : : "r"(virt_addr) : "memory");
    asm volatile("dsb" ::: "memory");
    asm volatile("isb" ::: "memory");
}

void init_vmm() {
    // 16KB required for ARM L1 table. Since pmm_alloc_page gives 4KB,
    // We have dummy allocs to reserve the 16KB block
    uint32_t phys_pd = 0;
    while (1) {
        phys_pd = (uint32_t) pmm_alloc_page();
        if ((phys_pd & 0x3FFF) == 0) break;
    }

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
    if (partial_pages_mask & MASK_3KB_RANGE) {
        size_t index = 63 - __builtin_clzll(partial_pages_mask & MASK_3KB_RANGE);
        void* page_phys = partial_pages[index];

        // 3KB used, so the free slot is the 4th (offset 3072)
        uint32_t table_phys = (uint32_t) page_phys + 3072;

        // Clear the registry entry
        partial_pages[index] = NULL;
        partial_pages_mask &= ~(1ULL << index);

        // Link it and finish
        vmm_map_existing_l2(pd_phys, (void*)table_phys, phys, virt, flags);

        return;
    }

    else if ((partial_pages_mask & MASK_2KB_RANGE) && ((~partial_pages_mask) & MASK_3KB_RANGE)) {
        size_t old_index = 63 - __builtin_clzll(partial_pages_mask & MASK_2KB_RANGE);
        void* page_phys = partial_pages[old_index];

        // Offset for the 3rd 1KB slot is 2048
        uint32_t table_phys = (uint32_t) page_phys + 2048;

        // Find an empty slot in Tier 3 (43-63)
        size_t new_index = 63 - __builtin_clzll((~partial_pages_mask) & MASK_3KB_RANGE);

        // Move the pointer and update mask
        partial_pages[new_index] = page_phys;
        partial_pages[old_index] = NULL;
        partial_pages_mask &= ~(1ULL << old_index);
        partial_pages_mask |= (1ULL << new_index);

        vmm_map_existing_l2(pd_phys, (void*)table_phys, phys, virt, flags);

        return;
    }

    else if ((partial_pages_mask & MASK_1KB_RANGE) && ((~partial_pages_mask) & MASK_2KB_RANGE)) {
        size_t old_index = 63 - __builtin_clzll(partial_pages_mask & MASK_1KB_RANGE);
        void* page_phys = partial_pages[old_index];

        // Offset for the 2nd 1KB slot is 1024
        uint32_t table_phys = (uint32_t)page_phys + 1024;

        size_t new_index = 63 - __builtin_clzll((~partial_pages_mask) & MASK_2KB_RANGE);

        partial_pages[new_index] = page_phys;
        partial_pages[old_index] = NULL;
        partial_pages_mask &= ~(1ULL << old_index);
        partial_pages_mask |= (1ULL << new_index);

        vmm_map_existing_l2(pd_phys, (void*)table_phys, phys, virt, flags);

        return;
    }

    if (last_page != NULL) {
        uint32_t table_phys = (uint32_t) last_page + (last_page_usage << 10);
        last_page_usage++;

        // If it's now 4KB used, it's full.
        if (last_page_usage == 4) {
            last_page = NULL;
            last_page_usage = 0;
            partial_pages_mask &= ~(1ULL << 0);
        }

        vmm_map_existing_l2(pd_phys, (void*)table_phys, phys, virt, flags);
    }

    else return vmm_map_page_new(pd_phys, phys, virt, flags);
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

uint32_t* vmm_get_current_directory() {
    uint32_t ttbr0;
    asm volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttbr0));
    return (uint32_t*)(ttbr0 & ~0x7F);
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
