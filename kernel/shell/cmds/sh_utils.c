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
    sh_print("memstat: This function is unsupported in the current version due to bugs in sh_print.\n");
    return;

    // Disable interrupts to prevent the scheduler from
    // switching tasks while we use the heap.
    system_int_off();

    sh_print("Heap Start: %p | End: %p\n", heap_start, heap_end);
    sh_print("----------------------------------------------------------------------\n");
    sh_print("Address    | Size      | Status | Caller Address\n");
    sh_print("----------------------------------------------------------------------\n");

    size_t total_kb  = 0;
    size_t heap_used = 0;

    HeapSegment* current = first_segment;

    size_t total_segs = 0;

    while (current != NULL) {
        sh_print("%p | %-9lu | %-6s | 0x%08lX\n",
                current,
                current->size,
                current->is_free ? "FREE" : "USED",
                current->caller);

        total_kb += current->size + sizeof(HeapSegment);
        if (!current->is_free) heap_used += current->size;

        current = current->next;
        total_segs++;
    }

    total_kb >>= 10;

    size_t used_kb = heap_used >> 10;
    size_t free_kb = total_kb - used_kb;

    int usage_pct = (total_kb > 0) ? (int)((used_kb * 100) / total_kb) : 0;

    sh_print("----------------------------------------------------------------------\n");
    sh_print("Total Used: %lu bytes\n", heap_used);
    sh_print("----------------------------------------------------------------------\n");

    sh_print("Total memory: %4lu KiB\n", total_kb);
    sh_print("Used memory:  %4lu KiB [%d%%]\n", used_kb, usage_pct);
    sh_print("Free memory:  %4lu KiB\n", free_kb);
    sh_print("Total segments: %d\n", total_segs);

    system_int_on();
}

void cmd_heapstat(UNUSED_ARG const char* args) {
    HeapSegment* current = first_segment;
    size_t count = 0;

    while (current != NULL) {
        // Magic Check
        if (current->magic != HEAP_MAGIC) {
            sh_print("HEAP CORRUPTION: Bad Magic at %p (Val: %lx)\n", current, current->magic);
            return;
        }

        // Alignment Check
        if (((uint32_t) current & 0x3) != 0) {
            sh_print("HEAP CORRUPTION: Unaligned segment pointer %p\n", current);
            return;
        }

        // Pointer Check
        if (current->next != NULL) {
            if (current->next <= current) {
                sh_print("HEAP CORRUPTION: Circular or backwards link at %p -> %p\n", current, current->next);
                return;
            }
            if (current->next->prev != current) {
                sh_print("HEAP CORRUPTION: Broken backlink! %p->next is %p, but that block's prev is %p\n",
                        current, current->next, current->next->prev);
                return;
            }
        }

        current = current->next;
        count++;
    }

    sh_print("Heap status is OK; %d segments verified, no corruption detected.\n", count);
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
