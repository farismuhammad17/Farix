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

#include <stdbool.h>
#include <stdint.h>

#include "drivers/acpi/acpi.h"
#include "drivers/terminal.h"

#include "syshw/thermal.h"

// Controls the hardware fan state (On / Off)
ACPI_STATUS system_set_fan_state(bool turn_on) {
    // We target the standard motherboard active cooling device node.
    // _ON and _OFF do not take parameters and do not return anything.
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT,
        (char*) (turn_on ? "\\_SB.COOL.FAN0._ON" : "\\_SB.COOL.FAN0._OFF"),
        NULL, NULL);

    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_set_fan_state: Failed to set fan state to %d (Status: %s)",
                   turn_on, AcpiFormatException(status));
    }

    return status;
}

// strength: 0 = off, 100 = maximum speed
ACPI_STATUS system_set_fan_speed(uint32_t strength) {
    // _FSL requires exactly one input argument: the integer speed level.
    ACPI_OBJECT arg_obj;
    arg_obj.Type = ACPI_TYPE_INTEGER;
    arg_obj.Integer.Value = (uint64_t) strength;

    // Package the argument object into the ACPI execution list wrapper
    ACPI_OBJECT_LIST args;
    args.Count = 1;
    args.Pointer = &arg_obj;

    // This passes the speed level directly down to the motherboard firmware.
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_SB.COOL.FAN0._FSL", &args, NULL);

    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_set_fan_speed: Failed to set fan speed to %d%% (Status: %s)",
                   strength, AcpiFormatException(status));
    }

    return status;
}

// Returns raw tenths of Kelvin
ACPI_STATUS system_get_cpu_temperature(uint32_t *temp_kelvin_tenths) {
    // The '_TMP' method doesn't take any inputs,
    // so we tell ACPICA that the argument count is 0.
    ACPI_OBJECT_LIST args = { .Count = 0, .Pointer = NULL };

    ACPI_BUFFER buffer = { .Length = ACPI_ALLOCATE_BUFFER, .Pointer = NULL };

    // Ask ACPICA to traverse the AML namespace tree to find '\_TZ.TZ00._TMP',
    // parse the underlying bytecode, execute it, and dump the output into our buffer.
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_TZ.TZ00._TMP", &args, &buffer);

    // Check if the AMl execution or namespace lookup completely failed.
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_get_cpu_temperature: AML evaluation failed (Status: %s)", AcpiFormatException(status));

        if (buffer.Pointer) AcpiOsFree(buffer.Pointer);
        return status;
    }

    ACPI_OBJECT *obj = (ACPI_OBJECT*) buffer.Pointer;

    if (likely(obj && obj->Type == ACPI_TYPE_INTEGER)) {
        *temp_kelvin_tenths = (uint32_t) obj->Integer.Value;
    } else {
        // If execution hits this branch, the firmware violated the ACPI spec.
        err_printf("system_get_cpu_temperature: Object type mismatch: %d",
                   obj ? obj->Type : 0);

        // Update our status to a type error so the caller knows the parsing failed
        status = AE_TYPE;
    }

    AcpiOsFree(buffer.Pointer);

    return status;
}

// Critical Shutdown Threshold (Returns raw tenths of Kelvin)
// Gets maximum temperature before forced hardware shutdown
ACPI_STATUS system_get_thermal_critical_limit(uint32_t *limit_kelvin_tenths) {
    ACPI_BUFFER buffer = { .Length = ACPI_ALLOCATE_BUFFER, .Pointer = NULL };

    // '_CRT' returns the threshold where the OS *must* instantly turn off power to prevent melting
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_TZ.TZ00._CRT", NULL, &buffer);
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_get_thermal_critical_limit: AML evaluation failed (Status: %s)", AcpiFormatException(status));

        if (buffer.Pointer) AcpiOsFree(buffer.Pointer);
        return status;
    }

    ACPI_OBJECT *obj = (ACPI_OBJECT*) buffer.Pointer;
    if (likely(obj && obj->Type == ACPI_TYPE_INTEGER)) {
        *limit_kelvin_tenths = (uint32_t) obj->Integer.Value;
    } else {
        err_printf("system_get_thermal_critical_limit: Object type mismatch: %d",
            obj ? obj->Type : 0);

        status = AE_TYPE;
    }

    AcpiOsFree(buffer.Pointer);

    return status;
}

