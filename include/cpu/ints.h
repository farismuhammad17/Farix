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

#ifndef INTS_H
#define INTS_H

/*

Interrupts

Making a kernel, a piece of software, manually manage everything by constantly
checking stuff like keyboard inputs, mouse inputs, page faults, etc. is really
slow, and throttles our performance. Often times, when such situations show up,
the hardware comes to the rescue.

Interrupts are architecture specified addresses that the CPU jumps to, whenever
a specific 'event' is triggered. If a key on the keyboard is pressed, it jumps
to the keyboard interrupt, and if we enounter a page fault, it jumps to its
address. This keeps everything lean, and throws all the overhead of these checks
to the CPU.

Since this depends on the CPU, it is actually architecture specific. For the
impelementation, refer:

x86_32 : src/arch/x86_32/idt.c
arm32  : src/arch/arm32/evt.c

void init_interrupts:
    This function sets all the addresses up, and informs whatever architecture
    the kernel is running on of all the things to watch out for. Since the CPU
    only jumps to these addresses, we actually have to tell it what to exactly
    do once we reach these addresses. These are defined in assembly functions
    that call functions defined in C.

*/

void init_interrupts();

#endif
