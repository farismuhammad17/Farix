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

#define __DEBUG__ 0

#ifdef __DEBUG__
    #define MAX_LOG_LEN 30

    // Just a number you can set for debugging purposes
    extern int logged_num;

    extern const char* call_log[MAX_LOG_LEN];
    extern int log_index;
    extern int last_call_finished;

    static inline void start_call_log(const char* func_name) {
        log_index = (log_index + 1) % MAX_LOG_LEN;
        call_log[log_index] = func_name;
        last_call_finished = 0;
    }

    static inline void end_call_log(void* res) {
        (void) res;
        last_call_finished = 1;
    }

    #define LOG_CALL() \
        start_call_log(__func__); \
        int _sentinel __attribute__((cleanup(end_call_log))) = 0

    #define LOG_NUM(n) (logged_num = (n))
#else
    #define LOG_CALL()
    #define LOG_NUM()
#endif

// Reorders the machine code to place these accordingly
#define likely(x)   __builtin_expect(!!(x), 1)  // x is more likely to be true than false
#define unlikely(x) __builtin_expect(!!(x), 0)  // x is more likely to be false than true
#define FREQ_FUNC   __attribute__((hot))        // Frequently called functions
#define RARE_FUNC   __attribute__((cold))       // Infrequently called functions

void RARE_FUNC early_kmain();
void RARE_FUNC kmain();

#endif
