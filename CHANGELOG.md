# Changelog

*Refer [journal](docs/journal.md) for implementation details.*

## Slab allocated ACPI - *Current*

- Arch
  - Assembly stubs from `stubs.h` to `hal.h`
  - All assembly stubs are inline
  - Architecture includes (headers) now remain in arch folder
- `kernel.h` defines macros to improve efficiency without much change:-
  - `unlikely(x)`: `x` is false most of the time
  - `likely(x)`: `x` is true most of the time
  - `RARE_FUNC`: Function is rarely used
  - `FREQ_FUNC`: Function is frequently used
- Spammed aforementioned macros throughout source code.
- Storage
  - Block Device (Abstraction) Layer for ATA and (WIP) AHCI.
  - `init_ata` takes in PCI device object
- ACPI
  - Uses slab allocator
  - Interrupts
- VMM
  - `PAGE_PCD` and `PAGE_PWT` flags for x86_32.
- Slab
  - Implemented `Slab32`, `Slab16`, `Slab8`
- MFuncs
  - Compiler includes kernel.h
  - Compiles with arch folder included for `hal.h`

## Docker build environment - *20th April, 2026*

- Dockerfile
  - `dock.cmd` to help run docker
- MFuncs
  - Ignore kshell during compilation
  - Accept `i686-linux-gnu-gcc` as x86 compiler
  - `get_deps` prints outputs
  - Removed `err_len`
  - QEMU and USB is restricted on Docker

## Slab allocator - *17th April, 2026*

- Fastest possible slab allocator (no clue how to make it faster)
- Terminal
  - No longer dependant on kernel shell
- Makefile
  - Windows support for 'm' command

## Shelf File Command - *17th April, 2026*

- Shelf
  - Implemented commands: `touch`, `mkdir`, `write`, `cat`, `ls`, `cd`, `rm`
  - Arranged into `sh_utils`, `sh_fs`, `sh_tasks`
  - Echoes initial text
- ELF
  - Renamed `exec` to `exec_elf`
- Syscalls
  - `SYS_MKDIR`, `SYS_EXEC` user syscalls
  - Separated arch specific stubs from arch independent
- Makefile
  - Compiles arch specific user libC code
  - `defs` command to index and locate functions
  - Moved in `lint`

## Shelf - *15th April, 2026*

- Kernel Shell replaces for ELF Shell
- ELFs finally die properly
- File system
  - Read and Write take `offset`
- Tasks
  - Privileges support super users
- Syscalls
  - Filled out all Newlib stubs
  - Implemented user syscalls
  - Added super user system calls for shelf
- Makefile
  - `m clean apps` to only delete build/apps

## Silicon Verification - *14th April, 2026*

- `last_call` with `LOG_CALL` macro to track function calls
- `last_init` to trace load crashes
- Used `t_print` to eliminate silent failures.
- Implemented Panic Shell
- ATA
  - Improved readability with defined variables
  - `init_ata` functions for newer ATAs using PCI
  - Made `ata_wait_ready` silicon proper
  - Maximum timeout to `ata_wait_ready`
  - Looks at BAR1 too
- PCI
  - Checks all 8 functions instead of just the 0th
- ACPI
  - Filled out Semaphore and lock stubs
  - Removed unmap functionality
- Keyboard
  - Made `init_keyboard` silicon proper
  - Implemented `keyboard_getc`
- Terminal
  - Implemented `t_printf`
  - Made `t_print` as raw as possible
  - `t_print` adjusts `cursor_y` down
  - `init_terminal` no longer clears the screen itself
  - `terminal_clear_phys` to clear the VGA buffers before VMM
- Shell
  - `int` command to execute specific interrupt
- Makefile
  - Split functionalities accross files for readability
  - Compiles apps and puts into kernel disk
  - Option to emulate in QEMU or write to USB
  - QEMU execution is much more realistic (though slower)
  - Optional `clean` exclusions

## ACPICA Inclusion - *11th April, 2026*

- Timer
  - Implemented `timer_stall` in 32-bit x86
  - Implemented `get_timer_uptime_microseconds` in 32-bit x86
- Shell
  - Implemented `heapstat`
- VMM
  - Implemented `vmm_is_mapped`
- Heap
  - `init_heap` now goes to 512 KB instead of 64 KB.
- ACPICA
  - `AcpiMappingCleanup` to clean up mappings if required later
- Makefile
  - `-elen` flag

## PCI Implementation - *8th April, 2026*

- `lint.py` to ensure the code is clean.
- PCI
  - Basic PCI implementation
- `early_kmain` to be called in arch init.
- x86 32 will now UART Multiboot error

## tlist Shell command - *5th April, 2026*

- Shell
  - `tlist` command to see bit-maps and tasks inside each list.
- Multitasking
  - Reduces list sizes from 16 to 8.

## Improved Scheduler Algorithm - *5th April, 2026*

