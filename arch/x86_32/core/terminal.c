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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch/stubs.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "memory/heap.h"
#include "memory/vmm.h"
#include "process/task.h"
#include "shell/shell.h"

#include "drivers/terminal.h"

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

size_t        cursor_x = 0;
size_t        cursor_y = 0;
uint8_t       terminal_color  = 0;
uint16_t*     terminal_buffer = (uint16_t*) PHYSICAL_TO_VIRTUAL(MEMORY);

TerminalCmd*  cmd_current_line  = NULL;
TerminalCmd*  cmd_history_head  = NULL;
TerminalCmd*  cmd_history_tail  = NULL;
int           cmd_history_count = 0;

uint16_t* line_history[MAX_TERMINAL_LINE_HISTORY_LEN] = {0};
size_t    history_write_index = 0;
size_t    history_total_count = 0;
int       scroll_offset       = 0;

bool is_scrolling = false;

bool special_char_mode = false;
char special_char_buffer[MAX_SPECIAL_CHAR_LEN] = "";

void init_terminal() {
	update_cursor(0, 0);

	terminal_color = terminal_color_entry(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
		const size_t index = y * WIDTH + x;
			terminal_buffer[index] = terminal_entry(' ', terminal_color);
		}
	}

	uint16_t blank_line[WIDTH] = {0};
    save_line_to_history(blank_line);
}

uint8_t terminal_color_entry(uint8_t fg, uint8_t bg) {
	return fg | bg << 4;
}

uint16_t terminal_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

void update_cursor(size_t x, size_t y) {
    uint16_t pos = y * WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void terminal_clear() {
    uint16_t empty_char  = terminal_entry(' ', terminal_color);

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        terminal_buffer[i] = empty_char;
    }

    cursor_x = 0;
    cursor_y = 0;
    update_cursor(0, 0);
}

void terminal_change_color(uint8_t color) {
    terminal_color = color;

    for (size_t i = 0; i < WIDTH * HEIGHT; i++) {
        terminal_buffer[i] = (terminal_buffer[i] & 0x00FF) | ((uint16_t) color << 8);
    }
}

void refresh_terminal_view() {
    is_scrolling = scroll_offset != 0;

    for (int y = 0; y < HEIGHT; y++) {
        int logical_index = (int) history_total_count - scroll_offset - HEIGHT + y + 1;

        if (logical_index >= 0) {
            size_t physical_index;
            if (history_total_count < MAX_TERMINAL_LINE_HISTORY_LEN) {
                physical_index = (size_t) logical_index;
            } else {
                physical_index = (history_write_index + logical_index) % MAX_TERMINAL_LINE_HISTORY_LEN;
            }

            if (line_history[physical_index]) {
                memcpy(&terminal_buffer[y * WIDTH], line_history[physical_index], WIDTH * sizeof(uint16_t));
            }
        } else {
            uint16_t blank = terminal_entry(' ', terminal_color);
            for (int x = 0; x < WIDTH; x++) {
                terminal_buffer[y * WIDTH + x] = blank;
            }
        }
    }
}

void new_line() {
    // Move lines up in terminal_buffer (Physical Memory)
    for (size_t y = 0; y < HEIGHT - 1; y++) {
        for (size_t x = 0; x < WIDTH; x++) {
            terminal_buffer[y * WIDTH + x] = terminal_buffer[(y + 1) * WIDTH + x];
        }
    }
    // Clear bottom line in terminal_buffer
    for (size_t x = 0; x < WIDTH; x++) {
        terminal_buffer[(HEIGHT - 1) * WIDTH + x] = terminal_entry(' ', terminal_color);
    }

    cursor_y = HEIGHT - 1;
    update_cursor(cursor_x, cursor_y);
}

