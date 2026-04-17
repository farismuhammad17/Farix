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
#include <string.h>
#include <unistd.h>

#include "cmds.h"

void cmd_write(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: write <file> <content...>\n");
        return;
    }

    // Separate filename from content
    char filename[64];
    const char* first_space = strchr(args, ' ');

    if (!first_space) {
        sh_print("write: No content provided.\n");
        return;
    }

    size_t name_len = first_space - args;
    if (name_len >= sizeof(filename)) name_len = sizeof(filename) - 1;

    strncpy(filename, args, name_len);
    filename[name_len] = '\0';

    const char* content = first_space + 1; // Content starts after the space

    char* path = full_path_to(filename);

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
