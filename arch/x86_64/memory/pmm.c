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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "multiboot.h"

#include "cpu/multicore.h"

#include "memory/pmm.h"
#include "memory/vmm.h"

#define FULL_MASK  0xFFFFFFFFFFFFFFFFULL
#define IS_FULL(i) (pmm_bitmap[i] == FULL_MASK)

// Defined in scripts/linker.ld
extern char _kernel_start;
extern char _kernel_end;

extern multiboot_info* mbi;

static uint64_t pmm_bitmap[PMM_BITMAP_SIZE];

static spinlock pmm_lock = 0;

/* Mark a page as USED (1) */
static inline void pmm_set_bit(uint64_t page_number) {
    uint64_t index = page_number >> 6;
    uint64_t bit   = page_number & 63;
    pmm_bitmap[index] |= (1ULL << bit);
}

/* Mark a page as FREE (0) */
static inline void pmm_clear_bit(uint64_t page_number) {
    uint64_t index = page_number >> 6;
    uint64_t bit   = page_number & 63;
    pmm_bitmap[index] &= ~(1ULL << bit);
}

/* Check if a page is in use */
static inline bool pmm_test_bit(uint64_t page_number) {
    uint64_t index = page_number >> 6;
    uint64_t bit   = page_number & 63;
    return (pmm_bitmap[index] & (1ULL << bit));
}

/*
Initialise PMM and set the bitmap on the pages for the multiboot, kernel (so that
we don't overwrite the kernel during runtime), and the bitmask itself.
*/
void init_pmm() {
    if (unlikely(!(mbi->flags & (1 << 6)))) return;

    // Start by marking everything as used (1)
    for (int i = 0; i < PMM_BITMAP_SIZE; i++) pmm_bitmap[i] = FULL_MASK;

    // Protect multiboot
    multiboot_mmap_entry* mmap = (multiboot_mmap_entry*)((uintptr_t) PHYSICAL_TO_VIRTUAL(mbi->mmap_addr));
    uintptr_t mmap_end = (uintptr_t) mmap + mbi->mmap_length;

    while ((uintptr_t) mmap < mmap_end) {
        uint32_t entry_size = *(uint32_t*)((uintptr_t) mmap - 4);
        if (mmap->type == 1) {
            uint64_t start_page  = mmap->addr / PAGE_SIZE;
            uint64_t total_pages = mmap->len  / PAGE_SIZE;

            for (uint64_t i = start_page; i < total_pages + start_page; i++) {
                if (likely(i < (uint64_t) PMM_BITMAP_SIZE * 64)) {
                    pmm_clear_bit(i);
                }
            }
        }

        mmap = (multiboot_mmap_entry*)((uintptr_t) mmap + entry_size + 4);
    }

    // Protect the first 1MB / IVT / BIOS areas
    for (uint64_t i = 0; i < 256; i++) pmm_set_bit(i);

    // Protect Kernel (_kernel_start to _kernel_end)
    uint64_t start_p = (uintptr_t) &_kernel_start / PAGE_SIZE;
    uint64_t end_p   = ((uintptr_t) &_kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = start_p; i <= end_p; i++) pmm_set_bit(i);

    // Protect the Bitmap itself
    uint64_t b_start = VIRTUAL_TO_PHYSICAL(pmm_bitmap) / PAGE_SIZE;
    uint64_t b_end   = (VIRTUAL_TO_PHYSICAL(pmm_bitmap) + sizeof(pmm_bitmap) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = b_start; i <= b_end; i++) pmm_set_bit(i);

    // Protect the entire Multiboot structure block safely
    uint64_t mbi_start_p = VIRTUAL_TO_PHYSICAL(mbi) / PAGE_SIZE;
    uint64_t mbi_end_p   = (VIRTUAL_TO_PHYSICAL(mbi) + sizeof(multiboot_info) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t i = mbi_start_p; i <= mbi_end_p; i++) pmm_set_bit(i);
}

/* Request a 4 KB page from the PMM using a first-fit algorithm. */
void* pmm_alloc_page() {
    spin_lock(&pmm_lock);

    for (uint64_t i = 0; i < PMM_BITMAP_SIZE; i++) {
        if (unlikely(!IS_FULL(i))) {
            uint64_t page_number = (i << 6) | __builtin_ctzll(~pmm_bitmap[i]);
            pmm_set_bit(page_number);

            spin_unlock(&pmm_lock);
            return (void*)((uintptr_t) page_number * PAGE_SIZE);
        }
    }

    spin_unlock(&pmm_lock);
    return NULL;
}

/*
Iterates through the PMM to find the first continuous block of pages that was
required, and if none is found, NULL.
*/
void* pmm_alloc_pages(size_t length) {
    if (unlikely(length <= 0)) return NULL;
    if (unlikely(length == 1)) return pmm_alloc_page();

    spin_lock(&pmm_lock);

    uint64_t max_bit = (uint64_t) PMM_BITMAP_SIZE * 64;
    for (uint64_t i = 0; i <= max_bit - length;) {
        // Skip quickly if the current bit is set
        if (pmm_test_bit(i)) {
            // If the whole 64-bit chunk is full, skip it safely
            if (IS_FULL(i >> 6)) {
                i = (i & ~63ULL) + 64;
            } else {
                i++;
            }
            continue;
        }

        // Check if the next 'length' bits are free
        uint64_t pages_found = 0;
        for (pages_found = 0; pages_found < length; pages_found++) {
            if (pmm_test_bit(i + pages_found)) break;
        }

        if (pages_found == length) {
            uint64_t idx = i >> 6;
            uint64_t bit_offset = i & 63;

            // Check if the whole range fits inside the current 64-bit word
            if (bit_offset + length <= 64) {
                pmm_bitmap[idx] |= ((1ULL << length) - 1) << bit_offset;
            } else {
                // Spans multiple words
                for (uint64_t j = i; j < i + length; j++) {
                    pmm_set_bit(j);
                }
            }

            spin_unlock(&pmm_lock);
            return (void*)((uintptr_t) i * PAGE_SIZE);
        }

        // Jump to the bit after the one that failed
        i += pages_found + 1;
    }

    spin_unlock(&pmm_lock);
    return NULL;
}

/* Unmark the page from the bitmask, letting another process overwrite it later */
void pmm_free_page(void* addr) {
    uintptr_t address    = (uintptr_t)addr;
    uint64_t page_number = address / PAGE_SIZE;

    spin_lock(&pmm_lock);
    pmm_clear_bit(page_number);
    spin_unlock(&pmm_lock);
}