void new_line_n(size_t n) {
    if (n == 0) return;

    // If we need to scroll more than a screen, we still only move
    // a maximum of HEIGHT-1 lines to keep the current "view"
    size_t effective_n = (n >= HEIGHT) ? HEIGHT - 1 : n;

    size_t move_size = (HEIGHT - effective_n) * WIDTH;
    memmove(terminal_buffer,
            terminal_buffer + (effective_n * WIDTH),
            move_size * sizeof(uint16_t));

    // Clear the newly opened area at the bottom
    uint16_t blank = terminal_entry(' ', terminal_color);
    for (size_t i = move_size; i < WIDTH * HEIGHT; i++) {
        terminal_buffer[i] = blank;
    }
}

void save_line_to_history(uint16_t* line_data) {
    // Check if we need to allocate this slot for the first time
    if (line_history[history_write_index] == NULL) {
        line_history[history_write_index] = (uint16_t*) kmalloc(WIDTH * sizeof(uint16_t));
        if (!line_history[history_write_index]) return;
    }

    // Copy the line data into our persistent buffer
    memcpy(line_history[history_write_index], line_data, WIDTH * sizeof(uint16_t));

    // Move the 'write head' forward
    history_write_index = (history_write_index + 1) % MAX_TERMINAL_LINE_HISTORY_LEN;

    // Track how many total lines we actually have stored
    if (history_total_count < MAX_TERMINAL_LINE_HISTORY_LEN) {
        history_total_count++;
    }
}

void update_line_history_current() {
    if (line_history[history_write_index] == NULL) {
        line_history[history_write_index] = (uint16_t*) kmalloc(WIDTH * sizeof(uint16_t));
        if (!line_history[history_write_index]) return;
    }

    memcpy(line_history[history_write_index], &terminal_buffer[cursor_y * WIDTH], WIDTH * sizeof(uint16_t));

    if (history_total_count == 0) history_total_count = 1;
}

void scroll_up() {
    if (history_total_count == 0) return;

    scroll_offset++;

    if (scroll_offset > (int) history_total_count) {
        scroll_offset = (int) history_total_count;
    }

    refresh_terminal_view();
}

void scroll_down() {
    scroll_offset--;

    if (scroll_offset < 0) {
        scroll_offset = 0;
    }

    refresh_terminal_view();
}

void save_cmd_to_history(const char* command) {
    TerminalCmd* newNode = (TerminalCmd*) kmalloc(sizeof(TerminalCmd));
    kmemset(newNode, 0, sizeof(TerminalCmd));

    newNode->command = (const char*) strdup(command);
    newNode->next    = NULL;
    newNode->prev    = cmd_history_tail;

    if (cmd_history_tail) {
        cmd_history_tail->next = newNode;
    } else {
        cmd_history_head = newNode;
    }

    cmd_history_tail = newNode;
    cmd_history_count++;

    if (cmd_history_count > MAX_TERMINAL_CMD_HISTORY_LEN) {
        TerminalCmd* toDelete = cmd_history_head;
        cmd_history_head = cmd_history_head->next;

        if (cmd_history_head != NULL) {
            cmd_history_head->prev = NULL;
        }

        kfree((void*) toDelete->command);
        kfree(toDelete);

        cmd_history_count--;
    }
}

void cmd_history_up() {
    if (cmd_history_head == NULL) return;

    if (cmd_current_line == NULL) {
        cmd_current_line = cmd_history_tail;
    } else if (cmd_current_line->prev) {
        cmd_current_line = cmd_current_line->prev;
    }

    while (cursor_x > strlen(shell_directory) + 2) {
        echo_char('\b');
    }

    printf("%s", cmd_current_line->command);
    strcpy(shell_buffer, cmd_current_line->command);
}

void cmd_history_down() {
    if (cmd_current_line == NULL) return;

    cmd_current_line = cmd_current_line->next;

    while (cursor_x > strlen(shell_directory) + 2) {
        echo_char('\b');
    }

    if (cmd_current_line != NULL) {
        printf("%s", cmd_current_line->command);
        strcpy(shell_buffer, cmd_current_line->command);
    } else {
        shell_buffer[0] = '\0';
    }
}

