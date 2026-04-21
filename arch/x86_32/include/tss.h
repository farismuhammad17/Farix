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

#ifndef TSS_H
#define TSS_H

typedef struct TSSEntry {
    uint32_t prev_tss;
    uint32_t esp0;     // Kernel stack pointer
    uint32_t ss0;      // Kernel stack segment
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed)) TSSEntry;

extern TSSEntry tss_entry;

void RARE_FUNC init_tss(uint32_t idx, uint32_t kss, uint32_t kesp);
void FREQ_FUNC set_kernel_stack(uint32_t stack);

#endif
