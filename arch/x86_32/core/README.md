This folder contains all the core drivers and specific hardware components that the kernel will need.

# PIC (Programmable Interrupt Controller)

The PIC acts as the CPU's receptionist for hardware signals. Since the CPU has only one main interrupt pin, the PIC manages up to 15 hardware interrupt lines (IRQs), prioritizes them, and feeds them to the processor one by one. In x86 systems, this is handled by two chips: the **Master** and the **Slave**, connected in a "cascade" configuration where the Slave signals the Master through IRQ 2.

By default, the PIC maps hardware interrupts to the same IDT indices reserved for CPU exceptions (0–31). To prevent a timer tick from being confused with a "Division by Zero" error, the kernel must perform a **PIC Remap** to move hardware interrupts to a safe range (indices 32–47).

```c
void pic_remap();
```

Sends a sequence of Initialization Command Words (ICW) to both PIC chips. This process resets the controllers, establishes the Master/Slave relationship, and remaps the interrupt vectors. Specifically, it maps IRQ 0–7 to IDT entries 32–39 and IRQ 8–15 to IDT entries 40–47. 

Finally, it applies an **Interrupt Mask** by writing to the PIC data ports. This allows the kernel to selectively enable only the hardware it is ready to handle (like the Timer and Keyboard) while ignoring "noisy" or unused lines.

```c
outb(PIC1_DATA, mask);
outb(PIC2_DATA, mask);
```

These writes to the Mask Registers use bitfields where a `0` enables the interrupt and a `1` disables (masks) it. This is the primary way the kernel controls which hardware devices are allowed to "wake up" the CPU.

| IRQ | Hardware | Status | Purpose |
| :--- | :--- | :--- | :--- |
| **0** | **Timer** | **Enabled** | Drives the scheduler and system uptime. |
| **1** | **Keyboard** | **Enabled** | Processes user keystrokes. |
| **2** | **Cascade** | **Enabled** | Necessary for the Slave PIC to communicate. |
| **4** | **COM1** | **Enabled** | Used for serial debugging and logs. |
| **12** | **PS/2 Mouse** | **Enabled** | Processes cursor movement and clicks. |

```c
outb(0x20, 0x20); // Master EOI
outb(0xA0, 0x20); // Slave EOI
```

The **End of Interrupt (EOI)** signal. After a hardware interrupt is handled, the kernel must send this command to the PIC. Without it, the PIC will assume the CPU is still busy and will refuse to send any further interrupts from that device.

# PCI (Peripheral Component Interconnect)

The PCI bus is the standard interface for connecting high-speed hardware, such as network cards, sound cards, and modern storage controllers, to the motherboard. Unlike older "Legacy" hardware (like the Keyboard or PIC) which exists at fixed, hardcoded addresses, PCI devices are discoverable. The kernel must iterate through the PCI bus to identify what hardware is actually plugged into the system and determine its capabilities.

Each PCI device is identified by a **Vendor ID** (who made it) and a **Device ID** (what it is). By reading the **Class Code**, the kernel can identify a device's function (e.g., "Mass Storage Controller" or "Network Controller") even if it doesn't have a specific driver for that exact model.

```c
void init_pci();
```

Performs a "PCI Bus Enumeration." It loops through every possible bus (0–255) and every possible device (0–31) to check for hardware. If it finds a device that does not return `0xFFFF` (a non-existent device), it records the Vendor, Device, and Class information into the `pci_devices` array. This registry acts as the source of truth for the kernel when loading specialized drivers later in the boot process.

```c
static uint32_t read_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
```

Accesses the **PCI Configuration Space** using I/O ports `0xCF8` (Address) and `0xCFC` (Data). Since the CPU cannot address PCI configuration memory directly, it must write a specifically formatted 32-bit integer to the Address port—containing the bus, device, and function numbers—and then read the resulting value from the Data port. This function is the fundamental building block for all PCI interaction.

| Field | Bit Range | Description |
| :--- | :--- | :--- |
| **Vendor ID** | 0-15 | Identifies the manufacturer (e.g., `0x8086` for Intel). |
| **Device ID** | 16-31 | Identifies the specific model of the hardware. |
| **Class Code** | 24-31 | Defines the general category (e.g., `0x01` for Storage). |
| **Subclass** | 16-23 | Provides more specific detail (e.g., `0x01` might be IDE). |

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

# Keyboard and Mouse

