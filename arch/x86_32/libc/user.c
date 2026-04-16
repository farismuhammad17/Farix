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

#include "farix.h"

// Writing inline assembly is tedious, this function just abstract that off.
// Unused arguments are set to 0
int32_t farix_syscall(uint32_t sys_id, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    int32_t ret;
    asm volatile (
        "mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "mov %4, %%edx\n"
        "int $0x80\n"
        : "=a"(ret)
        : "g"(sys_id), "g"(arg1), "g"(arg2), "g"(arg3)
        : "ebx", "ecx", "edx"
    );
    return ret;
}
