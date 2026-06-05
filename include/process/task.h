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

#define INIT_TASK_ID  1

#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_SLEEPING 2
#define TASK_DEAD     3

#define PRIV_KERNEL   0 // Kernel privilege
#define PRIV_USER     1 // User
#define PRIV_SUPER    2 // Super user

/*
Ensure this is exactly 8, 16, 32, or 64 exactly; files that use this header will
throw an error and will not compile if this requirement is not met.

The higher this value, the more tasks one task_list can accomodate. Unfortunately,
that also means if, while running, there aren't enough tasks to fill out a task
list, then the space the array takes up is wasted with empty nothings.

The lower this value, the less space the array takes up, but the more linked lists
that are made, which would slow down traversals due to cache misses.

This value must be properly tuned to required needs.
*/
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
    uint32_t id;              // Thread ID
    uint32_t stack_pointer;   // Current ESP
    uint32_t* page_directory; // 0 -> kernel_directory
    uint32_t heap_break;      // Limit for malloc
    uint32_t state;           // Running, Ready, etc.
    uint32_t* stack_origin;   // Memory allocated for the stack
    void (*entry_func)();
    void* args;
    int privilege;
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

void RARE_FUNC init_multitasking();

task* RARE_FUNC create_task(void (*entry_point)(void*), const char* name, const int privilege, void* args);
void  RARE_FUNC kill_task(uint32_t id);

void FREQ_FUNC schedule();

task* RARE_FUNC get_task(uint32_t id);

size_t RARE_FUNC clean_task_lists();

#endif
