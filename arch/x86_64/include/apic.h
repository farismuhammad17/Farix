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

#ifndef APIC_H
#define APIC_H

#include <stdint.h>

#define LAPIC_REG_ID       0x20
#define LAPIC_REG_ICR_LOW  0x300
#define LAPIC_REG_ICR_HIGH 0x310
#define LAPIC_ID_SHIFT     24

extern uint64_t lapic_virt;
extern uint64_t ioapic_virt;
extern uint8_t  irq0_pin;

void lapic_write(uint32_t reg, uint32_t data);
uint32_t lapic_read(uint32_t reg);

#endif
