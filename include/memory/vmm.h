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

#ifndef VMM_H
#define VMM_H

#include <stdbool.h>
#include <stdint.h>

#define VMM_INIT_MAP_SIZE_MB 32

#define PAGE_OFFSET    0xFFFFFFFF80000000ULL
#define USER_STACK_TOP 0x00007FFFFFFFF000ULL

#define PHYSICAL_TO_VIRTUAL(addr) ((void*)((uint64_t)(addr) + PAGE_OFFSET))
#define VIRTUAL_TO_PHYSICAL(addr) ((uint64_t)(uintptr_t)(addr) - PAGE_OFFSET)

extern const uint64_t PAGE_PRESENT;
extern const uint64_t PAGE_RW;
extern const uint64_t PAGE_USER;
extern const uint64_t PAGE_CACHE;
extern const uint64_t PAGE_PWT;
extern const uint64_t PAGE_PCD;

extern uint64_t* kernel_directory;

void RARE_FUNC init_vmm();

void     vmm_map_page(uint64_t* pd_phys, void* phys, void* virt, uint64_t flags);
uint64_t vmm_unmap_page(void* virt);

uint64_t* RARE_FUNC vmm_copy_kernel_directory();
void      RARE_FUNC vmm_switch_directory(uint64_t* page_directory);

uint64_t* RARE_FUNC vmm_get_current_directory();
uintptr_t RARE_FUNC vmm_get_phys(uint64_t* pd_phys, void* virt_addr);
int       RARE_FUNC vmm_is_mapped(uint64_t* pd_phys, void* virt);

#endif
