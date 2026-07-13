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

#ifndef TSS_H
#define TSS_H

#include <stdint.h>

typedef struct TSSEntry {
    uint32_t reserved0;
    uint64_t rsp0;      // Privilege Stack Table (Ring 0 Stack)
    uint64_t rsp1;      // Ring 1 Stack
    uint64_t rsp2;      // Ring 2 Stack
    uint64_t reserved1;
    uint64_t ist1;      // Interrupt Stack Table 1
    uint64_t ist2;      // Interrupt Stack Table 2
    uint64_t ist3;      // Interrupt Stack Table 3
    uint64_t ist4;      // Interrupt Stack Table 4
    uint64_t ist5;      // Interrupt Stack Table 5
    uint64_t ist6;      // Interrupt Stack Table 6
    uint64_t ist7;      // Interrupt Stack Table 7
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base; // Offset from TSS base to IO Permission Bitmap
} __attribute__((packed)) TSSEntry;

extern TSSEntry tss_entry;

void RARE_FUNC init_tss(uint32_t idx, uint32_t kss, uint64_t krsp);

#endif
