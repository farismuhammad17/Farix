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

#ifndef SLAB_H
#define SLAB_H

#define SLAB_MAGIC 0x51AB51AB

typedef struct Slab64 {
    uint64_t mask;
    uint64_t free_slots;
    uint32_t slab_magic;
    uint16_t obj_shift;

    struct Slab64* next;
    struct Slab64* prev;

    char data[] __attribute__((aligned(8)));
} __attribute__((aligned(64))) Slab64;

Slab64* create_slab(uint16_t object_size);
void    delete_slab(Slab64* slab);

void* slab_alloc(Slab64* head);
void  slab_free(void* ptr);

#endif
