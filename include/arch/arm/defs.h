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

#ifndef ARM_DEFS_H
#define ARM_DEFS_H

#define TIMER_BASE      0x3F003000
#define TIMER_CS        (TIMER_BASE + 0x00) // Control/Status
#define TIMER_CLO       (TIMER_BASE + 0x04) // Counter Low 32-bits
#define TIMER_C1        (TIMER_BASE + 0x10) // Compare register 1

#include <stdint.h>

extern uint32_t timer_interval;

#endif
