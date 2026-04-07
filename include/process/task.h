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

/*

My Scheduling algorithm

Not sure if it is made before, but this is how we go through it: Every single
task would store the following facts that do not do anything other than make
the scheduler move: the parent task (parent), whether we "passed" through
(has_passed), next (next), and the next neighbor (neighbor)

The idea for this algorithm is quite strange, and I have no idea how it came to me,
but I was trying to sleep when I thought of this, and I am not at all sure if this
is exactly a good algorithm, but we'll see. Also note, I might be a terrible explainer,
I am so sorry for this.

Of course, any new tasks, would just be child tasks to it, and if those tasks
call create_task, they become the parent task of the new task, and so on. We
essentially generate a tree of parent-child relationships.

Every single parent task, points to its first child. Every single child task,
points to the next neighboring task. So each neighboring task, each child, is
a circular singly linked list. When we move from init to its first child, we run
that child task for its time, then the scheduler moves its first child, and so on,
while labelling every task that we move through as has_passed = true. We determine
the task to move to from the parent to the child simply by the "next" pointer.

When we reach a leaf node, i.e. a task with no child task, we set the parent's
"next" task to be this leaf task's "neighbor" task, then move up to the parent.
At the scheduler, just check if the current task is "has_passed", if so, set the
parent's task to the neighbor's task, and move up, and repeat this process till
we reach init, who has no parent, but its "next" pointer is now the next task from
the first task we did, and then we just repeat this process.

It's all O(1), there is an overhead of still passing and checking has_passed, but
its elegant, it still moves through every task. I can't exactly see a way we don't,
and there is a problem with orphaned tasks, but I suppose we can just move them to
init or just delete it entirely with the parent. We don't really need to assign
anyone any "priority", because the tree system does that on its own, it's literally
part of the design.

It should be noted, this was my first idea, but when implementing, I realised it was
quite ugly, it looked bad, it needed to do that climb up logic, and it was just a mess,
so I thought of a better algorithm.

It's all the same pointer movements, but we do it on the way down instead, and when we
reach a leaf node, we just shoot current_task to main_task, instead of letting it climb
back up slowly.

*/

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

typedef struct cpu_state {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp;
    uint32_t eip, cs, eflags, esp, ss;
} cpu_state;

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
    char* name;
} task;

typedef struct task_list {
    task* tasks[TASKS_LIST_LEN];
    task_list_mask_t mask; // uint16_t since 16 tasks per list
    struct task_list* next;
} task_list;

typedef struct task_registers_t {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t eip;
} task_registers_t;

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
