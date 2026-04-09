# Drivers

A driver is basically a low-level software component that operates within the OS kernel, but it would act as a translator between the operating system and hardware devices.

## Keyboard

Expecting the kernel to stop and wait for a user to press a key would freeze the entire system. Instead, the keyboard acts as an asynchronous input device. Every time a key is pressed or released, the hardware sends a unique numeric scancode to the CPU, which triggers an interrupt to let the kernel process the input immediately without stalling.

Because hardware sends raw scancodes (like 0x1E for 'A'), the kernel must maintain a keymap to translate those numbers into readable ASCII characters. Since the user might type faster than a program can read, we use a circular buffer (or ring buffer) to store keystrokes. This ensures that no input is lost while the CPU is busy with other tasks.

```c
void init_keyboard();
```

Sets up the hardware to start sending interrupts and initializes the
internal buffers and shift-state tracking.

```c
void keyboard_handler();
```

The core logic called by the interrupt. It reads the raw scancode from
the hardware port, checks if it's a "make" (press) or "break" (release)
code, updates modifier states like shift_pressed, and pushes the
resulting character into the kbd_buffer.

## Mouse

Tracking a mouse is more complex than a keyboard because it doesn't just send a single character; it sends continuous updates about movement and button states. If the kernel had to manually ask the mouse for its position every millisecond, it would waste massive amounts of CPU cycles. Instead, the mouse hardware generates an interrupt whenever it is moved or clicked, providing a packet of relative movement data.

The mouse sends delta values—how much it moved in the X and Y directions—rather than absolute screen coordinates. The kernel must collect these packets, track the state of the left and right buttons, and store them in a MouseEvent buffer. By using an asynchronous handler, the cursor can move smoothly across the screen even while the kernel is performing heavy background calculations.

```c
void init_mouse();
```

Sends the necessary magic sequences to the hardware to enable "streaming mode" so the mouse starts sending interrupts, and clears the initial event buffer.

```c
void mouse_handler():
```

The interrupt service routine that pulls raw bytes from the hardware. It
assembles these bytes into a MouseEvent struct, calculating the
directional shift and button states before pushing the event onto the
circular buffer.

## UART (Universal Asynchronous Receiver-Transmitter)

The UART header defines the interface for serial communication, primarily used as a robust diagnostic and logging mechanism. Unlike the VGA terminal, which requires complex memory mapping and font rendering, UART transmits data as a raw stream of bits over a serial line (usually **COM1**). 

This simplicity makes it the most reliable communication channel during the early stages of kernel initialization, as it can function even if the display driver or memory management systems have not yet been established.

```c
void init_uart();
```

Prepares the serial controller hardware. This establishes the line protocol (baud rate, data bits, parity, and stop bits) required for the kernel to synchronize with an external debugging host.

```c
char uart_getc();
void uart_putc(char c);
```

The fundamental I/O primitives. `uart_putc` transmits a single character once the hardware's transmit buffer is empty, while `uart_getc` performs a blocking read from the receiver buffer.

```c
static inline void uart_print(const char* data);
```

A global definition in the header itself that hopes everything else is already working. It iterates through a null-terminated string and sends each character to the serial port. It automatically injects a Carriage Return (`\r`) before Newlines (`\n`) to ensure correct line termination in standard serial monitors.

```c
static inline void uart_printf(const char* format, ...);
```

A lightweight implementation of formatted printing for the serial line. It utilizes `vsnprintf` to process format specifiers (like `%x` or `%d`) into a local buffer before transmission. This is the primary tool for kernel-level logging, allowing developers to monitor register states, memory addresses, and system events in real-time via an external terminal.

# Terminal

The Terminal serves as the primary visual abstraction for human-to-computer interaction within the kernel. While low-level logs may go to a serial port (UART), the Terminal provides an on-screen workspace for the user, supporting text rendering, command execution, and interactive feedback. It acts as the bridge between the kernel's internal state and the user's input, transforming raw character data into an organized, readable interface.

* **Standard Output Representation:** Manages a fixed grid of characters (80x25), serving as the primary display for the system shell and user applications.
* **State Management:** Tracks the "active" state of the display, including the current cursor coordinates (`cursor_x`, `cursor_y`) and active styling attributes (foreground and background colors).
* **Scrollback & Persistence:** Maintains a history of previous output lines, allowing users to scroll back and view data that has moved off the top of the physical screen.
* **Command Management:** Stores a history of user-entered commands, enabling features like command retrieval and cycling (typically via arrow keys) to improve the user experience.
* **Character Handling:** Interprets both printable characters and control characters (like newlines, tabs, and backspaces) to manage text flow and formatting.
* **ANSI Escape Sequence Support:** Recognizes specialized escape codes to perform advanced terminal operations, such as dynamic color shifting, specific cursor jumps, and screen clearing.
* **Input Echoing:** Provides the logic to "echo" keyboard and mouse input back to the screen, ensuring the user sees immediate feedback for their actions.
