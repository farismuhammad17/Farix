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

#ifndef SYS_THERMAL_H
#define SYS_THERMAL_H

#include <stdbool.h>
#include <stdint.h>

#include "drivers/acpi/acpi.h"

ACPI_STATUS system_set_fan_state(bool turn_on);
ACPI_STATUS system_set_fan_speed(uint32_t strength);

ACPI_STATUS system_get_cpu_temperature(uint32_t *temp_kelvin_tenths);
ACPI_STATUS system_get_thermal_critical_limit(uint32_t *limit_kelvin_tenths);
ACPI_STATUS system_get_thermal_passive_limit(uint32_t *limit_kelvin_tenths);
ACPI_STATUS system_get_thermal_sampling_period(uint32_t *seconds_tenths);
ACPI_STATUS system_get_thermal_active_limit(uint32_t *limit_kelvin_tenths);

#endif
