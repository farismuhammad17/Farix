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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "klib/string.h"

#include "hal.h"

#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "fs/types/elf.h"

#include "process/task.h"

#define TASK_LIST_MASK_FULL ((task_list_mask_t)((1ULL << TASKS_LIST_LEN) - 1))

// From boot.s
extern uint64_t stack_top;
extern uint64_t stack_bottom;

/* Pushes all registers of the old task and moves to the new one */
void switch_task(uint64_t* old_rsp, uint64_t new_rsp);

task* main_task    = NULL;
task* current_task = NULL;
uint64_t next_pid  = INIT_TASK_ID;

task_list* first_task_list = NULL;
task_list* current_task_list = NULL;

/* Task trampoline that executes the task, then kills the task upon termination */
static void task_trampoline() {
    system_int_on();

    if (likely(current_task && current_task->entry_func)) {
        current_task->entry_func(current_task->args);
    }

    current_task->state = TASK_DEAD;

    while (1) task_yield();
}

/* Initialise multitasking by creating the init task */
void init_multitasking() {
    main_task = (task*) kmalloc(sizeof(task));
    memset(main_task, 0, sizeof(task));

    main_task->id    = next_pid++;
    main_task->state = TASK_READY;
    main_task->name  = "init";
    main_task->page_directory = (uint64_t*) VIRTUAL_TO_PHYSICAL(kernel_directory);
    main_task->stack_origin   = (uint64_t*) PHYSICAL_TO_VIRTUAL(&stack_bottom);

    main_task->next     = NULL;
    main_task->parent   = NULL;
    main_task->neighbor = NULL;

    current_task = main_task;

    current_task_list = (task_list*) kmalloc(sizeof(task_list));
    memset(current_task_list, 0, sizeof(task_list));

    current_task_list->tasks[0]  = main_task;
    current_task_list->mask     |= 1;
    current_task_list->next      = NULL;

    first_task_list = current_task_list;
}

/* Create new task to execute the `entry_point` with given name and privilege */
task* create_task(void (*entry_point)(void*), const char* name, const int privilege, void* args) {
    task* new_task = (task*) kmalloc(sizeof(task));
    memset(new_task, 0, sizeof(task));

    new_task->id             = next_pid++;
    new_task->entry_func     = (void(*)(void*)) entry_point;
    new_task->args           = args;
    new_task->name           = (char*) name;
    new_task->state          = TASK_READY;
    new_task->page_directory = (uint64_t*) VIRTUAL_TO_PHYSICAL(kernel_directory);
    new_task->heap_break     = 0;
    new_task->privilege      = privilege;
    new_task->parent         = current_task;
    new_task->next           = NULL;

    if (current_task->next == NULL) {
        new_task->neighbor = new_task;
        current_task->next = new_task;
    } else {
        task* head = current_task->next;
        task* last = head;

        while (last->neighbor != head) {
            last = last->neighbor;
        }

        last->neighbor = new_task;
        new_task->neighbor = head;
    }

    // Allocate page aligned 4KB execution stack space
    uint64_t* stack = (uint64_t*) kmalloc(PAGE_SIZE);
    uint64_t* rsp = stack + 512; // 512 * 8-bytes = 4096 (Top of stack boundary)

    // Where the thread goes when task_trampoline returns (safety exit)
    *(--rsp) = 0;

    // The address ret jumps to
    if (privilege == PRIV_KERNEL) {
        *(--rsp) = (uint64_t) task_trampoline;
    } else {
        *(--rsp) = (uint64_t) elf_user_trampoline;
    }

    // Saved registers popped by switch_task (RBP, RBX, R12, R13, R14, R15)
    *(--rsp) = 0; // RBP
    *(--rsp) = 0; // RBX
    *(--rsp) = 0; // R12
    *(--rsp) = 0; // R13
    *(--rsp) = 0; // R14

    // We pass the task's argument block inside R15, which assembly will pass to RDI
    *(--rsp) = (uint64_t) args; // R15

    new_task->stack_pointer = (uint64_t) rsp;
    new_task->stack_origin  = stack;

    if (unlikely(current_task_list->mask == TASK_LIST_MASK_FULL)) {
        task_list* new_task_list = (task_list*) kmalloc(sizeof(task_list));
        memset(new_task_list, 0, sizeof(task_list));

        new_task_list->next = NULL;
        current_task_list->next = new_task_list;
        current_task_list = new_task_list;
    }

    task_list_mask_t free_slots = ~current_task_list->mask;
    int slot = __builtin_ctzll(free_slots);
    current_task_list->tasks[slot] = new_task;
    current_task_list->mask |= (1ULL << slot);

    return new_task;
}

/* Given task at given ID */
void kill_task(uint64_t id) {
    system_int_off();

    task_list* list = first_task_list;
    task* target = NULL;
    int slot_index = -1;
    task_list* target_list = NULL;

    while (list != NULL) {
        if (unlikely(list->mask == 0)) {
            list = list->next;
            continue;
        }

        for (int i = 0; i < TASKS_LIST_LEN; i++) {
            if (list->mask & (1ULL << i) && list->tasks[i]->id == id) {
                target = list->tasks[i];
                target_list = list;
                slot_index = i;
                break;
            }
        }

        if (target) break;
        list = list->next;
    }

    if (likely(target)) {
        target->state = TASK_DEAD;

        task* p = target->parent;
        if (p) {
            if (p->next == target) {
                if (target->neighbor == target) {
                    p->next = NULL;
                } else {
                    p->next = target->neighbor;
                }
            }

            task* prev = target->neighbor;
            while (prev->neighbor != target) {
                prev = prev->neighbor;
            }
            prev->neighbor = target->neighbor;
        }

        target_list->mask &= ~(1ULL << slot_index);
        target_list->tasks[slot_index] = NULL;

        if (target->stack_origin) kfree(target->stack_origin);
        kfree(target);

        if (unlikely(target == current_task)) task_yield();
    }

    system_int_on();

    if (unlikely(target == current_task)) task_yield();
}

/* Scheduler, called by interrupts, switches to the next task. */
void schedule() {
    if (unlikely((next_pid >> 1) == INIT_TASK_ID)) return;

    task* last = current_task;
    task* next = current_task->next;

    if (unlikely(next == NULL || next->state == TASK_DEAD)) {
        next = main_task;
    } else {
        current_task->next = next->neighbor;
    }

    current_task = next;

    if (likely(next->stack_origin)) {
        // Updated to use uint64_t additions across a full 4KB boundary offset
        set_kernel_stack((uint64_t) next->stack_origin + PAGE_SIZE);
    }

    vmm_switch_directory(next->page_directory);

    switch_task(&last->stack_pointer, next->stack_pointer);
}

/* Get task at given ID */
task* get_task(uint64_t id) {
    task_list* list = first_task_list;

    while (list != NULL) {
        if (unlikely(list->mask == 0)) {
            list = list->next;
            continue;
        }

        for (int i = 0; i < TASKS_LIST_LEN; i++) {
            if (list->mask & (1ULL << i) && list->tasks[i]->id == id) {
                return list->tasks[i];
            }
        }

        list = list->next;
    }

    return NULL;
}

size_t clean_task_lists() {
    size_t count = 0;
    task_list* list = first_task_list;
    task_list* next;

    while (list != NULL) {
         next = list->next;

         if (unlikely(next == NULL)) break;
         else if (likely(next->mask != 0)) {
             list = next;
             continue;
         }

         list->next = next->next;
         kfree((void*) next);

         count++;
    }

    return count;
}
