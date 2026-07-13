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

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_ACCESS_PRESENT    0x80 // Must be 1 for valid segments
#define GDT_ACCESS_RING0      0x00 // Kernel privilege
#define GDT_ACCESS_RING3      0x60 // User privilege
#define GDT_ACCESS_CODE_DATA  0x10 // Must be 1 for code/data segments
#define GDT_ACCESS_EXECUTABLE 0x08 // 1 for Code ; 0 for Data
#define GDT_ACCESS_WRITABLE   0x02 // Readable for code, Writable for data
#define GDT_ACCESS_ACCESSED   0x01 // Set by CPU when segment is used

#define GDT_GRAN_4K    0x80 // Limit is in 4KB blocks
#define GDT_GRAN_64BIT 0x20

// 0: Null, 1: KCode, 2: KData, 3: UCode, 4: UData, 5 & 6: TSS (Double Entry)
#define GDT_TOTAL_ENTRIES 7

#define GDT_TSS_ACCESS_PRESENT   0x80
#define GDT_TSS_ACCESS_RING0     0x00
#define GDT_TSS_ACCESS_TYPE_TSS  0x09
#define GDT_TSS_ACCESS_FLAGS     (GDT_TSS_ACCESS_PRESENT | GDT_TSS_ACCESS_RING0 | GDT_TSS_ACCESS_TYPE_TSS)

#define GDT_TSS_GRAN_FLAGS       0x00

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t limit;
    uint64_t base; // Expanded to 64-bit for x86_64 long mode
} __attribute__((packed)) GDTPointer;

extern GDTEntry   gdt[GDT_TOTAL_ENTRIES];
extern GDTPointer gdt_ptr;

void RARE_FUNC init_gdt();

void RARE_FUNC gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void RARE_FUNC gdt_set_tss_entry(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif
