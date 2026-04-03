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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WIDTH  80
#define HEIGHT 25
#define MEMORY 0xB8000
#define VGA_MEMORY (uint16_t*) 0xB8000

#define ESC_CODE 0x1B

#define INDENT_LEN 4

#define MAX_TERMINAL_LINE_HISTORY_LEN 100
#define MAX_TERMINAL_CMD_HISTORY_LEN  50
#define MAX_SPECIAL_CHAR_LEN          32

extern size_t    cursor_x;
extern size_t    cursor_y;
extern uint8_t   terminal_color;
extern uint16_t* terminal_buffer;

#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_WHITE         15

typedef struct TerminalCmd {
    const char* command;
    struct TerminalCmd* next;
    struct TerminalCmd* prev;
} TerminalCmd;

typedef struct TerminalLine {
    uint16_t data[WIDTH]; // Store the characters AND the colors
    struct TerminalLine* next;
    struct TerminalLine* prev;
} TerminalLine;

inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
	return fg | bg << 4;
}
inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

void init_terminal();

void update_cursor(size_t x, size_t y);
void terminal_clear();
void terminal_change_color(uint8_t color);
void refresh_terminal_view();
void new_line();
void new_line_n(size_t n);

void save_line_to_history(uint16_t* line_data);
void update_line_history_tail(uint16_t* line_data);
void scroll_up();
void scroll_down();

void save_cmd_to_history(const char* command);
void cmd_history_up();
void cmd_history_down();

void echo_at   (char c, uint8_t color, size_t x, size_t y);
void echo_char (uint16_t c);
void echo_raw  (const char* data, size_t len);
void t_print   (char* text);

bool handle_special_chars(uint16_t c);
void handle_mouse();
void handle_ansi_char(uint16_t c);

#endif
