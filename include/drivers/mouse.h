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

#define MOUSE_BUFFER_LEN 16

struct MouseEvent {
    int8_t x, y;
    int8_t scroll;
    bool left, right;
};

extern MouseEvent mouse_buffer[16];
extern uint8_t    buffer_head;
extern uint8_t    buffer_tail;

void init_mouse();

extern "C" void mouse_handler();

#endif