- Renamed `shell/table` to `shell/cmds`
- Split shell commands into cleaner files
- Multitasking
  - An improved algorithm from the Robin-Round scheduler.
  - Store tasks in arrays linked to one another.
  - `get_task` function
- Terminal
  - Fixed BSOD miscoloring
  - Renamed `terminal_entry_color` to `terminal_color_entry`
- UART
  - Arch independant `print_uart`

## Universal Asynchronous Receiver-Transmitter - *4th April, 2026*

- UART implementation
- Moved `arch` to root folder
- Renamed `src` to `kernel`
- Documentation in header files
- x86_32
  - Cleaned init file
- ARM32
  - Fixed compile time errors in ARM32
- Makefile
  - Configured `make.py` to compile arm32, as well as any future architecture relatively easily.

## Instant Shell Outputs - *3rd April, 2026*

- Terminal
  - Added `echo_raw`
  - Added `new_line_n`
  - Uses arrays to store lines instead of doubly linked list
- Shell
  - Moved `memstat` logic from heap to shell
  - Implemented buffers and flushes to make outputs instant
- Newlib
  - `_write` uses `echo_raw` instead of printing line by line

## Hardware Abstraction Layer - *3rd April, 2026*

- Fully supports x86_32
- Half supports arm32

## Proper ELF Executor - *30th Mar, 2026*

- Refactored entire Kernel from C++ to C.
- Terminal
  - BSOD upon kernel panic
  - `t_print` for direct writing to VGA buffers
- Memory
  - VMM finally uses paging (forgot to enable)
  - IDT and GDT use virtual addresses
  - Fixed HeapSegment alignment
- Fixed every compiler warning I got

## Pipe Shell Operator - *8th Mar, 2026*

> [!WARNING]
> This commit comes with a partial implementation of the ELF executor, and, though it won't crash the kernel, it also doesn't work.

- **Shell**
  - Pipe operator
  - `grep` command
  - `peek` command
  - `exec` command
- **Terminal**
  - Increase maximum limit for storing lines from 500 to 1000.
- **Syscalls**
  - Renamed `registers_t` to `syscalls_registers_t`
- **VMM**
  - Implemented `vmm_unmap_page`
  - Implemented `vmm_get_current_directory`
- **Multitasking**
  - Original task is named 'init' now.

## Error handling - *6th Mar, 2026*

- **IDT**
  - Handles all possible error codes to print before crashing.

## ELF Executor - *4th Mar, 2026*

- **IDT**
  - `syscall_handler_stub` to handle user level system calls.
- **ELF**
  - Renamed `elf_load_file` to `exec`.
  - Refactors `exec` to not use `fs_read` twice.
- **User mode**
  - `farix.h` as a bridge between executables and the kernel.

## ELF Loader - *3rd Mar, 2026*

- **ELF**
  - Added logic to parse ELF headers and load program segments into virtual memory.
- **VMM**
  - Implemented Ring 3 user mode.
- **TSS**
  - Configured Task State Segment with `esp0` pointing to `stack_top` for safe interrupt handling.
- **GDT**
  - Added `jump_to_user_mode` in `gdt_flush.asm` to perform the `iret` leap into Ring 3.
- **Repository**
  - `.gitattributes` to make the header files be seen as C++ files.
  - `docs/journal.md` as a diary entry for implementations.
  - `docs/setup.md` to help run the prototype version of the OS.
  - Added the ordinal suffixes to dates in `CHANGELOG.md` wherever I forgot

## ANSI Escape codes - *2nd Mar, 2026*

- **Terminal**
  - ANSI support codes now work, though some syntax remain pending.
  - Replaced `char` usage for `echo_char` and similar function with `uint16_t` for future compatibility.

## FAT32 Sector limitation removal - *2nd Mar, 2026*

- **FAT32 File System**
  - Updated disk read routine to iterate through total sectors.
  - There is no longer a maximum buffer limit to reading a file.
- **Terminal**
  - Increase maximum lines remembered in history to 500 from 200.
  - Unifinished implementation for ANSI escape codes.
- **Removed**
  - Empty `config.h` file I forgot to delete.

## Terminal scrolling - *28th Feb, 2026*

- **Terminal**
  - Scrolls using the detected mouse scroll.
  - Mouse scroll detection is in a seperate thread.
- **Shell**
  - Implemented `kill` command.
- **Multithreading**
  - Implemented `kill_task`.

## Mouse implementation - *28th Feb, 2026*

- **Mouse**
  - Detects relative movement.
  - Detects left and right click.
  - Detects scroll.
- **Keyboard**
  - Implemented `init_keyboard`.
- **Make file**
  - `run` and `run_nofs` are informed of the mouse.

## Tasks command - *27th Feb, 2026*

- **Keyboard**
  - Moved `KEY_UP` and `KEY_DOWN` definitions from `terminal.h` to `keyboard.h`.
