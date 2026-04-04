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
#include <stdlib.h>
#include <string.h>

#include "arch/stubs.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/heap.h"
#include "process/task.h"
#include "shell/shell.h"

#include "shell/commands.h"

void cmd_help(UNUSED_ARG const char* args) {
    for (size_t i = 0; command_table[i].name != NULL; i++) {
        sh_print("%-*s %s\n",
                INDENT_LEN * 2, // *2 because some commands are longer than 4 characters
                command_table[i].name,
                command_table[i].help_text);
    }
}

void cmd_clear(UNUSED_ARG const char* args) {
    terminal_clear();
}

void cmd_echo(const char* args) {
    sh_print("%s\n", args);
}

void cmd_secho(const char* args) {
    if (!args) return;

    while (*args) {
        if (*args == '\n')
            uart_putc('\r');
        uart_putc(*args++);
    }

    // Always terminate with a fresh line on the serial side
    uart_putc('\r');
    uart_putc('\n');
}

void cmd_memstat(UNUSED_ARG const char* args) {
    // Disable interrupts to prevent the scheduler from
    // switching tasks while we use the heap.
    system_int_off();

    sh_print("Heap Start: %p | End: %p\n", heap_start, heap_end);
    sh_print("----------------------------------------------------------------------\n");
    sh_print("Address    | Size      | Status | Caller Address\n");
    sh_print("----------------------------------------------------------------------\n");

    size_t total_kb  = get_heap_total() >> 10;
    size_t heap_used = get_heap_used();

    size_t used_kb = heap_used >> 10;
    size_t free_kb = total_kb - used_kb;

    HeapSegment* current = first_segment;

    int usage_pct = (total_kb > 0) ? (int)((used_kb * 100) / total_kb) : 0;
    while (current != NULL) {
        sh_print("%p | %-9lu | %-6s | 0x%08lX\n",
                current,
                current->size,
                current->is_free ? "FREE" : "USED",
                current->caller);
        current = current->next;
    }

    sh_print("----------------------------------------------------------------------\n");
    sh_print("Total Used: %lu bytes\n", heap_used);
    sh_print("----------------------------------------------------------------------\n");

    sh_print("Total memory: %4lu KiB\n", total_kb);
    sh_print("Used memory:  %4lu KiB [%d%%]\n", used_kb, usage_pct);
    sh_print("Free memory:  %4lu KiB\n", free_kb);

    system_int_on();
}

void cmd_tasks(UNUSED_ARG const char* args) {
    size_t total_tasks = 0;

    // Disable interrupts to ensure atomicity
    system_int_off();

    task* head = current_task;
    task* curr = head;

    do {
        char* state_str = "READY";
        if (curr->state == TASK_RUNNING) state_str = "RUNNING";
        if (curr->state == TASK_SLEEPING) state_str = "SLEEPING";
        if (curr->state == TASK_DEAD) state_str = "DEAD";

        sh_print("%-4ld %-10s %s\n",
            curr->id,
            state_str,
            curr->name
        );
        curr = curr->next;

        total_tasks++;
    } while (curr != head);

    // Re-enable interrupts
    system_int_on();

    sh_print("Total Tasks: %ld\n", total_tasks);
}

void cmd_kill(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: kill <pid>\n");
        return;
    }

    uint32_t pid = atoi(args);
    kill_task(pid);
}

void cmd_peek(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: peek <pid>\n");
        return;
    }

    task* t = current_task;
    uint32_t target_pid = atoi(args);

    for (size_t i = 0; i < total_tasks; i++) {
        if(t->id == target_pid) {
            task_registers_t* regs = (task_registers_t*) t->stack_pointer;

            sh_print("--- Task Inspection: %s (PID: %d) ---\n", t->name, t->id);
            sh_print("State:           %d\n", t->state);
            sh_print("Next task:       %s\n", t->next->name);
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
