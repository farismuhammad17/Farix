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

#include <string>

#include "drivers/terminal.h"
#include "process/task.h"
#include "memory/heap.h"

#include "shell/commands.h"

void cmd_help(const std::string& args) {
    for (size_t i = 0; command_table[i].name != nullptr; i++) {
        printf("%-*s %s\n",
                INDENT_LEN * 2, // *2 because some commands are longer than 4 characters
                command_table[i].name,
                command_table[i].help_text);
    }
}

void cmd_clear(const std::string& args) {
    terminal_clear();
}

void cmd_echo(const std::string& args) {
    printf("%s\n", args.c_str());
}

void cmd_memstat(const std::string& args) {
    print_memstat();
}

void cmd_tasks(const std::string& args) {
    size_t total_tasks = -1;

    // Disable interrupts to ensure atomicity
    asm volatile("cli");

    task* head = current_task;
    task* curr = head;

    do {
        const char* state_str = "READY";
        if (curr->state == TASK_RUNNING) state_str = "RUNNING";
        if (curr->state == TASK_SLEEPING) state_str = "SLEEPING";
        if (curr->state == TASK_DEAD) state_str = "DEAD";

        printf("%-4ld %-5s %s\n",
            curr->id,
            state_str,
            curr->name.c_str()
        );
        curr = curr->next;

        total_tasks++;
    } while (curr != head);

    // Re-enable interrupts
    asm volatile("sti");

    printf("-------------------\n");
    printf("Total Tasks: %ld\n\n", total_tasks);
}
