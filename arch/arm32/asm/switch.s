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

.section .text
.global switch_task

switch_task:
    /* Save the context of the old task */
    push {r4-r11, lr}

    /* Save the current Stack Pointer into the 'old_sp' pointer */
    str sp, [r0]

    /* Load the new Stack Pointer from 'next_sp' */
    mov sp, r1

    /* Restore the context of the new task */
    pop {r4-r11, pc}
