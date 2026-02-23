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

#include "drivers/terminal.h"
#include "memory/heap.h"

#include "shell/commands.h"

void cmd_help(const string& args) {
    for (size_t i = 0; command_table[i].name != nullptr; i++) {
        string cmd = string(command_table[i].name);
        string indent = "\t";
        if (cmd.length() < INDENT_LEN) {
            indent = indent + '\t';
        }
        echo(cmd + indent + command_table[i].help_text);
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

    echo("Total:\t" + string::from_int(total));
    echo("Used:\t"  + string::from_int(used));
    echo("Free:\t"  + string::from_int(free));
}