- **Heap**
  - `kmalloc` automatically stores the caller's address for logging.
- **Shell**
  - Implemented `tasks` command.
  - Moved `memstat` logic to `heap.cpp` instead.
  - `memstat` now prints caller addresses.
  - `help` command now indents properly.

## Standard C Library - *26th Feb, 2026*

- Implemented `libc` through `newlib`.
- **Make file**
  - `get_deps` to download newlib.
  - `libc` to compile the standard library.
- **Keyboard**
  - Implemented keyboard buffer.
- **Heap**
  - Renamed `malloc` to `kmalloc`.
  - Renamed `free` to `kfree`.
  - Renamed `memcpy` to `kmemcpy`.
  - Implemented `kmemset`.
  - Implemented `check_heap`.
  - Heap functions will now always `check_heap`.
- **Terminal**
  - Removed `strlen`.
- **Refactoring**
  - Removed `config.h` and moved PAGE_SIZE to `pmm.h` instead.
  - Replaced all instance of string and map with standard library's.
- **Removed**
  - Every project file to swap out for libc equivalents.
  - Custom string and map implementations.

## Change Directory Shell Command - *24th Feb, 2026*

- **Shell**
  - `cd` command: Changes current directory (doesn't work perfectly with RAMDisk, will be fixed later).
- **String**
  - `substr` method: Returns the sub-string of given string between two indices.

## FAT32 file system - *23th Feb, 2026*

- **ATA drivers**
  - Now supported to enable storing data to disk.
- **FAT32 file system**
  - Integrate core logic, though `vfs_mount` still needs to explicitly be informed of the operations to work with in the kernel.
- **VFS**
  - File systems must now pass a `getall` function to the abstraction layer that returns a linked list of all the contents in the given absolute string path.
- **Terminal**
  - Terminal's `echo` can now take a string to end the echo with, instead of the default `\n`.
- **Shell**
  - `mkdir`: Creates folder.
  - `ls`: Lists directory contents.
  - Split commands into designated groups (in `src/shell/table`) for organization.
  - The shell stores its path, although not yet visible nor changeable.
- **Map**
  - `keys` method: Returns the keys in the map.
  - `capacity` method: Returns the total allocated slots in the map.
  - `getEntryInternal` method: Returns key-value pair at given index.
- **String**:
  - `upper` method: Returns string all in uppercase.
  - `substr` method: Returns the substring from given index till the end.
  - `find` method: Returns the index of the first instance of a character from the given index till the end.
  - `starts_with` method: Returns whether the string starts with a given string.
  - `ends_with` method: Returns whether the string ends with a given string.
  - `contains` method: Returns whether the string contains a given character.
  - Reduced memory usage as much as possible using `memcpy` instead of creating new strings each method.
- **Stack**
  - Size increased to 64 KiB from 16 KiB.
- **Heap**
  - Fixed mistake of not calculating `heap_end` in `init_heap`.
  - Implemented `memcpy`: Copies the memory blocks of one object to another.
  - `heap_expand` merges the segments.

## Global Descriptor Table - *22th Feb, 2026*

- **GDT**: Suspiciously works without refactoring anything else
- **I/O word**: `inw` and `outw` in `io.h`

## Ramdisk file system - *20th Feb, 2026*

- **Map data type**: Basic key-value pair data type.
- **String methods**: Split and count methods.
- **Implemented RAMDISK**: Files can be created inside the memory.
- **Implemented touch, cat, write, rm to shell**: Currently only interacts with files in ram (disks are unimplemented).
- **VFS Abstraction**: For scalability to disks.

## Multithreading - *19th Feb, 2026*

- **Implemented threads**
- **Integer parameted for terminal echo**: the `echo` function can now take an integer type as input.
- **Dynamic heap size**: Heap can now expand its size if it has to.

## Shell - *18th Feb, 2026*

- **Refactored project structure**: Each file in [include](include) and [src](src) are moved to a designated folder.
- **Strings**: Implemented basic strings.
- **Shell**: Implemented basic shell commands - `help`, `clear,` `memstat`, and `echo`.
- **Command line history**: Shell allowed UP and DOWN arrow to traverse history.

## Complete Memory Stack - *17th Feb, 2026*

- **Refactored project structure**: Header files into [include](include) and C++ files into [src](src)
- **PMM** (Physical Memory Manager): Implemented a bitmap-based allocator to track every 4KB page of system RAM.
- **VMM** (Virtual Memory Manager): Enabled x86 Paging with identity mapping for the first 4MB to ensure kernel stability.
- **Kernel Heap**: Implemented a linked-list-based heap manager with block splitting and coalescing (merging).
- **C++ Dynamic Memory**: Added `malloc` and `free`, and `new` and `delete`

## Initial commit - *16th Feb, 2026*

- Added core functionality
- Implemented kernel
- Implemented keyboard usability