void echo_at(char c, uint8_t color, size_t x, size_t y) {
    terminal_buffer[y * WIDTH + x] = terminal_entry(c, color);
}

void echo_char(uint16_t c) {
    if (is_scrolling) {
        scroll_offset = 0;
        refresh_terminal_view();
    }

    if (c == ESC_CODE || special_char_mode) {
        special_char_mode = true;
        handle_ansi_char(c);
        return;
    }

    if (!handle_special_chars(c)) {
        echo_at((char) c, terminal_color, cursor_x, cursor_y);
        update_line_history_current();

        if (++cursor_x == WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= HEIGHT) {
        new_line();
    }

    update_cursor(cursor_x, cursor_y);
}

// TODO: echo_raw does not handle ANSI escape codes
void echo_raw(const char* data, size_t len) {
    if (len == 0) return;

    // Pre-calcualte the total lines required by data
    size_t needed_lines = 0;
    size_t tx = cursor_x;
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n' || ++tx >= WIDTH) {
            needed_lines++;
            tx = 0;
        }
    }

    // Early scroll
    size_t lines_available = (HEIGHT - 1) - cursor_y;
    size_t skip_lines = 0;
    if (needed_lines > lines_available) {
        size_t scroll_amount = needed_lines - lines_available;

        if (needed_lines >= HEIGHT) {
            skip_lines = needed_lines - (HEIGHT - 1);
            scroll_amount = HEIGHT - 1;
        }

        new_line_n(scroll_amount);

        if (scroll_amount >= cursor_y) cursor_y = 0;
        else cursor_y -= scroll_amount;
    }

    // Dump to VGA buffer
    size_t current_line = 0;
    tx = cursor_x;

    for (size_t i = 0; i < len; i++) {
        char c = data[i];

        if (current_line < skip_lines) {
            if (c == '\n' || ++tx >= WIDTH) {
                current_line++;
                tx = 0;
            }
            continue;
        }

        if (!handle_special_chars(c)) {
            if (cursor_y < HEIGHT) {
                echo_at(c, terminal_color, cursor_x, cursor_y);
            }

            if (++cursor_x >= WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }
        }
    }

    update_line_history_current();
    update_cursor(cursor_x, cursor_y);
}

// Terminal print:
// Literally changes the VGA buffer directly
// doesn't rely on heap on anything to print
// anything, so is useful for anything that
// needs to be debugged
void t_print(char* text) {
    for (size_t i = 0; i < WIDTH; i++) {
        echo_at(text[i], VGA_COLOR_WHITE, i, 0);
    }
}

bool handle_special_chars(uint16_t c) {
    switch (c) {
        case '\n':
            save_line_to_history(&terminal_buffer[cursor_y * WIDTH]);

            cursor_x = 0;
            cursor_y++;

            return true;

        case '\b':
            size_t prompt_len = strlen(shell_directory) + 2;

            if (cursor_x <= prompt_len) {
                return true;
            }

            if (cursor_x > 0) {
                cursor_x--;
            } else if (cursor_y > 0) {
                // Wrap back to the previous line
                cursor_y--;
                cursor_x = WIDTH - 1;
            }

            echo_at(' ', terminal_color, cursor_x, cursor_y);

            return true;

        case '\t':
            cursor_x += INDENT_LEN - cursor_x % INDENT_LEN;

            if (cursor_x >= WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }

            return true;

        case KEY_UP:
            cmd_history_up();
            return true;

        case KEY_DOWN:
            cmd_history_down();
            return true;

        default: return false;
    }
}

void handle_mouse() {
    while (1) {
        while (buffer_tail != buffer_head) {
            MouseEvent event = mouse_buffer[buffer_tail];

            if (event.scroll != 0) {
                if (event.scroll == 1) {
                    // Scroll UP one line in the terminal
                    scroll_down();
                } else if (event.scroll == -1) {
                    // Scroll DOWN one line in the terminal
                    scroll_up();
                }
            }

            buffer_tail = (buffer_tail + 1) % MOUSE_BUFFER_LEN;
        }
    }
}

