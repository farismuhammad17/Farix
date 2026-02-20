# Changelog

## Ramdisk file system - *20 Feb, 2026*

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