The PS/2 controller handles communication for both the keyboard and the mouse using a specific set of I/O ports. While they share the same hardware controller, the kernel treats them as separate interrupt streams. Because hardware I/O is significantly slower than the CPU, both drivers implement **Ring Buffers**. This allows the hardware to "push" events into a queue immediately, which the kernel can then process at its own pace without losing input data.

The keyboard communicates using **Scancodes**. When a key is pressed or released, the keyboard sends a unique byte to the controller. The driver must maintain state (like whether `Shift` is currently held) to correctly translate these raw scancodes into ASCII characters for the rest of the system.

```c
void init_keyboard();
```

Performs a hardware reset and self-test of the keyboard. It polls the status register to ensure the controller is ready before sending commands, finally setting a `keyboard_ready` flag to begin processing input.

```c
void keyboard_handler();
```

The Interrupt Service Routine (ISR) triggered by the CPU. It reads the scancode from port `0x60`, handles "Extended" keys (like arrow keys prefixing with `0xE0`), and tracks the state of modifier keys. Valid characters are pushed into a circular buffer to be read by the shell or applications.

```c
void push_to_kbd_buffer(char c);
```

Manages the circular queue. It places the translated ASCII character into the `kbd_buffer` and increments the `head` pointer. If the buffer is full, it safely discards the new input to prevent memory corruption.

The mouse driver is more complex because it requires a multi-step initialization sequence to enable features like the scroll wheel (IntelliMouse mode). Unlike the keyboard, which sends single bytes, the mouse sends data in **4-byte packets** containing button states, X/Y movement, and scroll delta.

```c
void init_mouse();
```

Enables the mouse auxiliary port and sends a specific sequence of "Sample Rate" commands to the mouse to enable the **IntelliMouse** extension. It also modifies the PS/2 controller's configuration byte to ensure mouse interrupts are unmasked.

```c
void mouse_handler();
```

Collects individual bytes over four consecutive interrupts to assemble a complete `MouseEvent`. It distinguishes between keyboard and mouse data by checking the "Auxiliary Device" bit (0x20) in the status register. Once a 4-byte cycle is complete, the packet is pushed into the `mouse_buffer`.

```c
void mouse_write(uint8_t data);
```

A low-level helper that sends a command to the mouse. It first writes `0xD4` to the command port (`0x64`) to tell the controller that the subsequent data byte is destined for the mouse, not the keyboard.

```c
void mouse_wait(uint8_t type);
```

Synchronizes the CPU with the slow PS/2 controller by polling the status register. It waits for the input buffer to be empty before writing or the output buffer to be full before reading, preventing "buffer overflows" at the hardware level.

# ATA (Advanced Technology Attachment)

Directly communicating with storage drives using raw electrical signals is complex and error-prone. To standardize how software talks to hard disks and solid-state drives, we use the ATA interface. It provides a consistent protocol for the kernel to move data between physical disk sectors and system memory.

The ATA controller allows the kernel to issue commands—like "read" or "write" to specific disk locations called LBA (Logical Block Addressing). Instead of the CPU waiting idly for the slow mechanical disk to finish, we use status flags like BSY (Busy) and DRQ (Data Request) to synchronize the high-speed CPU with the slower storage hardware.

```c
void init_ata();
```

Initializes the disk controller, detects connected drives, and prepares the bus for data transfer.

```c
void ata_read_sector(uint32_t lba, uint8_t* buffer);
void ata_write_sector(uint32_t lba, uint8_t* buffer);
```

These functions translate high-level requests into low-level commands sent to the disk. They handle the transfer of 512-byte chunks of data (sectors) between the hardware and a provided memory buffer.

```c
void ata_wait_ready();
```

A helper that polls the status register. It ensures the drive is not busy and is ready to accept the next command or data packet before the kernel proceeds.

# PIT (Programmable Interval Timer)

The PIT is the kernel's primary rhythmic heartbeat. It is a dedicated chip that oscillates at a fixed frequency of 1.193182 MHz. By sending a specific "divisor" to the PIT, the kernel can control how often the hardware triggers an interrupt (IRQ 0). This constant stream of pulses is what allows the kernel to track time, manage delays, and—most importantly—perform preemptive multitasking by forcing a context switch.

```c
void init_timer(uint32_t frequency);
```

Sets the firing rate of the timer. It calculates a 16-bit divisor based on the PIT's internal clock speed. It sends the command byte `0x36` to port `0x43` to set the timer to "Square Wave Mode," then sends the low and high bytes of the divisor to port `0x40`. For example, a frequency of 100Hz will trigger the timer handler every 10ms.

```c
void timer_handler();
```

