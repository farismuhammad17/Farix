/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "klib/stdio.h"
#include "klib/string.h"

#include "hal.h"

#include "drivers/keyboard.h"
#include "drivers/terminal.h"
#include "process/task.h"

#include "shell/commands.h"
#include "shell/shell.h"

char shell_directory[MAX_DIRECTORY_PATH_LEN];
char shell_buffer[MAX_SHELL_BUFFER_LEN];
bool shell_buffer_ready = false;

static size_t shell_buffer_size = 0;

static void shell_parse(const char* input);

static char* trim(char* s) {
    if (unlikely(s == NULL)) return NULL;
    while (*s == ' ') s++;
    if (*s == '\0') return s;

    char* end = s + strlen(s) - 1;
    while (end > s && *end == ' ') end--;

    *(end + 1) = '\0';
    return s;
}

/* Initialises Kernel shell */
void init_shell() {
    // Initialize the directory array
    strncpy(shell_directory, "/", MAX_DIRECTORY_PATH_LEN - 1);
    shell_directory[MAX_DIRECTORY_PATH_LEN - 1] = '\0';

    // Clear the buffer
    memset(shell_buffer, 0, MAX_SHELL_BUFFER_LEN);
    shell_buffer_ready = false;

    printf(
        "Farix Kernel Shell\n"
        "Copyright (C) 2026 Faris Muhammad\n"
        "License: GNU AGPL v3 (Refer repository LICENSE for more information)\n"
        "Type \"help\" for more information\n"
    );
    printf("%s> ", shell_directory);
}

/*
Read the keyboard buffer and print the character. If the shell is moved into the
ready state, then it executes shell_parse.
*/
void shell_update() {
    while (kbd_tail != kbd_head) {
        char c = kbd_buffer[kbd_tail];
        kbd_tail = (kbd_tail + 1) % KBD_BUFFER_LEN;

        if (unlikely(c == '\n')) {
            shell_buffer[shell_buffer_size] = '\0';
            echo_char('\n');
            shell_buffer_ready = true;
            break;
        } else if (unlikely(c == '\b')) {
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
            shell_parse(shell_buffer);
        }

        shell_buffer[0]    = '\0';
        shell_buffer_size  = 0;
        shell_buffer_ready = false;

        printf("%s> ", shell_directory[0] == '\0' ? "/" : shell_directory);
    }
}

/*
Takes in the input text, matches it with the list of valid commands, and once
a command is found, it execute that command's function.
*/
static void shell_parse(const char* input) {
    if (unlikely(input == NULL || input[0] == '\0')) return;

    char buffer[MAX_SHELL_BUFFER_LEN];
    strncpy(buffer, input, MAX_SHELL_BUFFER_LEN - 1);
    buffer[MAX_SHELL_BUFFER_LEN - 1] = '\0';

    char* cmd_name = trim(buffer);
    char* args = strchr(cmd_name, ' ');

    if (args != NULL) {
        *args = '\0';
        args = trim(args + 1);
    } else {
        args = "";
    }

    // Execute command
    for (int j = 0; command_table[j].name != NULL; j++) {
        if (strcmp(cmd_name, command_table[j].name) == 0) {
            command_table[j].function(args);
            return;
        }
    }

    printf("Command not found: %s\n", cmd_name);
}
