# Changelog

## Standard C Library - *26 Feb, 2026*

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

## Change Directory Shell Command - *24 Feb, 2026*

- **Shell**
  - `cd` command: Changes current directory (doesn't work perfectly with RAMDisk, will be fixed later).
- **String**
  - `substr` method: Returns the sub-string of given string between two indices.

## FAT32 file system - *23 Feb, 2026*

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

## Global Descriptor Table - *22 Feb, 2026*

- **GDT**: Suspiciously works without refactoring anything else
- **I/O word**: `inw` and `outw` in `io.h`

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
