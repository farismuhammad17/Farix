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

#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "architecture/multiboot.h"
#include "architecture/io.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/pic.h"
#include "cpu/timer.h"
#include "process/task.h"
#include "drivers/ata.h"
#include "drivers/terminal.h"
#include "shell/shell.h"
#include "fs/vfs.h"
#include "fs/ramdisk.h"
#include "fs/fat32.h"

#define THREAD_HZ 100

void shell_thread() {
    while (1) {
        shell_update();  // Check if a command is ready
        asm volatile("hlt");
    }
}

extern "C" void _init();

extern "C" void kernel_main(uint32_t magic, multiboot_info* mbi) {
    init_gdt();
    init_terminal();

    init_pmm(mbi);
    init_vmm();
    init_heap();

    _init();

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("OS Error: Invalid Multiboot Magic Number");
        while(1) { asm volatile("hlt"); }
    }

    init_idt();
    pic_remap();

    // Enable interrupts
    asm volatile("sti");

    init_ata();

    init_ramdisk();
    init_fat32();

    vfs_mount(&fat32_ops);    // TODO: One day have a proper disk file system like EXT2 or FAT32 and mount onto it instead

    init_multitasking();
    init_timer(THREAD_HZ); // 100 Hz, i.e. every 10 ms

    init_shell();

    create_task(shell_thread, "Shell");

    // The OS must NEVER die.
    // Interrupts take back control from this loop whenever
    // they are called, so the OS is never stuck in the
    // while loop forever.
    while (1) {
        asm volatile("hlt");
    }
}
