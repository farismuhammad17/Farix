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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "farix.h"

#include "cmds.h"

#define MAX_FILES 64

void cmd_cd(const char* args) {
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
                    int f = open(work_path, O_RDONLY);
                    if (!f) {
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

// TODO: cat command might make the memory suffer with large files
// TODO IMP: cat command creates folder for somereason
void cmd_cat(const char* args) {
    if (args == NULL || args[0] == '\0') {
        sh_print("Usage: cat <filename>\n");
        return;
    }

    char path[MAX_DIRECTORY_PATH_LEN];
    full_path_to(args, path);

    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        sh_print("cat: %s: No such file\n", args);
        return;
    }

    // Move to end (SEEK_END) with 0 offset
    off_t size = lseek(fd, 0, SEEK_END);
    if (size <= 0) {
        close(fd);
        return;
    }

    // Seek back to the beginning so we can actually read it
    lseek(fd, 0, SEEK_SET);

    // Allocate the buffer +1 for the null terminator
    char* buffer = (char*) malloc(size + 1);
    if (!buffer) {
        sh_print("cat: out of memory for %d bytes\n", (int) size);
        close(fd);
        return;
    }

    // Read the whole thing
    int bytes_read = read(fd, buffer, size);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Safety null terminator
        sh_print("%s\n", buffer);
    }

    free(buffer);
    close(fd);
}

void cmd_write(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: write <file> <content...>\n");
        return;
    }

    // Separate filename from content
    char filename[MAX_FILENAME_LEN];
    const char* first_space = strchr(args, ' ');

    if (!first_space) {
        sh_print("Usage: write <file> <content...>\n");
        return;
    }

    size_t name_len = first_space - args;
    if (name_len >= sizeof(filename)) name_len = sizeof(filename) - 1;

    strncpy(filename, args, name_len);
    filename[name_len] = '\0';

    const char* content = first_space + 1; // Content starts after the space

    char path[MAX_DIRECTORY_PATH_LEN];
    full_path_to(args, path);

    // Open, Write, Close
    // O_WRONLY: Write only
    // O_CREAT: Create if missing
    // O_TRUNC: If it exists, wipe it (so we start fresh)
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == SYS_ERROR) {
        sh_print("write: Could not open '%s'\n", filename);
        return;
    }

    int bytes_written = write(fd, content, strlen(content));

    if (bytes_written < 0) {
        sh_print("write: Failed to write to file.\n");
    }

    close(fd);
}

void cmd_touch(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: touch <file name>\n");
        return;
    }

    char path[MAX_DIRECTORY_PATH_LEN];
    full_path_to(args, path);

    int fd = open(path, O_WRONLY | O_CREAT, 0644);

    if (fd != -1) {
        close(fd);
    } else {
        sh_print("touch: could not create '%s'\n", args);
    }
}

void cmd_mkdir(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: mkdir <folder name>\n");
        return;
    }

    char path[MAX_DIRECTORY_PATH_LEN];
    full_path_to(args, path);

    int res = mkdir(path, 0);

    if (res == SYS_ERROR) sh_print("mkdir: could not create '%s'\n", args);
}

void cmd_ls(const char* args) {
    char path[MAX_DIRECTORY_PATH_LEN];

    if (args[0] == '\0') {
        strncpy(path, shell_directory, MAX_DIRECTORY_PATH_LEN);
    } else {
        full_path_to(args, path);
    }

    FileData buffer[MAX_FILES];
    size_t total = (size_t) fx_dirscan(path, buffer, MAX_FILES);

    if (total == SYS_ERROR) { // response is stored in total
        sh_print("ls: Could not read directory: %s\n", path);
        return;
    }

    for (size_t i = 0; i < total; i++) {
        if (buffer[i].isdir) {
            sh_print("\033[36m%s\033[0m\n", buffer[i].name);
        } else {
            sh_print("%s\n", buffer[i].name);
        }
    }
}

void cmd_rm(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: rm <path>\n");
        return;
    }

    char path[MAX_DIRECTORY_PATH_LEN];
    full_path_to(args, path);

    int res = remove(path);

    if (res == SYS_ERROR) sh_print("rm: Could not delete '%s'\n", path);
}
