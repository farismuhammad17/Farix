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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "arch/stubs.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "memory/heap.h"

void*        heap_start    = (void*) PHYSICAL_TO_VIRTUAL(0x1000000);
void*        heap_end      = NULL;
HeapSegment* first_segment = NULL;

void init_heap() {
    uint32_t initial_pages = HEAP_INIT_SIZE >> 2;

    for (uint32_t i = 0; i < initial_pages; i++) {
        void* phys = pmm_alloc_page();
        vmm_map_page(kernel_directory, phys, (void*)((uint32_t) heap_start + (i << LOG2_PAGE_SIZE)), PAGE_PRESENT | PAGE_RW | PAGE_CACHE);
    }

    heap_end = (void*)((uint32_t) heap_start + (initial_pages << LOG2_PAGE_SIZE));

    first_segment = (HeapSegment*) heap_start;
    first_segment->size    = (initial_pages << LOG2_PAGE_SIZE) - sizeof(HeapSegment);
    first_segment->next    = NULL;
    first_segment->prev    = NULL;
    first_segment->is_free = true;
    first_segment->magic   = HEAP_MAGIC;
    first_segment->caller  = 0;
}

void* kmalloc(size_t size) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;

    HeapSegment* current = first_segment;

    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // We need enough space for the requested size + a new header + at least 4 bytes of data
            if (current->size > size + sizeof(HeapSegment) + 4) {
                size_t total_offset = sizeof(HeapSegment) + size;
                total_offset = (total_offset + 3) & ~3;
                HeapSegment* next_seg = (HeapSegment*) ((uint32_t) current + total_offset);

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
            current->caller  = (uint32_t) __builtin_return_address(0);

            return (void*)((uint32_t) current + sizeof(HeapSegment));
        }

        current = current->next;
    }

    // Reaching here means we are out of memory
    kheap_expand(size);
    return kmalloc(size);
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    HeapSegment* current = (HeapSegment*) ((uint32_t) ptr - sizeof(HeapSegment));
    if (current->magic != HEAP_MAGIC) return;

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
}

void kmemcpy(void* dest, const void* source, size_t n) {
    volatile uint8_t* dst = (volatile uint8_t*) dest;
    const uint8_t* src = (const uint8_t*) source;
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }

    // Ensure writes are finished
    cpu_mem_barrier();
}

void kmemset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*) s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t) c;
}

void kheap_expand(size_t size) {
    size_t total_needed = size + sizeof(HeapSegment);
    size_t pages_to_alloc = (total_needed + PAGE_SIZE - 1) >> LOG2_PAGE_SIZE;

    HeapSegment* new_seg = (HeapSegment*) heap_end;

    for (size_t i = 0; i < pages_to_alloc; i++) {
        void* phys = pmm_alloc_page();
        vmm_map_page(kernel_directory, phys, (void*)((uint32_t) heap_end + (i << LOG2_PAGE_SIZE)), PAGE_PRESENT | PAGE_RW | PAGE_CACHE);
    }

    heap_end = (void*)((uint32_t) heap_end + (pages_to_alloc << LOG2_PAGE_SIZE));

    new_seg->size = (pages_to_alloc << LOG2_PAGE_SIZE) - sizeof(HeapSegment);
    new_seg->is_free = true;
    new_seg->magic = HEAP_MAGIC;

    // Link it to the end of the chain
    HeapSegment* last = first_segment;
    while (last->next != NULL) last = last->next;

    last->next = new_seg;
    new_seg->prev = last;
    new_seg->next = NULL;

    // Merge this new free block with the previous one if possible
    // free function already handles merging
    kfree((void*)((uint32_t) new_seg + sizeof(HeapSegment)));
}

size_t get_heap_total() {
    size_t total = 0;
    HeapSegment* current = first_segment;
    while (current != NULL) {
        total += current->size + sizeof(HeapSegment);
        current = current->next;
    }
    return total;
}

size_t get_heap_used() {
    size_t used = 0;
    HeapSegment* current = first_segment;
    while (current != NULL) {
        if (!current->is_free) {
            used += current->size;
        }
        current = current->next;
    }
    return used;
}
