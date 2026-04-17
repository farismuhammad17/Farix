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

#include "cmds.h"

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
