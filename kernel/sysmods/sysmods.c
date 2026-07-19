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

#include <stddef.h>

#include "klib/stdio.h"

#include "hal.h"

#include "cpu/ints.h"
#include "cpu/irq.h"
#include "drivers/terminal.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "sysmods/devices.h"
#include "sysmods/interface.h"
#include "sysmods/loader.h"

loaded_sysmod_t sysmods_registry[MAX_LOADED_MODULES];

kernel_api_t sysmod_kernel_api = {
    .printf = printf,
    .err_printf = err_printf,
    .err_print = err_print,
    .vsnprintf = vsnprintf,

    .outb = outb,
    .outw = outw,
    .outl = outl,
    .inb = inb,
    .inw = inw,
    .inl = inl,

    .kmalloc = kmalloc,
    .kfree = kfree,

    .register_interrupt = register_interrupt,
    .unregister_interrupt = unregister_interrupt,
    .irq_send_eoi = irq_send_eoi,

    .schedule = schedule,

    .register_device = register_device,
    .unregister_device = unregister_device,
};

static int find_free_module_slot() {
    int slot = -1;
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        if (unlikely(!sysmods_registry[i].is_active)) {
            return i;
        }
    }

    return -1;
}

int load_sysmod(const char* path) {
    File* file_obj = fs_get(path);
    if (unlikely(!file_obj || file_obj->size == 0)) {
        err_printf("load_sysmod: File %s not found or empty", path);
        return -1;
    }

    uint8_t* buffer = (uint8_t*) kmalloc(file_obj->size);
    if (unlikely(!buffer)) {
        err_print("load_sysmod: Out of memory");
        return -1;
    }

    if (unlikely(!fs_read(path, buffer, file_obj->size, 0))) {
        err_print("load_sysmod: Failed to read file data");
        kfree(buffer);
        return -1;
    }

    // Pass the raw buffer into your tracking registry assignment loop
    int slot = load_sysmod_raw((void*) buffer, file_obj->size);
    if (unlikely(slot == -1)) {
        err_print("load_sysmod: System module failed or register full");
        kfree(buffer);
        return -1;
    }

    return slot;
}

int load_sysmod_raw(void* raw_binary_buffer, size_t binary_size) {
    sysmod_t* mod = (sysmod_t*) raw_binary_buffer;
    uint64_t base = (uint64_t)  raw_binary_buffer;

    int slot = find_free_module_slot();
    if (unlikely(slot == -1)) return -1;

    sysmods_registry[slot].interface = mod;
    sysmods_registry[slot].base_address = raw_binary_buffer;
    sysmods_registry[slot].size = binary_size;
    sysmods_registry[slot].is_active = 1;

    if (likely(mod->init_offset)) {
        int (*runtime_init)(kernel_api_t*, uint64_t) = (int(*)(kernel_api_t*, uint64_t))(base + mod->init_offset);

        int result = runtime_init(&sysmod_kernel_api, base);
        if (unlikely(result != 0)) {
            sysmods_registry[slot].is_active = 0;
            err_printf("load_sysmod_raw: Module returned %d", result);
            return -1;
        }
    }

    return slot;
}

int unload_sysmod(int slot_id) {
    if (unlikely(slot_id < 0 || slot_id >= MAX_LOADED_MODULES || !sysmods_registry[slot_id].is_active)) {
        return -1;
    }

    loaded_sysmod_t* mod_track = &sysmods_registry[slot_id];
    uint64_t base = (uint64_t) mod_track->base_address;

    // Fire the module's cleanup function using the relative offset
    if (mod_track->interface->exit_offset) {
        void (*runtime_exit)(void) = (void(*)(void))(base + mod_track->interface->exit_offset);
        runtime_exit();
    }

    // Clear the registry tracking slot
    mod_track->is_active = 0;
    mod_track->interface = NULL;

    // Now that loading and unloading routines are working predictably,
    // freeing the raw buffer block here is perfectly safe to uncomment.
    if (likely(mod_track->base_address)) {
        kfree(mod_track->base_address);
        mod_track->base_address = NULL;
    }

    return 0;
}