// Passive Cooling Threshold (Returns raw tenths of Kelvin)
// Temperature the kernel must cool at without fans
ACPI_STATUS system_get_thermal_passive_limit(uint32_t *limit_kelvin_tenths) {
    ACPI_BUFFER buffer = { .Length = ACPI_ALLOCATE_BUFFER, .Pointer = NULL };

    // '_PSV' is the threshold where the OS should engage P-state / C-state throttling
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_TZ.TZ00._PSV", NULL, &buffer);
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_get_thermal_passive_limit: AML evaluation failed (Status: %s)", AcpiFormatException(status));

        if (buffer.Pointer) AcpiOsFree(buffer.Pointer);
        return status;
    }

    ACPI_OBJECT *obj = (ACPI_OBJECT*) buffer.Pointer;
    if (likely(obj && obj->Type == ACPI_TYPE_INTEGER)) {
        *limit_kelvin_tenths = (uint32_t) obj->Integer.Value;
    } else {
        err_printf("system_get_thermal_passive_limit: Object type mismatch: %d",
            obj ? obj->Type : 0);

        status = AE_TYPE;
    }

    AcpiOsFree(buffer.Pointer);

    return status;
}

// Passive Polling Frequency (Returns tenths of a second)
// How often the kernel should check the temperature
ACPI_STATUS system_get_thermal_sampling_period(uint32_t *seconds_tenths) {
    ACPI_BUFFER buffer = { .Length = ACPI_ALLOCATE_BUFFER, .Pointer = NULL };

    // '_TSP' tells the kernel scheduler how often to evaluate passive cooling performance
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_TZ.TZ00._TSP", NULL, &buffer);
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_get_thermal_sampling_period: AML evaluation failed (Status: %s)", AcpiFormatException(status));

        if (buffer.Pointer) AcpiOsFree(buffer.Pointer);
        return status;
    }

    ACPI_OBJECT *obj = (ACPI_OBJECT*) buffer.Pointer;
    if (likely(obj && obj->Type == ACPI_TYPE_INTEGER)) {
        *seconds_tenths = (uint32_t) obj->Integer.Value;
    } else {
        err_printf("system_get_thermal_sampling_period: Object type mismatch: %d",
            obj ? obj->Type : 0);

        status = AE_TYPE;
    }

    AcpiOsFree(buffer.Pointer);
    return status;
}

// Active Cooling Threshold 0 (Returns raw tenths of Kelvin)
// The temperature where the primary cooling fan *must* turn on.
ACPI_STATUS system_get_thermal_active_limit(uint32_t *limit_kelvin_tenths) {
    ACPI_BUFFER buffer = { .Length = ACPI_ALLOCATE_BUFFER, .Pointer = NULL };

    // '_AC0' evaluates the primary temperature trip point for active fan engagement
    ACPI_STATUS status = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_TZ.TZ00._AC0", NULL, &buffer);
    if (unlikely(ACPI_FAILURE(status))) {
        err_printf("system_get_thermal_active_limit: AML evaluation failed (Status: %s)",
                   AcpiFormatException(status));

        if (buffer.Pointer) AcpiOsFree(buffer.Pointer);
        return status;
    }

    ACPI_OBJECT *obj = (ACPI_OBJECT*) buffer.Pointer;
    if (likely(obj && obj->Type == ACPI_TYPE_INTEGER)) {
        *limit_kelvin_tenths = (uint32_t) obj->Integer.Value;
    } else {
        err_printf("system_get_thermal_active_limit: Object type mismatch: %d",
                   obj ? obj->Type : 0);

        status = AE_TYPE;
    }

    AcpiOsFree(buffer.Pointer);
    return status;
}
