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

#include "architecture/io.h"

#include "drivers/mouse.h"

MouseEvent mouse_buffer[MOUSE_BUFFER_LEN];
uint8_t    buffer_head = 0;
uint8_t    buffer_tail = 0;

uint8_t    mouse_cycle = 0;
int8_t     mouse_bytes[4];

void mouse_wait(uint8_t type) {
    // Wait for the PIC to be ready to send/receive data
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

void mouse_write(uint8_t data) {
    mouse_wait(1);      // Wait for controller to be ready to take a command
    outb(0x64, 0xD4);   // Tell controller: "Next byte is for the mouse"
    mouse_wait(1);      // Wait for controller to be ready to take the data
    outb(0x60, data);   // Send the actual mouse data

    mouse_wait(0); // Wait for ACK from mouse
    inb(0x60);     // Read and discard ACK
}

void mouse_reset() {
    mouse_write(0xFF);  // Reset command
    mouse_wait(0);
    inb(0x60);          // Read ACK
    mouse_wait(0);
    inb(0x60);          // Read self-test result 1
    mouse_wait(0);
    inb(0x60);          // Read self-test result 2
}

void init_mouse() {
    uint8_t status;

    mouse_reset();

    // This tells the mouse to use the IntelliMouse extension
    mouse_write(0xF3); // Set Sample Rate
    mouse_write(200);
    mouse_write(0xF3); // Set Sample Rate
    mouse_write(100);
    mouse_write(0xF3); // Set Sample Rate
    mouse_write(80);

    // Enable the mouse device
    mouse_wait(1);
    outb(0x64, 0xA8);

    // Enable mouse interrupts
    mouse_wait(1);
    outb(0x64, 0x20); // Tell controller to send command to read status
    mouse_wait(0);
    status = inb(0x60) | 2; // Read status and enable mouse interrupt bit
    mouse_wait(1);
    outb(0x64, 0x60); // Tell controller to send command to set status
    mouse_wait(1);
    outb(0x60, status); // Send back the updated status

    // Tell mouse to use default settings
    mouse_write(0xF6);
    mouse_wait(0);
    inb(0x60); // Acknowledge

    // Enable data reporting
    mouse_write(0xF4);
    mouse_wait(0);
    inb(0x60); // Acknowledge
}

extern "C" void mouse_handler() {
    uint8_t status = inb(0x64);
    if (!(status & 0x01) || !(status & 0x20)) goto end;

    mouse_bytes[mouse_cycle] = inb(0x60);
    mouse_cycle++;

    if (mouse_cycle == 4) {
        mouse_cycle = 0;

        uint8_t flags = mouse_bytes[0];

        mouse_buffer[buffer_head].left   = (flags & 0x01);
        mouse_buffer[buffer_head].right  = (flags & 0x02);

        // Store data in the buffer
        mouse_buffer[buffer_head].x      = mouse_bytes[1];
        mouse_buffer[buffer_head].y      = mouse_bytes[2];
        mouse_buffer[buffer_head].scroll = mouse_bytes[3];

        buffer_head = (buffer_head + 1) % MOUSE_BUFFER_LEN;
    }

end:
    outb(0xA0, 0x20); // EOI Slave
    outb(0x20, 0x20); // EOI Master
}
