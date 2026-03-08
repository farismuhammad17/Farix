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
#include "process/task.h"
#include "cpu/tss.h"
#include "fs/vfs.h"

#include "fs/elf.h"

extern "C" uint32_t stack_top; // From boot.s

void elf_user_trampoline() {
    task* t = current_task;
    uint32_t entry_point = (uint32_t)t->entry_func;
    uint32_t user_stack = (uint32_t)t->stack_base;

    asm volatile(
        "mov $0x23, %%ax\n"     // User data segment selector
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "pushl $0x23\n"         // SS
        "pushl %1\n"            // ESP
        "pushl $0x202\n"        // EFLAGS (interrupts enabled)
        "pushl $0x1B\n"         // CS
        "pushl %0\n"            // EIP
        "iret\n"
        : : "r"(entry_point), "r"(user_stack) : "memory"
    );
}

bool exec(std::string path) {
    File* file_obj = fs_get(path);
    if (!file_obj || file_obj->size == 0) {
        printf("ELF Error: File %s not found or empty\n", path.c_str());
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
    if (header->e_ident[0] != 0x7F || header->e_ident[1] != 'E' || header->e_ident[2] != 'L' || header->e_ident[3] != 'F') {
        printf("ELF Error: Not a valid ELF executable\n");
        kfree(file_buffer);
        return false;
    }

    uint32_t* user_pd_phys = (uint32_t*) pmm_alloc_page();
    if (!user_pd_phys) {
        printf("ELF Error: Failed to allocate page directory\n");
        kfree(file_buffer);
        return false;
    }

    uint32_t* original_pd = vmm_get_current_directory();
    vmm_map_page(original_pd, user_pd_phys, (void*)VMM_TEMP_PAGE, PAGE_PRESENT | PAGE_RW);
    uint32_t* user_pd_virt_handle = (uint32_t*) VMM_TEMP_PAGE;

    for(int i = 0; i < 1024; i++) user_pd_virt_handle[i] = 0;

    vmm_map_page(original_pd, kernel_directory, (void*)(VMM_TEMP_PAGE + PAGE_SIZE), PAGE_PRESENT | PAGE_RW);
    uint32_t* kernel_pd_virt_handle = (uint32_t*)(VMM_TEMP_PAGE + PAGE_SIZE);

    user_pd_virt_handle[0] = kernel_pd_virt_handle[0];
    for(int i = 768; i < 1024; i++) {
        user_pd_virt_handle[i] = kernel_pd_virt_handle[i];
    }
    vmm_unmap_page(original_pd, (void*)(VMM_TEMP_PAGE + PAGE_SIZE));

    user_pd_virt_handle[1023] = (uint32_t)user_pd_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;

    vmm_unmap_page(original_pd, (void*) VMM_TEMP_PAGE);
    vmm_switch_directory(user_pd_phys);

    elf_program_header_t* phdr = (elf_program_header_t*)(file_buffer + header->e_phoff);
    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            for (uint32_t p = 0; p < phdr[i].p_memsz; p += PAGE_SIZE) {
                void* phys = pmm_alloc_page();
                void* virt = (void*)(phdr[i].p_vaddr + p);
                vmm_map_page(user_pd_phys, phys, virt, PAGE_PRESENT | PAGE_RW | PAGE_USER);

                uint8_t* virt_ptr = (uint8_t*)virt;
                for (int j = 0; j < PAGE_SIZE; j++) virt_ptr[j] = 0;

                uint32_t file_offset = phdr[i].p_offset + p;
                uint32_t size_to_copy = (phdr[i].p_filesz > p) ? (phdr[i].p_filesz - p) : 0;
                if (size_to_copy > PAGE_SIZE) size_to_copy = PAGE_SIZE;

                if (size_to_copy > 0) {
                    kmemcpy(virt, file_buffer + file_offset, size_to_copy);
                }
            }
        }
    }

    void* stack_phys = pmm_alloc_page();
    void* user_stack_base_virt = (void*)(USER_STACK_TOP - PAGE_SIZE);
    vmm_map_page(user_pd_phys, stack_phys, user_stack_base_virt, PAGE_PRESENT | PAGE_RW | PAGE_USER);

    vmm_switch_directory(original_pd);

    uint32_t entry_point = header->e_entry;
    kfree(file_buffer);

    tss_entry.esp0 = (uint32_t) &stack_top;

    task* elf_task = new task();
    elf_task->id             = next_pid++;
    elf_task->page_directory = user_pd_phys;
    elf_task->state          = TASK_READY;
    elf_task->name           = path;
    elf_task->entry_func     = (void(*)())entry_point;
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
