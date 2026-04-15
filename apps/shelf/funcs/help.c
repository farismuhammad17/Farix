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
#include <stdio.h>

#include "cmds.h"

void cmd_help(UNUSED_ARG const char* args) {
    for (size_t i = 0; command_table[i].name != NULL; i++) {
        printf("%-*s %s\n",
            INDENT_LEN * 2, // *2 because some commands are longer than INDENT_LEN
            command_table[i].name,
            command_table[i].help_text);
    }
}
