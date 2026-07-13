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

#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE       4096

// 64 bits per entry. To track 128 GB of RAM:
// (128 * 1024 * 1024 * 1024) / 4096 bytes per page / 64 bits per entry = 524,288 entries
#define PMM_BITMAP_SIZE 524288

void RARE_FUNC init_pmm();

void* pmm_alloc_page();
void* pmm_alloc_pages(size_t length);
void  pmm_free_page(void* addr);

#endif
