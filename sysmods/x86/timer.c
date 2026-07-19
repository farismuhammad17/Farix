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

#include "sysmods/devices.h"
#include "sysmods/interface.h"

#include "cpu/timer.h"

#define PIT_FREQ_HZ  1193182
#define FREQUENCY_HZ 100

static const uint64_t divisor = PIT_FREQ_HZ / FREQUENCY_HZ;

static uint64_t system_ticks = 0;

static kernel_api_t* k_api = NULL;
static timer_dev_t* dev = NULL;

void interrupt_handler() {
    system_ticks++;

    k_api->irq_send_eoi();
    k_api->schedule(); // Swap tasks
}

uint64_t get_timer_uptime_microseconds() {
    return (system_ticks * 1000000ULL) / FREQUENCY_HZ;
}

void stall(uint64_t microseconds) {
    if (unlikely(microseconds == 0)) return;

    uint64_t total_ticks = (microseconds * PIT_FREQ_HZ) / 1000000;
    uint64_t elapsed_ticks = 0;

    // Latch and read the initial counter value
    k_api->outb(0x43, 0x00);
    uint16_t last_value = k_api->inb(0x40);
    last_value |= (k_api->inb(0x40) << 8);

    while (elapsed_ticks < total_ticks) {
        k_api->outb(0x43, 0x00);
        uint16_t current_value = k_api->inb(0x40);
        current_value |= (k_api->inb(0x40) << 8);

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

int init_pit(kernel_api_t* api, uint64_t base_addr) {
    k_api = api;

    api->outb(0x43, 0x36);

    uint8_t low  = (uint8_t) (divisor & 0xFF);
    uint8_t high = (uint8_t) ((divisor >> 8) & 0xFF);

    api->outb(0x40, low);
    api->outb(0x40, high);

    dev = k_api->kmalloc(sizeof(timer_dev_t));
    dev->id = PIT_DEV_ID;

    dev->get_timer_uptime_microseconds = (void*) SYSMOD_TO_KERNEL(get_timer_uptime_microseconds);
    dev->stall = (void*) SYSMOD_TO_KERNEL(stall);

    api->register_device(DEV_TIMER, (void*) dev);

    api->register_interrupt(32, (void*) SYSMOD_TO_KERNEL(interrupt_handler));

    return 0;
}

void exit_pit() {
    // Disable the interrupts
    k_api->outb(0x43, 0x36);
    k_api->outb(0x40, 0xFF); // Set a very slow frequency
    k_api->outb(0x40, 0xFF);

    k_api->unregister_interrupt(32);

    k_api->unregister_device(DEV_TIMER, dev);

    k_api->kfree(dev);
}

SYSMOD_ENTRY sysmod_t test_module_entry = {
    .name = "PIT",
    .init_offset = (uint64_t) init_pit,
    .exit_offset = (uint64_t) exit_pit
};
