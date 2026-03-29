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

#include "drivers/terminal.h"
#include "memory/heap.h"
#include "process/task.h"
#include "shell/shell.h"

#include "memory/vmm.h" // TODOR REM

#include "shell/commands.h"

void cmd_help(const char* args) {
    for (size_t i = 0; command_table[i].name != NULL; i++) {
        sh_print("%-*s %s\n",
                INDENT_LEN * 2, // *2 because some commands are longer than 4 characters
                command_table[i].name,
                command_table[i].help_text);
    }
}

void cmd_clear(const char* args) {
    terminal_clear();
}

void cmd_echo(const char* args) {
    sh_print("%s\n", args);
}

void cmd_memstat(const char* args) {
    print_memstat();
}

void cmd_tasks(const char* args) {
    size_t total_tasks = 0;

    // Disable interrupts to ensure atomicity
    asm volatile("cli");

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
    asm volatile("sti");

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

// TODO REMOVE

void test_write(const char* args) {
    sh_print("--- HEAP BASIC TEST ---\n");

    uint32_t* ptr1 = (uint32_t*) kmalloc(128);
    uint32_t* ptr2 = (uint32_t*) kmalloc(256);

    if (!ptr1 || !ptr2) {
        sh_print("Error: kmalloc returned NULL\n");
        return;
    }

    sh_print("Allocated ptr1 at %x, ptr2 at %x\n", ptr1, ptr2);

    // 2. Test Write/Read to Heap
    *ptr1 = 0xAAAA1111;
    *ptr2 = 0xBBBB2222;

    if (*ptr1 == 0xAAAA1111 && *ptr2 == 0xBBBB2222) {
        sh_print("Heap Write/Read: SUCCESS\n");
    } else {
        sh_print("Heap Write/Read: FAILED! %x, %x\n", *ptr1, *ptr2);
    }

    kfree(ptr1);
    kfree(ptr2);

    sh_print("Blocks freed successfully.\n");
}

void test_read(const char* args) {
    sh_print("--- HEAP EXPANSION TEST ---\n");

    // Your init_heap only starts with 16 pages (64KB)
    // Let's force it to grow by allocating 128KB
    size_t big_size = 128 * 1024;
    sh_print("Allocating %u bytes (Should trigger expansion)...\n", big_size);

    void* big_ptr = kmalloc(big_size);

    if (big_ptr) {
        sh_print("Expansion SUCCESS: Allocated at %x\n", big_ptr);

        // Write to the end of the new memory to ensure it's actually mapped
        uint32_t* end_check = (uint32_t*)((uint32_t)big_ptr + big_size - 4);
        *end_check = 0xFEEDFACE;

        if (*end_check == 0xFEEDFACE) {
            sh_print("Integrity check passed at end of block.\n");
        }

        kfree(big_ptr);
        sh_print("Big block freed.\n");
    } else {
        sh_print("Expansion FAILED: kmalloc returned NULL\n");
    }
}
