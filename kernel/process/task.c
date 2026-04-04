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

task* current_task = NULL;
uint32_t next_pid  = 0;
size_t total_tasks = 0;

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
    task* main_task = (task*) kmalloc(sizeof(task));
    kmemset(main_task, 0, sizeof(task));

    main_task->id    = next_pid++;
    main_task->state = TASK_READY;
    main_task->next  = main_task;     // Point to itself for now
    main_task->name  = "init";
    main_task->page_directory = kernel_directory;
    main_task->stack_origin   = PHYSICAL_TO_VIRTUAL(&stack_bottom);

    current_task = main_task;

    total_tasks++;
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

    new_task->next     = current_task->next;
    current_task->next = new_task;

    total_tasks++;

    return new_task;
}

void kill_task(uint32_t id) {
    // Disable interrupts for safety
    system_int_off();

    task* target = current_task;

    for (size_t i = 0; i < total_tasks; i++) {
        if (target->id == id) break;
        target = target->next;
    }

    if (target) target->state = TASK_DEAD;

    system_int_on();

    if (target == current_task) {
        task_yield();
        return;
    }
}

// I have thought up of an algorithm much more efficient
// that works in O(1) time, and genuinely works significantly
// better than any other kernel's (as far as I have seen).
// Got it right before sleeping at night (yes, that occasionally
// happens from time-to-time), but I'm scared to changing this rn
// and leave some small error in here that I have no clue about
// and mess up the entire kernel in the future because of some
// weird scheduler reason. As a result, this circular singly linked
// list will do, and will later be swapped out for the different
// algorithm in my head.
void schedule() {
    if (!current_task || current_task->next == current_task) return;

    task* last = current_task;

    while (last->next->state == TASK_DEAD && last->next != current_task) {
        task* zombie = last->next;
        last->next = zombie->next;

        kfree(zombie->stack_origin);
        kfree(zombie);

        total_tasks--;
    }

    task* next = last->next;
    current_task = next;

    if (next->stack_origin) {
        set_kernel_stack((uint32_t) next->stack_origin + 4096);
    }

    vmm_switch_directory(next->page_directory);

    switch_task(&last->stack_pointer, next->stack_pointer);
}
