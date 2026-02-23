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

#include "fs/vfs.h"
#include "drivers/terminal.h"
#include "shell/shell.h"

#include "shell/commands.h"

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

void cmd_mkdir(const string& args) {
    fs_mkdir(args);
}

void cmd_rm(const string& args) {
    fs_remove(args);
}

void cmd_ls(const string& args) {
    FileNode* head = fs_getall(*shell_directory);

    while (head) {
        echo(head->file.name);
        head = head->next;
    }
}
