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

#include "architecture/io.h"
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

TerminalLine* line_history_head  = nullptr;
TerminalLine* line_history_tail  = nullptr;
int           line_history_count = 0;

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

void terminal_scroll() {
    save_line_to_history(&terminal_buffer[0]);

    for (size_t y = 0; y < HEIGHT - 1; y++) {
        for (size_t x = 0; x < WIDTH; x++) {
            const size_t current_index = y * WIDTH + x;
            const size_t next_line_index = (y + 1) * WIDTH + x;

            terminal_buffer[current_index] = terminal_buffer[next_line_index];
        }
    }

    const size_t last_row_start = (HEIGHT - 1) * WIDTH;
    uint16_t empty_char = vga_entry(' ', terminal_color);

    for (size_t x = 0; x < WIDTH; x++) {
        terminal_buffer[last_row_start + x] = empty_char;
    }

    cursor_y = HEIGHT - 1;
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

void save_cmd_to_history(string command) {
    TerminalCmd* newNode = new TerminalCmd();
    newNode->command = command;

    newNode->next = nullptr;
    newNode->prev = cmd_history_tail;

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

        delete toDelete;
        cmd_history_count--;
    }
}

void cmd_history_up() {
    if (cmd_history_head == nullptr) return;

    if (cmd_current_line == nullptr) cmd_current_line = cmd_history_tail;
    else if (cmd_current_line->prev) cmd_current_line = cmd_current_line->prev;

    // Prompt "farix> " is 7 chars
    while (cursor_x > 7) {
        echo_char('\b');
    }

    echo(cmd_current_line->command, '\0');

    *shell_buffer = cmd_current_line->command;
}

void cmd_history_down() {
    if (cmd_history_head == nullptr) return;

    if (cmd_current_line == nullptr) cmd_current_line = cmd_history_tail;
    else if (cmd_current_line->next) cmd_current_line = cmd_current_line->next;

    // Prompt "farix> " is 7 chars
    while (cursor_x > 7) {
        echo_char('\b');
    }

    echo(cmd_current_line->command, '\0');

    *shell_buffer = cmd_current_line->command;
}

void echo_at(char c, uint8_t color, size_t x, size_t y) {
	terminal_buffer[y * WIDTH + x] = vga_entry(c, color);
}

void echo_char(char c) {
    if (!handle_special_chars(c)) {
        echo_at(c, terminal_color, cursor_x, cursor_y);

        if (++cursor_x == WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (c == '\0') {
        cursor_x--;
    }

    if (cursor_y >= HEIGHT) {
        terminal_scroll();
    }

    update_cursor(cursor_x, cursor_y);
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

void echo(string data) {
    const char* buffer = data.c_str();
    for (size_t i = 0; i < data.length(); i++) {
        echo_char(buffer[i]);
    }
    echo_char('\n');
}

void echo(string data, char end) {
    const char* buffer = data.c_str();
    for (size_t i = 0; i < data.length(); i++) {
        echo_char(buffer[i]);
    }
    echo_char(end);
}

void echo(string data, string end) {
    const char* buffer = data.c_str();
    for (size_t i = 0; i < data.length(); i++) {
        echo_char(buffer[i]);
    }
    echo(end, "\0");
}

void echo(int data) {
    echo(string::from_int(data));
}

void echo(int data, char end) {
    echo(string::from_int(data), end);
}

bool handle_special_chars(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;

        save_cmd_to_history(*shell_buffer);

        return true;
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

        return true;
    }

    else if (c == '\t') {
        cursor_x += INDENT_LEN - cursor_x % INDENT_LEN;
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
