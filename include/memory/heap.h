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

#include <stdint.h>
#include <stddef.h>

struct HeapSegment {
    size_t size;
    HeapSegment* next;
    HeapSegment* prev;
    bool is_free;
    uint32_t magic;
};

void   init_heap();
void*  malloc(size_t size);
void   free(void* ptr);
void   memcpy(void* dest, const void* source, size_t n);

void   heap_expand(size_t size);

size_t get_heap_total();
size_t get_heap_used();

void*  operator new      (size_t size);
void*  operator new[]    (size_t size);
void   operator delete   (void* p);
void   operator delete[] (void* p);
void   operator delete   (void* p, size_t size);

#endif
