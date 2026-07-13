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

#include <stdint.h>

#include "tss.h"

#include "memory/vmm.h"

#include "gdt.h"

GDTEntry   gdt[GDT_TOTAL_ENTRIES];
GDTPointer gdt_ptr;

void gdt_flush(GDTPointer* ptr);

/* Initialises the 64-bit GDT */
void init_gdt() {
    gdt_ptr.limit = (sizeof(GDTEntry) * GDT_TOTAL_ENTRIES) - 1;
    gdt_ptr.base  = (uint64_t) &gdt;

    // 0x00: Null segment
    gdt_set_entry(0, 0, 0, 0, 0);

    // Common access configuration
    uint8_t base_access = GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA;

    // 0x08: Kernel Code Segment (Requires 64-bit flag set, Limit/Base ignored)
    gdt_set_entry(1, 0, 0,
                  base_access | GDT_ACCESS_RING0 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_WRITABLE,
                  GDT_GRAN_64BIT);

    // 0x10: Kernel Data Segment (Base/Limit ignored)
    gdt_set_entry(2, 0, 0,
                  base_access | GDT_ACCESS_RING0 | GDT_ACCESS_WRITABLE,
                  0);

    // 0x18: User Mode Code Segment (Requires 64-bit flag set, Limit/Base ignored)
    gdt_set_entry(3, 0, 0,
                  base_access | GDT_ACCESS_RING3 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_WRITABLE,
                  GDT_GRAN_64BIT);

    // 0x20: User Mode Data Segment (Base/Limit ignored)
    gdt_set_entry(4, 0, 0,
                  base_access | GDT_ACCESS_RING3 | GDT_ACCESS_WRITABLE,
                  0);

    // Perform far reload of selectors to apply new descriptors cleanly
    gdt_flush(&gdt_ptr);
}

/* Set standard 8-byte GDT entry (For Code and Data segments) */
void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access       = access;
}

/* Set extended 16-byte GDT entry (Consumes 2 sequential slots for 64-bit System TSS tracking) */
void gdt_set_tss_entry(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Fill first standard 8-byte GDT slot
    gdt_set_entry(num, (uint32_t) base, limit, access, gran);

    // Cast the next contiguous slot as an extended address container
    uint32_t* base_highest = (uint32_t*) &gdt[num + 1];

    // Store upper 32 bits of the absolute 64-bit pointer address and clear reserved bits
    base_highest[0] = (uint32_t)(base >> 32);
    base_highest[1] = 0;
}
