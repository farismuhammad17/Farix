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

#ifndef CMDS_H
#define CMDS_H

#include <stdarg.h>
#include <stdbool.h>

#define INDENT_LEN 4

#define MAX_DIRECTORY_PATH_LEN   256
#define MAX_SHELL_BUFFER_LEN    1024
#define MAX_LAST_CMD_OUTPUT_LEN 1024

#define PIPE_BUFFER_SIZE 8192 // 8 KB

#define MAX_FILENAME_LEN 64

extern char shell_directory[MAX_DIRECTORY_PATH_LEN];

extern char  last_cmd_output[MAX_LAST_CMD_OUTPUT_LEN];
extern char* pipe_buffer;
extern bool  is_piping;

// For avoiding unused argument compiler warnings cleanly
#define UNUSED_ARG __attribute__((unused))

typedef void (*command_func_t)(const char* args);

typedef struct ShellCommand {
    const char* name;
    command_func_t function;
    const char* help_text;
} ShellCommand;

extern ShellCommand command_table[];

void  sh_print(const char* format, ...);
char* full_path_to(const char* filename);

void cmd_help(const char* args);
void cmd_clear(const char* args);
void cmd_echo(const char* args);
void cmd_secho(const char* args);
void cmd_memstat(const char* args);
void cmd_heapstat(const char* args);
void cmd_int(const char* args);
void cmd_grep(const char* args);

void cmd_cd(const char* args);
void cmd_cat(const char* args);
void cmd_write(const char* args);
void cmd_touch(const char* args);
void cmd_mkdir(const char* args);
void cmd_rm(const char* args);
void cmd_ls(const char* args);
void cmd_exec(const char* args);

void cmd_tasks(const char* args);
void cmd_kill(const char* args);
void cmd_peek(const char* args);
void cmd_tlist(const char* args);

#endif
