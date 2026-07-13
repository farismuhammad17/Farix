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

#include <stdint.h>

#include "hal.h"

#include "cpu/irq.h"
#include "drivers/terminal.h"

#include "process/task.h"

#define PIT_FREQ_HZ  1193182

static uint64_t system_ticks = 0;
static uint64_t timer_freq   = 0;
static uint64_t divisor      = 65535;

void timer_handler() {
    system_ticks++;

    irq_send_eoi();
    schedule(); // Swap tasks
}

// TODO: Remove magic numbers
void init_timer(uint64_t frequency) {
    if (unlikely(frequency == 0)) {
        err_print("init_timer: Frequency set to 0");
        return;
    }

    divisor = PIT_FREQ_HZ / frequency;
    timer_freq = frequency;

    outb(0x43, 0x36);

    uint8_t low  = (uint8_t) (divisor & 0xFF);
    uint8_t high = (uint8_t) ((divisor >> 8) & 0xFF);

    outb(0x40, low);
    outb(0x40, high);
}

uint64_t get_timer_uptime_microseconds() {
    if (unlikely(timer_freq == 0)) return 0;
    return (system_ticks * 1000000ULL) / timer_freq;
}

void timer_stall(uint64_t microseconds) {
    if (unlikely(microseconds == 0)) return;

    uint64_t total_ticks = (microseconds * PIT_FREQ_HZ) / 1000000;
    uint64_t elapsed_ticks = 0;

    // Latch and read the initial counter value
    outb(0x43, 0x00);
    uint16_t last_value = inb(0x40);
    last_value |= (inb(0x40) << 8);

    while (elapsed_ticks < total_ticks) {
        outb(0x43, 0x00);
        uint16_t current_value = inb(0x40);
        current_value |= (inb(0x40) << 8);

        if (current_value < last_value) {
            elapsed_ticks += (last_value - current_value);
        }
        else if (current_value > last_value) {
            elapsed_ticks += (last_value + (divisor - current_value));
        }

        last_value = current_value;
        asm volatile("pause");
    }
}
