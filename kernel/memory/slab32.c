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

#include "drivers/uart.h"
#include "memory/pmm.h"

#include "memory/slab.h"

Slab32* create_slab32(uint16_t object_size) {
    Slab32* slab = (Slab32*) pmm_alloc_page();
    if (unlikely(!slab)) {
        uart_printf("create_slab32: pmm_alloc_page failed\n");
        return NULL;
    }

    slab->mask       = 0;
    slab->free_slots = 32;
    slab->next       = NULL;
    slab->prev       = NULL;
    slab->slab_magic = SLAB_MAGIC;

    // Slabs use powers of 2 for faster calculations
    if (unlikely(object_size <= 1)) {
        slab->obj_shift = 0;
    } else {
        // Find exponent of smallest power of 2 greater than given size
        slab->obj_shift = 32 - __builtin_clz(object_size - 1);
    }

    return slab;
}

void delete_slab32(Slab32* slab) {
    if (slab->prev)
        slab->prev->next = slab->next;
    if (slab->next)
        slab->next->prev = slab->prev;

    pmm_free_page(slab);
}

void* slab_alloc32(Slab32* head) {
    Slab32* curr = head;

    while (unlikely(curr->free_slots == 0)) {
        if (unlikely(curr->next == NULL)) {
            curr->next = create_slab32(1 << curr->obj_shift);
            curr->next->prev = curr;
            curr = curr->next; // Don't bother checking if a new slab is empty
            break;
        }

        curr = curr->next;
    }

    int slot = __builtin_ctz(~curr->mask);
    curr->mask |= (1U << slot);
    curr->free_slots--;

    return (void*)((uintptr_t) curr->data + (slot << curr->obj_shift));
}

void slab_free32(void* ptr) {
    // Jump to the start of the 4KB page this pointer lives in.
    // This works because the PMM always gives us page-aligned memory.
    Slab32* slab = (Slab32*)((uintptr_t) ptr & 0xFFFFF000);

    if (unlikely(slab->slab_magic != SLAB_MAGIC)) {
        uart_printf("slab_free32: Slab pointer %x has invalid magic", ptr);
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
    if (slab->free_slots == 32 && slab->prev != NULL) {
        // Stitch neighbors
        slab->prev->next = slab->next;
        if (slab->next)
            slab->next->prev = slab->prev;

        // Return the memory to the PMM
        pmm_free_page(slab);
    }
}
