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

#include "types/string.h"
#include "drivers/terminal.h"

#include "shell/shell.h"
#include "shell/commands.h"

string* shell_directory = nullptr;
string* shell_buffer    = nullptr;
bool shell_buffer_ready = false;

void init_shell() {
    shell_directory = new string("/");
    shell_buffer = new string("");
    shell_buffer_ready = false;

    echo("farix> ", '\0');
}

void shell_update() {
    if (shell_buffer_ready) {
        shell_parse(*shell_buffer);

        *shell_buffer = "";
        shell_buffer_ready = false;

        echo("farix> ", '\0');
    }
}

void shell_parse(const string& input) {
    if (input.length() == 0) return;

    string cmd_name  = "";
    string args      = "";
    bool found_space = false;

    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];

        if (!found_space && c == ' ') {
            found_space = true;
            continue;
        }

        if (!found_space) cmd_name += c;
        else              args += c;
    }

    for (int i = 0; command_table[i].name != nullptr; i++) {
        if (cmd_name == command_table[i].name) {
            command_table[i].function(args);
            return;
        }
    }

    echo("Command not found");
}
