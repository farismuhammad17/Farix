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

#include <stdint.h>

#include "arch/arm32/defs.h"
#include "arch/stubs.h"
#include "process/task.h"

#include "cpu/timer.h"

uint32_t timer_interval = 0;

void init_timer(uint32_t frequency) {
    // frequency of 100Hz = 10,000 microseconds per tick
    timer_interval = 1000000 / frequency;

    // Set the first alarm
    uint32_t current_val = inw(TIMER_CLO);
    outw(TIMER_C1, current_val + timer_interval);
}
