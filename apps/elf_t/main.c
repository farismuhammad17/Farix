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

#define SYS_WRITE 1

// Took from the journal (finally became useful)
int main(int argc, char** argv) {
    const char* msg = "Farix is alive!\n";
    int len = 16;

    asm volatile (
        "mov %0, %%eax\n"  // SYS_WRITE
        "mov %1, %%ebx\n"  // file descriptor 1 (stdout)
        "mov %2, %%ecx\n"  // buffer pointer
        "mov %3, %%edx\n"  // length
        "int $0x80\n"
        :
        : "i"(SYS_WRITE), "i"(1), "r"(msg), "r"(len)
        : "eax", "ebx", "ecx", "edx"
    );

    while(1);
}
