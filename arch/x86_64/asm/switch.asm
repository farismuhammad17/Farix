; -----------------------------------------------------------------------
; Farix Operating System
; Copyright (C) 2026  Faris Muhammad

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU Affero General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU Affero General Public License for more details.

; You should have received a copy of the GNU Affero General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; -----------------------------------------------------------------------

[BITS 64]
section .text

global switch_task

switch_task:
    ; System V ABI says the caller already saved RAX, RCX, RDX, RSI, RDI, R8-R11 if it needed them.
    ; We only manually preserve the "callee-saved" registers to capture the exact task state.
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; RDI = uint64_t* old_stack_pointer  (pointer to where we save the old stack)
    ; RSI = uint64_t  new_stack_pointer  (the raw stack address of the next task)

    mov [rdi], rsp      ; Save the current task's stack pointer into last->stack_pointer
    mov rsp, rsi        ; Load the next task's stack pointer into the RSP register

    ; This section is running the new task
    ; Pop its preserved state off the new stack in exact reverse order since its a stack.
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret
