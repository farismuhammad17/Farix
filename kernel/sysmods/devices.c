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

#include "drivers/terminal.h"

#include "cpu/timer.h"
#include "drivers/input.h"
#include "drivers/output.h"
#include "drivers/storage.h"
#include "drivers/vfs.h"

#include "sysmods/devices.h"

// Unit drivers
timer_dev_t*   timer_dev      = NULL;
storage_dev_t* storage_dev    = NULL;
vfs_driver_t*  vfs            = NULL;

// Chained drivers
input_dev_t*  input_dev_head  = NULL;
output_dev_t* output_dev_head = NULL;

void register_device(driver_type_t type, void* device) {
    void** head_ptr = NULL;

    switch (type) {
        case DRV_TIMER   : timer_dev = (timer_dev_t*) device;     return;
        case DRV_STORAGE : storage_dev = (storage_dev_t*) device; return;
        case DRV_VFS     : vfs = (vfs_driver_t*) device;          return;

        case DRV_INPUT  : head_ptr = (void**) &input_dev_head;  break;
        case DRV_OUTPUT : head_ptr = (void**) &output_dev_head; break;

        default:
            err_printf("Unknown device type: %d", type);
            return;
    }

    *(void**) device = *head_ptr;
    *head_ptr = device;
}

void unregister_device(driver_type_t type, void* device) {
    void** head_ptr = NULL;

    switch (type) {
        case DRV_TIMER   : timer_dev   = NULL; return;
        case DRV_STORAGE : storage_dev = NULL; return;
        case DRV_VFS     : vfs         = NULL; return;

        case DRV_INPUT  : head_ptr = (void**) &input_dev_head;  break;
        case DRV_OUTPUT : head_ptr = (void**) &output_dev_head; break;

        default:
            err_printf("Unknown device type: %d", type);
            return;
    }

    void** curr = head_ptr;
    while (*curr && *curr != device) {
        curr = (void**) *curr;
    }
    if (*curr) {
        *curr = *(void**) *curr;
    }
}

/* Required for system modules to dynamically get other devices */
void* get_device(driver_type_t dev_type) {
    switch (dev_type) {
        case DRV_OUTPUT  : return output_dev_head;
        case DRV_INPUT   : return input_dev_head;
        case DRV_TIMER   : return timer_dev;
        case DRV_STORAGE : return storage_dev;
        case DRV_VFS     : return vfs;

        default: return NULL;
    }
}
