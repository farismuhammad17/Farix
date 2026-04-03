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

#ifndef ASM_STUBS_H
#define ASM_STUBS_H

#include <stdint.h>

void     outb (uint16_t port, uint8_t val);
void     outw (uint16_t port, uint16_t val);
uint8_t  inb  (uint16_t port);
uint16_t inw  (uint16_t port);

void system_halt();
void system_int_on();  // Enable interrupts
void system_int_off(); // Disable interrupts
void system_pause();

uint32_t asm_get_random(uint8_t *success);

void cpu_mem_barrier();

void task_yield();

void set_kernel_stack(uint32_t stack);

#endif
