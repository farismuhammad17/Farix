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

#include <string.h>

#include "hal.h"

#include "drivers/terminal.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "fs/types/elf.h"

// Defined boot.s
extern uint32_t stack_top;

// Defined in elf.asm
void elf_user_trampoline_stub(uint32_t entry, uint32_t stack);

/* ELF trampoline to execute the ELF, then halt after termination */
void elf_user_trampoline() {
    task* t = current_task;

    uint32_t entry_point = (uint32_t) t->entry_func;
    uint32_t user_stack  = (uint32_t) t->stack_origin;

    elf_user_trampoline_stub(entry_point, user_stack);

    while(1) system_halt(); // Should never reach here
}

/*
Gets the ELF file and its contents, stored into a buffer, which gets returned.
The function also verifies if the given file is also a valid ELF file, since
an ELF file is to start with 0x7F, followed by the characters E, L, and F.
*/
static uint8_t* load_elf_file(const char* path, uint32_t* out_size) {
    File* file_obj = fs_get(path);
    if (unlikely(!file_obj || file_obj->size == 0)) {
        err_printf("load_elf_file: File %s not found or empty", path);
        return NULL;
    }

    uint8_t* buffer = (uint8_t*) kmalloc(file_obj->size);
    if (unlikely(!buffer)) {
        err_print("load_elf_file: Out of memory");
        return NULL;
    }

    if (unlikely(!fs_read(path, buffer, file_obj->size, 0))) {
        err_print("load_elf_file: Failed to read file data");
        kfree(buffer);
        return NULL;
    }

    elf_header_t* header = (elf_header_t*) buffer;
    if (unlikely(header->e_ident[0] != 0x7F || header->e_ident[1] != 'E' ||
                 header->e_ident[2] != 'L'  || header->e_ident[3] != 'F')) {
        err_print("load_elf_file: Not a valid ELF executable");
        kfree(buffer);
        return NULL;
    }

    if (out_size) {
        *out_size = file_obj->size;
    }

    return buffer;
}

/*
Allocates a page and writes the contents of the current page directory into
it, marked with PAGE_USER.
*/
static uint32_t* create_user_page_directory() {
    uint32_t* user_pd_phys = (uint32_t*) pmm_alloc_page();
    if (unlikely(!user_pd_phys)) {
        err_print("create_user_page_directory: PMM failed to allocate");
        return NULL;
    }

    uint32_t* current_pd_virt = (uint32_t*) PHYSICAL_TO_VIRTUAL(vmm_get_current_directory());
    uint32_t* user_pd_virt    = (uint32_t*) PHYSICAL_TO_VIRTUAL(user_pd_phys);

    for (int i = 0; i < 1024; i++) {
        if (current_pd_virt[i] != 0) {
            user_pd_virt[i] = current_pd_virt[i] | PAGE_USER;
        } else {
            user_pd_virt[i] = 0;
        }
    }

    return user_pd_phys;
}

