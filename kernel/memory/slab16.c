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
#include <string.h>

#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "memory/slab.h"

Slab16* create_slab16(uint16_t object_size) {
    void* phys = pmm_alloc_page();
    if (unlikely(!phys)) {
        t_print("create_slab16: pmm_alloc_page failed");
        uart_print("create_slab16: pmm_alloc_page failed\n");
        return NULL;
    }

    // Convert the physical page address to a virtual one
    Slab16* slab = (Slab16*) PHYSICAL_TO_VIRTUAL(phys);
    vmm_map_page(vmm_get_current_directory(), phys, (void*) slab, PAGE_PRESENT | PAGE_RW);

    // Zero out the page
    memset(slab, 0, PAGE_SIZE);

    slab->next  = NULL;
    slab->prev  = NULL;
    slab->magic = SLAB16_MAGIC;

    // Slabs use powers of 2 for faster calculations
    slab->obj_shift = (object_size <= 1) ? 0 : (32 - __builtin_clz(object_size - 1));

    uint32_t actual_capacity = (PAGE_SIZE - (uintptr_t) slab->data + (uintptr_t) slab) >> slab->obj_shift;
    if (actual_capacity > 16) actual_capacity = 16;

    slab->free_slots = actual_capacity;

    if (actual_capacity < 16) {
        slab->mask = ~((1ULL << actual_capacity) - 1);
    } else {
        slab->mask = 0;
    }

    return slab;
}

void delete_slab16(Slab16* slab) {
    if (slab->prev)
        slab->prev->next = slab->next;
    if (slab->next)
        slab->next->prev = slab->prev;

    pmm_free_page((void*) vmm_unmap_page(slab));
}

void* slab_alloc16(Slab16* head) {
    if (unlikely(!head || head->magic != SLAB16_MAGIC)) {
        t_print("slab_alloc16: Invalid head");
        uart_print("slab_alloc16: Invalid head\n");
        return NULL;
    }

    Slab16* curr = head;

    while (unlikely(curr->free_slots == 0)) {
        if (unlikely(curr->next == NULL)) {
            curr->next = create_slab16(1 << curr->obj_shift);
            curr->next->prev = curr;
            curr = curr->next; // Don't bother checking if a new slab is empty
            break;
        }

        curr = curr->next;
    }

    int slot = __builtin_ctz(~curr->mask);
    curr->mask |= (1U << slot);
    curr->free_slots--;

    uintptr_t addr = (uintptr_t) curr->data + (slot << curr->obj_shift);

    // Verify pointer is in the same page
    if (unlikely((addr & 0xFFFFF000) != ((uintptr_t) curr & 0xFFFFF000))) {
        t_printf("slab_alloc16: Page boundary overflow");
        uart_printf("slab_alloc16: Page boundary overflow");
        return NULL;
    }

    return (void*) addr;
}

void slab_free16(void* ptr) {
    // Jump to the start of the 4KB page this pointer lives in.
    // This works because the PMM always gives us page-aligned memory.
    Slab16* slab = (Slab16*)((uintptr_t) ptr & 0xFFFFF000);

    if (unlikely(slab->magic != SLAB16_MAGIC)) {
        uart_printf("slab_free16: Slab pointer %x has invalid magic", ptr);
        return;
    }

    // Find the bit index
    int bit_index = ((uintptr_t) ptr - (uintptr_t) slab->data) >> slab->obj_shift;

    // One instruction to clear the bit
    slab->mask &= ~(1U << bit_index);
    slab->free_slots++;

    // Free the slab if its empty
    // If no previous, this is head. Deleting head may lead to thrashing,
    // where the kernel pmm allocs and frees constantly. Manually free
    // from PMM if the slab is done.
    if (slab->free_slots == 16 && slab->prev != NULL) {
        // Stitch neighbors
        slab->prev->next = slab->next;
        if (slab->next)
            slab->next->prev = slab->prev;

        // Return the memory to the PMM
        pmm_free_page((void*) vmm_unmap_page(slab));
    }
}
