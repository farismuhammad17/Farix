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

#include <stdint.h>
#include <stdio.h>

#include "arch/kernel.h"
#include "arch/stubs.h"
#include "arch/x86/pic.h"
#include "arch/x86/gdt.h"
#include "arch/x86/tss.h"
#include "arch/x86/multiboot.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

multiboot_info* mbi = NULL;

// Defined in asm/boot/crti.asm
extern void _init();

void arch_kmain(uint32_t magic, multiboot_info* _mbi) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("OS Error: Invalid Multiboot Magic Number");
        while(1) asm volatile("hlt");
    }

    mbi = _mbi;

    pic_remap();

    init_pmm();
    init_vmm();

    init_gdt();

    _init();

    kernel_main();
}
