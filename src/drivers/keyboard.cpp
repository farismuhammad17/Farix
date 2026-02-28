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

#include <stdio.h>

#include "architecture/io.h"

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
constexpr bool echo_scancodes = false;

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

void init_keyboard() {
    // Wait for controller to be ready to accept a command
    while (inb(0x64) & 0x02);
    outb(0x60, 0xFF); // Send reset

    // Wait for ACK (0xFA)
    while (!(inb(0x64) & 0x01));
    inb(0x60); // Read and ignore

    // Wait for Self-Test Passed (0xAA)
    while (!(inb(0x64) & 0x01));
    inb(0x60); // Read and ignore

    keyboard_ready = true;
}

extern "C" void keyboard_handler() {
    uint8_t status = inb(0x64);
    if (!(status & 0x01) || (status & 0x20) || !keyboard_ready) { // Data is not ready ready OR it is mouse data OR the keyboard is not ready.
        outb(0x20, 0x20);
        return;
    }

    uint8_t scancode = inb(0x60);

    if (echo_scancodes) {
        printf("Status: 0x%x\n", status);
        printf("Scancode: 0x%x\n", scancode);
    }

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
