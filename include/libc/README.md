Newlib is a lightweight, open-source C standard library designed specifically for embedded systems and "bare-metal" environments like your kernel. It provides the standard functions that C programmers expect, such as `printf`, `malloc`, `strlen`, and `fopen`, without the massive overhead of a full library like `glibc`.

## The Bridge Between Code and Kernel

In a typical operating system, the C library handles the communication between an application and the kernel. Since every kernel has a different way of handling memory and hardware, Newlib is built to be "target-agnostic." It provides the logic for the functions, but it leaves "holes" for the system-specific implementation details.

## System Calls and Stubs

To make Newlib work, you provide a set of **system stubs** (often called syscalls). These are the low-level "glue" functions that Newlib calls whenever it needs to interact with the outside world.
* When `printf()` is called, Newlib does the string formatting and then calls your `_write` stub to send those characters to the screen.
* When `malloc()` is called, Newlib manages the heap logic and calls your `_sbrk` stub when it needs to ask the kernel for more physical memory pages.

## Why Use It?

Using Newlib saves from reinventing the wheel. Instead of writing a math library, string manipulation tools, and complex memory allocators from scratch, you implement about 20 basic stub functions. Once those are linked, your kernel suddenly gains the power to run almost any standard C program, as Newlib handles the complex logic of turning those high-level calls into the low-level operations your kernel understands.
