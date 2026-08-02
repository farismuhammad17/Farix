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

#ifndef KLIB_UTILS_H
#define KLIB_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Binary ---

// Find the first set bit scanning from the right
#define find_first_set_bit_right(x) ((int32_t)((x) ? __builtin_ctzll((uint64_t)(x)) : 0))

// Find the first set bit scanning from the left
#define first_first_set_bit_left(x) ((int32_t)((x) ? 63 - __builtin_clzll((uint64_t)(x)) : 0))

static inline bool is_power_of_2(uint64_t n) {
    return (n != 0) && ((n & (n - 1)) == 0);
}

static inline uint64_t round_up_to_power_of_2(uint64_t n) {
    if (n == 0) return 1;

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    return n + 1;
}

// --- Structures ---

#define array_size(arr) (sizeof(arr) / sizeof((arr)[0]))

// --- Math ---

#define min(x, y) ({ \
    typeof(x) _x = (x); \
    typeof(y) _y = (y); \
    (_x < _y) ? _x : _y; \
})

#define max(x, y) ({ \
    typeof(x) _x = (x); \
    typeof(y) _y = (y); \
    (_x > _y) ? _x : _y; \
})

#define clamp(val, lo, hi) (min(max(val, lo), hi))

// --- Alignment ---

static inline uintptr_t align_up(uintptr_t val, uintptr_t align) {
    return (val + (align - 1)) & ~(align - 1);
}

static inline uintptr_t align_down(uintptr_t val, uintptr_t align) {
    return val & ~(align - 1);
}

static inline uint64_t div_round_up(uint64_t n, uint64_t d) {
    return (n + d - 1) / d;
}

#endif
