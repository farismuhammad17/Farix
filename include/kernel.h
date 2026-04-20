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

#ifndef KERNEL_H
#define KERNEL_H

#define THREAD_HZ 100

// Debug to know the last init
extern char* last_init;
extern char* last_call;

#define LOG_CALL() last_call = __func__

// Reorders the machine code to place these accordingly
#define likely(x)   __builtin_expect(!!(x), 1)  // x is more likely to be true than false
#define unlikely(x) __builtin_expect(!!(x), 0)  // x is more likely to be false than true
#define FREQ_FUNC   __attribute__((hot))        // Frequently called functions
#define RARE_FUNC   __attribute__((cold))       // Infrequently called functions

void RARE_FUNC early_kmain();
void RARE_FUNC kmain();

void RARE_FUNC AcpiMappingCleanup();

#endif
