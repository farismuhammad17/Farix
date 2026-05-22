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

#include "drivers/acpi/acpi.h"
#include "drivers/terminal.h"

#include "syshw/battery.h"

uint32_t battery_design_capacity    = 0;
uint32_t battery_last_full_capacity = 0;

/*
Initialise battery constants:-

- battery_design_capacity: How much the battery was intended to hold
- battery_last_full_capacity: How much the battery is capable of actually holding.

As time passes and a battery ages, its capicity slowly reduces. When it comes
out of the factory, battery_last_full_capacity = battery_design_capacity, but slowly
wears down. The design capacity is actually useless for us, but can be a nice
bit of information for the user.
*/
ACPI_STATUS init_battery() {
    ACPI_BUFFER buffer = { .Length = ACPI_ALLOCATE_BUFFER, .Pointer = NULL };

    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_SB.BAT0._BIX", NULL, &buffer);
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("init_battery: AML evaluation failed (%s)", AcpiFormatException(status));

        if (buffer.Pointer) AcpiOsFree(buffer.Pointer);
        return status;
    }

    ACPI_OBJECT *obj = (ACPI_OBJECT*) buffer.Pointer;

    if (likely(obj && obj->Type == ACPI_TYPE_PACKAGE && obj->Package.Count >= 16)) {
        ACPI_OBJECT *elements = obj->Package.Elements;

        if (likely(elements[1].Type == ACPI_TYPE_INTEGER && elements[2].Type == ACPI_TYPE_INTEGER)) {
            battery_design_capacity    = (uint32_t) elements[1].Integer.Value;
            battery_last_full_capacity = (uint32_t) elements[2].Integer.Value;
        } else {
            status = AE_TYPE;
        }
    } else {
        status = AE_AML_PACKAGE_LIMIT;
    }

    AcpiOsFree(buffer.Pointer);
    return status;
}

/*
The battery status can be found through the ACPICA through just one command, so
instead of having separate functions to call the same command through the ACPI,
it is much more efficient to send the command once, and store all the data that
comes back from it into a usable struct.
*/
ACPI_STATUS system_get_battery_status(battery_status_t *status_out) {
    ACPI_BUFFER buffer = { .Length = ACPI_ALLOCATE_BUFFER, .Pointer = NULL };

    // BAT0 is the standard primary battery destination in the ACPI System Bus (\_SB)
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_SB.BAT0._BST", NULL, &buffer);

    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_get_battery_status: AML evaluation failed (Status: %s)", AcpiFormatException(status));

        if (buffer.Pointer) AcpiOsFree(buffer.Pointer);
        return status;
    }

    ACPI_OBJECT *obj = (ACPI_OBJECT*) buffer.Pointer;

    // Verify we received a multi-element PACKAGE array
    if (likely(obj && obj->Type == ACPI_TYPE_PACKAGE)) {

        // _BST must return exactly 4 integers per the ACPI specification
        if (unlikely(obj->Package.Count < 4)) {
            err_printf("system_get_battery_status: Package structure limit error (Elements: %d)",
                       obj->Package.Count);

            status = AE_AML_PACKAGE_LIMIT;
            AcpiOsFree(buffer.Pointer);
            return status;
        }

        ACPI_OBJECT *elements = obj->Package.Elements;

        // Ensure the hardware indices contain valid, parsable integers
        if (likely(elements[0].Type == ACPI_TYPE_INTEGER &&
                   elements[1].Type == ACPI_TYPE_INTEGER &&
                   elements[2].Type == ACPI_TYPE_INTEGER &&
                   elements[3].Type == ACPI_TYPE_INTEGER)) {

            // Populating the struct directly from the raw package offsets
            status_out->state              = (uint32_t) elements[0].Integer.Value;
            status_out->present_rate       = (uint32_t) elements[1].Integer.Value;
            status_out->remaining_capacity = (uint32_t) elements[2].Integer.Value;
            status_out->voltage            = (uint32_t) elements[3].Integer.Value;

        } else {
            err_printf("system_get_battery_status: Data type mismatch inside package layout");
            status = AE_TYPE;
        }

    } else {
        err_printf("system_get_battery_status: Object type mismatch. Expected PACKAGE, got %d",
                   obj ? obj->Type : 0);

        status = AE_TYPE;
    }

    // Free the dynamic heap allocation immediately after extraction
    AcpiOsFree(buffer.Pointer);
    return status;
}
