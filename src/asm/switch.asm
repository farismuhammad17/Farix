; -----------------------------------------------------------------------
; Copyright (C) 2026 Faris Muhammad

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; -----------------------------------------------------------------------

[BITS 32]
section .text

global switch_task
extern timer_handler
global timer_handler_stub

switch_task:
    pusha               ; Save current registers
    mov eax, [esp + 36] ; &last->stack_pointer
    mov [eax], esp

    mov esp, [esp + 40] ; Load next->stack_pointer
    popa                ; Restore new task's registers
    ret                 ; Return back into timer_handler -> timer_handler_stub

timer_handler_stub:
    pusha                ; Save everything before calling C++
    call timer_handler
    popa                 ; Restore everything (potentially from a NEW task stack!)
    iret                 ; Finish the hardware interrupt properly
