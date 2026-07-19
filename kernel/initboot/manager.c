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

#include "memory/pmm.h"
#include "memory/vmm.h"

#include "sysmods/devices.h"

#include "initboot.h"

// From linker.ld
extern uint64_t _initboot_start;
extern uint64_t _initboot_end;

void initboot() {
    initboot_timer();
}

void kill_bootstrap() {
    uint64_t start = (uint64_t) &_initboot_start;
    uint64_t end   = (uint64_t) &_initboot_end;

    // Convert to physical addresses
    uint64_t phys_start = VIRTUAL_TO_PHYSICAL(start);
    uint64_t phys_end   = VIRTUAL_TO_PHYSICAL(end);

    // Free every page in the range
    for (uint64_t addr = phys_start; addr < phys_end; addr += PAGE_SIZE) {
        pmm_free_page((void*) addr);
    }
}
