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

#include "hal.h"

#include "drivers/acpi/acpi.h"
#include "drivers/terminal.h"

#include "syshw/power.h"

/* Moves the system into a complete shutdown (S5) */
void system_set_power_s5() {
    // S5 state is shutdown
    ACPI_STATUS status = AcpiEnterSleepStatePrep(ACPI_STATE_S5);

    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_shutdown: Sleep preparation failed (Status: %s)", AcpiFormatException(status));
        return;
    }

    // Interrupts might interrupts the shutdown
    system_int_off();

    // Actually shutdown
    status = AcpiEnterSleepState(ACPI_STATE_S5);

    // If execution reaches this point, the motherboard hardware failed to drop power
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_shutdown: S5 state transition failed (Status: %s)", AcpiFormatException(status));
        return;
    }

    err_print("system_shutdown: Shutdown failed, halting CPU...");
    while (1) system_halt();
}

/* Executes a reboot via the ACPI */
void system_reboot() {
    // Reset command
    ACPI_STATUS status = AcpiReset();

    // If execution reaches this point, we didn't reboot
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_reboot: Reboot failed (Status: %s)", AcpiFormatException(status));
        return;
    }

    err_print("system_reboot: Reboot failed, halting CPU...");
    while (1) system_halt();
}
