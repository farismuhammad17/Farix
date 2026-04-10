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

#ifndef HEAP_H
#define HEAP_H

#define HEAP_INIT_SIZE 512 // KB

#define HEAP_MAGIC 0x12345678

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct HeapSegment {
    size_t size;
    struct HeapSegment* next;
    struct HeapSegment* prev;
    bool is_free;
    uint32_t magic;
    uint32_t caller;
} __attribute__((aligned(4))) HeapSegment;

extern void*        heap_start;
extern void*        heap_end;
extern HeapSegment* first_segment;

void   init_heap();

void*  kmalloc(size_t size);
void   kfree(void* ptr);

void   kmemcpy(void* dest, const void* source, size_t n);
void   kmemset(void* s, int c, size_t n);

void   kheap_expand(size_t size);

size_t get_heap_total();
size_t get_heap_used();

#endif
