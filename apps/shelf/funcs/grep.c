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

#include <stddef.h>

#include "cmds.h"

void cmd_grep(const char* args) {
    if (last_cmd_output[0] == '\0') {
        sh_print("grep: No input to search.\n");
        return;
    }
    else if (args == NULL || args[0] == '\0') {
        sh_print("grep: Usage: <command> | grep <pattern>\n");
        return;
    }

    char* start = last_cmd_output;
    char* end = strchr(start, '\n');

    while (end != NULL) {
        char saved_char = *end;
        *end = '\0';

        if (strstr(start, args) != NULL) {
            sh_print("%s\n", start);
        }

        *end = saved_char;
        start = end + 1;

        end = strchr(start, '\n');
    }

    if (*start != '\0' && strstr(start, args) != NULL) {
        sh_print("%s\n", start);
    }
}
