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

#include <stdlib.h>

#include "hal.h"

#include "process/task.h"

#include "shell/commands.h"
#include "shell/shell.h"

/* List task tree command */
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

/* Kill task command */
void cmd_kill(const char* args) {
    if (unlikely(args[0] == '\0')) {
        sh_print("Usage: kill <pid>\n");
        return;
    }

    uint32_t pid = atoi(args);
    kill_task(pid);
}

/* View tasks in task list command */
void cmd_tlist(const char* args) {
    task_list* list = first_task_list;

    if (args[0] == '\0') {
        size_t list_id = 0;

        do {
            sh_print("%d ", list_id);

            for (int i = TASKS_LIST_LEN - 1; i >= 0; i--) {
                sh_print("%d", (list->mask >> i) & 1);
                if (i > 0 && i % 8 == 0) sh_print(" ");
            }

            sh_print("\n");
            list_id++;
        } while (list->next != NULL);
    } else {
        size_t target = atoi(args);

        for (size_t i = 0; i < target; i++) {
            list = list->next;

            if (list == NULL) {
                sh_print("tlist: Only %d total lists\n", i + 1);
                return;
            }
            else if (i == target) break;
        }

        for (int i = 0; i < TASKS_LIST_LEN; i++) {
            if (list->mask & (1 << i)) {
                task* t = list->tasks[i];

                char* state_str = "READY";
                if (t->state == TASK_RUNNING) state_str = "RUNNING";
                if (t->state == TASK_SLEEPING) state_str = "SLEEPING";
                if (t->state == TASK_DEAD) state_str = "DEAD";

                sh_print("%-4ld%-10s%s\n",
                    t->id,
                    state_str,
                    t->name
                );
            }
        }
    }
}
