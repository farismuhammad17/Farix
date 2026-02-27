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

#include <stdio.h>

#include "memory/pmm.h"
#include "memory/vmm.h"

#include "memory/heap.h"

static void*        heap_start    = (void*) 0x1000000;
static void*        heap_end      = nullptr;
static HeapSegment* first_segment = nullptr;

bool check_heap() {
    HeapSegment* current = first_segment;
    while (current != nullptr) {
        // Magic Check
        if (current->magic != HEAP_MAGIC) {
            printf("HEAP CORRUPTION: Bad Magic at %p (Val: %x)\n", current, current->magic);
            return false;
        }

        // Alignment Check
        if (((uint32_t)current & 0x3) != 0) {
            printf("HEAP CORRUPTION: Unaligned segment pointer %p\n", current);
            return false;
        }

        // Pointer Check
        if (current->next != nullptr) {
            if (current->next <= current) {
                printf("HEAP CORRUPTION: Circular or backwards link at %p -> %p\n", current, current->next);
                return false;
            }
            if (current->next->prev != current) {
                printf("HEAP CORRUPTION: Broken backlink! %p->next is %p, but that block's prev is %p\n",
                        current, current->next, current->next->prev);
                return false;
            }
        }

        current = current->next;
    }

    return true;
}

void init_heap() {
    uint32_t initial_pages = 16;

    for (uint32_t i = 0; i < initial_pages; i++) {
        void* phys = pmm_alloc_page();
        vmm_map_page(phys, (void*)((uint32_t) heap_start + i * PAGE_SIZE), PAGE_PRESENT | PAGE_RW);
    }

    heap_end = (void*)((uint32_t) heap_start + (initial_pages * PAGE_SIZE));

    first_segment = (HeapSegment*) heap_start;
    first_segment->size    = (initial_pages * PAGE_SIZE) - sizeof(HeapSegment);
    first_segment->next    = nullptr;
    first_segment->prev    = nullptr;
    first_segment->is_free = true;
    first_segment->magic   = HEAP_MAGIC;
    first_segment->caller  = 0;
}

void* kmalloc(size_t size) {
    if (!check_heap()) {
        printf("malloc: Heap corrupted for size %u\n", size);
        while(1);
    }

    // Align size to 4 bytes
    if (size % 4 != 0) {
        size = (size & ~0x3) + 4;
    }

    HeapSegment* current = first_segment;

    while (current != nullptr) {
        if (current->is_free && current->size >= size) {
            // We need enough space for the requested size + a new header + at least 4 bytes of data
            if (current->size > size + sizeof(HeapSegment) + 4) {
                size_t total_offset = sizeof(HeapSegment) + size;
                if (total_offset % 4 != 0) total_offset = (total_offset & ~0x3) + 4;
                HeapSegment* next_seg = (HeapSegment*) ((uint32_t) current + total_offset);

                next_seg->size = current->size - total_offset;
                next_seg->is_free = true;
                next_seg->next    = current->next;
                next_seg->prev    = current;
                next_seg->magic   = HEAP_MAGIC;
                next_seg->caller  = 0;

                if (current->next != nullptr) {
                    current->next->prev = next_seg;
                }

                current->next = next_seg;
                current->size = total_offset - sizeof(HeapSegment);
            }

            current->is_free = false;
            current->caller  = (uint32_t) __builtin_return_address(0);

            return (void*)((uint32_t)current + sizeof(HeapSegment));
        }

        current = current->next;
    }

    // Reaching here means we are out of memory
    kheap_expand(size);
    return kmalloc(size);
}

void kfree(void* ptr) {
    if (!check_heap()) {
        printf("free: Heap corrupted of %p\n", ptr);
        while(1);
    }

    if (ptr == nullptr) return;
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
    uint8_t* dst = (uint8_t*) dest;
    uint8_t* src = (uint8_t*) source;
    for (size_t i = 0; i < n; i++) dst[i] = src[i];
}

void kmemset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*) s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t) c;
}

void kheap_expand(size_t size) {
    size_t total_needed = size + sizeof(HeapSegment);
    size_t pages_to_alloc = (total_needed + PAGE_SIZE - 1) / PAGE_SIZE;

    HeapSegment* new_seg = (HeapSegment*)heap_end;

    for (size_t i = 0; i < pages_to_alloc; i++) {
        void* phys = pmm_alloc_page();
        vmm_map_page(phys, (void*)((uint32_t) heap_end + (i * PAGE_SIZE)), PAGE_PRESENT | PAGE_RW);
    }

    heap_end = (void*)((uint32_t)heap_end + (pages_to_alloc * PAGE_SIZE));

    new_seg->size = (pages_to_alloc * PAGE_SIZE) - sizeof(HeapSegment);
    new_seg->is_free = true;
    new_seg->magic = HEAP_MAGIC;

    // Link it to the end of the chain
    HeapSegment* last = first_segment;
    while (last->next != nullptr) last = last->next;

    last->next = new_seg;
    new_seg->prev = last;
    new_seg->next = nullptr;

    // Merge this new free block with the previous one if possible
    // free function already handles merging
    kfree((void*)((uint32_t) new_seg + sizeof(HeapSegment)));
}

size_t get_heap_total() {
    check_heap();

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

void print_memstat() {
    printf("\n--- HEAP MAP ---\n");
    printf("Start: %p | End: %p\n", heap_start, heap_end);
    printf("----------------------------------------------------------------------\n");
    printf("Address    | Size      | Status | Caller Address\n");
    printf("----------------------------------------------------------------------\n");

    // Disable interrupts to prevent the scheduler from
    // switching tasks while we use the heap.
    asm volatile("cli");

    HeapSegment* current = first_segment;
    while (current != nullptr) {
        printf("%p | %-9u | %-6s | 0x%08X\n",
                current,
                current->size,
                current->is_free ? "FREE" : "USED",
                current->caller);
        current = current->next;
    }
    printf("----------------------------------------------------------------------\n");
    printf("Total Used: %u bytes\n", get_heap_used());
    printf("----------------------------------------------------------------------\n\n");

    size_t total_kb = get_heap_total() / 1024;
    size_t used_kb  = get_heap_used()  / 1024;

    asm volatile("sti");

    size_t free_kb  = total_kb - used_kb;

    int usage_pct = (total_kb > 0) ? (int)((used_kb * 100) / total_kb) : 0;

    printf("Memory Statistics (KiB):\n");
    printf("------------------------\n");
    printf("Total memory: %8u KiB\n", total_kb);
    printf("Used memory:  %8u KiB [%d%%]\n", used_kb, usage_pct);
    printf("Free memory:  %8u KiB\n", free_kb);
}
