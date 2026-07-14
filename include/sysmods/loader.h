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

#ifndef SYSMODS_LOADER_H
#define SYSMODS_LOADER_H

#include "sysmods/interface.h"

#define MAX_LOADED_MODULES 16

typedef struct {
    sysmod_t* interface;
    void* base_address;     // Where the raw binary was loaded in RAM
    size_t size;          // Memory footprint size
    int is_active;
} loaded_sysmod_t;

extern loaded_sysmod_t sysmods_registry[MAX_LOADED_MODULES];

int load_sysmod(const char* path);

int load_sysmod_raw(void* raw_binary_buffer, size_t binary_size);
int unload_sysmod(int slot_id);

#endif
