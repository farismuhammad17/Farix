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

#ifndef MULTICORE_H
#define MULTICORE_H

#include <stdint.h>

#include "hal.h"

#define MAX_CORES 256

typedef volatile uint32_t spinlock;

extern uint8_t  core_apic_ids[MAX_CORES];
extern uint32_t core_count;

void RARE_FUNC init_multicore();

/* Lock given spinlock */
static inline void spin_lock(spinlock *lock) {
    while (__sync_lock_test_and_set(lock, 1)) {
        system_pause();
    }
}

/* Unlock given spinlock */
static inline void spin_unlock(spinlock *lock) {
    __sync_lock_release(lock);
}

#endif
