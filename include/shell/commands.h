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

#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

#include <string>

typedef void (*command_func_t)(const std::string& args);

struct ShellCommand {
    const char* name;
    command_func_t function;
    const char* help_text;
};

extern ShellCommand command_table[];

void cmd_help(const std::string& args);
void cmd_clear(const std::string& args);
void cmd_echo(const std::string& args);
void cmd_memstat(const std::string& args);
void cmd_tasks(const std::string& args);
void cmd_kill(const std::string& args);

void cmd_cd(const std::string& args);
void cmd_cat(const std::string& args);
void cmd_write(const std::string& args);
void cmd_touch(const std::string& args);
void cmd_mkdir(const std::string& args);
void cmd_rm(const std::string& args);
void cmd_ls(const std::string& args);

#endif
