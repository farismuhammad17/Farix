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

#include "gdt.h"
#include "multiboot.h"
#include "pic.h"
#include "tss.h"

#include "cpu/pci.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include "kernel.h"

multiboot_info* mbi = NULL;

/* Defined in asm/boot/crti.asm */
void _init();

/*
x86 specific initialisations, called right between early_kmain
and kmain, both of which are architecture independant.
*/
void arch_kmain(uint32_t magic, multiboot_info* _mbi) {
    early_kmain();

    if (unlikely(magic != MULTIBOOT_BOOTLOADER_MAGIC)) {
        err_printf("arch_kmain: Invalid Multiboot Magic Number: %x", magic);
        while(1) asm volatile("hlt");
    }

    mbi = _mbi;

    init_pic();
    init_pci();

    init_pmm();
    init_vmm();

    init_gdt();

    _init();

    kmain();
}
