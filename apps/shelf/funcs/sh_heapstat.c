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

#include "cmds.h"

void cmd_heapstat(UNUSED_ARG const char* args) {
    int fault_addr;
    int status = HEAP_AUDIT(fault_addr);

    switch (status) {
        case 0: sh_print("Heap verified\n");
            break;
        case 1: sh_print("HEAP CORRUPTION: Bad Magic at %p", fault_addr);
            break;
        case 2: sh_print("HEAP CORRUPTION: Unaligned segment pointer %p\n", fault_addr);
            break;
        case 3: sh_print("HEAP CORRUPTION: Circular or backwards link at %p", fault_addr);
            break;
        case 4: sh_print("HEAP CORRUPTION: Broken backlink at %p", fault_addr);
            break;

        default:
            sh_print("heapstat: Returns unknown status %d", status);
            break;
    }
}
