# Setup

Your system must be targeting the x86 architecture, since the OS is specifically designed for it.

## Prerequisites

* **Toolchain**: `i686-elf` or `i386-elf`
* **Compiler:** `i686-elf-gcc` or `i386-elf-gcc` & `g++`
* **Assembler:** `nasm`
* **Emulator:** `qemu-system-i386`
* **Disk Tools:** `mtools` or `hdiutil` (macOS) / `mkfs.fat` (Linux & Windows)

## Initialising environment

Assuming you have cloned the GitHub repository, the program requries some external modules. You can download these easily with the [Makefile](../Makefile).

[!NOTE]  
> These commands may take some time to finish executing, so be sure to have coffee ready.

```bash
make get_deps
make libc
```

## Running the OS

To run it, use either of the following, and the [Makefile](../Makefile) handles the rest:

```bash
make run        # Full screen
make run_nofs   # Runs in a window
```

Alternatively, if you want to compile the OS:

```bash
make
```

Create the disk image:

```bash
make disk.img
```
