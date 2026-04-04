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

#ifndef MOUSE_H
#define MOUSE_H

/*

Mouse

Tracking a mouse is more complex than a keyboard because it doesn't just send a
single character; it sends continuous updates about movement and button states.
If the kernel had to manually ask the mouse for its position every millisecond,
it would waste massive amounts of CPU cycles. Instead, the mouse hardware
generates an interrupt whenever it is moved or clicked, providing a packet of
relative movement data.

The mouse sends delta values—how much it moved in the X and Y directions—rather
than absolute screen coordinates. The kernel must collect these packets, track
the state of the left and right buttons, and store them in a MouseEvent buffer.
By using an asynchronous handler, the cursor can move smoothly across the screen
even while the kernel is performing heavy background calculations.

Because the mouse usually communicates through a controller (like the PS/2
controller on x86 or USB HID on modern systems), the implementation is split:

x86_32: src/arch/x86_32/mouse.c

void init_mouse():
    Sends the necessary magic sequences to the hardware to enable "streaming mode"
    so the mouse starts sending interrupts, and clears the initial event buffer.

void mouse_handler():
    The interrupt service routine that pulls raw bytes from the hardware. It
    assembles these bytes into a MouseEvent struct, calculating the
    directional shift and button states before pushing the event onto the
    circular buffer.

mouse_buffer, buffer_head, buffer_tail:
    A circular queue of MouseEvent objects. This allows the GUI or a user
    application to "drain" the movement history at its own pace without
    missing a single click or twitch of the cursor.

*/

#include <stdbool.h>

#define MOUSE_BUFFER_LEN 16

typedef struct MouseEvent {
    int8_t x, y;
    int8_t scroll;
    bool left, right;
} MouseEvent;

extern MouseEvent mouse_buffer[16];
extern volatile uint8_t buffer_head;
extern volatile uint8_t buffer_tail;

void init_mouse();
extern void mouse_handler();

#endif
