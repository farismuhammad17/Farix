Newlib is a lightweight C standard library designed for embedded systems. Since Newlib doesn't know how your specific kernel handles files or memory, you have to provide "stubs"—a set of glue functions that translate standard C calls (like `printf` or `malloc`) into your kernel's internal system calls.

# User C library

To let the user applications access kernel functions, we have to port them through a syscall. An application cannot call kernel functions directly, so we define them in the `syscalls.c` file. Since the function that calls the syscall itself is assembly, it is defined inside the `arch` folder, since its architecture dependant.

<p align="center">
    <img src="../../readme-assets/syscall-chain.svg">
</p>

This long boring chain is the only way to make user applications communicate with the kernel without any lag. Of course, we *can* have applications inside the kernel itself to speed it up, but that wouldn't be the best idea for safety.

# System Stubs

These stubs act as the interface between the standard C library and your kernel hardware abstractions. They allow user-space programs to use familiar functions while the kernel handles the heavy lifting of disk IO and memory management.

## Memory Management

```c
void* _sbrk(int incr);
```

This is the core of `malloc`. When a program needs more memory, Newlib calls `_sbrk` to move the "heap break." If the task is running in virtual memory, the stub allocates new physical pages via the PMM, maps them into the task's page directory via the VMM, and zeroes them out. It returns the old break address as the start of the new allocation.

## File Descriptor (FD) Table

```c
static const char* fd_table[20];
```

A simple tracking table that maps integer file descriptors to actual file names.
* **Standard Streams:** FD 0 is hardcoded to `stdin`, while 1 and 2 are `stdout` and `stderr`.
* **Custom Files:** FDs 3 through 19 are reserved for files opened from the disk via `_open`.

## Input / Output (I/O)

```c
int _read(int file, char *ptr, int len);
int _write(int file, char *ptr, int len);
```

The primary functions for data transfer:
* **Read:** For FD 0, it polls the keyboard buffer and blocks (halts) the system until keys are available. For disk files, it calls `fs_read`.
* **Write:** For FDs 1 and 2, it calls `echo_raw` to print to the terminal. For disk files, it uses `fs_write` to save data to the persistent file system.

```c
int _open(const char *name, int flags, int mode);
```

Validates that a file exists using `fs_get` and finds the first empty slot in the `fd_table` to assign a new descriptor. If the table is full or the file isn't found, it sets the global `errno` and fails.

## Process Control

```c
void _exit(int status);
```

Triggered when a program finishes or calls `exit()`. It marks the current task as `TASK_DEAD` and immediately calls the scheduler to switch to a different process. It never returns.

```c
int _getpid();
int _kill(int pid, int sig);
```

Minimal implementations for process identification and signaling. In this specific stub set, `_getpid` returns a constant and `_kill` is a stub that returns an error, as complex signal handling isn't implemented here.

## Entropy and Hardware Support

```c
int getentropy(void *ptr, size_t len);
```

Provides high-quality random data, usually for security or seeding. It uses an assembly helper (`asm_get_random`) to pull random bits directly from the CPU's hardware random number generator. If the hardware pool is temporarily empty, it pauses to let the entropy pool refill.

## Standard Utility Overrides

```c
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
void* memmove(void* dest, const void* src, size_t n);
```

These wrap your internal kernel memory functions (`kmemcpy`, `kmemset`). `memmove` is specially implemented to handle overlapping memory regions safely by copying backward if the destination is higher than the source.
