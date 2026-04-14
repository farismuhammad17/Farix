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

#include <stdbool.h>
#include <stddef.h>

#include "arch/stubs.h"
#include "drivers/terminal.h"

#include "drivers/keyboard.h"

bool shift_pressed  = false;
bool keyboard_ready = false;

unsigned char kbd[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',

    // Shifted
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

// For DEBUG: Print the raw hex value to terminal
// Uncomment later line that prints it
// const bool echo_scancodes = false;

static bool is_extended = false;

char kbd_buffer[KBD_BUFFER_LEN];
volatile int kbd_head = 0;
volatile int kbd_tail = 0;

void push_to_kbd_buffer(char c) {
    int next = (kbd_head + 1) % KBD_BUFFER_LEN;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

// TODO: Remove magic numbers
void init_keyboard() {
    // Tell the controller we want to read the 'Command Byte'
    while (inb(0x64) & 0x02); // Wait for input buffer empty
    outb(0x64, 0x20);

    // Wait for the byte to arrive and read it
    while (!(inb(0x64) & 0x01));
    uint8_t cb = inb(0x60);

    // Modify the byte
    cb |= 0x01; // Enable Keyboard IRQ
    cb |= 0x02; // Enable Mouse IRQ
    cb |= 0x40; // Enable Translation

    // Tell the controller we are writing the byte back
    while (inb(0x64) & 0x02);
    outb(0x64, 0x60);

    // Send the modified byte
    while (inb(0x64) & 0x02);
    outb(0x60, cb);

    // Read everything until the status register says it's empty
    while (inb(0x64) & 0x01) inb(0x60);

    // Enable the Keyboard Port on the Controller
    while (inb(0x64) & 0x02); // Wait for input buffer empty
    outb(0x64, 0xAE);         // 0xAE = Enable 1st PS/2 port

    // Reset the actual Keyboard
    while (inb(0x64) & 0x02);
    outb(0x60, 0xFF);

    // ACK check
    // Don't just read and ignore; check if it's actually 0xFA
    // If it's 0xFE, the keyboard is asking to Resend
    while (!(inb(0x64) & 0x01));
    if (inb(0x60) != 0xFA) {
        t_print("init_keyboard: Reset failed or got NACK");
        return;
    }

    // Wait for Self-Test (0xAA)
    while (!(inb(0x64) & 0x01));
    if (inb(0x60) != 0xAA) {
        t_print("init_keyboard: Keyboard hardware failure");
        return;
    }

    keyboard_ready = true;
}

extern void keyboard_handler() {
    uint8_t status = inb(0x64);
    if (!(status & 0x01) || (status & 0x20) || !keyboard_ready) { // Data is not ready ready OR it is mouse data OR the keyboard is not ready.
        outb(0x20, 0x20);
        return;
    }

    uint8_t scancode = inb(0x60);

    // if (echo_scancodes) {
    //     t_print("Scancode detected");
    //     printf("Status: 0x%x\n", status);
    //     printf("Scancode: 0x%x\n", scancode);
    // }

    if (scancode == 0xE0) {
        is_extended = true;
    }
    else if (is_extended) {
        if (scancode == 0x48) push_to_kbd_buffer(KEY_UP);
        else if (scancode == 0x50) push_to_kbd_buffer(KEY_DOWN);
        is_extended = false;
    }
    else {
        if (scancode == 0x2A || scancode == 0x36) shift_pressed = true;
        else if (scancode == 0xAA || scancode == 0xB6) shift_pressed = false;
        else if (!(scancode & 0x80)) {
            size_t offset = shift_pressed ? KBD_LEN : 0;
            char c = kbd[scancode + offset];
            if (c > 0) push_to_kbd_buffer(c);
        }
    }

    outb(0x20, 0x20);
}
