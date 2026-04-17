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
#include <unistd.h>

#include "cmds.h"

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
