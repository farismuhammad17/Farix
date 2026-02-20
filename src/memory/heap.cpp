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

#include "config.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "memory/heap.h"

static void* heap_start = (void*) 0x1000000;
static void* heap_end   = (void*) ((uint32_t) heap_start + PAGE_SIZE);
static HeapSegment* first_segment = nullptr;

void init_heap() {
    uint32_t initial_pages = 16; // 64KB - Plenty for 2 stacks + headers

    for (uint32_t i = 0; i < initial_pages; i++) {
        void* phys = pmm_alloc_page();
        // We call the existing vmm_map_page
        vmm_map_page(phys, (void*)((uint32_t)heap_start + i * PAGE_SIZE), PAGE_PRESENT | PAGE_RW);
    }

    first_segment = (HeapSegment*) heap_start;

    first_segment->size    = PAGE_SIZE - sizeof(HeapSegment);
    first_segment->next    = nullptr;
    first_segment->prev    = nullptr;
    first_segment->is_free = true;
    first_segment->magic   = 0x12345678;
}

void* malloc(size_t size) {
    // Align size to 4 bytes
    if (size % 4 != 0) {
        size = (size & ~0x3) + 4;
    }

    HeapSegment* current = first_segment;

    while (current != nullptr) {
        if (current->is_free && current->size >= size) {
            // We need enough space for the requested size + a new header + at least 4 bytes of data
            if (current->size > size + sizeof(HeapSegment) + 4) {
                HeapSegment* next_seg = (HeapSegment*) ((uint32_t) current + sizeof(HeapSegment) + size);

                next_seg->size    = current->size - size - sizeof(HeapSegment);
                next_seg->is_free = true;
                next_seg->next    = current->next;
                next_seg->prev    = current;
                next_seg->magic   = 0x12345678;

                if (current->next != nullptr) {
                    current->next->prev = next_seg;
                }

                current->next = next_seg;
                current->size = size;
            }

            current->is_free = false;

            return (void*)((uint32_t)current + sizeof(HeapSegment));
        }

        current = current->next;
    }

    // Reaching here means we are out of memory
    heap_expand(size);
    return malloc(size);
}

void free(void* ptr) {
    if (ptr == nullptr) return;

    HeapSegment* current = (HeapSegment*) ((uint32_t) ptr - sizeof(HeapSegment));

    if (current->magic != 0x12345678) return;

    current->is_free = true;

    // Merge with Next (Merge Right)
    if (current->next != nullptr && current->next->is_free) {
        current->size += current->next->size + sizeof(HeapSegment);

        current->next = current->next->next;
        if (current->next != nullptr) {
            current->next->prev = current;
        }
    }

    // Merge with Prev (Merge Left)
    if (current->prev != nullptr && current->prev->is_free) {
        current->prev->size += current->size + sizeof(HeapSegment);
        current->prev->next =  current->next;

        if (current->next != nullptr) {
            current->next->prev = current->prev;
        }
    }
}

void heap_expand(size_t size) {
    size_t total_needed = size + sizeof(HeapSegment);
    size_t pages_to_alloc = (total_needed / PAGE_SIZE) + 1;

    for (size_t i = 0; i < pages_to_alloc; i++) {
        void* phys = pmm_alloc_page();
        vmm_map_page(phys, heap_end, PAGE_PRESENT | PAGE_RW);
        heap_end = (void*)((uint32_t)heap_end + PAGE_SIZE);
    }

    HeapSegment* new_seg = (HeapSegment*)((uint32_t)heap_end - (pages_to_alloc * PAGE_SIZE));
    new_seg->size = (pages_to_alloc * PAGE_SIZE) - sizeof(HeapSegment);
    new_seg->is_free = true;
    new_seg->magic = 0x12345678;

    HeapSegment* current = first_segment;
    while (current->next != nullptr) current = current->next;

    current->next = new_seg;
    new_seg->prev = current;
    new_seg->next = nullptr;

    free((void*)((uint32_t)new_seg + sizeof(HeapSegment)));
}

size_t get_heap_total() {
    size_t total = 0;
    HeapSegment* current = first_segment;
    while (current != nullptr) {
        total += current->size + sizeof(HeapSegment);
        current = current->next;
    }
    return total;
}

size_t get_heap_used() {
    size_t used = 0;
    HeapSegment* current = first_segment;
    while (current != nullptr) {
        if (!current->is_free) {
            used += current->size;
        }
        current = current->next;
    }
    return used;
}

void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void* p) {
    free(p);
}

void operator delete(void* p, size_t size) {
    free(p);
}

void operator delete[](void* p) {
    free(p);
}

void operator delete[](void* p, size_t size) {
    free(p);
}