/*
Parses an ELF file buffer to allocate and map loadable program segments into a user
page directory. It iterates through the program headers, aligns segment memory to
4KB, and copies the binary's raw data into newly allocated physical frames while
tracking the highest virtual address for future heap placement.
*/
static uint32_t map_elf_segments(uint32_t* user_pd_phys, uint8_t* file_buffer) {
    elf_header_t* header = (elf_header_t*) file_buffer;
    elf_program_header_t* phdr = (elf_program_header_t*)(file_buffer + header->e_phoff);
    uint32_t highest_vaddr = 0;

    system_int_off();

    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;

        uint32_t start_vaddr = phdr[i].p_vaddr & ~0xFFF;
        uint32_t end_vaddr   = (phdr[i].p_vaddr + phdr[i].p_memsz + 0xFFF) & ~0xFFF;

        if (end_vaddr > highest_vaddr) {
            highest_vaddr = end_vaddr;
        }

        for (uint32_t v = start_vaddr; v < end_vaddr; v += PAGE_SIZE) {
            void* phys = pmm_alloc_page();
            vmm_map_page(user_pd_phys, phys, (void*) v, PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_CACHE);

            uint8_t* kernel_vaddr = (uint8_t*) PHYSICAL_TO_VIRTUAL(phys);
            memset(kernel_vaddr, 0, PAGE_SIZE);

            uint32_t segment_start    = phdr[i].p_vaddr;
            uint32_t segment_data_end = phdr[i].p_vaddr + phdr[i].p_filesz;

            uint32_t copy_start = (v > segment_start) ? v : segment_start;
            uint32_t copy_end   = (v + PAGE_SIZE < segment_data_end) ? (v + PAGE_SIZE) : segment_data_end;

            if (copy_start < copy_end) {
                uint32_t offset_in_page = copy_start - v;
                uint32_t offset_in_file = copy_start - segment_start;
                uint32_t len = copy_end - copy_start;

                memcpy(kernel_vaddr + offset_in_page,
                       file_buffer + phdr[i].p_offset + offset_in_file,
                       len);
            }
        }
    }

    system_int_on();

    return highest_vaddr;
}

/*
Creates and initializes a brand new task from an ELF file. It allocates a new
task structure, maps the ELF segments into a new page directory, sets up a fresh
user stack, and returns the new task pointer.
*/
task* exec_elf(const char* path) {
    uint8_t* file_buffer = load_elf_file(path, NULL);
    if (unlikely(!file_buffer)) {
        return NULL;
    }

    uint32_t* user_pd_phys = create_user_page_directory();
    if (unlikely(!user_pd_phys)) {
        kfree(file_buffer);
        return NULL;
    }

    uint32_t highest_vaddr = map_elf_segments(user_pd_phys, file_buffer);
    elf_header_t* header = (elf_header_t*) file_buffer;

    task* elf_task = create_task((void(*)()) header->e_entry, path, PRIV_USER, NULL);
    elf_task->page_directory = user_pd_phys;
    elf_task->heap_break     = highest_vaddr;
    elf_task->stack_origin   = (uint32_t*) USER_STACK_TOP;

    void* stack_phys = pmm_alloc_page();
    uint32_t stack_virt_addr = USER_STACK_TOP - PAGE_SIZE;
    vmm_map_page(user_pd_phys, stack_phys, (void*) stack_virt_addr, PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_CACHE);

    uint32_t* stack_ptr = (uint32_t*) PHYSICAL_TO_VIRTUAL(stack_phys);
    memset(stack_ptr, 0, PAGE_SIZE);

    kfree(file_buffer);
    return elf_task;
}

/*
Replaces the execution context of an existing task with a new ELF file. It maps
the new ELF into a fresh page directory and stack, overwrites the provided task's
memory structures and entry point in-place, and immediately switches to the new
memory space.
*/
bool exec_elf_inplace(const char* path, task* t) {
    uint8_t* file_buffer = load_elf_file(path, NULL);
    if (unlikely(!file_buffer)) {
        return false;
    }

    uint32_t* user_pd_phys = create_user_page_directory();
    if (unlikely(!user_pd_phys)) {
        kfree(file_buffer);
        return false;
    }

    uint32_t highest_vaddr = map_elf_segments(user_pd_phys, file_buffer);
    elf_header_t* header = (elf_header_t*) file_buffer;

    void* stack_phys = pmm_alloc_page();
    uint32_t stack_virt_addr = USER_STACK_TOP - PAGE_SIZE;
    vmm_map_page(user_pd_phys, stack_phys, (void*) stack_virt_addr, PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_CACHE);
    memset(PHYSICAL_TO_VIRTUAL(stack_phys), 0, PAGE_SIZE);

    t->page_directory = user_pd_phys;
    t->heap_break     = highest_vaddr;
    t->entry_func     = (void(*)()) header->e_entry;

    vmm_switch_directory(t->page_directory);

    kfree(file_buffer);
    return true;
}
