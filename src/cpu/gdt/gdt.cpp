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

#include "cpu/gdt.h"

GDTEntry   gdt[5];
GDTPointer gdt_ptr;

void init_gdt() {
    gdt_ptr.limit = (sizeof(GDTEntry) * 5) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    // 0x00: Null segment
    gdt_set_entry(0, 0, 0, 0, 0);

    // Common flags for all our segments
    uint8_t base_access = GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA;
    uint8_t base_gran   = GDT_GRAN_4K | GDT_GRAN_32BIT;

    // 0x08: Kernel Code
    gdt_set_entry(1, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING0 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F); // 0x0F represents the high 4 bits of the 0xFFFFF limit

    // 0x10: Kernel Data
    gdt_set_entry(2, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING0 | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F);

    // 0x18: User Mode Code
    gdt_set_entry(3, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING3 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F);

    // 0x20: User Mode Data
    gdt_set_entry(4, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING3 | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F);

    gdt_flush((uint32_t) &gdt_ptr);
}

void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Descriptor base address
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    // Descriptor limits
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    // Granularity and access flags
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access       = access;
}
