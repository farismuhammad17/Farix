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

#include "klib/string.h"

#include "hal.h"

#include "gdt.h"

#include "memory/heap.h"
#include "memory/vmm.h"

#include "include/tss.h"

TSSEntry tss_entry;

void load_tss();

void init_tss(uint32_t idx, uint32_t kss, uint64_t krsp) {
    uint64_t base = (uint64_t) PHYSICAL_TO_VIRTUAL(&tss_entry);
    uint32_t limit = sizeof(TSSEntry) - 1;

    memset(&tss_entry, 0, sizeof(TSSEntry));

    tss_entry.rsp0 = krsp;

    // Set the I/O map base to the size of the structure to effectively disable it
    tss_entry.iomap_base = sizeof(TSSEntry);

    // Write the 16-byte double-slot GDT entry for the TSS
    gdt_set_tss_entry(idx, base, limit, GDT_TSS_ACCESS_FLAGS, GDT_TSS_GRAN_FLAGS);

    load_tss();
}

/* Changes RSP0 to given stack frame for handling Ring 3 -> Ring 0 transitions */
void set_kernel_stack(uint64_t stack) {
    tss_entry.rsp0 = stack;
}