The high-frequency ISR that executes on every timer tick. After signaling the PIC that the interrupt has been handled (EOI), it calls the scheduler. This is the core of "preemption": the timer interrupts whatever task is currently running and gives the kernel a chance to swap it out for another, ensuring no single process can hog the CPU indefinitely.

# UART (Universal Asynchronous Receiver-Transmitter)

The UART driver enables serial communication, which is the primary method for "headless" debugging in the kernel. By sending data through the COM1 serial port, the kernel can transmit logs and diagnostic information to a host machine even if the VGA display is not yet initialized or has crashed. Unlike the complex VGA buffer, UART is a stream of bytes sent over a single wire, making it incredibly reliable for low-level monitoring.

```c
void init_uart();
```

Configures the 16550 UART controller at port `0x3F8`. It sets the communication speed to **115200 baud** by enabling the Divisor Latch Access Bit (DLAB) and setting the divisor to 1. It also configures the line protocol for "8N1", i.e. 8 data bits, no parity bits, and one stop bit, and enables the internal FIFO buffers to handle high-speed data bursts without dropping characters.

```c
void uart_putc(char c);
```

The fundamental output function. It polls the Line Status Register (LSR) at `PORT + 5` until the "Transmit Holding Register Empty" bit is set. Once the hardware is ready, it writes the character to the data port, pushing it out across the serial line.

```c
char uart_getc();
```

The input counterpart to `putc`. It waits in a loop until the "Data Ready" bit (bit 0) of the status register is high, indicating that a byte has been received from the outside world and is waiting in the receiver buffer.

```c
int is_uart_transmit_empty();
int is_uart_received();
```

Low-level helper functions that check specific bits in the status register. These allow the kernel to perform "non-blocking" checks on the serial port, determining if it is safe to send or receive data without halting the CPU.

# Terminal

The Terminal driver provides the interface for text-based interaction, managing the VGA text buffer, located at `0xB8000`. Beyond simple character rendering, it implements advanced features like a scrollback buffer (line history), command history (for shell interaction), and a subset of ANSI escape codes for text styling and cursor manipulation.

```c
void init_terminal();
```

Initializes the VGA buffer with a light-grey on black color scheme. It clears the screen, sets the initial cursor position, and prepares the line history mechanism by allocating the first blank entry.

```c
void echo_char(uint16_t c);
```

The primary output function. It handles incoming characters by either processing them as ANSI escape sequences, special control characters (like `\n` or `\b`), or standard printable glyphs. It automatically manages cursor progression and triggers screen scrolling when the cursor exceeds the terminal height.

```c
void echo_raw(const char* data, size_t len);
```

Back in the dark-ages, we used to print text by repeating `echo_char`, but this was very slow, as it kept doing a bunch of overhead each time. If only we had a function to do what `echo_char` could do, but also for multiple characters, much more efficiently. It works like this:

1. If there is no text, print nothing.
2. Pre-calculate the total lines its going to use, but simulating the cursor quickly.
3. Early-scroll down to where we would be after the text is done printing.
4. Dump the data directly to the VGA buffers
5. Update terminal

This made the terminal literally instant.

```c
void t_print(char* text);
```

This was useful when I used to have no UART, so I needed to print stuff to the terminal to see what happened. Of course, the other echo functions rely on a proper heap and stuff to run, whereas this just directly writes to the VGA, no extra calculations, and is (was) just for debugging.

```c
void update_cursor(size_t x, size_t y);
```

Communicates with the VGA hardware via I/O ports `0x3D4` and `0x3D5`. It tells the hardware controller exactly where to draw the blinking underscore by sending the high and low bytes of the 1D buffer index.

```c
void refresh_terminal_view();
```

Redraws the entire terminal window based on the current `scroll_offset`. It maps the logical lines from the `line_history` array back into the physical VGA memory, allowing the user to view previous output using the mouse wheel or keyboard.

```c
void save_line_to_history(uint16_t* line_data);
```

Persistent storage for terminal lines. When a line scrolls off the top of the screen or a newline is issued, this function saves the 80-character row into a heap-allocated buffer. This allows for the "infinite scroll" effect.

```c
void handle_ansi_char(uint16_t c);
```

A state-machine parser for ANSI escape codes. It intercepts characters following an `ESC [` sequence to perform complex terminal actions, such as changing text colors (`m`), clearing portions of the screen (`J`, `K`), or jumping the cursor to specific coordinates (`H`).

```c
void save_cmd_to_history(const char* command);
```

Maintains a doubly-linked list of previously entered shell commands. This is used by `cmd_history_up()` and `cmd_history_down()` to let the user cycle through their command history using the arrow keys.
