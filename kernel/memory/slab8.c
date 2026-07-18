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

#include "klib/string.h"

#include "cpu/multicore.h"
#include "drivers/terminal.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "memory/slab.h"

static spinlock slab_lock = 0;

/* Create and initialise new Slab8 */
Slab8* create_slab8(uint16_t object_size) {
    void* phys = pmm_alloc_page();
    if (unlikely(!phys)) {
        err_print("create_slab8: pmm_alloc_page failed");
        return NULL;
    }

    spin_lock(&slab_lock);

    // Convert the physical page address to a virtual one
    Slab8* slab = (Slab8*) PHYSICAL_TO_VIRTUAL(phys);
    vmm_map_page(vmm_get_current_directory(), phys, (void*) slab, PAGE_PRESENT | PAGE_RW);

    // Zero out the page
    memset(slab, 0, PAGE_SIZE);

    slab->next  = NULL;
    slab->prev  = NULL;
    slab->magic = SLAB8_MAGIC;

    // Slabs use powers of 2 for faster calculations
    slab->obj_shift = (object_size <= 1) ? 0 : (64 - __builtin_clzl((unsigned long)(object_size - 1)));

    uint32_t actual_capacity = (PAGE_SIZE - (uintptr_t) slab->data + (uintptr_t) slab) >> slab->obj_shift;
    if (actual_capacity > 8) actual_capacity = 8;

    slab->free_slots = actual_capacity;

    if (actual_capacity < 8) {
        slab->mask = ~((1ULL << actual_capacity) - 1);
    } else {
        slab->mask = 0;
    }

    spin_unlock(&slab_lock);

    return slab;
}

/* Free slab from memory */
void delete_slab8(Slab8* slab) {
    spin_lock(&slab_lock);

    if (slab->prev)
        slab->prev->next = slab->next;
    if (slab->next)
        slab->next->prev = slab->prev;

    spin_unlock(&slab_lock);

    pmm_free_page((void*) vmm_unmap_page(slab));
}

/* alloc space in the slab */
void* slab_alloc8(Slab8* head) {
    if (unlikely(!head || head->magic != SLAB8_MAGIC)) {
        err_print("slab_alloc8: Invalid head");
        return NULL;
    }

    spin_lock(&slab_lock);

    Slab8* curr = head;

    while (unlikely(curr->free_slots == 0)) {
        if (unlikely(curr->next == NULL)) {
            spin_unlock(&slab_lock);
            Slab8* new_slab = create_slab8(1 << curr->obj_shift);

            if (unlikely(!new_slab)) {
                return NULL;
            }

            spin_lock(&slab_lock);

            if (likely(curr->next == NULL)) {
                curr->next = new_slab;
                new_slab->prev = curr;
                curr = curr->next;
            } else {
                spin_unlock(&slab_lock);
                pmm_free_page((void*) vmm_unmap_page(new_slab));
                spin_lock(&slab_lock);
                continue;
            }
            break;
        }

        curr = curr->next;
    }

    int slot = __builtin_ctz(~(uint32_t) curr->mask & 0xFF);

    uintptr_t addr = (uintptr_t) curr->data + (slot << curr->obj_shift);

    uintptr_t slab_limit = (uintptr_t) curr + PAGE_SIZE;
    uintptr_t object_end = addr + (1 << curr->obj_shift);

    if (unlikely(object_end > slab_limit)) {
        err_printf("slab_alloc8: Slab at %p, Object at %p extends to %p (Limit: %p)",
                   curr, addr, object_end, slab_limit);
        spin_unlock(&slab_lock);
        return NULL;
    }

    curr->mask |= (1ULL << slot);
    curr->free_slots--;

    spin_unlock(&slab_lock);

    return (void*) addr;
}

/* Free alloc-ed space in slab */
void slab_free8(void* ptr) {
    // Jump to the start of the 4KB page this pointer lives in.
    // Handles 64-bit addresses safely by clearing the lower 12 bits.
    Slab8* slab = (Slab8*)((uintptr_t) ptr & ~(uintptr_t) 0xFFF);

    spin_lock(&slab_lock);

    if (unlikely(slab->magic != SLAB8_MAGIC)) {
        err_printf("slab_free8: Slab pointer %p has invalid magic", ptr);
        spin_unlock(&slab_lock);
        return;
    }

    // Find the bit index
    int bit_index = ((uintptr_t) ptr - (uintptr_t) slab->data) >> slab->obj_shift;

    // One instruction to clear the bit
    slab->mask &= ~(1ULL << bit_index);
    slab->free_slots++;

    bool should_free_slab = (slab->free_slots == 8 && slab->prev != NULL);
    if (should_free_slab) {
        // Stitch neighbors
        slab->prev->next = slab->next;
        if (slab->next) {
            slab->next->prev = slab->prev;
        }
    }

    spin_unlock(&slab_lock);

    // Free the slab if its empty
    // If no previous, this is head. Deleting head may lead to thrashing,
    // where the kernel pmm allocs and frees constantly. Manually free
    // from PMM if the slab is done.
    if (should_free_slab) {
        // Return the memory to the PMM
        pmm_free_page((void*) vmm_unmap_page(slab));
    }
}
