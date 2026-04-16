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
#include <unistd.h>

#include "cmds.h"

// TODO: cat command might make the memory suffer with large files
void cmd_cat(const char* args) {
    if (args == NULL || args[0] == '\0') {
        sh_print("Usage: cat <filename>\n");
        return;
    }

    char* path = full_path_to(args);
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
