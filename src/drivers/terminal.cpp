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

#include <stdlib.h>
#include <string.h>

#include "architecture/io.h"
#include "process/task.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "shell/shell.h"

#include "drivers/terminal.h"

size_t        cursor_x = 0;
size_t        cursor_y = 0;
uint8_t       terminal_color  = 0x07;
uint16_t*     terminal_buffer = (uint16_t*) MEMORY;

TerminalCmd*  cmd_current_line  = nullptr;
TerminalCmd*  cmd_history_head  = nullptr;
TerminalCmd*  cmd_history_tail  = nullptr;
int           cmd_history_count = 0;

TerminalLine* line_history_head    = nullptr;
TerminalLine* line_history_tail    = nullptr;
int           line_history_count   = 0;
int           scroll_offset        = 0;

bool          is_scrolling = false;

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

void terminal_clear() {
    uint16_t* vga_buffer = (uint16_t*) 0xB8000;
    uint16_t empty_char = ' ' | (terminal_color << 8);

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        vga_buffer[i] = empty_char;
    }

    cursor_x = 0;
    cursor_y = 0;
    update_cursor(0, 0);
}

void refresh_terminal_view() {
    is_scrolling = scroll_offset != 0;

    TerminalLine* temp = line_history_tail;
    for (int i = 0; i < scroll_offset; i++) {
        if (temp->prev) temp = temp->prev;
    }

    // Copy history directly into the screen buffer
    for (int y = HEIGHT - 1; y >= 0; y--) {
        if (temp) {
            for (int x = 0; x < WIDTH; x++) {
                terminal_buffer[y * WIDTH + x] = temp->data[x];
            }
            temp = temp->prev; // Walk backwards
        } else {
            // Fill top lines with spaces if not enough history
            for (int x = 0; x < WIDTH; x++) {
                terminal_buffer[y * WIDTH + x] = vga_entry(' ', terminal_color);
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
        terminal_buffer[(HEIGHT - 1) * WIDTH + x] = vga_entry(' ', terminal_color);
    }

    cursor_y = HEIGHT - 1;
    update_cursor(cursor_x, cursor_y);
}

void save_line_to_history(uint16_t* line_data) {
    TerminalLine* newNode = new TerminalLine();

    for(int i = 0; i < WIDTH; i++) {
        newNode->data[i] = line_data[i];
    }

    newNode->next = nullptr;
    newNode->prev = line_history_tail;

    if (line_history_tail) {
        line_history_tail->next = newNode;
    } else {
        line_history_head = newNode;
    }

    line_history_tail = newNode;
    line_history_count++;

    if (line_history_count > MAX_TERMINAL_LINE_HISTORY_LEN) {
        TerminalLine* toDelete = line_history_head;
        line_history_head = line_history_head->next;

        if (line_history_head != nullptr) {
            line_history_head->prev = nullptr;
        }

        delete toDelete;
        line_history_count--;
    }
}

void update_line_history_tail(uint16_t* line_data) {
    for(int i = 0; i < WIDTH; i++) {
        line_history_tail->data[i] = line_data[i];
    }
}

void scroll_up() {
    if (!line_history_tail) return;

    scroll_offset++;
    if (scroll_offset > line_history_count) scroll_offset = line_history_count;

    refresh_terminal_view();
}

void scroll_down() {
    scroll_offset--;
    if (scroll_offset < 0) scroll_offset = 0;

    refresh_terminal_view();
}

void save_cmd_to_history(const char* command) {
    TerminalCmd* newNode = new TerminalCmd();

    newNode->command = strdup(command);
    newNode->next    = nullptr;
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

        if (cmd_history_head != nullptr) {
            cmd_history_head->prev = nullptr;
        }

        free((void*) toDelete->command);
        delete toDelete;

        cmd_history_count--;
    }
}

void cmd_history_up() {
    if (cmd_history_head == nullptr) return;

    if (cmd_current_line == nullptr) {
        cmd_current_line = cmd_history_tail;
    } else if (cmd_current_line->prev) {
        cmd_current_line = cmd_current_line->prev;
    }

    while (cursor_x > shell_directory.size() + 2) {
        echo_char('\b');
    }

    printf("%s", cmd_current_line->command);
    shell_buffer = cmd_current_line->command;
}

void cmd_history_down() {
    if (cmd_history_head == nullptr || cmd_current_line == nullptr) return;

    cmd_current_line = cmd_current_line->next;

    if (cmd_current_line == nullptr) cmd_current_line = cmd_history_tail;
    else if (cmd_current_line->next) cmd_current_line = cmd_current_line->next;

    while (cursor_x > shell_directory.size() + 2) {
        echo_char('\b');
    }

    if (cmd_current_line != nullptr) {
        printf("%s", cmd_current_line->command);
        shell_buffer = cmd_current_line->command;
    } else {
        // We went past the most recent command, clear the buffer
        shell_buffer = "";
    }
}

void echo_at(char c, uint8_t color, size_t x, size_t y) {
    terminal_buffer[y * WIDTH + x] = vga_entry(c, color);
}

void echo_char(char c) {
    if (is_scrolling) {
        scroll_offset = 0;
        refresh_terminal_view();
    }

    if (!handle_special_chars(c)) {
        echo_at(c, terminal_color, cursor_x, cursor_y);

        update_line_history_tail(&terminal_buffer[cursor_y * WIDTH]);

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

bool handle_special_chars(char c) {
    if (c == '\n') {
        save_line_to_history(&terminal_buffer[cursor_y * WIDTH]);

        cursor_x = 0;
        cursor_y++;

        return true;
    }

    else if (c == '\b') {
        size_t prompt_len = 2;

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
    }

    else if (c == '\t') {
        cursor_x += INDENT_LEN - cursor_x % INDENT_LEN;

        if (cursor_x >= WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }

        return true;
    }

    else if (c == KEY_UP) {
        cmd_history_up();
        return true;
    }

    else if (c == KEY_DOWN) {
        cmd_history_down();
        return true;
    }

    return false;
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
