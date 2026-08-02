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

#ifndef KLIB_DEBUG_H
#define KLIB_DEBUG_H

#include "klib/stdio.h"

#include "hal.h"

// --- Assertion Checking Functions ---

// Checks the condition, if false, throw error, and stop the kernel
#define ASSERT(cond) \
    if (unlikely(!(cond))) { \
        err_printf("%s (%s:%d): Assert %s", \
               __func__, __FILE__, __LINE__, #cond); \
        while(1) system_halt(); \
    }

// Checks the condition, if false, throw error
#define WARNON(cond) \
    if (unlikely(!(cond))) { \
        err_printf("%s (%s:%d): Warn %s", \
               __func__, __FILE__, __LINE__, #cond); \
    }

// Makes the compiler throw an error if a condition is not met during compilation
#define ASSERT_COMPILE(cond) ((void) sizeof(char[1 - 2*!!(cond)]))

// --- Time Profiling Functions ---

// Start time profile
#define PROFILE_START(name) \
    uint64_t _debug_profile_start_##name = get_cpu_cycles();

// End time profile and print result
#define PROFILE_END(name) \
    uint64_t _debug_profile_end_##name = get_cpu_cycles(); \
    printf("Profile %s took %lu cycles\n", #name, _debug_profile_end_##name - _debug_profile_start_##name);

// --- Dumps ---

static inline void dump_bytes(const void *ptr, size_t num_bytes) {
    const unsigned char* byte_ptr = (const unsigned char*) ptr;

    for (size_t i = 0; i < num_bytes; i++) {
        printf("%02x ", byte_ptr[i]);
    }

    printf("\n");
}

#endif
