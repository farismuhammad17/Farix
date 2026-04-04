/*
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include "arch/stubs.h"

#include "drivers/uart.h"

#define PORT 0x3F8          // COM1

void init_uart() {
    outb(PORT + 1, 0x00);    // Disable interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
    outb(PORT + 1, 0x00);    //                  (hi byte)
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_uart_transmit_empty() {
    return inb(PORT + 5) & 0x20;
}

int is_uart_received() {
    return inb(PORT + 5) & 1; // Bit 0 is "Data Ready"
}

char uart_getc() {
    while (is_uart_received() == 0);
    return inb(PORT);
}

void uart_putc(char c) {
    while (is_uart_transmit_empty() == 0);
    outb(PORT, c);
}
