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

#ifndef SYSMODS_INTERFACE_H
#define SYSMODS_INTERFACE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "sysmods/devices.h"

#define SYSMOD_ENTRY __attribute__((section(".sysmod_header"), used))

typedef struct {
    char name[16];        // 16 bytes
    uint64_t init_offset; // 8 bytes
    uint64_t exit_offset; // 8 bytes = 32 bytes
} __attribute__((packed)) sysmod_t;

typedef struct {
    void (*printf)(const char* format, ...);
    int (*vsnprintf)(char* str, size_t size, const char* format, va_list args);

    void (*outb)(uint16_t port, uint8_t val);
    void (*outw)(uint16_t port, uint16_t val);
    void (*outl)(uint16_t port, uint32_t val);
    uint8_t (*inb)(uint16_t port);
    uint16_t (*inw)(uint16_t port);
    uint32_t (*inl)(uint16_t port);

    void* (*kmalloc)(size_t size);
    void (*kfree)(void* ptr);

    void (*register_device)(dev_type_t dev_type, void* device);
    void (*unregister_device)(dev_type_t dev_type, void* device);
} kernel_api_t;

#endif
