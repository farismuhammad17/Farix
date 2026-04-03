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

// Defined in linker.ld
extern char _kernel_start;
extern char _kernel_end;

static uint32_t pmm_bitmap[BITMAP_SIZE];

// Mark a page as USED (1)
static void pmm_set_bit(uint32_t page_number) {
    uint32_t index = page_number >> 5;
    uint32_t bit   = page_number & 31;
    pmm_bitmap[index] |= (1 << bit);
}

// Mark a page as FREE (0)
static void pmm_clear_bit(uint32_t page_number) {
    uint32_t index = page_number >> 5;
    uint32_t bit   = page_number & 31;
    pmm_bitmap[index] &= ~(1 << bit);
}

// Check if a page is in use
// CURRENTLY UNUSED
// static bool pmm_test_bit(uint32_t page_number) {
//     uint32_t index = page_number >> 5;
//     uint32_t bit   = page_number & 31;
//     return (pmm_bitmap[index] & (1 << bit));
// }

void init_pmm() {
    // Mark everything as USED (1) to be safe
    for (int i = 0; i < BITMAP_SIZE; i++) pmm_bitmap[i] = 0xFFFFFFFF;

    // Free the RAM (0x0 to 0x40000000 - 1GB)
    // 1024 * 1024 * 1024 / PAGE_SIZE = 262144 pages
    for (uint32_t i = 0; i < 262144; i++) {
        pmm_clear_bit(i);
    }

    // Protect the "Vector Table" area (Bottom 1MB)
    // ARM needs address 0x0 for exception vectors
    for (uint32_t i = 0; i < 256; i++) pmm_set_bit(i);

    // Protect Kernel
    uint32_t start_p = (uint32_t)  &_kernel_start >> LOG2_PAGE_SIZE;
    uint32_t end_p   = ((uint32_t) &_kernel_end + 4095) >> LOG2_PAGE_SIZE;
    for (uint32_t i = start_p; i <= end_p; i++) pmm_set_bit(i);

    // Protect the Bitmap itself
    uint32_t b_start = (uint32_t) pmm_bitmap >> LOG2_PAGE_SIZE;
    uint32_t b_end   = ((uint32_t) pmm_bitmap + sizeof(pmm_bitmap) + PAGE_SIZE - 1) >> LOG2_PAGE_SIZE;
    for (uint32_t i = b_start; i <= b_end; i++) pmm_set_bit(i);
}

void* pmm_alloc_page() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) { // First-Fit algorithm
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t bit = (1 << j);

                if (!(pmm_bitmap[i] & bit)) {
                    uint32_t page_number = (i << 5) | j;
                    pmm_set_bit(page_number);

                    // Address = Page Number * 4096 (4KB)
                    return (void*)(page_number << LOG2_PAGE_SIZE);
                }
            }
        }
    }

    // If we're out of memory
    return NULL;
}

void pmm_free_page(void* addr) {
    uint32_t address     = (uint32_t) addr;
    uint32_t page_number = address >> LOG2_PAGE_SIZE;

    pmm_clear_bit(page_number);
}
