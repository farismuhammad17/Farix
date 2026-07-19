/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "klib/stdio.h"

#include "hal.h"

#include "drivers/terminal.h"
#include "memory/heap.h"

#include "shell/commands.h"
#include "shell/shell.h"

/* Help command */
void cmd_help(UNUSED_ARG const char* args) {
    for (size_t i = 0; command_table[i].name != NULL; i++) {
        printf("%-*s %s\n",
                INDENT_LEN * 2, // *2 because some commands are longer than 4 characters
                command_table[i].name,
                command_table[i].help_text);
    }
}

/* Clear terminal command */
void cmd_clear(UNUSED_ARG const char* args) {
    terminal_clear();
}

/* Echo to terminal command */
void cmd_echo(const char* args) {
    printf("%s\n", args);
}

/* Echo through UART command */
void cmd_secho(const char* args) {
    if (unlikely(!args)) return;

    // TODO (Undone since WIP)
    // Iterate through output devices
    // Find device with UART_DEV_ID
    // Print using.

    // uart_print(args);

    // // Always terminate with a fresh line on the serial side
    // uart_putc('\r');
    // uart_putc('\n');
}

/* Heap usage command */
void cmd_memstat(UNUSED_ARG const char* args) {
    // Disable interrupts to prevent the scheduler from
    // switching tasks while we use the heap.
    system_int_off();

    printf("Heap Start: %p | End: %p\n", heap_start, heap_end);
    printf("----------------------------------------------------------------------\n");
    printf("Address    | Size      | Status | Caller Address\n");
    printf("----------------------------------------------------------------------\n");

    size_t total_kb  = 0;
    size_t heap_used = 0;

    HeapSegment* current = first_segment;

    size_t total_segs = 0;

    while (current != NULL) {
        printf("%p | %-9lu | %-6s | 0x%08lX\n",
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

    printf("----------------------------------------------------------------------\n");
    printf("Total Used: %lu bytes\n", heap_used);
    printf("----------------------------------------------------------------------\n");

    printf("Total memory: %4lu KiB\n", total_kb);
    printf("Used memory:  %4lu KiB [%d%%]\n", used_kb, usage_pct);
    printf("Free memory:  %4lu KiB\n", free_kb);
    printf("Total segments: %d\n", total_segs);

    system_int_on();
}

/* Heap health checker command */
void cmd_heapstat(UNUSED_ARG const char* args) {
    HeapSegment* current = first_segment;
    size_t count = 0;

    while (current != NULL) {
        // Magic Check
        if (unlikely(current->magic != HEAP_MAGIC)) {
            printf("HEAP CORRUPTION: Bad Magic at %p (Val: %lx)\n", current, current->magic);
            return;
        }

        // Alignment Check
        if (unlikely(((uint32_t) current & 0x3) != 0)) {
            printf("HEAP CORRUPTION: Unaligned segment pointer %p\n", current);
            return;
        }

        // Pointer Check
        if (current->next != NULL) {
            if (unlikely(current->next <= current)) {
                printf("HEAP CORRUPTION: Circular or backwards link at %p -> %p\n", current, current->next);
                return;
            }
            if (unlikely(current->next->prev != current)) {
                printf("HEAP CORRUPTION: Broken backlink! %p->next is %p, but that block's prev is %p\n",
                        current, current->next, current->next->prev);
                return;
            }
        }

        current = current->next;
        count++;
    }

    printf("Heap status is OK; %d segments verified, no corruption detected.\n", count);
}

/* Run given interrupt command */
void cmd_int(const char *args) {
    // TODO: x86 only, use HAL

    // #define CASE(n)   case n: asm volatile("int %0" :: "i"(n)); break;
    // #define REP2(n)   CASE(n) CASE(n+1)
    // #define REP4(n)   REP2(n) REP2(n+2)
    // #define REP8(n)   REP4(n) REP4(n+4)
    // #define REP16(n)  REP8(n) REP8(n+8)
    // #define REP32(n)  REP16(n) REP16(n+16)
    // #define REP64(n)  REP32(n) REP32(n+32)
    // #define REP128(n) REP64(n) REP64(n+64)
    // #define REP256(n) REP128(n) REP128(n+128)

    // switch(atoi(args)) {
    //     REP256(0)
    //     default: break;
    // }

    // #undef CASE
    // #undef REP2
    // #undef REP4
    // #undef REP8
    // #undef REP16
    // #undef REP32
    // #undef REP64
    // #undef REP128
    // #undef REP256
}
