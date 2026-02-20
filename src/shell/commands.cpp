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
#include "fs/vfs.h"

#include "drivers/terminal.h"
#include "shell/commands.h"

ShellCommand command_table[] = {
    {"help", cmd_help, "Display help information"},
    {"clear", cmd_clear, "Clear the terminal screen"},
    {"echo", cmd_echo, "Echoes to the terminal"},
    {"memstat", cmd_memstat, "Memory statistics"},
    {"cat", cmd_cat, "Read file"},
    {"write", cmd_write, "Write to a file"},
    {"touch", cmd_touch, "Create new file"},
    {"rm", cmd_rm, "Delete file"},

    {nullptr, nullptr, nullptr} // to mark the end
};

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

    echo(string("Total:") + '\t' + string::from_int(total) + " B");
    echo(string("Used:")  + '\t' + string::from_int(used)  + " B");
    echo(string("Free:")  + '\t' + string::from_int(free)  + " B");
}

void cmd_cat(const string& args) {
    File* f = fs_get(args);
    if (!f) {
        echo("File not found");
        return;
    }

    char buffer[f->size + 1]; // String isn't needed, we know the exact length needed

    if (fs_read(args, buffer, f->size)) {
        buffer[f->size] = '\0'; // Null-terminate the string
        echo(buffer);
    } else {
        echo("Error reading file | File might be empty");
    }
}

void cmd_write(const string& args) {
    size_t out_count = 0;
    string* parts = args.split(' ', 1, out_count);

    if (out_count != 2) {
        echo("Syntax: write filename content");
        return;
    }

    string filename = parts[0];
    string content  = parts[1];

    if (!fs_write(filename, content.c_str(), content.length())) {
        echo("Error: Could not write to file.");
    }
}

void cmd_touch(const string& args) {
    fs_create(args);
}

void cmd_rm(const string& args) {
    fs_remove(args);
}
