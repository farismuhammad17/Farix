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

#ifndef SHELL_H
#define SHELL_H

#include <stdarg.h>
#include <stdbool.h>

#define MAX_DIRECTORY_PATH_LEN   256
#define MAX_SHELL_BUFFER_LEN    1024
#define MAX_LAST_CMD_OUTPUT_LEN 1024

#define PIPE_BUFFER_SIZE 8192 // 8 KB

#define MAX_FILENAME_LEN 64

extern char shell_directory[MAX_DIRECTORY_PATH_LEN];
extern char shell_buffer[MAX_SHELL_BUFFER_LEN];
extern bool shell_buffer_ready;

extern char  last_cmd_output[MAX_LAST_CMD_OUTPUT_LEN];
extern char* pipe_buffer;
extern bool  is_piping;

void init_shell();
void shell_update();

void shell_parse(const char* input);

void sh_print(const char* format, ...);
void shell_flush();

#endif
