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

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>
#include <stdint.h>

#define WIDTH   80
#define HEIGHT  25
#define MEMORY  0xB8000

#define INDENT_LEN 4

#define MAX_TERMINAL_LINE_HISTORY_LEN 200
#define MAX_TERMINAL_CMD_HISTORY_LEN  50

#define KEY_UP    0x11
#define KEY_DOWN  0x12

extern size_t    cursor_x;
extern size_t    cursor_y;
extern uint8_t   terminal_color;
extern uint16_t* terminal_buffer;

enum vga_color {
	VGA_COLOR_BLACK         = 0,
	VGA_COLOR_BLUE          = 1,
	VGA_COLOR_GREEN         = 2,
	VGA_COLOR_CYAN          = 3,
	VGA_COLOR_RED           = 4,
	VGA_COLOR_MAGENTA       = 5,
	VGA_COLOR_BROWN         = 6,
	VGA_COLOR_LIGHT_GREY    = 7,
	VGA_COLOR_DARK_GREY     = 8,
	VGA_COLOR_LIGHT_BLUE    = 9,
	VGA_COLOR_LIGHT_GREEN   = 10,
	VGA_COLOR_LIGHT_CYAN    = 11,
	VGA_COLOR_LIGHT_RED     = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN   = 14,
	VGA_COLOR_WHITE         = 15,
};

struct TerminalCmd {
    const char*  command;
    TerminalCmd* next;
    TerminalCmd* prev;
};

struct TerminalLine {
    uint16_t data[WIDTH]; // Store the characters AND the colors
    TerminalLine* next;
    TerminalLine* prev;
};

inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}
inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

void init_terminal();

void update_cursor(size_t x, size_t y);
void terminal_clear();

void terminal_scroll();
void save_line_to_history (uint16_t* line_data);
void save_cmd_to_history  (const char* command);
void cmd_history_up   ();
void cmd_history_down ();

void echo_at   (char c, uint8_t color, size_t x, size_t y);
void echo_char (char c);

bool handle_special_chars(char c);

#endif
