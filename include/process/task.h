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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_SLEEPING 2
#define TASK_DEAD     3

// Ensure this is a proper power of 2, no more than 64
#define TASKS_LIST_LEN 8

#if TASKS_LIST_LEN == 8
    typedef uint8_t task_list_mask_t;
#elif TASKS_LIST_LEN == 16
    typedef uint16_t task_list_mask_t;
#elif TASKS_LIST_LEN == 32
    typedef uint32_t task_list_mask_t;
#else
    typedef uint64_t task_list_mask_t;
#endif

#define TASK_LIST_MASK_FULL ((task_list_mask_t)((1ULL << TASKS_LIST_LEN) - 1))

// TODO: Find out why the hell I have two register structs
typedef struct cpu_state {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp;
    uint32_t eip, cs, eflags, esp, ss;
} cpu_state;

typedef struct task_registers_t {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t eip;
} task_registers_t;

typedef struct task {
    struct task* next;        // Next child task
    struct task* parent;      // Caller
    struct task* neighbor;    // Neighbor task in linked list
    uint32_t id;              // Thread ID
    uint32_t stack_pointer;   // Current ESP
    uint32_t* page_directory; // 0 -> kernel_directory
    uint32_t heap_break;      // Limit for malloc
    uint32_t state;           // Running, Ready, etc.
    uint32_t* stack_origin;   // Memory allocated for the stack
    void (*entry_func)();
    bool privilege;           // 0 -> Kernel ; 1 -> User
    const char* name;
} task;

typedef struct task_list {
    task* tasks[TASKS_LIST_LEN];
    task_list_mask_t mask; // uint16_t since 16 tasks per list
    struct task_list* next;
} task_list;

extern task* main_task;
extern task* current_task;
extern uint32_t next_pid;

extern task_list* first_task_list;

void init_multitasking();

task* create_task(void (*entry_point)(), const char* name, const bool privilege);
void  kill_task(uint32_t id);

void  schedule();

task* get_task(uint32_t id);

#endif
