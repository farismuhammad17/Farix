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

#include <string.h>

#include "funcs/cmds.h"

static char path_buffer[MAX_SHELL_BUFFER_LEN];

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
