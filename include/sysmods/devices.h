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

#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h> // TODO REM When timer device is moved out

#define UART_DEV_ID          1
#define PIT_DEV_ID           2

// DEVELOPER NOTE:
// Every device struct MUST have a next pointer
// to itself placed at the start of the struct.

typedef enum {
    DEV_OUTPUT,
    DEV_INPUT,
    DEV_TIMER
} dev_type_t;

typedef struct timer_dev_t {
    struct timer_dev_t* next;
    uint8_t id;

    uint64_t (*get_timer_uptime_microseconds)();
    void (*timer_stall)(uint64_t microseconds);

    void (*interrupt_handler)();
} timer_dev_t;

extern timer_dev_t* timer_dev_head;

void register_device(dev_type_t type, void* device);
void unregister_device(dev_type_t type, void* device);

#endif
