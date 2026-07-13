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

#include "apic.h"

#include "cpu/timer.h"
#include "drivers/acpi/acpi.h"

#include "cpu/multicore.h"

/* All Excluding Self: Bits 18-19 set to 11b -> 0xC0000 */
#define IPI_ALL_BUT_SELF             0x000C0000

#define IPI_CMD_BROADCAST_INIT       (IPI_ALL_BUT_SELF | 0x0000C500)
#define IPI_CMD_BROADCAST_STARTUP    (IPI_ALL_BUT_SELF | 0x0000C600)

/* 0x08 * 4096 = Physical Address 0x8000 */
#define TRAMPOLINE_PAGE_VECTOR       0x08

// These hold the data collected during the early MADT parse pass
uint8_t  core_apic_ids[MAX_CORES];
uint32_t core_count = 0;

/* Broadcasts the INIT signal to every secondary core on the system bus at once. */
static void broadcast_init_ipi() {
    lapic_write(LAPIC_REG_ICR_LOW, IPI_CMD_BROADCAST_INIT);
}

/* Broadcasts the STARTUP signal along with the execution vector page to everyone at once. */
static void broadcast_startup_ipi(uint8_t vector) {
    lapic_write(LAPIC_REG_ICR_LOW, IPI_CMD_BROADCAST_STARTUP | vector);
}

void init_multicore() {
    // If the system only has 1 core, nothing to do
    if (unlikely(core_count <= 1)) return;

    // Reset all application processors simultaneously
    broadcast_init_ipi();
    timer_stall(10000); // 10 ms flat delay for the whole system

    // Wake all application processors up to hit the 0x8000 trampoline
    broadcast_startup_ipi(TRAMPOLINE_PAGE_VECTOR);
    timer_stall(1000);  // 1 ms flat delay for the whole system
}
