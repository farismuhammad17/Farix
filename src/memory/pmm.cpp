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

// Defined in scripts/linker.ld
extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

static uint32_t pmm_bitmap[BITMAP_SIZE];

// Mark a page as USED (1)
static void pmm_set_bit(uint32_t page_number) {
    uint32_t index = page_number / 32;
    uint32_t bit   = page_number % 32;
    pmm_bitmap[index] |= (1 << bit);
}

// Mark a page as FREE (0)
static void pmm_clear_bit(uint32_t page_number) {
    uint32_t index = page_number / 32;
    uint32_t bit   = page_number % 32;
    pmm_bitmap[index] &= ~(1 << bit);
}

// Check if a page is in use
static bool pmm_test_bit(uint32_t page_number) {
    uint32_t index = page_number / 32;
    uint32_t bit   = page_number % 32;
    return (pmm_bitmap[index] & (1 << bit));
}

void init_pmm(multiboot_info* mbi) {
    if (!(mbi->flags & (1 << 6))) {
        return;
    }

    for (int i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    multiboot_mmap_entry* mmap = (multiboot_mmap_entry*) mbi->mmap_addr;
    // uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

    while((uint32_t) mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (mmap->type == 1) {
            uint32_t start_page  = (uint32_t)mmap->addr / PAGE_SIZE;
            uint32_t total_pages = (uint32_t)mmap->len / PAGE_SIZE;

            for (uint32_t i = 0; i < total_pages; i++) {
                pmm_clear_bit(start_page + i);
            }
        }

        mmap = (multiboot_mmap_entry*)((uint32_t)mmap + mmap->size + 4);
    }

    for (uint32_t i = 0; i < 256; i++) pmm_set_bit(i);

    uint32_t kernel_start_page = (uint32_t) &_kernel_start / PAGE_SIZE;
    uint32_t kernel_end_page   = (uint32_t) &_kernel_end   / PAGE_SIZE;

    for (uint32_t i = kernel_start_page; i <= kernel_end_page; i++) {
        pmm_set_bit(i);
    }
}

void* pmm_alloc_page() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t bit = (1 << j);

                if (!(pmm_bitmap[i] & bit)) {
                    uint32_t page_number = (i * 32) + j;
                    pmm_set_bit(page_number);

                    // Address = Page Number * 4096 (4KB)
                    return (void*) (page_number * PAGE_SIZE);
                }
            }
        }
    }

    // If we're out of memory
    return nullptr;
}

void pmm_free_page(void* addr) {
    uint32_t address     = (uint32_t)addr;
    uint32_t page_number = address / PAGE_SIZE;

    pmm_clear_bit(page_number);
}
