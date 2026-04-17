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

#include <sys/stat.h>
#include <sys/types.h>

#include "farix.h"

#include "cmds.h"

void cmd_mkdir(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: mkdir <folder name>\n");
        return;
    }

    char* path = full_path_to(args);
    int   res  = mkdir(path, 0);

    if (res == SYS_ERROR) {
        sh_print("mkdir: could not create '%s'\n", args);
    }
}
