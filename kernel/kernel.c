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

#include "hal.h"

#include "cpu/ints.h"
#include "cpu/irq.h"
#include "cpu/timer.h"
#include "drivers/acpi/acpi.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/storage/bdl.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "fs/elf.h"
#include "fs/fat32.h"
#include "fs/ramdisk.h"
#include "fs/vfs.h"
#include "memory/heap.h"
#include "process/task.h"

#include "kernel.h"

const char* call_log[MAX_LOG_LEN] = {0};
int log_index = 0;
int last_call_finished = 0;

// Called before the architecture initialisations. Everything here
// must be so raw; it should not depend on literally anything else.
void early_kmain() {
    terminal_clear_phys();

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

    AcpiInitializeSubsystem();
    AcpiInitializeTables(NULL, 16, FALSE);
    AcpiLoadTables();
    AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);

    init_irq_controller();

    init_multitasking();
    init_timer(THREAD_HZ);

    init_keyboard();
    init_mouse();

    init_storage();

    // Enable interrupts
    system_int_on();

    init_ramdisk();
    init_fat32();

    vfs_mount(&fat32_ops); // TODO: One day have a proper disk file system like EXT2 or FAT32 and mount onto it instead

    create_task(handle_mouse, "Terminal mouse handler", 0);

    task* shelf_task = exec_elf("shelf.elf");
    shelf_task->privilege = PRIV_SUPER;

    // The OS must NEVER die.
    // Interrupts take back control from this loop whenever
    // they are called, so the OS is never stuck in the
    // while loop forever.
    while (1) system_halt();
}
