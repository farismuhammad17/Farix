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

#include <string>
#include <vector>
#include <stdio.h>

#include "fs/vfs.h"
#include "memory/heap.h"
#include "drivers/terminal.h"
#include "shell/shell.h"

#include "shell/commands.h"

std::string full_path_to(const std::string& filename) {
    std::string path;

    if (filename[0] == '/') {
        // User provided absolute path
        return filename.substr(1); // Removes the leading '/'
    } else {
        // User provided relative path
        std::string dir = shell_directory;

        if (!dir.empty() && dir[0] == '/') dir = dir.substr(1); // Remove leading '/'

        if (dir == "" || dir == "/") {
            return filename;
        } else {
            return dir + "/" + filename;
        }
    }
}

void cmd_cd(const std::string& args) {
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = args.find('/', start)) != std::string::npos) {
        parts.push_back(args.substr(start, end - start));
        start = end + 1;
    }
    parts.push_back(args.substr(start));

    std::string temp_path;

    if (!args.empty() && args[0] == '/') {
        temp_path = "/";
    } else {
        temp_path = shell_directory;
    }

    for (size_t i = 0; i < parts.size(); i++) {
        if (parts[i] == "..") {
            if (temp_path == "/" || temp_path == "") continue;

            int last_slash = -1;
            for (int j = 0; j < (int)temp_path.length() - 1; j++) {
                if (temp_path[j] == '/') last_slash = j;
            }

            if (last_slash <= 0) temp_path = "/";
            else temp_path = temp_path.substr(0, last_slash);

        } else if (parts[i] == "." || parts[i] == "") {
            continue;
        } else {
            if (!temp_path.empty() && temp_path.back() != '/') temp_path = temp_path + "/";
            temp_path = temp_path + parts[i];

            File* f = fs_get(temp_path);
            if (!f || !f->is_directory) {
                printf("cd: no such directory: %s\n", parts[i].c_str());
                return;
            }
        }
    }

    shell_directory = temp_path;
}

void cmd_cat(const std::string& args) {
    if (args.empty()) {
        printf("Usage: cat <filename>\n");
        return;
    }

    std::string filename = full_path_to(args);
    File* f = fs_get(filename);

    if (!f) {
        printf("cat: %s: No such file\n", args.c_str());
        return;
    }

    if (f->size == 0) return;

    if (f->size > 65536) {
        printf("cat: File too large for buffer\n");
        return;
    }

    char* buffer = (char*) kmalloc(f->size + 1);
    if (!buffer) {
        printf("cat: out of memory\n");
        return;
    }

    kmemset(buffer, 0, f->size + 1);

    if (fs_read(filename, buffer, f->size)) {
        buffer[f->size] = '\0';
        printf("%s\n", buffer);
    } else {
        printf("cat: error reading file\n");
    }

    kfree(buffer);

    get_heap_total();
}

void cmd_write(const std::string& args) {
    size_t first_space = args.find(' ');
    if (first_space == std::string::npos) return;

    std::string filename = args.substr(0, first_space);
    std::string content = args.substr(first_space + 1);

    std::string path = full_path_to(filename);

    if (!fs_write(path, content.c_str(), content.length())) {
        printf("Error: Could not write to file %s\n", filename.c_str());
    }
}

void cmd_touch(const std::string& args) {
    if (args.empty()) return;
    std::string path = full_path_to(args);
    fs_create(path);
}

void cmd_mkdir(const std::string& args) {
    if (args.empty()) return;
    std::string path = full_path_to(args);
    fs_mkdir(path);
}

void cmd_rm(const std::string& args) {
    if (args.empty()) return;
    std::string path = full_path_to(args);
    fs_remove(path);
}

void cmd_ls(const std::string& args) {
    std::string target_path = args.empty() ? shell_directory : full_path_to(args);

    FileNode* head = fs_getall(target_path);
    FileNode* temp = nullptr;

    while (head) {
        printf("%s\n", head->file.name.c_str());
        temp = head->next;
        kfree(head);
        head = temp;
    }
}
