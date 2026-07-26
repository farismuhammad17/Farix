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

#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#include "sysmods/devices.h"

typedef struct input_dev_t {
    struct input_dev_t* next;
    uint8_t id;
    dev_type_t type;

    void (*on_event)(void* data);
} input_dev_t;

extern input_dev_t* input_dev_head;

#endif
