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

#include <stdbool.h>

#include "klib/string.h"

#include "hal.h"

#include "cpu/pci.h"
#include "drivers/storage.h"
#include "drivers/terminal.h"
#include "drivers/vfs.h"
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

static inline const char* find_storage_device() {
    bool has_ata = false;

    for (int i = 0; i < PCI_MAX_DEVICES; i++) {
        pci_device_t* dev = &pci_devices[i];

        if (dev->class_code == PCI_CLASS_CODE_STORAGE) {
            // Check for AHCI first
            if (dev->subclass == PCI_AHCI_SUBCLASS && dev->progif == 0x01) {
                return "AHCI";
            }

            // Keep track of an ATA device in case we don't find AHCI
            if (dev->subclass == PCI_ATA_SUBCLASS) {
                has_ata = true;
            }
        }
    }

    // Fallback: only if no AHCI was found
    if (likely(has_ata)) {
        return "ATA";
    } else {
        err_print("find_storage_device: No supported storage device found");
    }
}

void initboot() {
    // --- Timer Device ---

    initboot_timer();

    // --- Initboot blob ---

    if (unlikely(!initboot_blob)) {
        err_print("initboot: Initboot Blob not found");
        while (1) system_halt(); // Critical failure, causes unpredictable errors
    }

    uint8_t* base_ptr = (uint8_t*) initboot_blob;

    // --- Storage Driver ---

    const char* st_dev = find_storage_device();
    initboot_entry_t* st_entry = find_initboot_module(st_dev);

    if (unlikely(!st_entry)) {
        err_printf("initboot: Storage device '%s' not found in blob", st_dev);
        while (1) system_halt();
    }

    load_sysmod_raw(
        (void*)(base_ptr + st_entry->offset),
        (size_t) st_entry->size
    );

    if (unlikely(!storage_dev)) {
        err_print("initboot: Storage device not initialised");
        while (1) system_halt();
    }

    // --- VFS (FAT32) Driver ---

    initboot_entry_t* vfs_entry = find_initboot_module("FAT32");

    if (unlikely(!vfs_entry)) {
        err_print("initboot: VFS device 'FAT32' not found in blob");
        while (1) system_halt();
    }

    load_sysmod_raw(
        (void*)(base_ptr + vfs_entry->offset),
        (size_t) vfs_entry->size
    );

    if (unlikely(!vfs)) {
        err_print("initboot: VFS not initialised");
        while (1) system_halt();
    }
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
