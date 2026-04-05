#include <stdlib.h>

#include "arch/stubs.h"
#include "process/task.h"
#include "shell/shell.h"

#include "shell/commands.h"

void cmd_tasks(UNUSED_ARG const char* args) {
    system_int_off();

    task* curr = main_task;

    int indent = 0;
    while (curr != NULL) {
        char* state_str = "READY";
        if (curr->state == TASK_RUNNING) state_str = "RUNNING";
        if (curr->state == TASK_SLEEPING) state_str = "SLEEPING";
        if (curr->state == TASK_DEAD) state_str = "DEAD";

        for (int i = 0; i < indent; i++) sh_print(" ");
        sh_print("%s (%ld) - %s\n",
            curr->name,
            curr->id,
            state_str
        );

        // Try to go DOWN (to children)
        if (curr->next != NULL) {
            curr = curr->next;
            indent++;
            continue;
        }

        // If no children, try to go SIDEWAYS (to neighbors)
        if (curr->parent != NULL && curr->neighbor != curr->parent->next) {
            curr = curr->neighbor;
            continue;
        }

        // If no children and no more neighbors, go UP
        while (curr != NULL) {
            task* p = curr->parent;
            if (p == NULL) {
                curr = NULL; // We are back at init and done
                break;
            }

            indent--;
            // Check if the parent has another neighbor we haven't visited
            if (curr->neighbor != p->next) {
                curr = curr->neighbor;
                break;
            }

            // Otherwise, keep climbing
            curr = p;
            if (curr == main_task) {
                curr = NULL; // Finished the whole tree
                break;
            }
        }
    }

    system_int_on();
}

void cmd_kill(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: kill <pid>\n");
        return;
    }

    uint32_t pid = atoi(args);
    kill_task(pid);
}

void cmd_peek(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: peek <pid>\n");
        return;
    }

    uint32_t target_pid = atoi(args);
    task* t = get_task(target_pid);

    if (t == NULL) {
        sh_print("Task not found\n");
        return;
    }

    task_registers_t* regs = (task_registers_t*) t->stack_pointer;

    sh_print("Name            %s\n", t->name);
    sh_print("State:          %d\n", t->state);
    if (t->next)
        sh_print("Next task:      %s (%d)\n", t->next->name, t->next->id);
    if (t->parent)
        sh_print("Parent:         %s (%d)\n", t->parent->name, t->parent->id);
    sh_print("Page directory: %p\n", t->page_directory);
    sh_print("Stack origin:   %p\n", t->stack_origin);
    sh_print("EAX: %08x   EBX: %08x   ECX: %08x\n", regs->eax, regs->ebx, regs->ecx);
    sh_print("EDX: %08x   ESI: %08x   EDI: %08x\n", regs->edx, regs->esi, regs->edi);
    sh_print("EIP: %08x   EBP: %08x   ESP: %08x\n", regs->eip, regs->ebp, t->stack_pointer);
}
