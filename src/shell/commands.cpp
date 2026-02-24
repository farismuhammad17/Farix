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

#include "shell/commands.h"

ShellCommand command_table[] = {
    {"help", cmd_help, "Display help information"},
    {"clear", cmd_clear, "Clear the terminal screen"},
    {"echo", cmd_echo, "Echoes to the terminal"},
    {"memstat", cmd_memstat, "Memory statistics"},
    {"cd", cmd_cd, "Change directory"},
    {"cat", cmd_cat, "Read file"},
    {"write", cmd_write, "Write to a file"},
    {"touch", cmd_touch, "Create new file"},
    {"mkdir", cmd_mkdir, "Create new folder"},
    {"rm", cmd_rm, "Delete file"},
    {"ls", cmd_ls, "List directory contents"},

    {nullptr, nullptr, nullptr} // to mark the end
};
