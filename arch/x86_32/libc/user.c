/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include "farix.h"

/*
Writing inline assembly is tedious, this function just abstract that off.
Unused arguments are to be set to 0.
*/
int32_t farix_syscall(uint32_t sys_id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    int32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)     // Return value comes back in EAX
        : "a"(sys_id),  // EAX
          "b"(arg1),    // EBX
          "c"(arg2),    // ECX
          "d"(arg3),    // EDX
          "S"(arg4),    // ESI
          "D"(arg5)     // EDI
        : "memory"      // Tells compiler that thZ memory might change
    );
    return ret;
}
