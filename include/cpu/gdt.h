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

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

enum GDT_Access : uint8_t {
    GDT_ACCESS_PRESENT     = 0x80, // Must be 1 for valid segments
    GDT_ACCESS_RING0       = 0x00, // Kernel privilege
    GDT_ACCESS_RING3       = 0x60, // User privilege
    GDT_ACCESS_CODE_DATA   = 0x10, // Must be 1 for code/data segments
    GDT_ACCESS_EXECUTABLE  = 0x08, // 1 for Code, 0 for Data
    GDT_ACCESS_WRITABLE    = 0x02, // Readable for code, Writable for data
    GDT_ACCESS_ACCESSED    = 0x01  // Set by CPU when segment is used
};

enum GDT_Granularity : uint8_t {
    GDT_GRAN_4K            = 0x80, // Limit is in 4KB blocks
    GDT_GRAN_32BIT         = 0x40, // 32-bit protected mode
    GDT_GRAN_64BIT         = 0x20  // Not used in i386
};

struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct GDTPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern "C" void gdt_flush(uint32_t gdt_ptr_addr);

void init_gdt();
void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif
