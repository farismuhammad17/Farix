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
#include "shell/shell.h"
#include "syshw/battery.h"

#include "kernel.h"

/* Whether to boot into kernel shell or not */
#define BOOT_INTO_KSHELL 1

int logged_num = 0;

const char* call_log[MAX_LOG_LEN] = {0};
int log_index = 0;
int last_call_finished = 0;

/* Kernel shell main loop thread. */
static inline void shell_thread() {
    init_shell();
    while (1) {
        shell_update();
        system_halt();
    }
}

/*
This is called right after boot.s is done, and is just to set up important, arch-
independant items before actually running into arch_kmain. It is mandatory that this
function rely on nothing other than everything before itself, since, at this stage,
nothing exists. It initialises the following:

- terminal_clear_phys: Clears out the terminal trash the BIOS might leave
- init_uart: Debugging needs it
*/
void early_kmain() {
    terminal_clear_phys();

    init_uart();
}

/*
Initialises the kernel. It is called during boot, right after arch_kmain, which is
unique for each processor architecture. The initialisation sequence is as follows:

- init_interrupts
- init_heap
- init_terminal

- ACPI

- init_irq_controller

- init_multitasking
- init_timer

- init_keyboard
- init_mouse

- init_storage
- system_int_on: Turn on system interrupts
- File systems: init_ramdisk, init_fat32

- init_battery

Once the sequence is complete, terminal's mouse handler is created as a separate
task, after which, the shell is created. The shell is just an interactive environment
so that the user can do something (graphics doesn't exist yet). If `BOOT_INTO_KSHELL`
is defined to a truthy value, it would boot into the built-in kernel shell, else, it
would boot into the `shelf.elf` file. If the shelf executable is absent, it would run
the kernel shell anyway as a fallback.

Once everything is done, the kernel must run into a forever halt loop. Without this,
the CPU would finish executation, and terminate, which would turn off the computer.
Making it halt would also cause the CPU to not use up energy while doing nothing.
*/
void kmain() {
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

    system_int_on();

    init_ramdisk();
    init_fat32();

    vfs_mount(&fat32_vfs); // TODO: One day have a proper disk file system like EXT2 or FAT32 and mount onto it instead

    init_battery();

    create_task(handle_mouse, "Terminal mouse handler", 0);

    File* shelf_file = fs_get("shelf.elf");
    if (shelf_file && !BOOT_INTO_KSHELL) {
        task* shelf_task = exec_elf("shelf.elf");
        shelf_task->privilege = PRIV_SUPER;
    } else {
        create_task(shell_thread, "Shell", 0);
    }
    kfree(shelf_file);

    // The OS must NEVER die.
    // Interrupts take back control from this loop whenever
    // they are called, so the OS is never stuck in the
    // while loop forever.
    while (1) system_halt();
}
