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

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "funcs/cmds.h"

char shell_directory[MAX_DIRECTORY_PATH_LEN];
char shell_buffer[MAX_SHELL_BUFFER_LEN];
bool shell_buffer_ready = false;

char  last_cmd_output[MAX_LAST_CMD_OUTPUT_LEN] = {0};
char* pipe_buffer = NULL;
bool  is_piping = false;

static char shell_output_buffer[8192];
static size_t shell_output_ptr = 0;

static size_t shell_buffer_size = 0;

ShellCommand command_table[] = {
    {"help", cmd_help, "Display help information"},
    {"clear", cmd_clear, "Clear the terminal screen"},
    {"echo", cmd_echo, "Echoes to the terminal"},
    {"secho", cmd_secho, "Write text to the serial port (COM1)"},
    {"memstat", cmd_memstat, "Memory statistics"},
    {"heapstat", cmd_heapstat, "Verify heap health"},
    {"int", cmd_int, "Jump to given interrupt"},
    {"grep", cmd_grep, "Searches text for matching patterns"},
    {"cd", cmd_cd, "Change directory"},
    {"cat", cmd_cat, "Read file"},
    {"write", cmd_write, "Write to a file"},
    {"touch", cmd_touch, "Create new file"},
    {"mkdir", cmd_mkdir, "Create new folder"},
    {"rm", cmd_rm, "Delete file"},
    {"ls", cmd_ls, "List directory contents"},
    {"exec", cmd_exec, "Execute ELF file"},
    {"tasks", cmd_tasks, "List of running tasks"},
    {"kill", cmd_kill, "Kill a task given the process ID"},
    {"peek", cmd_peek, "Inspect a task"},
    {"tlist", cmd_tlist, "Bit map of every task list"},

    {NULL, NULL, NULL} // to mark the end
};

static char* trim(char* s) {
    if (s == NULL) return NULL;
    while (*s == ' ') s++;
    if (*s == '\0') return s;

    char* end = s + strlen(s) - 1;
    while (end > s && *end == ' ') end--;

    *(end + 1) = '\0';
    return s;
}

void shell_parse(const char* input);

int main() {
    char input[256];

    while (1) {
        printf("/> ");

        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = 0;

        shell_parse(input);
    }

    return 0;
}

void shell_parse(const char* input) {
    if (input == NULL || input[0] == '\0') return;

    char* segments[2];
    size_t num_segments = 0;
    char* pipe_ptr = strchr((char*) input, '|');

    if (pipe_ptr != NULL) {
        *pipe_ptr = '\0';
        segments[0] = (char*) input;
        segments[1] = pipe_ptr + 1;
        num_segments = 2;
    } else {
        segments[0] = (char*) input;
        num_segments = 1;
    }

    last_cmd_output[0] = '\0';

    for (size_t i = 0; i < num_segments; i++) {
        char* segment = trim(segments[i]);
        if (segment[0] == '\0') continue;

        is_piping = (i < num_segments - 1);
        shell_output_ptr = 0; // Clear buffer for this segment's output

        char* cmd_name = segment;
        char* args = strchr(segment, ' ');
        if (args != NULL) {
            *args = '\0';
            args = trim(args + 1);
        } else {
            args = "";
        }

        bool found = false;
        for (int j = 0; command_table[j].name != NULL; j++) {
            if (strcmp(cmd_name, command_table[j].name) == 0) {
                command_table[j].function(args);
                found = true;
                break;
            }
        }

        if (!found) {
            printf("Command not found: %s\n", cmd_name);
            is_piping = false;
            return;
        }

        // Capture output for next pipe stage
        strncpy(last_cmd_output, shell_output_buffer, MAX_LAST_CMD_OUTPUT_LEN - 1);
    }

    is_piping = false;
}

void cmd_heapstat(const char* args){}
void cmd_int(const char* args){}
void cmd_grep(const char* args){}

void cmd_cd(const char* args){}
void cmd_cat(const char* args){}
void cmd_write(const char* args){}
void cmd_touch(const char* args){}
void cmd_mkdir(const char* args){}
void cmd_rm(const char* args){}
void cmd_ls(const char* args){}
void cmd_exec(const char* args){}

void cmd_tasks(const char* args){}
void cmd_kill(const char* args){}
void cmd_peek(const char* args){}
void cmd_tlist(const char* args){}
