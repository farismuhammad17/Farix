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

#include "cpu/ints.h"

extern uint32_t _vector_table;

void init_interrupts() {
    uint32_t addr = (uint32_t) &_vector_table;

    // Set VBAR
    asm volatile("mcr p15, 0, %0, c12, c0, 0" : : "r"(addr));

    // Clear SCTLR.V (bit 13) to ensure we use VBAR, not 0xFFFF0000
    uint32_t sctlr;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr &= ~(1 << 13);
    asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(sctlr));
}