void handle_ansi_char(uint16_t c) {
    if (c == ESC_CODE || (c == '[' && special_char_mode)) return;

    if (c == 'm') { // TODO: Only supports foreground, should implement background later, as well as letting ';' be used as a seperator
             if (strcmp(special_char_buffer, "0") == 0)  terminal_color = terminal_color_entry(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "30") == 0) terminal_color = terminal_color_entry(VGA_COLOR_BLACK, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "31") == 0) terminal_color = terminal_color_entry(VGA_COLOR_RED, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "32") == 0) terminal_color = terminal_color_entry(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "33") == 0) terminal_color = terminal_color_entry(VGA_COLOR_BROWN, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "34") == 0) terminal_color = terminal_color_entry(VGA_COLOR_BLUE, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "35") == 0) terminal_color = terminal_color_entry(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "36") == 0) terminal_color = terminal_color_entry(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        else if (strcmp(special_char_buffer, "37") == 0) terminal_color = terminal_color_entry(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        special_char_mode   = false;
        special_char_buffer[0] = '\0';
    } else if (c == 'H' || c == 'f') {
        int separator_idx = -1;
        for (int i = 0; special_char_buffer[i] != '\0'; i++) {
            if (special_char_buffer[i] == ';') {
                separator_idx = i;
                break;
            }
        }

        if (separator_idx != -1) {
            special_char_buffer[separator_idx] = '\0';

            cursor_y = atoi(special_char_buffer) - 1;
            cursor_x = atoi(&special_char_buffer[separator_idx + 1]) - 1;

            update_cursor(cursor_x, cursor_y);
        }
    } else if (c == 'J') {
        switch (special_char_buffer[0]) {
            case '0':
                for (size_t i = cursor_y * WIDTH + cursor_x; i < WIDTH * HEIGHT; i++)
                    terminal_buffer[i] = terminal_entry(' ', terminal_color);
                break;
            case '1':
                for (size_t i = 0; i <= cursor_y * WIDTH + cursor_x; i++)
                    terminal_buffer[i] = terminal_entry(' ', terminal_color);
                break;
            case '2': terminal_clear(); break;
            case '3':
                terminal_clear();
                history_total_count = 0;
                history_write_index = 0;
                scroll_offset = 0;
                break;
        }
    } else if (c == 'K') {
        switch (special_char_buffer[0]) {
            case '0':
                for (size_t i = cursor_y * WIDTH + cursor_x; i < (cursor_y + 1) * WIDTH; i++)
                    terminal_buffer[i] = terminal_entry(' ', terminal_color);
                break;
            case '1':
                for (size_t i = cursor_y * WIDTH; i < cursor_y * WIDTH + cursor_x; i++)
                    terminal_buffer[i] = terminal_entry(' ', terminal_color);
                break;
            case '2':
                for (size_t i = cursor_y * WIDTH; i < (cursor_y + 1) * WIDTH; i++)
                    terminal_buffer[i] = terminal_entry(' ', terminal_color);
                break;
        }
    } else if (c == 'A') { // TODO Clamp the values so that the cursor doesn't fall outside of the terminal
        cursor_y -= atoi(special_char_buffer);
        update_cursor(cursor_x, cursor_y);
    } else if (c == 'B') {
        cursor_y += atoi(special_char_buffer);
        update_cursor(cursor_x, cursor_y);
    } else if (c == 'C') {
        cursor_x += atoi(special_char_buffer);
        update_cursor(cursor_x, cursor_y);
    } else if (c == 'D') {
        cursor_x -= atoi(special_char_buffer);
        update_cursor(cursor_x, cursor_y);
    } else {
        int len = strlen(special_char_buffer);
        if (len < MAX_SPECIAL_CHAR_LEN - 1) {
            special_char_buffer[len] = (char)c;
            special_char_buffer[len + 1] = '\0';
        }
    }
}
