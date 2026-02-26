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

#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#include "architecture/multiboot.h"

#define PAGE_SIZE   4096
#define BITMAP_SIZE 32768   // 32768 integers * 32 bits * 4096 bytes = 4GB of RAM management.

void init_pmm(multiboot_info* mbi);

void* pmm_alloc_page();
void  pmm_free_page(void* addr);

#endif
