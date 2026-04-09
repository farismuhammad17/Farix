# x86 32-bit

This is the most standard part of the kernel, and there are almost no new fancy implementations of anything here. This is because x86 is very defined, and I don't have much to add to it to improve anything anywhere.

Inside the core folder, you can find the implementation for every driver that the kernel will need. Under the memory folder, we define the PMM and VMM implementation.

## Kernel Boot Sequence

After the bootloader finishes its job, the CPU is handed over to the kernel. This transition moves the system from a "bare" state into the organized environment defined by the linker script and assembly entry point.

### Linker Script

The linker script is the blueprint for the kernel's memory layout. It tells the compiler exactly where to place code and data in the final binary.

* **Entry Point:** `ENTRY(_start)` tells the bootloader that the very first instruction to execute is at the `_start` label.
* **Load Address:** `. = 1M;` ensures the kernel is loaded at the 1 MiB mark. This is standard practice to avoid the "Real Mode" memory holes and BIOS data areas below 1 MiB.
* **Multiboot Placement:** The `.multiboot` section is placed at the very beginning of the `.text` block. This is critical because GRUB scans the start of the file for the "Magic Numbers" to verify it’s a valid kernel.
* **Symbol Exports:** `_kernel_start` and `_kernel_end` are exported symbols. These aren't variables in the code; they are memory addresses that the **PMM** uses to calculate which pages must be protected from allocation.

### Assembly Entry (`boot.s`)

This is the bridge between the bootloader and C code. Since C requires a stack to function (for local variables and function calls), we must set one up manually in Assembly.

```asm
_start:
    mov $stack_top, %esp
```

The bootloader leaves the CPU in **32-bit Protected Mode** but doesn't provide a stack. This instruction points the Stack Pointer (`ESP`) to the top of the 64 KiB reserve in the `.bss` section.

```asm
push %ebx    /* Multiboot Info Struct pointer */
push %eax    /* Multiboot Magic Number */
call arch_kmain
```

Before calling C, we preserve the values in `EAX` and `EBX`. The bootloader puts a "Magic Number" in `EAX` to prove it’s Multiboot-compliant and a pointer to the **Multiboot Information (MBI)** structure in `EBX`. These become the two arguments for `arch_kmain`.

### Architecture Initialization (`init.c`)

This function acts as the "Director," calling every subsystem in the correct order to transform the system from a single-tasking environment into a fully managed OS.

```c
void arch_kmain(uint32_t magic, multiboot_info* _mbi);
```

1. **Magic Verification:** Validates that the kernel was actually booted by a Multiboot-compliant loader. If the magic number is wrong, the hardware is in an undefined state, and we halt.
2. **PIC Remapping:** Moves hardware interrupts away from the CPU's internal exception vectors (moving them to index 32+).
3. **Memory Management (PMM/VMM):**
  * **PMM:** Indexes physical RAM using the memory map found in `mbi`.
  * **VMM:** Enables paging and sets up the "Higher Half" kernel mapping.
4. **Hardware Discovery (PCI):** Scans the bus to see what physical devices (NICs, Sound, Storage) are present.
5. **GDT Re-initialization:** Replaces the temporary "bootloader GDT" with the kernel's specific segments and privilege levels.
6. **Global Constructors (`_init`):** Calls initialization functions for C++ global objects or static initializers.
7. **Transition to Kernel Proper:** Finally calls `kmain()`, which typically initializes the scheduler and starts the first user-mode process or shell.

## Handlers

This is a slightly higher level piece of code, and requires you to understand the IDT, GDT, and related topics. The documentation for these can be found inside the core folder itself.

Once the IDT is configured, the kernel needs a robust way to handle the transitions between user space and kernel space, as well as a way to handle critical errors. The `handlers.c` file implements the high-level C logic for two distinct types of interrupts: **System Calls** (intended requests for kernel services) and **Exceptions** (unintended hardware or software errors).

Both mechanisms rely on a `registers` structure that represents the state of the CPU at the exact moment the interrupt occurred. This state is captured on the stack by the assembly stubs before being passed to these C functions.

### System Calls (Syscalls)

System calls are the gateway for user programs to interact with hardware. Instead of calling kernel functions directly, which is prohibited by the GDT, the program places a syscall number in the `EAX` register and triggers a specific interrupt (usually `0x80`).

```c
void syscall_handler(syscalls_registers_x86_32_t* regs);
```

The central dispatcher for all system requests. It examines `regs->eax` to identify the requested service, such as `SYS_WRITE` for terminal output or `SYS_EXIT` to terminate a process. It executes the corresponding kernel function and places the return value back into `regs->eax` so the user program can read it upon its return to Ring 3.

### Exception Handling & Diagnostics

Exceptions are triggered by the CPU when it encounters an instruction it cannot execute, such as a "Division by Zero" or a "Page Fault." The kernel must catch these to prevent a system-wide crash, often called a "Triple fault", which just looks like the computer rebooting itself.

```c
void exception_handler(syscalls_registers_x86_32_t* regs);
```

The kernel's "Panic" mechanism. When a critical error occurs, this function halts all execution (`cli`), logs the error to the UART serial port for debugging, and triggers a "Blue Screen" on the terminal to symbolise a BSOD. It provides a detailed post-mortem analysis by dumping the instruction pointer (`EIP`), error codes, and the specific reason for the fault, which makes life easier when debugging.

```c
static void dump_register_info(syscalls_registers_x86_32_t* regs);
static void dump_multitasking_info();
```

Diagnostic helpers that print the state of general-purpose registers and the currently active task. For **Page Faults** (Interrupt 14), the handler also reads the `CR2` register to report exactly which memory address caused the violation, allowing the developer to trace the crash back to a specific pointer or memory leak.
