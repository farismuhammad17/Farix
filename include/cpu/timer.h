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

#ifndef TIMER_H
#define TIMER_H

/*

System Timer

Without a hardware timer, the kernel has no concept of time passing. It wouldn't
be able to multitask, as a single process could hold the CPU forever. To solve this,
we use a hardware timer chip that acts like the heartbeat of the system.

The timer is a hardware device that sends a periodic interrupt signal to the CPU at
a fixed frequency. Every time this "tick" occurs, the CPU pauses current execution
and jumps to the timer interrupt handler. This allows the kernel to regain control,
update system uptime, and decide if it's time to switch to a different task
(preemptive multitasking).

Since timer hardware varies significantly between platforms (e.g., PIT on x86 vs.
Generic Timer on ARM), the implementation is split:

x86_32 : src/arch/x86_32/core/timer.c
arm32 : src/arch/arm32/core/timer.c

void init_timer:
    This function calculates the necessary divisors for the hardware clock to
    fire at the requested frequency (in Hz). It then registers the timer
    interrupt handler so the kernel can begin tracking time and performing
    context switches.

*/

#include <stdint.h>

void init_timer(uint32_t frequency);

#endif
