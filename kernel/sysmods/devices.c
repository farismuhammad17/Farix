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
#include "drivers/output.h"

#include "sysmods/devices.h"

output_dev_t* output_dev_head = NULL;
timer_dev_t*  timer_dev       = NULL;

void register_device(dev_type_t type, void* device) {
    void** head_ptr = NULL;

    switch (type) {
        case DEV_OUTPUT : head_ptr = (void**) &output_dev_head; break;

        case DEV_TIMER  : timer_dev = (timer_dev_t*) device; return;

        default:
            err_printf("Unknown device type: %d", type);
            return;
    }

    *(void**) device = *head_ptr;
    *head_ptr = device;
}

void unregister_device(dev_type_t type, void* device) {
    void** head_ptr = NULL;

    switch (type) {
        case DEV_OUTPUT : head_ptr = (void**) &output_dev_head; break;

        case DEV_TIMER  : timer_dev = NULL; return;

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
