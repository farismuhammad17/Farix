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

string full_path_to(const string& filename) {
    string path;

    if (filename.starts_with("/")) {
        // User provided absolute path
        return filename.substr(1); // Removes the leading '/'
    } else {
        // User provided relative path
        string dir = *shell_directory;

        if (dir.starts_with("/")) dir = dir.substr(1); // Remove leading '/'

        if (dir == "" || dir == "/") {
            return filename;
        } else {
            return dir + "/" + filename;
        }
    }
}

void cmd_cd(const string& args) {
    size_t  count = 0;
    string* parts = args.split('/', 0, count);

    string temp_path;

    if (args.starts_with("/")) {
        temp_path = "/";
    } else {
        temp_path = *shell_directory;
    }

    for (size_t i = 0; i < count; i++) {
        if (parts[i] == "..") {
            if (temp_path == "/" || temp_path == "") continue;

            int last_slash = -1;
            for (int j = 0; j < (int) temp_path.length() - 1; j++) {
                if (temp_path[j] == '/') last_slash = j;
            }

            if (last_slash <= 0) temp_path = "/";
            else temp_path = temp_path.substr(0, last_slash);

        } else if (parts[i] == "." || parts[i] == "") {
            continue;
        } else {
            if (!temp_path.ends_with("/")) temp_path = temp_path + "/";
            temp_path = temp_path + parts[i];

            File* f = fs_get(temp_path);
            if (!f || !f->is_directory) {
                echo("cd: no such directory: ", '\0');
                echo(parts[i]);

                delete[] parts;
                return;
            }
        }
    }

    *shell_directory = temp_path;

    delete[] parts;
}

void cmd_cat(const string& args) {
    string filename = full_path_to(args);

    File* f = fs_get(filename);
    if (!f) {
        echo("File not found");
        return;
    }

    char buffer[f->size + 1]; // String isn't needed, we know the exact length needed

    if (fs_read(filename, buffer, f->size)) {
        buffer[f->size] = '\0'; // Null-terminate the string
        echo(buffer);
    } else {
        echo("Error reading file | File might be empty");
    }
}

void cmd_write(const string& args) {
    size_t  out_count = 0;
    string* parts = args.split(' ', 1, out_count);

    if (out_count != 2) {
        echo("Syntax: write filename content");
        return;
    }

    string filename = parts[0];
    string content  = parts[1];

    if (!fs_write(full_path_to(filename), content.c_str(), content.length())) {
        echo("Error: Could not write to file.");
    }
}

void cmd_touch(const string& args) {
    fs_create(full_path_to(args));
}

void cmd_mkdir(const string& args) {
    fs_mkdir(full_path_to(args));
}

void cmd_rm(const string& args) {
    fs_remove(full_path_to(args));
}

void cmd_ls(const string& args) {
    FileNode* head = fs_getall(*shell_directory);

    while (head) {
        echo(head->file.name);
        head = head->next;
    }
}
