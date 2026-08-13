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

#include <stddef.h>

#include "klib/ctype.h"
#include "klib/stdio.h"
#include "klib/string.h"
#include "klib/utils.h"

#include "hal.h"

#include "cpu/ints.h"
#include "cpu/irq.h"
#include "cpu/pci.h"
#include "drivers/terminal.h"
#include "drivers/vfs.h"
#include "fs/types/elf.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "process/task.h"

#include "sysmods/devices.h"
#include "sysmods/interface.h"
#include "sysmods/loader.h"

loaded_sysmod_t sysmods_registry[MAX_SYSMODS] = {NULL};

kernel_api_t sysmod_kernel_api = {
    .printf = printf,
    .err_printf = err_printf,
    .err_print = err_print,
    .vsnprintf = vsnprintf,

    .outb = outb,
    .outw = outw,
    .outl = outl,
    .inb = inb,
    .inw = inw,
    .inl = inl,

    .pmm_alloc_page = pmm_alloc_page,
    .pmm_alloc_pages = pmm_alloc_pages,
    .vmm_map_page = vmm_map_page,
    .vmm_get_current_directory = vmm_get_current_directory,
    .kmalloc = kmalloc,
    .kfree = kfree,
    .memset = memset,
    .memcpy = memcpy,
    .memcmp = memcmp,
    .strrchr = strrchr,
    .strchr = strchr,
    .strcmp = strcmp,
    .strncpy = strncpy,
    .strlen = strlen,
    .toupper = toupper,

    .register_interrupt = register_interrupt,
    .unregister_interrupt = unregister_interrupt,
    .irq_send_eoi = irq_send_eoi,
    .irq_mask = irq_mask,
    .irq_unmask = irq_unmask,

    .schedule = schedule,

    .pci_read = pci_read,
    .pci_write = pci_write,
    .pci_devices = pci_devices,

    .register_device = register_device,
    .unregister_device = unregister_device,

    .get_device = get_device
};

static int find_free_module_slot() {
    for (int i = 0; i < MAX_SYSMODS; i++) {
        if (unlikely(!sysmods_registry[i].is_active)) {
            return i;
        }
    }

    return -1;
}

int load_sysmod(const char* path) {
    File* file_obj = vfs->get(path);
    if (unlikely(!file_obj || file_obj->size == 0)) {
        err_printf("load_sysmod: File %s not found or empty", path);
        return -1;
    }

    uint8_t* buffer = (uint8_t*) kmalloc(file_obj->size);
    if (unlikely(!buffer)) {
        err_print("load_sysmod: Out of memory");
        return -2;
    }

    if (unlikely(!vfs->read(path, buffer, file_obj->size, 0))) {
        err_print("load_sysmod: Failed to read file data");
        kfree(buffer);
        return -3;
    }

    // Pass the raw buffer into your tracking registry assignment loop
    int slot = load_sysmod_raw((void*) buffer, file_obj->size);
    if (unlikely(slot < 0)) {
        err_printf("load_sysmod: Binary loading failed (%d)", -slot);
        kfree(buffer);
        return -3 + slot;
    }

    return slot;
}

