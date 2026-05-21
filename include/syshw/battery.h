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

#ifndef SYS_BATTERY_H
#define SYS_BATTERY_H

#include <stdint.h>

typedef struct {
    uint32_t state;              // Bitmask: 1 = Discharging, 2 = Charging, 4 = Critical
    uint32_t present_rate;       // Power draw/input rate (mW or mA)
    uint32_t remaining_capacity; // Current charge level (mWh or mAh)
    uint32_t voltage;            // Precise terminal voltage (mV)
} battery_status_t;

// How much the battery was designed to hold
extern uint32_t battery_design_capacity;

// From the factory, battery_last_full_capacity = battery_design_capacity,
// but chemical degradation reduces that battery's total capacity. This is
// how much the battery can actually hold, i.e. how much is inside when the
// battery is "at 100%".
extern uint32_t battery_last_full_capacity;

ACPI_STATUS RARE_FUNC init_battery();

ACPI_STATUS system_get_battery_status(battery_status_t *status_out);

#endif
