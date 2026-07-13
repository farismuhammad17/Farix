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

#include "hal.h"

#include "cpu/multicore.h"
#include "drivers/terminal.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "memory/heap.h"

#define HEAP_INIT_SIZE_KB 512

void* heap_start = (void*) PHYSICAL_TO_VIRTUAL(0x1000000);
void* heap_end   = NULL;
HeapSegment* first_segment = NULL;

static spinlock heap_lock = 0;

/* Initialises heap by carving out the required memory */
void init_heap() {
    uint64_t initial_pages = (HEAP_INIT_SIZE_KB * 1024) / PAGE_SIZE;

    for (uint64_t i = 0; i < initial_pages; i++) {
        void* phys = pmm_alloc_page();
        vmm_map_page((uint64_t*) VIRTUAL_TO_PHYSICAL(kernel_directory),
            phys, (void*)((uint64_t) heap_start + (i * PAGE_SIZE)),
            PAGE_PRESENT | PAGE_RW | PAGE_CACHE);
    }

    heap_end = (void*)((uint64_t) heap_start + (initial_pages * PAGE_SIZE));

    first_segment = (HeapSegment*) heap_start;
    first_segment->size    = (initial_pages * PAGE_SIZE) - sizeof(HeapSegment);
    first_segment->next    = NULL;
    first_segment->prev    = NULL;
    first_segment->is_free = true;
    first_segment->magic   = HEAP_MAGIC;
    first_segment->caller  = 0;
}

/* Kernel malloc */
void* kmalloc(size_t size) {
    if (unlikely(size == 0)) return NULL;

    // Align size to 16 bytes
    size = (size + 15) & ~(size_t) 15;

    spin_lock(&heap_lock);

    HeapSegment* current = first_segment;

    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // We need enough space for the requested size + a new header + at least 16 bytes of data
            if (current->size > size + sizeof(HeapSegment) + 16) {
                size_t total_offset = sizeof(HeapSegment) + size;
                total_offset = (total_offset + 15) & ~(size_t) 15;
                HeapSegment* next_seg = (HeapSegment*) ((uint64_t) current + total_offset);

                next_seg->size = current->size - total_offset;
                next_seg->is_free = true;
                next_seg->next    = current->next;
                next_seg->prev    = current;
                next_seg->magic   = HEAP_MAGIC;
                next_seg->caller  = 0;

                if (current->next != NULL) {
                    current->next->prev = next_seg;
                }

                current->next = next_seg;
                current->size = total_offset - sizeof(HeapSegment);
            }

            current->is_free = false;
            current->caller  = (uint64_t) __builtin_return_address(0);

            spin_unlock(&heap_lock);
            return (void*)((uint64_t) current + sizeof(HeapSegment));
        }

        current = current->next;
    }

    spin_unlock(&heap_lock);

    // Reaching here means we are out of memory
    kheap_expand(size);
    return kmalloc(size);
}

/* Free memory malloc-ed by kernel */
void kfree(void* ptr) {
    if (unlikely(ptr == NULL)) return;

    spin_lock(&heap_lock);

    HeapSegment* current = (HeapSegment*)((uint64_t) ptr - sizeof(HeapSegment));

    // Verify magic field within lock protection before reading/writing nodes
    if (unlikely(current->magic != HEAP_MAGIC)) {
        err_printf("kfree: invalid magic field at %p\n", ptr);
        spin_unlock(&heap_lock);
        return;
    }

    current->is_free = true;
    current->caller  = 0;

    // Merge Right
    if (current->next && current->next->is_free && current->next->magic == HEAP_MAGIC) {
        current->size += current->next->size + sizeof(HeapSegment);
        current->next = current->next->next;
        if (current->next) current->next->prev = current;
    }

    // Merge Left
    if (current->prev && current->prev->is_free && current->prev->magic == HEAP_MAGIC) {
        HeapSegment* prev = current->prev;
        prev->size += current->size + sizeof(HeapSegment);
        prev->next = current->next;
        if (current->next) current->next->prev = prev;

        current->magic  = 0;
        current->caller = 0;
    }

    spin_unlock(&heap_lock);
}

/* Expand heap when exceeded */
void kheap_expand(size_t size) {
    size_t total_needed = size + sizeof(HeapSegment);
    size_t pages_to_alloc = (total_needed + PAGE_SIZE - 1) / PAGE_SIZE;

    // Keep slow physical frame generation and page table mapping outside the spinlock range
    for (size_t i = 0; i < pages_to_alloc; i++) {
        void* phys = pmm_alloc_page();
        vmm_map_page((uint64_t*) VIRTUAL_TO_PHYSICAL(kernel_directory),
            phys, (void*)((uint64_t) heap_end + (i * PAGE_SIZE)),
            PAGE_PRESENT | PAGE_RW | PAGE_CACHE);
    }

    spin_lock(&heap_lock);

    HeapSegment* new_seg = (HeapSegment*) heap_end;
    heap_end = (void*)((uint64_t) heap_end + (pages_to_alloc * PAGE_SIZE));

    new_seg->size = (pages_to_alloc * PAGE_SIZE) - sizeof(HeapSegment);
    new_seg->is_free = true;
    new_seg->magic = HEAP_MAGIC;

    // Link it to the end of the chain
    HeapSegment* last = first_segment;
    while (last->next != NULL) last = last->next;

    last->next = new_seg;
    new_seg->prev = last;
    new_seg->next = NULL;

    spin_unlock(&heap_lock);

    // Safely execute merge cleanup via kfree wrapper
    kfree((void*)((uint64_t) new_seg + sizeof(HeapSegment)));
}

/* Total used memory by heap */
size_t get_heap_total() {
    size_t total = 0;

    spin_lock(&heap_lock);

    HeapSegment* current = first_segment;
    while (current != NULL) {
        total += current->size + sizeof(HeapSegment);
        current = current->next;
    }

    spin_unlock(&heap_lock);

    return total;
}

/* Total memory used by non-free heap segments */
size_t get_heap_used() {
    size_t used = 0;

    spin_lock(&heap_lock);

    HeapSegment* current = first_segment;
    while (current != NULL) {
        if (!current->is_free) {
            used += current->size;
        }
        current = current->next;
    }

    spin_unlock(&heap_lock);

    return used;
}