int load_sysmod_raw(uint8_t* file_buffer, size_t file_size) {
    elf_header_t* header = (elf_header_t*) file_buffer;

    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E'  ||
        header->e_ident[2] != 'L'  ||
        header->e_ident[3] != 'F'
    ) {
        err_print("load_sysmod_raw: Not a valid ELF file");
        return -1;
    }

    elf_program_header_t* phdr = (elf_program_header_t*)(file_buffer + header->e_phoff);

    // Find size of the executable binary
    uint64_t min_vaddr = 0xFFFFFFFFFFFFFFFF;
    uint64_t max_vaddr = 0;

    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;
        if (phdr[i].p_vaddr < min_vaddr) min_vaddr = phdr[i].p_vaddr;
        uint64_t end = phdr[i].p_vaddr + phdr[i].p_memsz;
        if (end > max_vaddr) max_vaddr = end;
    }
    size_t total_mem_size = max_vaddr - min_vaddr;

    // Create execution block
    uint8_t* base_addr = (uint8_t*) kmalloc(total_mem_size);
    if (unlikely(!base_addr)) {
        err_print("load_sysmod_raw: Failed to allocate runtime memory");
        return -2;
    }
    memset(base_addr, 0, total_mem_size);

    // Write the executable segments into block
    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;

        uint8_t* segment_dest = base_addr + (phdr[i].p_vaddr - min_vaddr);
        memcpy(segment_dest, file_buffer + phdr[i].p_offset, phdr[i].p_filesz);
    }

    // Pointer relocation
    elf_section_header_t* shdrs = (elf_section_header_t*)(file_buffer + header->e_shoff);
    elf_section_header_t* strtab_shdr = &shdrs[header->e_shstrndx];
    const char* shstrtab = (const char*)(file_buffer + strtab_shdr->sh_offset);

    for (int i = 0; i < header->e_shnum; i++) {
        const char* sec_name = shstrtab + shdrs[i].sh_name;

        if (strncmp(sec_name, ".rela.", 6) == 0) {
            elf_rela_t* relas = (elf_rela_t*)(file_buffer + shdrs[i].sh_offset);
            int rela_count = shdrs[i].sh_size / sizeof(elf_rela_t);

            for (int j = 0; j < rela_count; j++) {
                uint32_t type = ELF64_R_TYPE(relas[j].r_info);

                // r_offset is the virtual address within the module
                uint8_t* patch_loc = base_addr + (relas[j].r_offset - min_vaddr);

                if (type == R_X86_64_64 || type == 8) { // Added R_X86_64_RELATIVE (8)
                    uint64_t* val_ptr = (uint64_t*) patch_loc;
                    *val_ptr = (uint64_t) base_addr + relas[j].r_addend;
                } else if (type == R_X86_64_32S) {
                    int32_t* val_ptr = (int32_t*) patch_loc;
                    int64_t computed = (int64_t) base_addr + relas[j].r_addend;
                    *val_ptr = (int32_t) computed;
                }
            }
        }
    }

    // Find system module header
    uint64_t sysmod_header_vaddr = 0;
    for (int i = 0; i < header->e_shnum; i++) {
        const char* sec_name = shstrtab + shdrs[i].sh_name;
        if (strcmp(sec_name, ".sysmod_header") == 0) {
            sysmod_header_vaddr = shdrs[i].sh_addr;
            break;
        }
    }

    if (unlikely(sysmod_header_vaddr == 0)) {
        err_print("load_sysmod_raw: '.sysmod_header' section not found in ELF");
        kfree(base_addr);
        return -3;
    }

    sysmod_t* mod = (sysmod_t*)(base_addr + (sysmod_header_vaddr - min_vaddr));

    int slot = find_free_module_slot();
    if (unlikely(slot == -1)) {
        err_print("load_sysmod_raw: Failed to find free slot");
        kfree(base_addr);
        return -4;
    }

    sysmods_registry[slot].interface = mod;
    sysmods_registry[slot].base_address = base_addr;
    sysmods_registry[slot].size = total_mem_size;
    sysmods_registry[slot].is_active = 1;

    int result = mod->init(&sysmod_kernel_api);

    if (unlikely(result != 0)) {
        err_printf("load_sysmod_raw: Module init returned non-0 value (%d)", result);
        sysmods_registry[slot].is_active = 0;
        kfree(base_addr);
        return -5;
    }

    return slot;
}

int unload_sysmod(int slot_id) {
    if (unlikely(slot_id < 0 || slot_id >= MAX_SYSMODS || !sysmods_registry[slot_id].is_active)) {
        err_print("unload_sysmod: Invalid slot ID or inactive module");
        return -1;
    }

    loaded_sysmod_t* mod_track = &sysmods_registry[slot_id];
    sysmod_t* mod = mod_track->interface;

    // Call internal exit
    if (mod && mod->exit) {
        int result = mod->exit();

        if (unlikely(result != 0)) {
            err_printf("unload_sysmod: Module exit returned non-0 value (%d)", result);
            return -2;
        }
    }

    // Clear the registry tracking slot
    mod_track->is_active = 0;
    mod_track->interface = NULL;

    // Free the dynamically allocated execution memory block
    if (likely(mod_track->base_address)) {
        kfree(mod_track->base_address);
        mod_track->base_address = NULL;
    }

    return 0;
}
