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

#ifndef UART_H
#define UART_H

#include <stdio.h>
#include <stdarg.h>

void init_uart();

int  is_uart_transmit_empty();
int  is_uart_received();
char uart_getc();
void uart_putc(char c);

static inline void uart_print(const char* data) {
    while (*data) {
        if (*data == '\n')
            uart_putc('\r');
        uart_putc(*data++);
    }
}

static inline void uart_printf(const char* format, ...) {
    char buffer[64];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0) uart_print(buffer);
}

#endif
