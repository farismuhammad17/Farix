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

#include <stdlib.h>
#include <string.h>

#include "drivers/terminal.h"
#include "fs/elf.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "process/task.h"
#include "shell/shell.h"

#include "shell/commands.h"

static char path_buffer[256];

char* full_path_to(const char* filename) {
    if (filename[0] == '/')
        return (char*)(filename + 1); // Remove leading slash

    char* dir = shell_directory;
    if (dir[0] == '/') dir++;

    if (dir[0] == '\0')
        return (char*) filename;

    // dir + "/" + filename
    memset(path_buffer, 0, sizeof(path_buffer));
    strcpy(path_buffer, dir);
    strcat(path_buffer, "/");
    strcat(path_buffer, filename);

    return path_buffer;
}

void cmd_cd(const char* args) {
    if (!args || args[0] == '\0') return;

    char work_path[MAX_DIRECTORY_PATH_LEN];
    char component[MAX_FILENAME_LEN];

    // Initialize work_path
    if (args[0] == '/') {
        strcpy(work_path, "/");
    } else {
        strncpy(work_path, shell_directory, MAX_DIRECTORY_PATH_LEN - 1);
        work_path[MAX_DIRECTORY_PATH_LEN - 1] = '\0';
    }

    int i = (args[0] == '/') ? 1 : 0;
    int comp_idx = 0;

    while (1) {
        char c = args[i++];

        if (c == '/' || c == '\0') {
            if (comp_idx > 0) {
                component[comp_idx] = '\0';

                if (strcmp(component, "..") == 0) {
                    if (strcmp(work_path, "/") != 0) {
                        char* last = strrchr(work_path, '/');
                        if (last == work_path) strcpy(work_path, "/");
                        else if (last) *last = '\0';
                    }
                } else if (strcmp(component, ".") != 0) {
                    size_t len = strlen(work_path);
                    if (len > 0 && work_path[len-1] != '/') strcat(work_path, "/");
                    strcat(work_path, component);

                    // Immediate validation
                    File* f = fs_get(work_path);
                    if (!f || !f->is_directory) {
                        sh_print("cd: %s: Not a directory\n", component);
                        return; // Exit immediately on failure
                    }
                }
                comp_idx = 0;
            }
            if (c == '\0') break;
        } else {
            if (comp_idx < 63) {
                component[comp_idx++] = c;
            }
        }
    }

    strncpy(shell_directory, work_path, MAX_DIRECTORY_PATH_LEN - 1);
}

// TODO IMP: Certain large files don't print fully
void cmd_cat(const char* args) {
    if (args == NULL || args[0] == '\0') {
        sh_print("Usage: cat <filename>\n");
        return;
    }

    char* filename = full_path_to(args);
    File* f = fs_get(filename);

    if (!f) {
        sh_print("cat: %s: No such file\n", args);
        return;
    }

    if (f->size == 0) return;

    char* buffer = (char*) malloc(f->size + 1);
    if (!buffer) {
        sh_print("cat: out of memory\n");
        return;
    }

    memset(buffer, 0, f->size + 1);

    if (fs_read(filename, (uint8_t*) buffer, f->size, 0))
        sh_print("%s\n", buffer);

    kfree(buffer);
}

void cmd_write(const char* args) {
    if (args == NULL || args[0] == '\0') return;

    const char* content = NULL;
    char filename[MAX_FILENAME_LEN];
    memset(filename, 0, MAX_FILENAME_LEN);

    // Find space to separate filename from inline content
    char* first_space = (char*) strchr(args, ' ');

    if (first_space == NULL) {
        if (last_cmd_output[0] == '\0') return;

        strncpy(filename, args, MAX_FILENAME_LEN - 1);
        content = last_cmd_output;
    } else {
        size_t name_len = first_space - args;
        if (name_len >= MAX_FILENAME_LEN) name_len = MAX_FILENAME_LEN - 1;

        strncpy(filename, args, name_len);
        filename[name_len] = '\0';

        if (last_cmd_output[0] == '\0') {
            content = first_space + 1; // Inline content starts after the space
        } else {
            content = last_cmd_output; // Prioritize piped output if it exists
        }
    }

    char* path = full_path_to(filename);

    if (!fs_write(path, (uint8_t*) content, strlen(content), 0)) {
        sh_print("Error: Could not write to file %s\n", filename);
    }
}

void cmd_touch(const char* args) {
    if (args != NULL && args[0] != '\0')
        fs_create(full_path_to(args));
}

void cmd_mkdir(const char* args) {
    if (args != NULL && args[0] != '\0')
        fs_mkdir(full_path_to(args));
}

void cmd_rm(const char* args) {
    if (args != NULL && args[0] != '\0')
        fs_remove(full_path_to(args));
}

void cmd_ls(const char* args) {
    const char* target_path = (args[0] == '\0') ? shell_directory : full_path_to(args);

    FileNode* head = fs_getall(target_path);
    FileNode* temp = NULL;

    while (head) {
        sh_print("%s\n", head->file.name);

        temp = head->next;
        kfree(head);
        head = temp;
    }
}

void cmd_exec(const char* args) {
    if (args[0] != '\0')
        exec_elf(full_path_to(args));
}
