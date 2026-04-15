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
