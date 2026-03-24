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

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stddef.h>

#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_SLEEPING 2
#define TASK_DEAD     3

typedef struct cpu_state {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp;
    uint32_t eip, cs, eflags, esp, ss;
} cpu_state;

typedef struct task {
    struct task* next;        // Linked list for the scheduler
    uint32_t id;              // Thread ID
    uint32_t stack_pointer;   // Current ESP
    uint32_t* page_directory; // 0 -> kernel_directory
    uint32_t heap_break;      // Limit for malloc
    uint32_t* stack_base;     // Memory allocated for the stack
    uint32_t state;           // Running, Ready, etc.
    uint32_t* stack_origin;
    uint32_t elf_entry_point;
    void (*entry_func)();
    char* name;
} task;

typedef struct task_registers_t {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t eip;
} task_registers_t;

extern task* current_task;
extern uint32_t next_pid;
extern size_t total_tasks;

void init_multitasking();

task* create_task(void (*entry_point)(), const char* name);
void  kill_task(uint32_t id);
void  schedule();

void yield();

#endif
