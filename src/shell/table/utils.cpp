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

#include <string>

#include "drivers/terminal.h"
#include "memory/heap.h"

#include "shell/commands.h"

void cmd_help(const std::string& args) {
    for (size_t i = 0; command_table[i].name != nullptr; i++) {
        printf("%-*s %s\n",
                INDENT_LEN,
                command_table[i].name,
                command_table[i].help_text);
    }
}

void cmd_clear(const std::string& args) {
    terminal_clear();
}

void cmd_echo(const std::string& args) {
    printf("%s\n", args.c_str());
}

void cmd_memstat(const std::string& args) {
    // Disable interrupts to prevent the scheduler from
    // switching tasks while we use the heap.
    asm volatile("cli");

    size_t total_kb = get_heap_total() / 1024;
    size_t used_kb  = get_heap_used()  / 1024;

    asm volatile("sti");

    size_t free_kb  = total_kb - used_kb;

    int usage_pct = (total_kb > 0) ? (int)((used_kb * 100) / total_kb) : 0;

    printf("Memory Statistics (KiB):\n");
    printf("------------------------\n");
    printf("Total memory: %8u KiB\n", total_kb);
    printf("Used memory:  %8u KiB [%d%%]\n", used_kb, usage_pct);
    printf("Free memory:  %8u KiB\n", free_kb);
}
