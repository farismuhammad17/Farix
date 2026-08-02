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

#include <stdint.h>

#include "initboot.h"

#include "gdt.h"
#include "multiboot.h"
#include "pic.h"
#include "tss.h"

#include "cpu/pci.h"
#include "drivers/terminal.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "kernel.h"

multiboot_info* INITBOOT_DAT_SECTION mbi = NULL;

/* Defined in asm/boot/crti.asm */
void _init();

static void init_initboot_blob() {
    if (unlikely(!(mbi->flags & (1 << 3)))) {
        err_print("init_initboot_blob: No modules flags set");
        return;
    }

    if (unlikely(mbi->mods_count == 0)) {
        err_print("init_initboot_blob: No modules found in mbi->mods_count");
        return;
    }

    multiboot_mod_list_t* mods = (multiboot_mod_list_t*) PHYSICAL_TO_VIRTUAL(mbi->mods_addr);
    initboot_blob = (void*) PHYSICAL_TO_VIRTUAL(mods[0].mod_start);
}

/*
x86 specific initialisations, called right between early_kmain
and kmain, both of which are architecture independant.
*/
void INITBOOT_TXT_SECTION arch_kmain(uint64_t magic, uint64_t mbi_phys) {
    early_kmain();

    if (unlikely((uint32_t) magic != MULTIBOOT_BOOTLOADER_MAGIC)) {
        err_print("arch_kmain: Invalid Multiboot Magic Number!");
        while (1) asm volatile("hlt");
    }

    mbi = (multiboot_info*) PHYSICAL_TO_VIRTUAL(mbi_phys);

    init_pic();
    init_pci();

    init_pmm();
    init_vmm();

    init_initboot_blob();

    init_gdt();

    // Initialise the TSS structure at index 5 (takes slots 5 and 6)
    uint64_t current_rsp;
    asm volatile("mov %%rsp, %0" : "=r"(current_rsp));
    init_tss(5, 0x10, current_rsp);

    _init();

    kmain();
}
