# Documentation

> [!NOTE]
> This file is obsolete, and was never fully finished. It will be replaced later for a more cleaner documentation style inside the FXTools.

* Programming the Kernel
  * [Code Separation](#code-separation)
  * [Code Conventions](#code-conventions)
  * [M-Functions](#m-functions)
  * [Debugging](#debugging)
* C-Library
  * [Kernel C-Library](#kernel-c-library)
  * [Standard C Library]
* CPU Architecture
  * [HAL Requirements]
  * [x86 32-bit]
* Kernel
  * [Booting]
  * [Memory Management]
  * [Storage system]
  * [Task manager]
  * [Multicore]
  * [ACPICA]
  * [Kernel Shell]
* Applications
  * [System calls]
  * [System Applications]
* Setup
* [License](#license)

# Code Separation

Every file being in the same folder makes for an ugly project; the following details how the code must be separated accross the source code in order to maintain a clean standard.

## Architecture specific code and Kernel code

Differences in CPU architectures make it impossible to write one piece of kernel code for every architecture. Hence, we use an HAL (Hardware Abstraction Layer):-

* Architecture specific code: Located inside the [`arch`](./arch) folder, it contains all the code specific to that architecture.
* Kernel code: Located inside the [`kernel`](./kernel) folder, it contains all the code that does not depend on architecture, even if the underlying functions use arch-specific functions.

### Architecture specific code

Inside the [`arch`](./arch) folder must be folders named with the architecture that the code is for. Currently, there are code for the following architectures:

* [x86_32](./arch/x86_32)
* [ARM32](./arch/arm32) *(WIP)*

Each of these folders must consist of the following required files:

* `boot.s`: Assembly code that is run before any C code; is responsible to get the CPU from the bootloader into where the kernel needs to be when the kernel initialisations begin.
* `linker.ld`: Linker file that dictates where the kernel code must be placed in memory during runtime.
* `hal.h`: Assembly stubs required for the kernel code to use, since assembly instructions vary between architectures. Currently, the following functions must be in place:
  * `outb(uint32_t port, uint8_t val)`: Same as x86 `outb`
  * `outw(uint32_t port, uint16_t val)`: Same as x86 `outw`
  * `outl(uint16_t port, uint32_t val)`: Same as x86 `outwl`
  * `inb(uint32_t port)`: Same as x86 `inb`
  * `inw(uint32_t port)`: Same as x86 `inw`
  * `inl(uint16_t port)`: Same as x86 `inl`
  * `system_halt()`: Halts the CPU
  * `system_pause()`: Pauses the CPU
  * `system_int_on()`: Enables interrupts
  * `system_int_off()`: Disables interrupts
  * `asm_get_random(uint8_t *success)`: Get random value, sets `success` as return value
  * `cpu_mem_barrier()`: Memory barrier instruction
  * `task_yield()`: Move to next task
  * `save_disable_interrupts()`: Save current CPU flags state and clear the interrupt flag to disable hardware interrupts.
  * `restore_interrupts(uint32_t flags)`: Restore CPU flags state from previously saved value, resetting interrupt status.
* `init.c`: Architecture specific initialisations must be done here from however `boot.s` moves from itself to C code. Generally, `boot.s` is made to call `arch_kmain`, a C function, which **must** call `early_kmain` at the start and `kmain` at the end, and anything else in the middle.

Refer [HAL Requirements](#hal-requirements) for implementation details.

# Code Conventions

> [!NOTE]
> These are my, Faris', personal preferences for writing code. You are free to object to accept it, but the consistency of having code in the same format is preferred.

The Python linter, accessible by `m lint`, comes with a sequence of checks performed of every file of the project folder. It is required that the coding style match what the linter expects.

* Includes, in any file, must be in alphabetical order.
* Explicit conversions must have a space between the type and the thing being converted, unless there is a bracket in between.
* No `#ifdef`, `#ifndef` and stuff like that in C files, only `#` thing I like is `#define` and `#undef` (for every corrosponding macro), anything else, and it is ugly, unless it is absolutely required for some reason.
* Functions must have `{` in the same line as the header:

```c
// Do
void function(args) {
   // Stuff 
}
```

```c
// Avoid
void function(args)
{
    // Stuff
}
void function(args){ // No space after arguments
    // Stuff
}
```

* Structs must use aliases always, avoid naming structs, unless the struct references itself.

```c
// Do
typedef struct {
    // Stuff
} struct_t;
```

```c
// Avoid
typedef struct struct_t {
    // Stuff
};
typedef struct struct_t {
    // Stuff
} struct_t;
```

* Do not use `RARE_FUNC` or `FREQ_FUNC` macros in `C` files, only headers.
* Any macros defined in a C files, inside a function, must be undefined at the end.
* If a function can be static, it should be static.
* Files named `README-UNFINISHED.md` are gitignored, but are warned by the linter.
* When pushing code, set `kernel.h/__DEBUG__` macro to `0`.
* Avoid using `LOG_CALL` or `LOG_NUM`, used for debugging, when pushing code.
* Mention the GNU AGPL v3 license header on every file.
* Any `TODO` blocks will be listed, `TODO REM` and `TODO IMP` getting different colours.
* A valid checksum (from `m checksum`) must be passed in with the changelog

# M-Functions

Instead of a standard makefile, this project uses Python.

- **Inside Docker / Windows:** The `m` command is pre-configured.
- **Linux / MacOS:** Run `source m.env` (and potentially `chmod +x make.py` for Linux) to initialise the alias.

*If you have problems with this setup, you can use [`python ./make.py`](make.py) directly, and run it like a normal python file in the terminal itself, and pass the necessary arguments after. The `m` is just for convenience.*

The most basic command, `m help`, outputs the list of commands that can be provided after `m`. Providing one of the entries listed outputs the given command's documentation. *Eg:* `m help lint` provides documentation on the `lint` command.

# Debugging

Two things are gauranteed in life: bugs, and software that only works on my machine *(please refer a therapist if you're suffering from the latter)*. The solution to the former is much simpler, atleast here, well as far as a kernel can make it.

Inside the `kernel.h` file is a macro, `__DEBUG__`, that you ought to set to `1` to enable the functions that let you debug. If `__DEBUG__` is `1`, then the panic screen will print these details in the panic screen.

> [!NOTE]
> Set `__DEBUG__` to `0` when committing any code.

## Log Number

```c
LOG_NUM(n);
```

Sets `logged_num` to `n`, which can be read from the `kernel.h` header.

At times when it is useful to count the number of times a function is called, or simply to keep track of a number (this has been useful atleast once), or to make some label like thing to keep track of whether you reached a part of code or not, use the `LOG_NUM` function.

1. Number of times a function is called: Have a global variable, `n`, that increments each time the function is called, then call `LOG_NUM(n)`.
2. To keep track of the part a function fails at: Say you have a function that does `A`, `B`, then `C`. You can place `LOG_NUM(1)`, `LOG_NUM(2)`, etc. to find at which point the function fails at.

## Log call

```c
LOG_CALL();
```

Writes the name of the current function into an array.

You can place this at the top of suspected functions to figure out where it crashes exactly, or you can place it here and there to track if the function comes back from that function properly or not.

You can just print out the `call_log` array to see the list, but, to maintain it inside an array, while still letting an infinite amount of functions to run, we wrap around this array. The array has a length of `MAX_LOG_LEN`, defined in `kernel.h`, and the `log_index` increments each time `LOG_CALL` is called, but wrap around that maximum length. If the last call that was called was finished, then the `last_call_finished` boolean is set to `true`, else, it is `false` if the kernel crashed while in the function.

## Using the logged calls data

The panic screen prints these details by this functions:

```c
static inline void dump_call_log(int funcs_per_line) {
    if (likely(!__DEBUG__)) return;

    panic_err_printf("--- Call log ---\n");
    for (size_t i = 0; i < MAX_LOG_LEN; i++) {
        panic_err_printf("%d:%s ", i, call_log[i]);
        if ((i + 1) % funcs_per_line == 0) {
            panic_err_printf("\n");
        }
    }
    panic_err_printf("(%s at %d)\n", last_call_finished ? "Finished" : "Unfinished", log_index - 1);
}
```

As can be seen, if `__DEBUG__` is `0`, then function never executes, since the `kernel.h` header sets `LOG_CALL` and `LOG_NUM` to empty functions, which removes them in case they are left by accident (the linter still flags them).

# Kernel C-Library

*Implementation: [kernel/libc/klib](./kernel/libc/klib)*

Normally, when compiling, the compiler uses the operating system to provide standard C-Libraries, like `stdlib.h`, which provide almost everything everyone needs in life. Unfortunately, we **are** the operating system. We cannot do this, so we have to create our own C-Library for the kernel space. Of course, we can also *not* do this, but that is not nearly as fun.

The major reason to do this is quite simple: implementing everything everytime we need something makes messy code, and having a standard library for everything makes it very readable for something coming in, especially if the functions are already like normal the normal C-library, you don't even have to look at the Kernel C-Library if that's the case.

Free standing GCC cross compilers, that we are using to compile the kernel for each architecture, still provide a few standard headers:-

* `stdarg.h`
* `stdint.h`
* `stddef.h`
* `stdbool.h`
* `float.h`
* `limits.h`
* `iso646.h`

Having these is a delight, no need to implement them, but the rest, we have to. Fortuantely, we don't have to define *everything* from the standard C-Library *(would be a nightmare and a toffee's coffee)*. Instead, we simply define **only** the functions and definitions we *actually* need, and not care about what we don't need. Benefit 1: Reduces bloat, makes the kernel compile faster. Benefit 2: Control over these functions.

The following is a list of the available headers inside `klib` (implementation is same as normal C):-

* `string.h`
  * `memcpy`
  * `memset`
  * `memmove`
  * `strdup`
  * `strcmp`
  * `strcpy`
  * `strncpy`
  * `strlen`
  * `strchr`
* `stdio.h`
  * `vsnprintf`
  * `printf`
