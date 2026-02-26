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

#include <stdio.h>
#include <string>

#include "drivers/keyboard.h"
#include "drivers/terminal.h"

#include "shell/shell.h"
#include "shell/commands.h"

std::string shell_directory;
std::string shell_buffer;
bool        shell_buffer_ready = false;

void init_shell() {
    setvbuf(stdout, NULL, _IONBF, 0);

    shell_directory = "/";
    shell_buffer = "";
    shell_buffer_ready = false;

    printf("%s> ", shell_directory.c_str());
}

void shell_update() {
    while (kbd_tail != kbd_head) {
        char c = kbd_buffer[kbd_tail];
        kbd_tail = (kbd_tail + 1) % KBD_BUFFER_LEN;

        if (c == '\n') {
            echo_char('\n'); // Standard newline
            shell_buffer_ready = true;
            break;
        } else if (c == '\b') {
            if (shell_buffer.length() > 0) {
                shell_buffer.pop_back();
                echo_char('\b');
            }
        } else {
            shell_buffer += c;
            echo_char(c);
        }
    }

    if (shell_buffer_ready) {
        if (!shell_buffer.empty()) {
            // TODO: This causes heap corruptions,
            // will be implemented later, it's literally
            // 1:27 am as I write this.
            // save_cmd_to_history(shell_buffer.c_str());
            shell_parse(shell_buffer);
        }

        shell_buffer = "";
        shell_buffer_ready = false;

        const char* display_dir = shell_directory.empty() ? "/" : shell_directory.c_str();
        printf("%s> ", display_dir);
    }
}

void shell_parse(const std::string& input) {
    if (input.empty()) return;

    std::string cmd_name;
    std::string args;

    size_t space_pos = input.find(' ');

    if (space_pos != std::string::npos) {
        // We found a space: split into cmd and args
        cmd_name = input.substr(0, space_pos);
        args     = input.substr(space_pos + 1);
    } else {
        // No space: the whole input is the command
        cmd_name = input;
        args     = "";
    }

    for (int i = 0; command_table[i].name != nullptr; i++) {
        if (cmd_name == command_table[i].name) {
            command_table[i].function(args);
            return;
        }
    }

    printf("Command not found: %s\n", cmd_name.c_str());
}
