/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INIT_TASK_ID  1

#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_SLEEPING 2
#define TASK_DEAD     3

#define PRIV_KERNEL   0 // Kernel privilege
#define PRIV_USER     1 // User
#define PRIV_SUPER    2 // Super user

#define TASKS_LIST_LEN 8

#if TASKS_LIST_LEN == 8
    typedef uint8_t task_list_mask_t;
#elif TASKS_LIST_LEN == 16
    typedef uint16_t task_list_mask_t;
#elif TASKS_LIST_LEN == 32
    typedef uint32_t task_list_mask_t;
#elif TASKS_LIST_LEN == 64
    typedef uint64_t task_list_mask_t;
#else
    #error "task.h: macro TASKS_LIST_LEN must be 8, 16, 32, or 64 exactly"
#endif

typedef struct task {
    struct task* next;        // Next child task
    struct task* parent;      // Caller
    struct task* neighbor;    // Neighbor task in linked list
    uint64_t id;              // Thread ID (Scaled to 64-bit)
    uint64_t stack_pointer;   // Current RSP (Changed from uint32_t)
    uint64_t* page_directory; // 0 -> kernel_directory PML4 (Changed from uint32_t*)
    uint64_t heap_break;      // Limit for malloc (Changed from uint32_t)
    uint64_t state;           // Running, Ready, etc. (Changed from uint32_t)
    uint64_t* stack_origin;   // Memory allocated for the stack (Changed from uint32_t*)
    void (*entry_func)(void* args);
    void* args;
    int privilege;
    const char* name;
} task;

typedef struct task_list {
    task* tasks[TASKS_LIST_LEN];
    task_list_mask_t mask;
    struct task_list* next;
} task_list;

extern task* main_task;
extern task* current_task;
extern uint64_t next_pid;

extern task_list* first_task_list;

void RARE_FUNC init_multitasking();

task* RARE_FUNC create_task(void (*entry_point)(void*), const char* name, const int privilege, void* args);
void  RARE_FUNC kill_task(uint64_t id);

void FREQ_FUNC schedule();

task* RARE_FUNC get_task(uint64_t id);

size_t RARE_FUNC clean_task_lists();

#endif
