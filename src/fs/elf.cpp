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

#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "cpu/tss.h"
#include "fs/vfs.h"

#include "fs/elf.h"

extern "C" uint32_t stack_top; // From boot.s
uint32_t kernel_stack_top = (uint32_t) &stack_top;

bool elf_load_file(std::string path) {
    // Read everything into kernel memory first
    elf_header_t header;
    if (!fs_read(path, &header, sizeof(elf_header_t))) return false;

    File* file_obj = fs_get(path);
    uint8_t* file_buffer = (uint8_t*) kmalloc(file_obj->size);
    fs_read(path, file_buffer, file_obj->size);

    // Get program headers from the buffer
    elf_program_header_t* phdr = (elf_program_header_t*)(file_buffer + header.e_phoff);

    uint32_t* user_pd = (uint32_t*) pmm_alloc_page();
    kmemset(user_pd, 0, PAGE_SIZE);
    for(int i = 0; i < 1024; i++) user_pd[i] = kernel_directory[i];

    for (int i = 0; i < header.e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint32_t pages = (phdr[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
            for (uint32_t p = 0; p < pages; p++) {
                void* phys = pmm_alloc_page();
                void* virt = (void*)(phdr[i].p_vaddr + (p * PAGE_SIZE));
                vmm_map_page(user_pd, phys, virt, PAGE_PRESENT | PAGE_RW | PAGE_USER);
            }
        }
    }

    // Map User Stack
    uint32_t user_stack_top = 0xC0000000;
    vmm_map_page(user_pd, pmm_alloc_page(), (void*)(user_stack_top - PAGE_SIZE), PAGE_PRESENT | PAGE_RW | PAGE_USER);

    vmm_switch_directory(user_pd);

    for (int i = 0; i < header.e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            kmemcpy((void*) phdr[i].p_vaddr, file_buffer + phdr[i].p_offset, phdr[i].p_filesz);

            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                kmemset((void*)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
            }
        }
    }

    tss_entry.esp0 = (uint32_t) &stack_top;

    printf("Entry: %x, Stack: %x\n", header.e_entry, user_stack_top);
    jump_to_user_mode(header.e_entry, user_stack_top);
    return true;
}
