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

#ifndef SYSCALLS_H
#define SYSCALLS_H

#define SYS_WRITE 1
#define SYS_READ  2
#define SYS_EXIT  3

#include <stdint.h>

struct registers_t {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                             // Pushed manually in stub
    uint32_t eip, cs, eflags, useresp, ss;                 // Pushed by CPU on interrupt
};

extern "C" void syscall_handler(registers_t* regs);

#endif
