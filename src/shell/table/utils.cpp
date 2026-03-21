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
#include "shell/shell.h"

#include "shell/commands.h"

#include "test.h" // TODO REMOVE

void cmd_help(const std::string& args) {
    for (size_t i = 0; command_table[i].name != nullptr; i++) {
        sh_print("%-*s %s\n",
                INDENT_LEN * 2, // *2 because some commands are longer than 4 characters
                command_table[i].name,
                command_table[i].help_text);
    }
}

void cmd_clear(const std::string& args) {
    terminal_clear();
}

void cmd_echo(const std::string& args) {
    sh_print("%s\n", args.c_str());
}

void cmd_memstat(const std::string& args) {
    print_memstat();
}

void cmd_tasks(const std::string& args) {
    size_t total_tasks = 0;

    // Disable interrupts to ensure atomicity
    asm volatile("cli");

    task* head = current_task;
    task* curr = head;

    do {
        const char* state_str = "READY";
        if (curr->state == TASK_RUNNING) state_str = "RUNNING";
        if (curr->state == TASK_SLEEPING) state_str = "SLEEPING";
        if (curr->state == TASK_DEAD) state_str = "DEAD";

        sh_print("%-4ld %-10s %s\n",
            curr->id,
            state_str,
            curr->name.c_str()
        );
        curr = curr->next;

        total_tasks++;
    } while (curr != head);

    // Re-enable interrupts
    asm volatile("sti");

    sh_print("Total Tasks: %ld\n", total_tasks);
}

void cmd_kill(const std::string& args) {
    if (args.empty()) {
        sh_print("Usage: kill <pid>\n");
        return;
    }

    uint32_t pid = std::stoi(args);
    kill_task(pid);
}

void cmd_peek(const std::string& args) {
    task* t = current_task;
    for (int i = 0; i < total_tasks; i++) {
        if(t->id == std::stoi(args)) {
            task_registers_t* regs = (task_registers_t*) t->stack_pointer;

            sh_print("--- Task Inspection: %s (PID: %d) ---\n", t->name.c_str(), t->id);
            sh_print("State:           %d\n", t->state);
            sh_print("Next task:       %s\n", t->next->name.c_str());
            sh_print("Page directory:  %p\n", t->page_directory);
            sh_print("Stack base:      %p\n", t->stack_base);
            sh_print("Stack origin:    %p\n", t->stack_origin);
            sh_print("ELF Entry Point: %d\n", t->elf_entry_point);
            sh_print("--- Stack dump ---\n");
            sh_print("EAX: %08x   EBX: %08x   ECX: %08x\n", regs->eax, regs->ebx, regs->ecx);
            sh_print("EDX: %08x   ESI: %08x   EDI: %08x\n", regs->edx, regs->esi, regs->edi);
            sh_print("EIP: %08x   EBP: %08x   ESP: %08x\n", regs->eip, regs->ebp, t->stack_pointer);

            return;
        }
        t = t->next;
    }

    sh_print("Task not found\n");
}

void cmd_grep(const std::string& args) {
    // If someone types just 'grep' without a pipe or args
    if (last_cmd_output.empty()) {
        sh_print("grep: No input to search.\n");
        return;
    }
    if (args.empty()) {
        sh_print("grep: Usage: <command> | grep <pattern>\n");
        return;
    }

    size_t start = 0;
    size_t end = last_cmd_output.find('\n');

    while (end != std::string::npos) {
        std::string line = last_cmd_output.substr(start, end - start);

        if (line.find(args) != std::string::npos) {
            sh_print("%s\n", line.c_str());
        }

        start = end + 1;
        end = last_cmd_output.find('\n', start);
    }

    std::string last_line = last_cmd_output.substr(start);
    if (!last_line.empty() && last_line.find(args) != std::string::npos) {
        sh_print("%s\n", last_line.c_str());
    }
}

void cmd_test(const std::string& args) { // TODO REMOVE
    printf("test_pt_virt: %d\n", test_pt_virt);
    printf("test_pt_first_entry: %d\n", test_pt_first_entry);
    printf("test_value_int: %d\n", test_value_int);
}
