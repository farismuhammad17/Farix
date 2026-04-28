# PIC (Programmable Interrupt Controller)

The PIC acts as the CPU's receptionist for hardware signals. Since the CPU has only one main interrupt pin, the PIC manages up to 15 hardware interrupt lines (IRQs), prioritises them, and feeds them to the processor one by one. In x86 systems, this is handled by two chips: the **Master** and the **Slave**, connected in a "cascade" configuration where the Slave signals the Master through IRQ 2.

By default, the PIC maps hardware interrupts to the same IDT indices reserved for CPU exceptions (0–31). To prevent a timer tick from being confused with a "Division by Zero" error, the kernel must perform a **PIC Remap** to move hardware interrupts to a safe range (indices 32–47).

```c
void pic_remap();
```

Sends a sequence of Initialisation Command Words (ICW) to both PIC chips. This process resets the controllers, establishes the Master/Slave relationship, and remaps the interrupt vectors. Specifically, it maps IRQ 0–7 to IDT entries 32–39 and IRQ 8–15 to IDT entries 40–47.

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

The **End of Interrupt (EOI)** signal. After a hardware interrupt is handled, the kernel must send this command to the PIC. Without it, the PIC will assume the CPU is still busy and will refuse to send any further interrupts from that device.

# APIC (Advanced Programmable Interrupt Controller)

The entire PIC is for legacy hardware, and is unfortunately quite unusable for the modern world. As a result, the APIC is implemented. The entire thing works similarly. The ACPI provides the APIC's required pointer, the MADT; using which, we map the VMM to the CPU's desired address. There, we write to the local APIC and unmask the required interrupts for the IDT to function.

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
