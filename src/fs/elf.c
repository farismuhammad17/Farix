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

#include <stdio.h>
#include <string.h>

#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "process/task.h"
#include "cpu/tss.h"
#include "fs/vfs.h"

#include "fs/elf.h"

extern uint32_t stack_top; // From boot.s
extern void elf_user_trampoline_stub(uint32_t entry, uint32_t stack);

static void elf_user_trampoline() {
    task* t = current_task;

    vmm_switch_directory(t->page_directory);
    set_kernel_stack((uint32_t) t->stack_origin + 4096);

    uint32_t entry_point = (uint32_t) t->entry_func;
    uint32_t user_stack  = (uint32_t) t->stack_base;

    elf_user_trampoline_stub(entry_point, user_stack);

    while(1); // Should never reach here
}

bool exec(const char* path) {
    File* file_obj = fs_get(path);
    if (!file_obj || file_obj->size == 0) {
        printf("ELF Error: File %s not found or empty\n", path);
        return false;
    }

    uint8_t* file_buffer = (uint8_t*) kmalloc(file_obj->size);
    if (!file_buffer) {
        printf("ELF Error: Failed to allocate file buffer\n");
        return false;
    }

    if (!fs_read(path, file_buffer, file_obj->size)) {
        printf("ELF Error: Failed to read file data\n");
        kfree(file_buffer);
        return false;
    }

    elf_header_t* header = (elf_header_t*) file_buffer;

    if (header->e_ident[0] != ELFMAG0 || // An ELF file always starts with 0x7F, then 'E', 'L', and 'F'
        header->e_ident[1] != ELFMAG1 ||
        header->e_ident[2] != ELFMAG2 ||
        header->e_ident[3] != ELFMAG3)
    {
        printf("ELF Error: Not a valid ELF executable\n");
        kfree(file_buffer);
        return false;
    }

    uint32_t* current_pd_phys = vmm_get_current_directory();
    uint32_t* user_pd_phys = (uint32_t*) pmm_alloc_page();

    if (!user_pd_phys) {
        printf("ELF Error: Failed to allocate page directory\n");
        kfree(file_buffer);
        return false;
    }

    uint32_t* current_pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(current_pd_phys);
    uint32_t* user_pd_virt_handle = (uint32_t*) PHYSICAL_TO_VIRTUAL(user_pd_phys);

    for (int i = 0; i < 768; i++)    user_pd_virt_handle[i] = 0;
    for (int i = 768; i < 1024; i++) user_pd_virt_handle[i] = current_pd_virt[i];

    elf_program_header_t* phdr = (elf_program_header_t*)(file_buffer + header->e_phoff);

    uint32_t highest_vaddr = 0;
    vmm_switch_directory(user_pd_phys);

    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint32_t start_vaddr = phdr[i].p_vaddr & ~0xFFF;
            uint32_t end_vaddr   = (phdr[i].p_vaddr + phdr[i].p_memsz + 0xFFF) & ~0xFFF;

            if (end_vaddr > highest_vaddr) {
                highest_vaddr = end_vaddr;
            }

            for (uint32_t v = start_vaddr; v < end_vaddr; v += PAGE_SIZE) {
                void* phys = pmm_alloc_page();

                vmm_map_page(user_pd_phys, phys, (void*) v, PAGE_PRESENT | PAGE_RW | PAGE_USER);
                kmemset((void*) PHYSICAL_TO_VIRTUAL(phys), 0, PAGE_SIZE);
            }

            if (phdr[i].p_filesz > 0) {
                kmemcpy((void*) phdr[i].p_vaddr, (void*)(file_buffer + phdr[i].p_offset), phdr[i].p_filesz);
            }
        }
    }

    vmm_switch_directory(current_pd_phys);

    kfree(file_buffer);

    void*    stack_phys = pmm_alloc_page();
    uint32_t stack_virt_addr = USER_STACK_TOP - PAGE_SIZE;

    vmm_map_page(user_pd_phys, stack_phys, (void*) stack_virt_addr, PAGE_PRESENT | PAGE_RW | PAGE_USER);

    uint32_t* stack_ptr = (uint32_t*) PHYSICAL_TO_VIRTUAL(stack_phys);
    for (int i = 0; i < 1024; i++) stack_ptr[i] = 0;

    task* elf_task = (task*) kmalloc(sizeof(task));
    kmemset(elf_task, 0, sizeof(task));

    elf_task->id             = next_pid++;
    elf_task->page_directory = user_pd_phys;
    elf_task->heap_break     = highest_vaddr;
    elf_task->state          = TASK_READY;

    strncpy(elf_task->name, path, 31);
    elf_task->name[31] = '\0';

    elf_task->entry_func     = (void(*)()) header->e_entry;
    elf_task->stack_base     = (uint32_t*) USER_STACK_TOP;

    uint32_t* stack = (uint32_t*) kmalloc(4096);
    uint32_t* esp   = stack + 1024;

    *(--esp) = (uint32_t) elf_user_trampoline;
    for (int i = 0; i < 8; i++) *(--esp) = 0;

    elf_task->stack_pointer = (uint32_t) esp;
    elf_task->stack_origin  = stack;

    elf_task->next     = current_task->next;
    current_task->next = elf_task;
    total_tasks++;

    return true;
}
