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

#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_PRESENT  0x1   // 01 in binary  - If page is in RAM
#define PAGE_RW       0x2   // 10 in binary  - 0 = Read-only,   1 = Read/Write
#define PAGE_USER     0x4   // 100 in binary - 0 = Kernel only, 1 = Everyone

#define ENTRIES_PER_TABLE 1024

void init_vmm();

void vmm_map_page(void* phys, void* virt, uint32_t flags);
void vmm_switch_directory(uint32_t* directory);

#endif
