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

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drivers/keyboard.h"
#include "drivers/terminal.h"

#include "shell/commands.h"
#include "shell/shell.h"

char shell_directory[MAX_DIRECTORY_PATH_LEN];
char shell_buffer[MAX_SHELL_BUFFER_LEN];
bool shell_buffer_ready = false;

char  last_cmd_output[MAX_LAST_CMD_OUTPUT_LEN] = {0};
char* pipe_buffer = NULL;
bool  is_piping = false;

static char shell_output_buffer[8192];
static size_t shell_output_ptr = 0;

static size_t shell_buffer_size = 0;

char* trim(char* s) {
    if (s == NULL) return NULL;
    while (*s == ' ') s++;
    if (*s == '\0') return s;

    char* end = s + strlen(s) - 1;
    while (end > s && *end == ' ') end--;

    *(end + 1) = '\0';
    return s;
}

void init_shell() {
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize the directory array
    strncpy(shell_directory, "/", MAX_DIRECTORY_PATH_LEN - 1);
    shell_directory[MAX_DIRECTORY_PATH_LEN - 1] = '\0';

    // Clear the buffer
    memset(shell_buffer, 0, MAX_SHELL_BUFFER_LEN);
    shell_buffer_ready = false;

    printf(
        "Farix Kernel Shell\n"
        "Copyright (C) 2026 Faris Muhammad\n"
        "License: GNU GPL v3 (Refer repository LICENSE for more information)\n"
        "Type \"help\" for more information\n"
    );
    printf("%s> ", shell_directory);
}

void shell_update() {
    while (kbd_tail != kbd_head) {
        char c = kbd_buffer[kbd_tail];
        kbd_tail = (kbd_tail + 1) % KBD_BUFFER_LEN;

        if (c == '\n') {
            shell_buffer[shell_buffer_size] = '\0';
            echo_char('\n');
            shell_buffer_ready = true;
            break;
        } else if (c == '\b') {
            if (shell_buffer_size > 0) {
                shell_buffer_size--;
                shell_buffer[shell_buffer_size] = '\0';
                echo_char('\b');
            }
        } else if (shell_buffer_size < (MAX_SHELL_BUFFER_LEN - 1)) {
            shell_buffer[shell_buffer_size] = c;
            shell_buffer_size++;
            echo_char(c);
        }
    }

    if (shell_buffer_ready) {
        if (shell_buffer_size != 0) {
            // TODO: This causes heap corruptions,
            // will be implemented later, it's literally
            // 1:27 am as I write this.
            //
            // This is like a month old, I am still lazy
            // to fix this some day.
            //
            // save_cmd_to_history(shell_buffer.c_str());
            shell_parse(shell_buffer);
        }

        shell_buffer[0]    = '\0';
        shell_buffer_size  = 0;
        shell_buffer_ready = false;

        printf("%s> ", shell_directory[0] == '\0' ? "/" : shell_directory);
    }
}

void shell_parse(const char* input) {
    if (input == NULL || input[0] == '\0') return;

    char* segments[2];
    size_t num_segments = 0;
    char* pipe_ptr = strchr((char*)input, '|');

    if (pipe_ptr != NULL) {
        *pipe_ptr = '\0';
        segments[0] = (char*)input;
        segments[1] = pipe_ptr + 1;
        num_segments = 2;
    } else {
        segments[0] = (char*)input;
        num_segments = 1;
    }

    last_cmd_output[0] = '\0';

    for (size_t i = 0; i < num_segments; i++) {
        char* segment = trim(segments[i]);
        if (segment[0] == '\0') continue;

        is_piping = (i < num_segments - 1);
        shell_output_ptr = 0; // Clear buffer for this segment's output

        char* cmd_name = segment;
        char* args = strchr(segment, ' ');
        if (args != NULL) {
            *args = '\0';
            args = trim(args + 1);
        } else {
            args = "";
        }

        bool found = false;
        for (int j = 0; command_table[j].name != NULL; j++) {
            if (strcmp(cmd_name, command_table[j].name) == 0) {
                command_table[j].function(args);
                found = true;
                break;
            }
        }

        if (!found) {
            printf("Command not found: %s\n", cmd_name);
            is_piping = false;
            return;
        }

        // Capture output for next pipe stage
        strncpy(last_cmd_output, shell_output_buffer, MAX_LAST_CMD_OUTPUT_LEN - 1);
    }

    // Final output to screen
    shell_flush();
    is_piping = false;
}

// TODO IMP: Memory fragile, messes the terminal up if
// it has to flush or print too much
void sh_print(const char* format, ...) {
    static char local_buf[1024];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(local_buf, sizeof(local_buf), format, args);
    va_end(args);

    if (len <= 0) return;

    char* src = local_buf;
    int remaining = len;

    while (remaining > 0) {
        size_t space_left = sizeof(shell_output_buffer) - shell_output_ptr - 1;

        if (space_left == 0) {
            shell_flush();
            space_left = sizeof(shell_output_buffer) - 1;
        }

        size_t to_copy = (remaining < (int) space_left) ? (size_t) remaining : space_left;

        memcpy(shell_output_buffer + shell_output_ptr, src, to_copy);

        shell_output_ptr += to_copy;
        shell_output_buffer[shell_output_ptr] = '\0';

        src += to_copy;
        remaining -= to_copy;

        if (shell_output_ptr >= sizeof(shell_output_buffer) - 1) {
            shell_flush();
        }
    }
}

void shell_flush() {
    if (shell_output_ptr > 0) {
        printf("%s", shell_output_buffer);
        shell_output_ptr = 0;
        shell_output_buffer[0] = '\0';
    }
}
