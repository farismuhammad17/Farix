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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "farix.h"

#include "cmds.h"

void cmd_help(const char* args) {
    for (size_t i = 0; command_table[i].name != NULL; i++) {
        sh_print("%-*s %s\n",
            INDENT_LEN * 2, // *2 because some commands are longer than INDENT_LEN
            command_table[i].name,
            command_table[i].help_text);
    }
}

void cmd_echo(const char* args) {
    if (args[0] == '\0') return;

    sh_print("%s\n", args);
}

void cmd_secho(const char* args) {
    if (args[0] == '\0') return;

    UART_PUTS(args);
    UART_PUTS("\n\r");
}

void cmd_clear(UNUSED_ARG const char* args) {
    printf("\033[2J\033[H%s> ", shell_directory);
}

void cmd_memstat(const char* args) {
    if (args[0] == '\0') {
        sh_print("Usage: memstat <total segments>\n");
        return;
    }

    size_t max_segs = atoi(args);
    if (max_segs <= 0 || max_segs > 1024) {
        sh_print("memstat: Please specify 1-1024 total segments.\n");
        return;
    }

    HeapData* entries = malloc(max_segs * sizeof(HeapData));
    if (!entries) {
        sh_print("memstat: Out of memory when allocating output space.\n");
        return;
    }

    SYSTEM_INT_OFF(); // Might be a safety problem.
    int count = GET_HEAP_DATA(entries, max_segs);
    SYSTEM_INT_ON();

    sh_print("Heap Start: %p | End: %p\n", GET_HEAP_START(), GET_HEAP_END());
    sh_print("----------------------------------------------------------------------\n");
    sh_print("Address    | Size      | Status | Caller Address\n");
    sh_print("----------------------------------------------------------------------\n");

    size_t total_kb  = 0;
    size_t heap_used = 0;

    for (size_t i = 0; i < count; i++) {
        HeapData* current = (HeapData*) &entries[i];

        sh_print("%p | %-9lu | %-6s | 0x%08lX\n",
                current,
                current->size,
                current->is_free ? "FREE" : "USED",
                current->caller);

        total_kb += current->size;
        if (!current->is_free) heap_used += current->size;
    }

    free(entries);

    total_kb = (total_kb + count * GET_HEAP_SEG_SIZE()) >> 10;

    size_t used_kb = heap_used >> 10;
    size_t free_kb = total_kb - used_kb;

    int usage_pct = (total_kb > 0) ? (int)((used_kb * 100) / total_kb) : 0;

    sh_print("----------------------------------------------------------------------\n");
    sh_print("Total Used: %lu bytes\n", heap_used);
    sh_print("----------------------------------------------------------------------\n");

    sh_print("Total memory:     %lu KiB\n", total_kb);
    sh_print("Used memory:      %lu KiB [%d%%]\n", used_kb, usage_pct);
    sh_print("Free memory:      %lu KiB\n", free_kb);
    sh_print("Segments counted: %d\n", count);
}

void cmd_heapstat(UNUSED_ARG const char* args) {
    int fault_addr;
    int status = HEAP_AUDIT(fault_addr);

    switch (status) {
        case 0: sh_print("Heap verified\n");
            break;
        case 1: sh_print("HEAP CORRUPTION: Bad Magic at %p", fault_addr);
            break;
        case 2: sh_print("HEAP CORRUPTION: Unaligned segment pointer %p\n", fault_addr);
            break;
        case 3: sh_print("HEAP CORRUPTION: Circular or backwards link at %p", fault_addr);
            break;
        case 4: sh_print("HEAP CORRUPTION: Broken backlink at %p", fault_addr);
            break;

        default:
            sh_print("heapstat: Returns unknown status %d", status);
            break;
    }
}

void cmd_int(const char* args) {
    if (args[0] == '\0') {
        sh_print("int: Interrupt ID unspecified.");
        return;
    }

    SYSTEM_INT_EXEC(atoi(args));
}

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

void cmd_grep(const char* args) {
    if (last_cmd_output[0] == '\0') {
        sh_print("grep: No input to search.\n");
        return;
    }
    else if (args == NULL || args[0] == '\0') {
        sh_print("grep: Usage: <command> | grep <pattern>\n");
        return;
    }

    char* start = last_cmd_output;
    char* end = strchr(start, '\n');

    while (end != NULL) {
        char saved_char = *end;
        *end = '\0';

        if (strstr(start, args) != NULL) {
            sh_print("%s\n", start);
        }

        *end = saved_char;
        start = end + 1;

        end = strchr(start, '\n');
    }

    if (*start != '\0' && strstr(start, args) != NULL) {
        sh_print("%s\n", start);
    }
}
