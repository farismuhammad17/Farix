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

#include "arch/stubs.h"
#include "cpu/ints.h"
#include "cpu/timer.h"
#include "drivers/acpi/acpi.h"
#include "drivers/ata.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "fs/elf.h"
#include "fs/fat32.h"
#include "fs/ramdisk.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "process/task.h"
#include "shell/shell.h"

#include "arch/kernel.h"

void shell_thread() {
    init_shell();
    while (1) {
        shell_update();  // Check if a command is ready
        system_halt();
    }
}

// Called before the architecture initialisations. Everything here
// must be so raw, it should not depend on literally anything else.
void early_kmain() {
    init_uart();
}

void kmain() {
    // Each architecture's boot.s calls this function independantly
    // through a seperate arch_kmain, which does all the arch specific
    // checks and initializations before calling this function.
    //
    // This function is a general kernel_main function, and does not
    // care about the architecture it's running on.

    init_interrupts();
    init_heap();
    init_terminal();

    init_keyboard();
    init_mouse();

    init_multitasking();
    init_timer(THREAD_HZ); // 100 Hz, i.e. every 10 ms

    // AcpiInitializeSubsystem();
    // AcpiInitializeTables(NULL, 16, FALSE);
    // AcpiLoadTables();

    // AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    // AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);

    init_ata();


    // Enable interrupts
    system_int_on();

    init_ramdisk();
    init_fat32();

    vfs_mount(&fat32_ops);    // TODO: One day have a proper disk file system like EXT2 or FAT32 and mount onto it instead

    create_task(shell_thread, "Shell", 0);
    create_task(handle_mouse, "Terminal mouse handler", 0);

    // The OS must NEVER die.
    // Interrupts take back control from this loop whenever
    // they are called, so the OS is never stuck in the
    // while loop forever.
    while (1) system_halt();
}
