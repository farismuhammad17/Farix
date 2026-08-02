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

#include "cpu/pci.h"
#include "cpu/timer.h"

#include "sysmods/devices.h"

#define SYSMOD_HEADER __attribute__((section(".sysmod_header"), used))

// Kernel System Module internal macros
#define SYSMOD_TO_KERNEL(s)    ((uint64_t)(s) + base_addr)
#define SYS_ICALL(func, ...)   ((__typeof__(func)*) SYSMOD_TO_KERNEL(func))(__VA_ARGS__)

typedef struct {
    // Output
    void (*printf)(const char* format, ...);
    void (*err_printf)(const char* format, ...);
    void (*err_print)(const char* data);
    int (*vsnprintf)(char* str, size_t size, const char* format, va_list args);

    // Assembly
    // TODO: Since these are just plain assembly, you should just be able to
    // include hal.h inside the system module and use it directly. This would
    // save us 6 * 8 bytes = 48 bytes. Obviously, that's just 48 bytes in total
    // from the ENTIRE kernel, since this struct is to only exist once. Regardless,
    // 48 bytes is 48 bytes.
    // It is obvious that I *can* remove these, but, when testing in a branch, I
    // ran into errors, and I am already into this commit here, so I decided to not
    // spend me time debugging another error. For now, these stay, will be removed
    // next commit, hopefully, if they can be removed that is.
    // The error was that the timer seemed to not be functioning in some manner,
    // where the interrupt was failing. I will debug this fact later.
    void (*outb)(uint16_t port, uint8_t val);
    void (*outw)(uint16_t port, uint16_t val);
    void (*outl)(uint16_t port, uint32_t val);
    uint8_t (*inb)(uint16_t port);
    uint16_t (*inw)(uint16_t port);
    uint32_t (*inl)(uint16_t port);

    // Memory Mangement (PMM, VMM, Heap)
    void* (*pmm_alloc_page)();
    void* (*pmm_alloc_pages)(size_t length);
    void (*vmm_map_page)(uint64_t* pd_phys, void* phys, void* virt, uint64_t flags);
    uint64_t* (*vmm_get_current_directory)();
    void* (*kmalloc)(size_t size);
    void (*kfree)(void* ptr);
    void* (*memset)(void* s, int c, size_t n);
    void* (*memcpy)(void* restrict dest, const void* restrict src, size_t n);

    // IRQ
    void (*register_interrupt)(uint8_t, void*);
    void (*unregister_interrupt)(uint8_t);
    void (*irq_send_eoi)();
    void (*irq_unmask)(uint8_t pin, uint8_t vector);

    // Tasks
    void (*schedule)();

    // PCI
    uint32_t (*pci_read)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
    void (*pci_write)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);
    pci_device_t* pci_devices;

    // Device controllers
    void (*register_device)(dev_type_t dev_type, void* device);
    void (*unregister_device)(dev_type_t dev_type, void* device);

    // Other devices
    timer_dev_t* (*get_timer_dev)();
} kernel_api_t;

typedef struct {
    char name[16];
    int (*init)(kernel_api_t* api, uint64_t base_addr);
    int (*exit)(); // 8 bytes = 32 bytes
} __attribute__((packed)) sysmod_t;

#endif
