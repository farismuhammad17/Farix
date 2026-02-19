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

#include "memory/heap.h"

#include "drivers/terminal.h"
#include "shell/commands.h"

ShellCommand command_table[] = {
    {"help", cmd_help, "Display help information"},
    {"clear", cmd_clear, "Clear the terminal screen"},
    {"echo", cmd_echo, "Echoes to the terminal"},
    {"memstat", cmd_memstat, "Memory statistics"},
    {nullptr, nullptr,   nullptr} // to mark the end
};

void cmd_help(const string& args) {
    for (size_t i = 0; command_table[i].name != nullptr; i++) {
        echo(string(command_table[i].name) + '\t' + command_table[i].help_text);
    }
}

void cmd_clear(const string& args) {
    terminal_clear();
}

void cmd_echo(const string& args) {
    echo(args);
}

void cmd_memstat(const string& args) {
    size_t total = get_heap_total() / 1024;
    size_t used  = get_heap_used()  / 1024;
    size_t free  = total - used;

    echo(string("Total:") + '\t' + string::from_int(total) + " B");
    echo(string("Used:")  + '\t' + string::from_int(used)  + " B");
    echo(string("Free:")  + '\t' + string::from_int(free)  + " B");
}
