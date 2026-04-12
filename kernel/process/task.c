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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "arch/stubs.h"
#include "memory/heap.h"
#include "memory/vmm.h"

#include "fs/elf.h"

#include "process/task.h"

// From boot.s
extern uint32_t stack_top;
extern uint32_t stack_bottom;
extern void switch_task(uint32_t* old_esp, uint32_t new_esp);

task* main_task    = NULL;
task* current_task = NULL;
uint32_t next_pid  = 1;

task_list* first_task_list = NULL;
task_list* current_task_list = NULL;

void task_trampoline() {
    system_int_on();

    if (current_task && current_task->entry_func) {
        current_task->entry_func();
    }

    current_task->state = TASK_DEAD;

    while(1) {
        task_yield(); // Keep yielding until the scheduler deletes the task
    }
}

void init_multitasking() {
    // Create the first task (the one we are currently in)
    main_task = (task*) kmalloc(sizeof(task));
    kmemset(main_task, 0, sizeof(task));

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
    kmemset(current_task_list, 0, sizeof(task_list));

    current_task_list->tasks[0]  = main_task;
    current_task_list->mask     |= 1;
    current_task_list->next      = NULL;

    first_task_list = current_task_list;
}

task* create_task(void (*entry_point)(), const char* name, const bool privilege) {
    task* new_task = (task*) kmalloc(sizeof(task));
    kmemset(new_task, 0, sizeof(task));

    new_task->id             = next_pid++;
    new_task->entry_func     = entry_point;
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

    if (privilege) { // 0 -> Kernel task
        *(--esp) = (uint32_t) elf_user_trampoline;
    } else {
        *(--esp) = (uint32_t) task_trampoline;  // EIP (Where the task starts)
    }

    // The PUSHA placeholders (8 registers)
    for (int i = 0; i < 8; i++) *(--esp) = 0;

    new_task->stack_pointer = (uint32_t) esp;
    new_task->stack_origin  = stack;

    if (current_task_list->mask == TASK_LIST_MASK_FULL) {
        task_list* new_task_list = (task_list*) kmalloc(sizeof(task_list));
        kmemset(new_task_list, 0, sizeof(task_list));

        current_task_list->next = new_task_list;
        current_task_list = new_task_list;
    }

    // Add new task to current_task_list
    task_list_mask_t free_slots = ~current_task_list->mask;
    int slot = __builtin_ctz(free_slots); // I found out this exists today
    current_task_list->tasks[slot] = new_task;
    current_task_list->mask |= (1 << slot);

    return new_task;
}

void kill_task(uint32_t id) {
    system_int_off();

    task_list* list = first_task_list;
    task* target = NULL;
    int slot_index = -1;
    task_list* target_list = NULL;

    // Walk the lists
    while (list != NULL) {
        if (list->mask == 0) { // TODO: Do something here, delete it, fix the linked list, etc.
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

    if (target) {
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
    }

    system_int_on();

    if (target == current_task) task_yield();
}

void schedule() {
    task* last = current_task;
    task* next = current_task->next;

    if (next == NULL || next->state == TASK_DEAD) {
        next = main_task;
    } else {
        current_task->next = next->neighbor;
    }

    if (last == next) return;

    current_task = next;

    if (next->stack_origin) {
        set_kernel_stack((uint32_t) next->stack_origin + 4096);
    }

    vmm_switch_directory(next->page_directory);

    switch_task(&last->stack_pointer, next->stack_pointer);
}

task* get_task(uint32_t id) {
    task_list* list = first_task_list;

    while (list != NULL) {
        if (list->mask == 0) {
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
