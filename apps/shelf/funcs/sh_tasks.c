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

#include "process/task.h"

#include "farix.h"

#include "cmds.h"

void cmd_tasks(const char* args) {
    TaskData info;
    uint32_t current_id = INIT_TASK_ID;
    int indent = 0;

    // Track the first child's ID for each indentation level
    // to detect when we've circled back to the start of the neighbor list.
    uint32_t first_child_at_level[16] = {0};

    while (1) {
        // Is this slow? Yes.
        // Do I care? No, atleast not yet.
        if (GET_TASK_DATA(current_id, &info)) break;

        char* state_str = "READY";
        if (info.state == TASK_RUNNING) state_str = "RUNNING";
        else if (info.state == TASK_SLEEPING) state_str = "SLEEPING";
        else if (info.state == TASK_DEAD) state_str = "DEAD";

        for (int i = 0; i < indent; i++) sh_print("  ");
        sh_print("%s (%u) - %s\n", info.name, info.id, state_str);

        // Try to go DOWN (to children)
        if (info.next_id != 0) {
            first_child_at_level[indent + 1] = info.next_id; // Record the "Start" of this level
            current_id = info.next_id;
            indent++;
            continue;
        }

        // If no children, try to go SIDEWAYS (to neighbors)
        if (info.neighbor_id != 0 && info.neighbor_id != first_child_at_level[indent]) {
             current_id = info.neighbor_id;
             continue;
        }

        bool finished = false;
        while (1) {
            uint32_t p_id = info.parent_id;
            if (p_id == INIT_TASK_ID) {
                current_id = 0; // Back at init and done
                finished = true;
                break;
            }

            indent--;

            // We need parent's info to check neighbor vs parent's first child
            TaskData p_info;
            GET_TASK_DATA(p_id, &p_info);

            // NOTE: first_child_at_level[indent+1] IS p->next from the previous level
            if (info.neighbor_id != first_child_at_level[indent + 1]) {
                current_id = info.neighbor_id;
                finished = true;
                break;
            }

            // Otherwise, keep climbing
            current_id = p_id;
            if (current_id == INIT_TASK_ID) { // back at init
                current_id = 0;
                finished = true;
                break;
            }

            // Update info to parent's info for the next loop of climbing
            info = p_info;
        }

        if (finished) {
            if (current_id == 0) break; // Exit the main loop
            continue; // Start again with the new neighbor we found
        }
    }
}

void cmd_kill(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: kill <pid>\n");
        return;
    }

    TASK_KILL(atoi(args));
}

void cmd_tlist(const char* args) {
    TaskListData data;

    if (args[0] == '\0') {
        for (uint32_t i = 0;; i++) {
            if (GET_TASK_LIST(i, (uint32_t) &data) == SYS_ERROR) break;

            sh_print("%u ", i);
            for (int i = TASKS_LIST_LEN - 1; i >= 0; i--) {
                sh_print("%d", (data.mask >> i) & 1);
                if (i > 0 && i % 8 == 0) sh_print(" ");
            }
            sh_print("\n");
        }
    }

    else {
        uint32_t target = atoi(args);
        if (GET_TASK_LIST(target, (uint32_t) &data) == SYS_ERROR) {
            sh_print("tlist: List %u not found.\n", target);
            return;
        }

        for (int i = 0; i < 16; i++) {
            if (data.mask & (1 << i)) {
                TaskData info;
                if (GET_TASK_DATA(data.pids[i], (uint32_t) &info) == SYS_DONE) {
                    sh_print("%-4u %s\n", info.id, info.name);
                }
            }
        }
    }
}
