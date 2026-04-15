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

#include "farix.h"

#include "cmds.h"

void cmd_peek(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: peek <pid>\n");
        return;
    }

    uint32_t target_pid = atoi(args);

    TaskData t;
    if (GET_TASK_DATA(target_pid, &t) == SYS_ERROR) {
        sh_print("Task not found\n");
        return;
    }

    sh_print("Name            %s\n", t.name);
    sh_print("State:          %d\n", t.state);

    if (t.parent_id) {
        TaskData t_parent;
        GET_TASK_DATA(t.parent_id, &t_parent);

        sh_print("Parent:         %s (%d)\n", t_parent.name, t_parent.id);
    }

    if (t.next_id) {
        TaskData t_next;
        GET_TASK_DATA(t.next_id, &t_next);

        sh_print("Next task:      %s (%d)\n", t_next.name, t_next.id);
    }

    sh_print("Page directory: %p\n", t.page_dir);
    sh_print("Stack origin:   %p\n", t.stack_origin);
    sh_print("EAX: %08x   EBX: %08x   ECX: %08x\n", t.eax, t.ebx, t.ecx);
    sh_print("EDX: %08x   ESI: %08x   EDI: %08x\n", t.edx, t.esi, t.edi);
    sh_print("EIP: %08x   EBP: %08x   ESP: %08x\n", t.eip, t.ebp, t.stack_ptr);
}
