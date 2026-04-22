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

typedef struct Slab32 {
    uint32_t mask;
    uint32_t free_slots;
    uint32_t slab_magic;
    uint16_t obj_shift;

    struct Slab32* next;
    struct Slab32* prev;

    char data[] __attribute__((aligned(8)));
} __attribute__((aligned(64))) Slab32;

typedef struct Slab16 {
    uint16_t mask;
    uint16_t free_slots;
    uint32_t slab_magic;
    uint16_t obj_shift;

    struct Slab16* next;
    struct Slab16* prev;

    char data[] __attribute__((aligned(8)));
} __attribute__((aligned(64))) Slab16;

typedef struct Slab8 {
    uint8_t mask;
    uint8_t free_slots;
    uint32_t slab_magic;
    uint16_t obj_shift;

    struct Slab8* next;
    struct Slab8* prev;

    char data[] __attribute__((aligned(8)));
} __attribute__((aligned(64))) Slab8;

Slab64* create_slab64 (uint16_t object_size);
Slab32* create_slab32 (uint16_t object_size);
Slab16* create_slab16 (uint16_t object_size);
Slab8*  create_slab8  (uint16_t object_size);

void    delete_slab64 (Slab64* slab);
void    delete_slab32 (Slab32* slab);
void    delete_slab16 (Slab16* slab);
void    delete_slab8  (Slab8*  slab);

void*   slab_alloc64  (Slab64* head);
void*   slab_alloc32  (Slab32* head);
void*   slab_alloc16  (Slab16* head);
void*   slab_alloc8   (Slab8*  head);

void    slab_free64   (void* ptr);
void    slab_free32   (void* ptr);
void    slab_free16   (void* ptr);
void    slab_free8    (void* ptr);

#endif
