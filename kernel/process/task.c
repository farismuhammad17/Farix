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

#include "hal.h"

#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "fs/types/elf.h"

#include "process/task.h"

#define TASK_LIST_MASK_FULL ((task_list_mask_t)((1ULL << TASKS_LIST_LEN) - 1))

// From boot.s
extern uint32_t stack_top;
extern uint32_t stack_bottom;

/* Pushes all registers of the old task and moves to the new one */
void switch_task(uint32_t* old_esp, uint32_t new_esp);

task* main_task    = NULL;
task* current_task = NULL;
uint32_t next_pid  = INIT_TASK_ID;

task_list* first_task_list = NULL;
task_list* current_task_list = NULL;

/* Task trampoline that executes the task, then kills the task upon termination */
static void task_trampoline() {
    system_int_on();

    if (likely(current_task && current_task->entry_func)) {
        current_task->entry_func(current_task->args);
    }

    current_task->state = TASK_DEAD;

    while(1) task_yield(); // Keep yielding till deletion
}

/* Initialise multitasking by creating the init task */
void init_multitasking() {
    // Create the first task (the one we are currently in)
    main_task = (task*) kmalloc(sizeof(task));
    memset(main_task, 0, sizeof(task));

    main_task->id    = next_pid++;
    main_task->state = TASK_READY;
    main_task->name  = "init";
    main_task->page_directory = kernel_directory;
    main_task->stack_origin   = PHYSICAL_TO_VIRTUAL(&stack_bottom);

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
    new_task->entry_func     = entry_point;
    new_task->args           = args;
    new_task->name           = (char*) name;
    new_task->state          = TASK_READY;
    new_task->page_directory = kernel_directory;
    new_task->heap_break     = 0;
    new_task->privilege      = privilege;
    new_task->parent         = current_task;
    new_task->next           = NULL;

    // I feel this can be made better, but i'm prototyping
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

    uint32_t* stack = (uint32_t*) kmalloc(4096);
    uint32_t* esp = stack + 1024; // High address

    // Push the void* argument (it sits above EIP in memory)
    *(--esp) = (uint32_t) args;

    // Push a dummy return address (what entry_point returns to if it exits)
    *(--esp) = 0;

    if (privilege == PRIV_KERNEL) {
        *(--esp) = (uint32_t) task_trampoline;
    } else {
        *(--esp) = (uint32_t) elf_user_trampoline;
    }

    // The PUSHA placeholders (8 registers)
    for (int i = 0; i < 8; i++) *(--esp) = 0;

    new_task->stack_pointer = (uint32_t) esp;
    new_task->stack_origin  = stack;

    if (unlikely(current_task_list->mask == TASK_LIST_MASK_FULL)) {
        task_list* new_task_list = (task_list*) kmalloc(sizeof(task_list));
        memset(new_task_list, 0, sizeof(task_list));

        new_task_list->next = NULL;
        current_task_list->next = new_task_list;
        current_task_list = new_task_list;
    }

    // Add new task to current_task_list
    task_list_mask_t free_slots = ~current_task_list->mask;
    int slot = __builtin_ctz(free_slots);
    current_task_list->tasks[slot] = new_task;
    current_task_list->mask |= (1 << slot);

    return new_task;
}

/* Given task at given ID */
void kill_task(uint32_t id) {
    system_int_off();

    task_list* list = first_task_list;
    task* target = NULL;
    int slot_index = -1;
    task_list* target_list = NULL;

    // Walk the lists
    while (list != NULL) {
        if (unlikely(list->mask == 0)) { // TODO: Do something here, delete it, fix the linked list, etc.
            list = list->next;
            continue;
        }

        for (int i = 0; i < TASKS_LIST_LEN; i++) {
            // Check the mask to see if the slot is even occupied
            if (list->mask & (1 << i) && list->tasks[i]->id == id) {
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
            // Move pointer from parent if this was next
            if (p->next == target) {
                if (target->neighbor == target) {
                    p->next = NULL; // No more children
                } else {
                    p->next = target->neighbor;
                }
            }

            // Unlink from neighbor linked list
            task* prev = target->neighbor;
            while (prev->neighbor != target) {
                prev = prev->neighbor;
            }
            prev->neighbor = target->neighbor;
        }

        target_list->mask &= ~(1 << slot_index);
        target_list->tasks[slot_index] = NULL;

        if (target->stack_origin) kfree(target->stack_origin);
        kfree(target);

        if (unlikely(target == current_task)) task_yield();
    }

    system_int_on();

    if (unlikely(target == current_task)) task_yield();
}

/*
Scheduler, called by interrupts, switches to the next task. The algirhtm that
chooses the task is as follows:

Each new task records the task that created it as its `parent`, and has no
children at creation. Its neighbors are a circular singly linked list of all
the children of its parent. Each parent is not informed of all the children,
but has one pointer to a child, `next`. When a new task is created, we snuggle
in the task into this parent-child tree strcture.
*/
void schedule() {
    // TODO: Scheduler doesn't deal with sleeping tasks

    /*
    If init is the only task, nothing to do.

    next_pid is set to 1, and incremented upon init_multitasking,
    setting next_pid to 2. We could check if next_pid - 1 equals
    the INIT_TASK_ID (which is 1), but bitwise operators are a bit
    faster than subtraction. (10 >> 1) = 1 == INIT_TASK_ID.

    Of course, this solely relies on the fact that INIT_TASK_ID is
    1, which must be noted. If this if isn't caught, the CPU will
    try to schedule to the current task, init, and crash.
    */
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
        set_kernel_stack((uint32_t) next->stack_origin + PAGE_SIZE);
    }

    vmm_switch_directory(next->page_directory);

    switch_task(&last->stack_pointer, next->stack_pointer);
}

/*
Get task at given ID by iterating through the task_list, a linked list of arrays
that stores all the tasks we have for quick look ups. It is just a few extra space
in memory, but makes these lookups much more simpler and faster.

While traversing through the task lists, if the function finds a task_list with no
tasks inside, i.e. the bitmask is exactly 000..., then the function would move to
the next task list.

If no task is found with the given ID, the function returns NULL.
*/
task* get_task(uint32_t id) {
    task_list* list = first_task_list;

    while (list != NULL) {
        if (unlikely(list->mask == 0)) {
            list = list->next;
            continue;
        }

        for (int i = 0; i < TASKS_LIST_LEN; i++) {
            if (list->mask & (1 << i) && list->tasks[i]->id == id) {
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
