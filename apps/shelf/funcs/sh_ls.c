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

#include "farix.h"

#include "cmds.h"

#define MAX_FILES 64

void cmd_ls(const char* args) {
    char* path;

    if (args[0] == '\0') {
        path = "";
    } else {
        path = full_path_to(args);
    }

    FileData buffer[MAX_FILES];
    size_t total = (size_t) fx_dirscan(path, buffer, MAX_FILES);

    for (size_t i = 0; i < total; i++) {
        if (buffer[i].isdir) {
            sh_print("\033[36m%s\033[0m\n", buffer[i].name);
        } else {
            sh_print("%s\n", buffer[i].name);
        }
    }
}
