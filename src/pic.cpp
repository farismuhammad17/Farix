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

#include "../include/pic.h"

void pic_remap() {
    // ICW1
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2
    outb(PIC1_DATA, 0x20); // Master -> 32
    outb(PIC2_DATA, 0x28);         // Slave -> 40

    // ICW3
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    // ICW4
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Use 0xFD to ONLY enable Keyboard (IRQ 1). Disable Timer (IRQ 0) for now.
    outb(PIC1_DATA, 0xFD);
    outb(PIC2_DATA, 0xFF);

    outb(0x64, 0xAE); // Enable keyboard
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
}
