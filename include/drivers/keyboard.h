/*
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

/*

Keyboard

Expecting the kernel to stop and wait for a user to press a key would freeze the
entire system. Instead, the keyboard acts as an asynchronous input device. Every
time a key is pressed or released, the hardware sends a unique numeric scancode
to the CPU, which triggers an interrupt to let the kernel process the input
immediately without stalling.

Because hardware sends raw scancodes (like 0x1E for 'A'), the kernel must maintain
a keymap to translate those numbers into readable ASCII characters. Since the user
might type faster than a program can read, we use a circular buffer (or ring buffer)
to store keystrokes. This ensures that no input is lost while the CPU is busy with
other tasks.

Since the way the CPU receives these signals depends on the interrupt controller (like
the PIC on x86 or GIC on ARM), the low-level handling is architecture-specific:

x86_32 : src/arch/x86_32/keyboard.c

void init_keyboard():
    Sets up the hardware to start sending interrupts and initializes the
    internal buffers and shift-state tracking.

void keyboard_handler():
    The core logic called by the interrupt. It reads the raw scancode from
    the hardware port, checks if it's a "make" (press) or "break" (release)
    code, updates modifier states like shift_pressed, and pushes the
    resulting character into the kbd_buffer.

kbd_buffer, kbd_head, kbd_tail:
    These variables implement the circular queue. The head moves as we
    add characters in the interrupt, and the tail moves when a program
    reads them, allowing the kernel to decouple input from processing.

*/

#include <stdbool.h>

#define KEY_UP   0x11
#define KEY_DOWN 0x12

#define KBD_LEN        58
#define KBD_BUFFER_LEN 1024

extern char kbd_buffer[KBD_BUFFER_LEN];
extern volatile int kbd_head;
extern volatile int kbd_tail;

extern bool shift_pressed;
extern unsigned char kbd[128];

void init_keyboard();
extern void keyboard_handler();

#endif
