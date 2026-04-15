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

#include <stdio.h>
#include <stdlib.h>

#include "farix.h"

#include "cmds.h"

void cmd_memstat(const char* args) {
    if (!args) {
        printf("Usage: memstat <total segments>\n");
        return;
    }

    size_t max_segs = atoi(args);
    if (max_segs <= 0 || max_segs > 1024) {
        printf("memstat: Please specify 1-1024 total segments.\n");
        return;
    }

    HeapData* entries = malloc(max_segs * sizeof(HeapData));
    if (!entries) {
        printf("memstat: Out of memory when allocating output space.\n");
        return;
    }

    SYSTEM_INT_OFF(); // Might be a safety problem.
    int count = GET_HEAP_DATA(entries, max_segs);
    SYSTEM_INT_ON();

    printf("Heap Start: %p | End: %p\n", GET_HEAP_START(), GET_HEAP_END());
    printf("----------------------------------------------------------------------\n");
    printf("Address    | Size      | Status | Caller Address\n");
    printf("----------------------------------------------------------------------\n");

    size_t total_kb  = 0;
    size_t heap_used = 0;

    for (size_t i = 0; i < count; i++) {
        HeapData* current = (HeapData*) &entries[i];

        printf("%p | %-9lu | %-6s | 0x%08lX\n",
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

    printf("----------------------------------------------------------------------\n");
    printf("Total Used: %lu bytes\n", heap_used);
    printf("----------------------------------------------------------------------\n");

    printf("Total memory:     %lu KiB\n", total_kb);
    printf("Used memory:      %lu KiB [%d%%]\n", used_kb, usage_pct);
    printf("Free memory:      %lu KiB\n", free_kb);
    printf("Segments counted: %d\n", count);
}
