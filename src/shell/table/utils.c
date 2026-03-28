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

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "drivers/terminal.h"
#include "process/task.h"
#include "memory/heap.h"
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

static void* pmm_alloc_page_SAFE() {
    void* ptr = pmm_alloc_page();
    if ((uint32_t)ptr >= (31 * 1024 * 1024)) {
        sh_print("PMM SAFETY: Refusing page at %x (Out of 32MB window)\n", ptr);
        return NULL;
    }
    sh_print("PMM ALLOC AT: %x\n", ptr);
    return ptr;
}

static void* test_phys_page = NULL;
static uint32_t* test_pd = NULL; // Stores physical address of our test PD

void test_write(const char* args) {
    asm volatile ("cli");

    // 1. Create a directory that inherits kernel mappings (Identity + High Half)
    test_pd = vmm_copy_kernel_directory();

    // 2. Move into this new context
    vmm_switch_directory(test_pd);

    // 3. Allocate the data page and map it at 0x50000000
    test_phys_page = pmm_alloc_page_SAFE();
    vmm_map_page(test_pd, test_phys_page, (void*)0x50000000, PAGE_PRESENT | PAGE_RW | PAGE_USER);

    // 4. Write data to the VIRTUAL address
    uint32_t* virt_ptr = (uint32_t*)0x50000000;
    *virt_ptr = 0xDEADBEEF;

    sh_print("Write Cmd: PD %x | Phys %x | Val: %x | Curr: %x\n", test_pd, test_phys_page, *virt_ptr, vmm_get_current_directory());

    // NOTE: We don't switch back to the old PD here because we want the
    // system to stay in this context for the read test, OR
    // you can switch back and the read test will handle switching back in.
    asm volatile ("sti");
}

void test_read(const char* args) {
    asm volatile ("cli");

    if (!test_phys_page || !test_pd) {
        sh_print("Test Fail: Run test_write first\n");
        asm volatile ("sti");
        return;
    }

    // 1. Switch to the directory where the mapping actually exists
    uint32_t* old_pd = vmm_get_current_directory();
    vmm_switch_directory(test_pd);

    sh_print("Switched to PD: %x (from) %x\n", test_pd, old_pd);

    // 2. Access the virtual address directly
    uint32_t* virt_ptr = (uint32_t*)0x50000000;
    uint32_t val = *virt_ptr;

    // 3. Access physical page via Direct Mapping (C0000000 + phys)
    // This works because vmm_copy_kernel_directory copied the high-half tables!
    uint32_t* phys_view = (uint32_t*) PHYSICAL_TO_VIRTUAL(test_phys_page);

    sh_print("Virt(50000000)=%x\nPhys(%x)=%x\n", val, test_phys_page, *phys_view);

    if (val == 0xDEADBEEF && *phys_view == 0xDEADBEEF) {
        sh_print("VMM TEST PASSED!\n");
    } else {
        sh_print("VMM TEST FAILED! %x and %x\n", val, *phys_view);
    }

    // 4. Clean up: Return to the previous directory
    vmm_switch_directory(old_pd);

    asm volatile ("sti");
}
