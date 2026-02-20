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

#include "memory/heap.h"

#include "process/task.h"

task* current_task = nullptr;
uint32_t next_pid  = 0;

size_t total_tasks = 0;

void task_trampoline() {
    asm volatile("sti");

    if (current_task && current_task->entry_func) {
        current_task->entry_func();
    }

    current_task->state = TASK_DEAD;

    while(1) {
        yield(); // Keep yielding until the scheduler deletes the task
    }
}

void init_multitasking() {
    // Create the first task (the one we are currently in)
    task* main_task  = new task();
    main_task->id    = next_pid++;
    main_task->state = TASK_READY;
    main_task->next  = main_task;     // Point to itself for now

    current_task = main_task;

    total_tasks++;
}

task* create_task(void (*entry_point)(), string name) {
    task* new_task = new task();

    new_task->id         = next_pid++;
    new_task->entry_func = entry_point;
    new_task->name       = name;

    uint32_t* stack = (uint32_t*) malloc(4096);
    uint32_t* esp = stack + 1024; // High address

    // The IRET Frame (Pushed in the order IRET pops them)
    *(--esp) = 0x0202;                      // EFLAGS (The 'sti' is inside here!)
    *(--esp) = 0x08;                        // CS (Kernel Code Segment)
    *(--esp) = (uint32_t) task_trampoline;  // EIP (Where the task starts)

    // The PUSHA placeholders (8 registers)
    for (int i = 0; i < 8; i++) {
        *(--esp) = 0;
    }

    new_task->stack_pointer = (uint32_t) esp;
    new_task->stack_origin  = stack;

    new_task->next     = current_task->next;
    current_task->next = new_task;

    total_tasks++;

    return new_task;
}

void schedule() {
    if (!current_task || current_task->next == current_task) return;

    task* last = current_task;

    while (last->next->state == TASK_DEAD && last->next != current_task) {
        task* zombie = last->next;
        last->next = zombie->next;

        free(zombie->stack_origin);
        delete zombie;

        total_tasks--;
    }

    task* next = last->next;
    current_task = next;
    switch_task(&last->stack_pointer, next->stack_pointer);
}

void yield() {
    asm volatile("int $0x20"); // Manually trigger the timer interrupt
}
