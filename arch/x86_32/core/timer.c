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

#include "hal.h"

#include "process/task.h"

#define PIT_FREQ_HZ  1193182
#define PIT_FREQ_MHZ 1.193

static uint64_t system_ticks = 0;
static uint64_t timer_freq = NULL;
static uint32_t divisor = NULL;

void timer_handler() {
    system_ticks++;

    irq_send_eoi();
    schedule(); // Swap tasks
}

// TODO: Remove magic numbers
void init_timer(uint32_t frequency) {
    divisor = PIT_FREQ_HZ / frequency;
    timer_freq = frequency;

    outb(0x43, 0x36);

    uint8_t low  = (uint8_t) (divisor & 0xFF);
    uint8_t high = (uint8_t) ((divisor >> 8) & 0xFF);

    outb(0x40, low);
    outb(0x40, high);
}

uint64_t get_timer_uptime_microseconds() {
    return (system_ticks * 1000000ULL) / timer_freq;
}

void timer_stall(uint32_t microseconds) {
    if (microseconds <= 0) return;

    uint32_t total_ticks = microseconds * PIT_FREQ_MHZ;
    uint32_t elapsed_ticks = 0;

    // Latch and read the initial counter value
    outb(0x43, 0x00);
    uint16_t last_value = inb(0x40);
    last_value |= (inb(0x40) << 8);

    while (elapsed_ticks < total_ticks) {
        // Latch again to read the current value
        outb(0x43, 0x00);
        uint16_t current_value = inb(0x40);
        current_value |= (inb(0x40) << 8);

        // The PIT counts DOWN.
        if (current_value < last_value) {
            elapsed_ticks += (last_value - current_value);
        }
        else if (current_value > last_value) {
            // The counter wrapped around (hit 0 and reset to your divisor)
            elapsed_ticks += (last_value + (divisor - current_value));
        }

        last_value = current_value;

        // Keep the CPU in a low-power "spin" while waiting
        __asm__ volatile("pause");
    }
}
