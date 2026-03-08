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
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>

#include "drivers/keyboard.h"
#include "drivers/terminal.h"

#include "shell/shell.h"
#include "shell/commands.h"

std::string shell_directory;
std::string shell_buffer;
bool        shell_buffer_ready = false;

std::string last_cmd_output = "";
char*       pipe_buffer     = nullptr;
bool        is_piping       = false;

static auto trim = [](std::string& s) { // Remove whitespaces from both ends
    size_t first = s.find_first_not_of(' ');
    if (std::string::npos == first) return s;
    size_t last = s.find_last_not_of(' ');
    s = s.substr(first, (last - first + 1));
    return s;
};

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

    // Allocate the buffer once if it's null (8KB)
    if (!pipe_buffer) pipe_buffer = (char*) malloc(8192);

    size_t pipe_pos = input.find('|');

    std::vector<std::string> segments;
    if (pipe_pos != std::string::npos) {
        segments.push_back(input.substr(0, pipe_pos));
        segments.push_back(input.substr(pipe_pos + 1));
    } else {
        segments.push_back(input);
    }

    last_cmd_output = ""; // Reset for the new line

    for (size_t i = 0; i < segments.size(); i++) {
        std::string segment = segments[i];
        trim(segment);

        is_piping = (i < segments.size() - 1);
        pipe_buffer[0] = '\0'; // Clear the C-string buffer

        std::string cmd_name, args;
        size_t space_pos = segment.find(' ', segment.find_first_not_of(' ')); // Skip leading spaces

        if (space_pos != std::string::npos) {
            cmd_name = segment.substr(0, space_pos);
            args     = segment.substr(space_pos + 1);
        } else {
            cmd_name = segment;
            args     = "";
        }

        // Execute command
        bool found = false;
        for (int j = 0; command_table[j].name != nullptr; j++) {
            if (cmd_name == command_table[j].name) {
                command_table[j].function(args);
                found = true;
                break;
            }
        }

        if (!found) {
            printf("Command not found: %s\n", cmd_name.c_str());
            is_piping = false;
            return;
        }

        last_cmd_output = pipe_buffer;
    }

    is_piping = false; // Safety reset
}

void sh_print(const char* format, ...) {
    char local_buf[1024]; // Temporary buffer for this specific print

    va_list args;
    va_start(args, format);

    // This function evaluates the %s, %d, etc., and puts it into local_buf
    vsnprintf(local_buf, sizeof(local_buf), format, args);
    va_end(args);

    if (is_piping) { // Append local_buf to our global pipe_buffer instead of printing
        strcat(pipe_buffer, local_buf);
    } else { // No pipe
        printf("%s", local_buf);
    }
}
