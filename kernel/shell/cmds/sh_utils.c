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
#include <string.h>

#include "arch/stubs.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/heap.h"
#include "shell/shell.h"

#include "shell/commands.h"

void cmd_help(UNUSED_ARG const char* args) {
    for (size_t i = 0; command_table[i].name != NULL; i++) {
        sh_print("%-*s %s\n",
                INDENT_LEN * 2, // *2 because some commands are longer than 4 characters
                command_table[i].name,
                command_table[i].help_text);
    }
}

void cmd_clear(UNUSED_ARG const char* args) {
    terminal_clear();
}

void cmd_echo(const char* args) {
    sh_print("%s\n", args);
}

void cmd_secho(const char* args) {
    if (!args) return;

    while (*args) {
        if (*args == '\n')
            uart_putc('\r');
        uart_putc(*args++);
    }

    // Always terminate with a fresh line on the serial side
    uart_putc('\r');
    uart_putc('\n');
}

void cmd_memstat(UNUSED_ARG const char* args) {
    // Disable interrupts to prevent the scheduler from
    // switching tasks while we use the heap.
    system_int_off();

    sh_print("Heap Start: %p | End: %p\n", heap_start, heap_end);
    sh_print("----------------------------------------------------------------------\n");
    sh_print("Address    | Size      | Status | Caller Address\n");
    sh_print("----------------------------------------------------------------------\n");

    size_t total_kb  = get_heap_total() >> 10;
    size_t heap_used = get_heap_used();

    size_t used_kb = heap_used >> 10;
    size_t free_kb = total_kb - used_kb;

    HeapSegment* current = first_segment;

    int usage_pct = (total_kb > 0) ? (int)((used_kb * 100) / total_kb) : 0;
    while (current != NULL) {
        sh_print("%p | %-9lu | %-6s | 0x%08lX\n",
                current,
                current->size,
                current->is_free ? "FREE" : "USED",
                current->caller);
        current = current->next;
    }

    sh_print("----------------------------------------------------------------------\n");
    sh_print("Total Used: %lu bytes\n", heap_used);
    sh_print("----------------------------------------------------------------------\n");

    sh_print("Total memory: %4lu KiB\n", total_kb);
    sh_print("Used memory:  %4lu KiB [%d%%]\n", used_kb, usage_pct);
    sh_print("Free memory:  %4lu KiB\n", free_kb);

    system_int_on();
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
