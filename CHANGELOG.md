# Changelog

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
