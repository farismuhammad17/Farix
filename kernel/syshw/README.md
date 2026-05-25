System hardware is mostly managed using the ACPI

# Power

Shutdown, power states, reboot, etc. are all here.

```c
void system_set_power_state(uint8_t state);
```

Changes the system's power state, state being a value from S0 to S5, inclusive.

| ACPI State | Description |
| --- | --- |
| **`ACPI_STATE_S0` (Working)** | The system is fully awake. Components power up and down dynamically (via P/C-states). |
| **`ACPI_STATE_S1` (Positional Sleep)** | The CPU stops executing instructions (clocks are halted), but the CPU, RAM, and chipset keep full power. Wakeup is almost instant. |
| **`ACPI_STATE_S2` (Deeper Sleep)** | The CPU is completely powered off. System RAM and core motherboard chipsets remain powered. Rarely implemented by hardware vendors. |
| **`ACPI_STATE_S3` (Suspend to RAM)** | The CPU, cache, and most motherboard chipsets lose power completely. Only the RAM stays alive in a low-power self-refresh mode. |
| **`ACPI_STATE_S4` (Hibernate)** | Everything loses power. The entire RAM context must be written to non-volatile storage before entering this state. |
| **`ACPI_STATE_S5` (Shutdown)** | Complete system power-off. No memory state is saved anywhere. The machine requires a cold boot sequence to start again. |

# Thermal

Handles active cooling, dynamic fan speeds, and monitoring temperature thresholds.

## Fan Control

```c
ACPI_STATUS system_set_fan_state(bool turn_on);
ACPI_STATUS system_set_fan_speed(uint32_t strength);
```

Manages hardware active cooling components. The functions target `\_SB.COOL.FAN0` via AML evaluation.

* **`Set fan state`**: Evaluates native `_ON` or `_OFF` methods. These do not accept parameter inputs.
* **`Set fan speed`**: Passes an explicit integer value directly to the firmware using the `_FSL` (Fan Set Level) method. `0` is completely off, and `100` is maximum speed.

## Getters

```c
ACPI_STATUS system_get_cpu_temperature(uint32_t *temp_kelvin_tenths);
ACPI_STATUS system_get_thermal_critical_limit(uint32_t *limit_kelvin_tenths);
ACPI_STATUS system_get_thermal_passive_limit(uint32_t *limit_kelvin_tenths);
ACPI_STATUS system_get_thermal_active_limit(uint32_t *limit_kelvin_tenths);
ACPI_STATUS system_get_thermal_sampling_period(uint32_t *seconds_tenths);

```

Queries active zones (`\_TZ.TZ00`) within the hardware namespace to monitor safety thresholds and manage task scheduling.

| ACPI Object | Output Unit | Description |
| --- | --- | --- |
| **`_TMP` (Current Temp)** | Tenths of Kelvin | Current operational temperature reading of the target hardware core zone. |
| **`_CRT` (Critical Limit)** | Tenths of Kelvin | Safety threshold limit. Reaching this forces immediate emergency motherboard hardware power down. |
| **`_PSV` (Passive Limit)** | Tenths of Kelvin | Threshold where the scheduler must engage P-state/C-state processor throttling routines. |
| **`_AC0` (Active Limit)** | Tenths of Kelvin | Core temperature point where active hardware cooling systems (fans) must switch on. |
| **`_TSP` (Sampling Period)** | Tenths of a Second | Optimal checking frequency interval recommended for the kernel thermal polling loop. |

# Battery

Handles power subsystem status monitoring, charge metrics, and battery hardware capacity profiling.

## Static Hardware Constants

```c
extern uint32_t battery_design_capacity;
extern uint32_t battery_last_full_capacity;

ACPI_STATUS init_battery();
```

Caches unchanging physical hardware properties at system boot to avoid redundant AML tree evaluation during runtime status updates. Targets the primary battery node `\_SB.BAT0`. It evaluates the `_BIX` (Battery Information Extended) multi-element package to extract original design metrics and physical degradation updates.

| Global Variable | Data Unit | Description |
| --- | --- | --- |
| **`battery_design_capacity`** | mWh or mAh | Factory-specified maximum capacity of the battery when brand new. |
| **`battery_last_full_capacity`** | mWh or mAh | Maximum capacity the cells can physically hold now on a full charge cycle. |

---

## Dynamic State Parsing

```c
ACPI_STATUS system_get_battery_status(battery_status_t *status_out);
```

Queries live hardware power metrics on-demand by fetching and parsing dynamic ACPI packages from `\_SB.BAT0._BST`. It unpacks a 4-element integer package containing the immediate power drain, state, and terminal voltage properties.

| Struct Field (`battery_status_t`) | Representation | Description |
| --- | --- | --- |
| **`state`** | Bitmask | Power source mode: `0x1` = Discharging, `0x2` = Charging, `0x4` = Critical. |
| **`present_rate`** | mW or mA | Live power draw or input rate. |
| **`remaining_capacity`** | mWh or mAh | Current remaining charge left in the battery cells. |
| **`voltage`** | mV | Precise voltage across the battery terminals. |
