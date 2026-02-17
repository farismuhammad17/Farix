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

#include "../include/terminal.h"

size_t    cursor_x = 0;
size_t    cursor_y = 0;
uint8_t   terminal_color = 0x07;
uint16_t* terminal_buffer = (uint16_t*) MEMORY;

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void init_terminal() {
	cursor_x = 0;
	cursor_y = 0;

	update_cursor(0, 0);

	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
		const size_t index = y * WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void update_cursor(size_t x, size_t y) {
    uint16_t pos = y * WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void echo_at(char c, uint8_t color, size_t x, size_t y) {
	terminal_buffer[y * WIDTH + x] = vga_entry(c, color);
}

void echo_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        }
        else if (cursor_y > 0) {
            cursor_y--;

            cursor_x = WIDTH - 1;
            while (cursor_x > 0) {
                uint16_t entry = terminal_buffer[cursor_y * WIDTH + cursor_x];
                unsigned char char_at_pos = (unsigned char)(entry & 0xFF);

                if (char_at_pos != ' ') {
                    if (cursor_x < WIDTH - 1) cursor_x++;
                    break;
                }

                cursor_x--;
            }
        }

        echo_at(' ', terminal_color, cursor_x, cursor_y);
    }
    else {
        echo_at(c, terminal_color, cursor_x, cursor_y);
        if (++cursor_x == WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    update_cursor(cursor_x, cursor_y);

    if (cursor_y >= HEIGHT) {
        cursor_y = 0;
    }
}

void echo(const char* data) {
    size_t size = strlen(data);
	for (size_t i = 0; i < size; i++) {
	    echo_char(data[i]);
	}
	echo_char('\n');
}

void echo(const char* data, char end) {
    size_t size = strlen(data);
	for (size_t i = 0; i < size; i++) {
	    echo_char(data[i]);
	}
	echo_char(end);
}
