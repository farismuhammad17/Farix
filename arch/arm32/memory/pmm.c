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

#include "memory/pmm.h"

#define FULL_MASK  0xFFFFFFFF
#define IS_FULL(i) (pmm_bitmap[i] == FULL_MASK)

// Defined in linker.ld
extern char _kernel_start;
extern char _kernel_end;

static uint32_t pmm_bitmap[PMM_BITMAP_SIZE];

/* Mark a page as USED (1) */
static inline void pmm_set_bit(uint32_t page_number) {
    uint32_t index = page_number >> 5;
    uint32_t bit   = page_number & 31;
    pmm_bitmap[index] |= (1 << bit);
}

/* Mark a page as FREE (0) */
static inline void pmm_clear_bit(uint32_t page_number) {
    uint32_t index = page_number >> 5;
    uint32_t bit   = page_number & 31;
    pmm_bitmap[index] &= ~(1 << bit);
}

/* Check if a page is in use */
static inline bool pmm_test_bit(uint32_t page_number) {
    uint32_t index = page_number >> 5;
    uint32_t bit   = page_number & 31;
    return (pmm_bitmap[index] & (1 << bit));
}

void init_pmm() {
    // Mark everything as USED (1) to be safe
    for (int i = 0; i < PMM_BITMAP_SIZE; i++) pmm_bitmap[i] = FULL_MASK;

    // Free the RAM (0x0 to 0x40000000 - 1GB)
    for (uint32_t i = 0; i < (1 << 30) / PAGE_SIZE; i++) {
        pmm_clear_bit(i);
    }

    // Protect the "Vector Table" area (Bottom 1MB)
    // ARM needs address 0x0 for exception vectors
    for (uint32_t i = 0; i < 256; i++) pmm_set_bit(i);

    // Protect Kernel
    uint32_t start_p = (uint32_t)  &_kernel_start / PAGE_SIZE;
    uint32_t end_p   = ((uint32_t) &_kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = start_p; i <= end_p; i++) pmm_set_bit(i);

    // Protect the Bitmap itself
    uint32_t b_start = (uint32_t) pmm_bitmap / PAGE_SIZE;
    uint32_t b_end   = ((uint32_t) pmm_bitmap + sizeof(pmm_bitmap) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = b_start; i <= b_end; i++) pmm_set_bit(i);
}

/* Request a 4 KB page from the PMM using a first-fit algorithm. */
void* pmm_alloc_page() {
    for (uint32_t i = 0; i < PMM_BITMAP_SIZE; i++) { // First-Fit algorithm
        if (unlikely(!IS_FULL(i))) {
            // Find first zero bit by inverting and counting trailing zeros
            uint32_t page_number = (i << 5) | __builtin_ctz(~pmm_bitmap[i]);
            pmm_set_bit(page_number);

            return (void*)((uintptr_t) page_number * PAGE_SIZE);
        }
    }

    // If we're out of memory
    return NULL;
}

/*
Iterates through the PMM To find the first continuous block of pages that was
required, and if none is found, NULL.
*/
void* pmm_alloc_pages(size_t length) {
    if (unlikely(length == 0)) return NULL;
    if (unlikely(length == 1)) return pmm_alloc_page();

    for (uint32_t i = 0; i <= (PMM_BITMAP_SIZE * 32) - length;) {
        // Skip quickly if the current bit is set
        if (pmm_test_bit(i)) {
            // If the whole 32-bit chunk is full, skip it
            if (IS_FULL(i >> 5)) {
                i = (i & ~31) + 32;
            } else {
                i++;
            }

            continue;
        }

        // Check if the next 'length' bits are free
        uint32_t pages_found = 0;
        for (pages_found = 0; pages_found < length; pages_found++) {
            if (pmm_test_bit(i + pages_found)) break;
        }

        if (pages_found == length) {
            uint32_t idx = i >> 5;
            uint32_t bit_offset = i & 31;

            // Check if the whole range fits inside the current 32-bit word
            if (bit_offset + length <= 32) {
                pmm_bitmap[idx] |= ((1ULL << length) - 1) << bit_offset;
            } else {
                // Spans multiple words
                for (uint32_t j = i; j < i + length; j++) {
                    pmm_set_bit(j);
                }
            }

            return (void*)((uintptr_t) i * PAGE_SIZE);
        }

        // Jump to the bit after the one that failed
        i += pages_found + 1;
    }

    return NULL;
}

void pmm_free_page(void* addr) {
    uint32_t address     = (uint32_t) addr;
    uint32_t page_number = address / PAGE_SIZE;

    pmm_clear_bit(page_number);
}
