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

#include "hal.h"

#include "drivers/acpi/acpi.h"
#include "drivers/terminal.h"

#include "syshw/power.h"

/*
Sets device power state, from D0 to D3, inclusive.

* D0 (Fully On):
The device is completely powered, its clock signals are active, and it is
fully responsive to software commands. Waking up from any lower D-state
requires transitioning here via the device's native '._PS0' ACPI method.
* D1 (Light Standby):
An optional, light low-power state designed to save power during short
idle periods by reducing internal clock speeds. The device retains its
internal configuration state, but requires a brief recovery delay before
processing data again. Evaluated via '._PS1' (rarely implemented).
* D2 (Deeper Sleep):
An optional, deeper low-power state with a longer recovery delay than D1.
It reduces power further by clock-gating or lowering internal voltages
while still preserving configuration data. Evaluated via '._PS2' (rarely
implemented).
* D3 (Off):
Evaluated via the '._PS3' method. Depending on the device and motherboard
wiring, this transitions the device into one of two sub-states:
- D3hot (Software Off): The device is logically off and clocks are cut,
but the main power rail remains energized so it can still respond to
software configuration scans.
- D3cold (Absolute Off): Power is completely cut off from the underlying
hardware slot's electrical plane. The device receives zero electricity,
vanishes from the system bus, and drops all internal state context.
*/
ACPI_STATUS device_set_power_state(uint8_t state, const char *device_path) {
    char method_path[256];
    strcpy(method_path, device_path);

    switch (state) {
        case 3: {
            strcat(method_path, "._PS3");
            break;
        }
        case 2: {
            strcat(method_path, "._PS2");
            break;
        }
        case 1: {
            strcat(method_path, "._PS1");
            break;
        }
        case 0: {
            strcat(method_path, "._PS0");
            break;
        }
        default: {
            err_printf("device_set_power_state: Unsupported or invalid D%d state target", state);
            return AE_BAD_PARAMETER;
        }
    }

    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, method_path, NULL, NULL);

    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("device_set_power_state: Failed %s (%s)",
                   method_path, AcpiFormatException(status));
    }

    return status;
}

/*
Changes the system's power state, state being a value from S0 to S5, inclusive.

* S0 (Working):  The system is fully awake. Components power up and down dynamically
(via P/C-states).
* S1 (Positional Sleep):  The CPU stops executing instructions (clocks are halted),
but the CPU, RAM, and chipset keep full power. Wakeup is almost instant.
* S2 (Deeper Sleep):  The CPU is completely powered off. System RAM and core
motherboard chipsets remain powered. Rarely implemented by hardware vendors.
* S3 (Suspend to RAM):  The CPU, cache, and most motherboard chipsets lose power
completely. Only the RAM stays alive in a low-power self-refresh mode.
* S4 (Hibernate):  Everything loses power. The entire RAM context must be written
to non-volatile storage before entering this state.
* S5 (Shutdown):  Complete system power-off. No memory state is saved anywhere.
The machine requires a cold boot sequence to start again.
*/
void system_set_power_state(uint8_t state) {
    uint8_t acpi_state;

    switch (state) {
        case 5: {
            acpi_state = ACPI_STATE_S5;
            break;
        }
        case 4: {
            acpi_state = ACPI_STATE_S4;
            break;
        }
        case 3: {
            acpi_state = ACPI_STATE_S3;
            break;
        }
        case 2: {
            acpi_state = ACPI_STATE_S2;
            break;
        }
        case 1: {
            acpi_state = ACPI_STATE_S1;
            break;
        }
        case 0: {
            acpi_state = ACPI_STATE_S0;
            break;
        }
        default: {
            err_printf("system_set_power_state: Invalid state: %d", state);
            return;
        }
    }

    // S5 state is shutdown
    ACPI_STATUS status = AcpiEnterSleepStatePrep(acpi_state);

    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_set_power_state: S%d preparation failed (Status: %s)", state, AcpiFormatException(status));
        return;
    }

    // Interrupts might interrupts the shutdown
    system_int_off();

    // Actually shutdown
    status = AcpiEnterSleepState(acpi_state);

    // If execution reaches this point, the motherboard hardware failed to drop power
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_set_power_state: S%d state transition failed (Status: %s)", state, AcpiFormatException(status));
        return;
    }

    err_printf("system_set_power_state: S%d failed, halting CPU...", state);
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
