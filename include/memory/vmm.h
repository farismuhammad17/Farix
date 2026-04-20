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

#include <stdbool.h>
#include <stdint.h>

#define VMM_INIT_MAP_SIZE 32 // MB

#define PAGE_OFFSET    0xC0000000 // 3 GB
#define USER_STACK_TOP 0xC0000000

#define VIRTUAL_TO_PHYSICAL(addr) ((uintptr_t)(addr) - PAGE_OFFSET)
#define PHYSICAL_TO_VIRTUAL(addr) ((void*)((uintptr_t)(addr) + PAGE_OFFSET))

extern const uintptr_t PAGE_PRESENT;
extern const uintptr_t PAGE_RW;
extern const uintptr_t PAGE_USER;
extern const uintptr_t PAGE_CACHE;

extern uint32_t* kernel_directory;

void RARE_FUNC init_vmm();

void      vmm_map_page(uint32_t* pd_phys, void* phys, void* virt, uint32_t flags);
uint32_t  vmm_unmap_page(void* virt);

uint32_t* RARE_FUNC vmm_copy_kernel_directory();
void      RARE_FUNC vmm_switch_directory(uint32_t* page_directory);

uint32_t* RARE_FUNC vmm_get_current_directory();
uint32_t  RARE_FUNC vmm_get_phys(uint32_t* pd_phys, void* virt_addr);
int       RARE_FUNC vmm_is_mapped(uint32_t* pd_phys, void* virt);

#endif
