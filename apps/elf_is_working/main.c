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

int main(int argc, char** argv) {
    const char* msg = "ELF is working!\n";

    for (int i = 0; msg[i] != '\0'; i++) {
        asm volatile (
            "mov $1, %%eax\n"   // Syscall number 1 (Example: putc)
            "mov %0, %%bl\n"    // Load the character into bl
            "int $0x80\n"       // Call the kernel
            :
            : "r"(msg[i])       // Input: current character
            : "eax", "ebx"      // Clobbered registers
        );
    }

    return 0;
}
