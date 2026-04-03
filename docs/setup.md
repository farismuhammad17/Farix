# Setup

Your system must be targeting the x86 architecture, since the OS is specifically designed for it.

## Prerequisites

* **Toolchain**: `i686-elf` or `i386-elf`
* **Compiler:** `i686-elf-gcc` or `i386-elf-gcc` & `g++`
* **Assembler:** `nasm`
* **Emulator:** `qemu-system-i386`
* **Disk Tools:** `mtools` or `hdiutil` (macOS) / `mkfs.fat` (Linux & Windows)

## Initialising environment

Assuming you have cloned the GitHub repository, the program requries some external modules. You can download these easily with the [make.py](../make.py) (assuming you have python on your machine). You may run it directly with python, but for making it nicer to use, you can use an alias:

```bash
source make.env
```

[!NOTE]  
> These commands may take some time to finish executing, so be sure to have coffee ready.

```bash
m get_deps
m libc
```

## Running the OS

To run it, use either of the following, and the [make.py](../make.py) handles the rest:

```bash
m run        # Full screen
m run_       # Runs in a window
```

Alternatively, if you want to compile the OS:

```bash
m -arch [ARCHITECTURE NAME]
```

Create the disk image:

```bash
m disk.img
```

Additionally, you can use help with it:

```bash
m help
```
