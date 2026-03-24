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

#include "cpu/pic.h"

void pic_remap() {
    // ICW1
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2
    outb(PIC1_DATA, 0x20); // Master -> 32
    outb(PIC2_DATA, 0x28); // Slave -> 40

    // ICW3
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    // ICW4
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* For understanding:
     *
     * 0 = Enabled | 1 = Disabled
     *
     * In the Master PIC (PIC1):
     * Bit | IRQ | Hardware      | Current  | Description
     * 0   | 0   | Timer         | Enabled  | Important for timing stuff, multitasking, etc.
     * 1   | 1   | Keyboard      | Enabled  |
     * 2   | 2   | Slave cascade | Enabled  | Bridge to second PIC.
     * 3   | 3   | COM2          | Disabled | Serial port 2; ancient way to connect modems or link two PCs.
     * 4   | 4   | COM1          | Disabled | Serial port 1.
     * 5   | 5   | LPT2          | Disabled | Parallel port 1; used for very old printers or scanners.
     * 6   | 6   | Floppy disk   | Disabled |
     * 7   | 7   | LPT1          | Disabled | Parallel port 2; often fires randomly if the hardware is noisy.
     *
     * In the Slave PIC (PIC2):
     * Bit | IRQ | Hardware      | Current  | Description
     * 0   | 8   | RTC           | Disabled | Real time clock; important for system clock, date, etc.
     * 1   | 9   | ACPI          | Disabled | Handles power buttons and "Sleep/Wake" signals.
     * 2   | 10  | PCI Device    | Disabled | Usually assigned to network cards, sound cards, etc.
     * 3   | 11  | PCI Device    | Disabled | Extra slot for more hardware.
     * 4   | 12  | PS/2 mouse    | Enabled  |
     * 5   | 13  | FPU           | Disabled | Math co-processor.
     * 6   | 14  | Primary ATA   | Disabled | Hard Drive Controller.
     * 7   | 15  | Secondary ATA | Disabled | Second hard drive or CD-ROM controller.
     */
    outb(PIC1_DATA, 0b11111000);
    outb(PIC2_DATA, 0b11101111);

    outb(0x64, 0xAE); // Enable keyboard
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
}
