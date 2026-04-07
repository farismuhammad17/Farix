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

#include "drivers/terminal.h"

// For avoiding unused argument compiler warnings cleanly
// TODO: Remove after these empty functions are done
#define UNUSED_ARG __attribute__((unused))

void init_terminal() {}

void echo_char(uint16_t c) {
    uart_print((char*) &c);
}

void echo_raw(const char* data, UNUSED_ARG size_t len) {
    uart_print(data);
}

void terminal_clear() {}
