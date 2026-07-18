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

#include <stdarg.h>
#include <stdint.h>

#include "sysmods/devices.h"
#include "sysmods/interface.h"

#define DEVICE_ID 1

#define PORT 0x3F8 // COM1

static kernel_api_t* k_api;
static output_dev_t* dev;

static char buffer[256];

static inline int is_uart_transmit_empty() {
    return k_api->inb(PORT + 5) & 0x20;
}

static inline int is_uart_received() {
    return k_api->inb(PORT + 5) & 1; // Bit 0 is "Data Ready"
}

static inline char uart_getc() {
    while (is_uart_received() == 0);
    return k_api->inb(PORT);
}

static inline void uart_putc(char c) {
    while (is_uart_transmit_empty() == 0);
    k_api->outb(PORT, c);
}

static inline void uart_print(const char* data) {
    while (*data) {
        if (*data == '\n') {
            uart_putc('\r');
        }

        uart_putc(*data++);
    }
}

static inline void uart_vprintf(const char* format, va_list args) {
    int len = k_api->vsnprintf(buffer, sizeof(buffer), format, args);
    if (len > 0) uart_print(buffer);
}

void uart_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    uart_vprintf(format, args);
    va_end(args);
}

int init_uart(kernel_api_t* api, uint64_t base_addr) {
    k_api = api;

    api->outb(PORT + 1, 0x00);    // Disable interrupts
    api->outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    api->outb(PORT + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
    api->outb(PORT + 1, 0x00);    //                  (hi byte)
    api->outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    api->outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    api->outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    dev = k_api->kmalloc(sizeof(output_dev_t));
    dev->id = DEVICE_ID;
    dev->printf = (void*)((uint64_t) uart_printf + base_addr);

    api->register_device(DEV_OUTPUT, (void*) dev);

    return 0;
}

void exit_uart() {
    // Disable UART hardware so it doesn't fire interrupts or send noise
    k_api->outb(PORT + 1, 0x00); // Disable interrupts
    k_api->outb(PORT + 4, 0x00); // Disable RTS/DSR/IRQ

    k_api->unregister_device(DEV_OUTPUT, (void*) dev);

    // This must be at the end, else you free the memory holding
    // this very function.
    k_api->kfree(dev);
}

SYSMOD_ENTRY sysmod_t test_module_entry = {
    .name = "UART",
    .init_offset = (uint64_t) init_uart,
    .exit_offset = (uint64_t) exit_uart
};
