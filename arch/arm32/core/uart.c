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

#include "drivers/uart.h"

#define UART_BASE 0x3F201000
#define UART_DR   ((volatile unsigned int*)(UART_BASE + 0x00)) // Data Register
#define UART_FR   ((volatile unsigned int*)(UART_BASE + 0x18)) // Flag Register
#define UART_IBRD ((volatile unsigned int*)(UART_BASE + 0x24)) // Integer Baud Rate
#define UART_FBRD ((volatile unsigned int*)(UART_BASE + 0x28)) // Fractional Baud Rate
#define UART_LCRH ((volatile unsigned int*)(UART_BASE + 0x2C)) // Line Control
#define UART_CR   ((volatile unsigned int*)(UART_BASE + 0x30)) // Control Register

// Raspberry Pi 2/3 GPIO Base (0x20200000 for Pi 1/Zero)
#define GPIO_BASE 0x3F200000
#define GPPUD     ((volatile unsigned int*)(GPIO_BASE + 0x94))
#define GPPUDCLK0 ((volatile unsigned int*)(GPIO_BASE + 0x98))

// This seems to be required by hardware, but idk how
// static void delay(int count) {
//     while(count--) { asm volatile("nop"); }
// }

void init_uart() {
    *UART_CR = 0;
    *UART_LCRH = (1 << 4) | (1 << 5) | (1 << 6);
    *UART_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

int is_uart_transmit_empty() {
    // On PL011, Bit 5 of the Flag Register is "Transmit FIFO Full"
    // We want to return true (1) if it's NOT full
    return !(*UART_FR & (1 << 5));
}

int is_uart_received() {
    // Bit 4 is "Receive FIFO Empty"
    return !(*UART_FR & (1 << 4));
}

char uart_getc() {
    while (!is_uart_received());
    return (char)(*UART_DR);
}

void uart_putc(char c) {
    while (!is_uart_transmit_empty());
    *UART_DR = c;
}
