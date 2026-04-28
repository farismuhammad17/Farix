# IDT (Interrupt Descriptor Table)

The Interrupt Descriptor Table (IDT) is the x86 architecture’s mechanism for handling asynchronous events and exceptions. It functions as a jump table containing 256 "gates," where each entry points to a specific function (handler) that the CPU should execute when a corresponding interrupt occurs.

Without a properly configured IDT, any hardware event, like a keypress, or software error, like a division by zero, would cause the CPU to enter a "Triple Fault" and reboot (which looks like a crash). The IDT provides the bridge between hardware signals and the kernel’s high-level C logic.

```c
void init_interrupts();
```

Initializes the IDT by populating it with entry points for CPU exceptions, hardware IRQs, and system calls. It wraps the entire configuration by executing the `LIDT` (Load IDT) instruction, which provides the CPU with the memory address and size limit of the table.

```c
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
```

A helper that formats a handler’s memory address into the split bit-fields required by the processor. It also assigns the "Gate Type" and privilege level; for example, setting `IDT_GATE_USER` allows user-space applications to trigger a specific interrupt (like `int 0x80` for syscalls), while hardware interrupts are restricted to `IDT_GATE_KERNEL`.

# GDT (Global Descriptor Table)

The CPU needs a way to define how different regions of memory are accessed and what "privilege level" the current code possesses. The Global Descriptor Table (GDT) serves as a fundamental security and memory structure that the processor consults to determine if an operation—like executing a instruction or writing to a memory address—is permitted via Kernel Mode (Ring 0) and User Mode (Ring 3).

```c
void init_gdt();
```

Sets up the six primary segments: the mandatory Null segment, Kernel Code/Data, User Code/Data, and the Task State Segment (TSS). It calculates the table's limit and base address before loading them into the CPU's `GDTR` register.

```c
void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
```

A helper function that packs base addresses, segment limits, and access flags into the complex 8-byte format required by the x86 architecture. This includes setting the "Present" bit, privilege levels, and granularity (determining if the limit is in bytes or 4KB pages).

# TSS (Task State Segment)

In a multitasking environment, the CPU needs a mechanism to safely transition between different execution contexts, specifically when moving from User Mode (Ring 3) to Kernel Mode (Ring 0). The Task State Segment (TSS) is a dedicated structure that stores critical processor state information used by the hardware during these transitions.

While modern x86 kernels primarily use software-based multitasking rather than the hardware task-switching features of the TSS, the structure remains mandatory for one vital purpose: defining the **Kernel Stack Pointer**. When an interrupt or system call occurs while the CPU is in Ring 3, the hardware automatically consults the TSS to locate a valid, secure stack in Ring 0 to prevent user applications from corrupting kernel execution.

```c
void init_tss(uint32_t idx, uint32_t kss, uint32_t kesp);
```

Initializes the TSS structure and registers it within the GDT. It sets the Kernel Stack Segment (`ss0`); typically pointing to the kernel's data segment, and the initial Kernel Stack Pointer (`esp0`). It also configures the I/O Map Base to effectively disable hardware-level I/O permission checks, delegating that control to other kernel subsystems.

```c
void set_kernel_stack(uint32_t stack);
```

Updates the `esp0` field within the TSS. This is called during every process switch; it ensures that when the next task eventually triggers a system call or interrupt, the CPU switches to that specific process's unique kernel stack rather than a global one, preventing data leakage between tasks.
