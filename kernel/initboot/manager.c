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

#include "klib/string.h"

#include "hal.h"

#include "drivers/terminal.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "sysmods/loader.h"

#include "initboot.h"

// From linker.ld
extern uint64_t _initboot_start;
extern uint64_t _initboot_end;

void* initboot_blob = NULL;

// Helper to find a specific module inside the blob by name
static initboot_entry_t* find_initboot_module(const char* target_name) {
    uint8_t* base_ptr    = (uint8_t*)   initboot_blob;
    uint32_t header_size = *(uint32_t*) base_ptr;
    uint32_t num_modules = (header_size - 4) / sizeof(initboot_entry_t);

    initboot_entry_t* entries = (initboot_entry_t*)(base_ptr + 4);

    for (uint32_t i = 0; i < num_modules; i++) {
        if (strncmp(entries[i].name, target_name, 32) == 0) {
            return &entries[i];
        }
    }

    return NULL;
}

void initboot() {
    if (unlikely(!initboot_blob)) {
        err_print("initboot: Initboot Blob not found");
        while (1) system_halt(); // Critical failure, causes unpredictable errors
    }

    // --- Storage Driver ---

    // TODO IMP: Only considers AHCI, adding ATA is trivial.

    uint8_t* base_ptr = (uint8_t*) initboot_blob;

    initboot_entry_t* entry = find_initboot_module("AHCI");

    if (unlikely(!entry)) {
        err_print("initboot: AHCI module not found in blob");
        while (1) system_halt();
    }

    void*  binary_buffer = (void*)(base_ptr + entry->offset);
    size_t binary_size   = (size_t) entry->size;

    load_sysmod_raw(binary_buffer, binary_size);

    // --- Timer Device ---

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
