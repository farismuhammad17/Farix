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

    ; Save current RSP to old_stack_pointer stack tracking block
    mov [rdi], rsp
    ; Switch stack context pointers over to target thread
    mov rsp, rsi

    ; Pop new thread state off the fresh stack
    pop r15 ; task args pointer
    pop r14 ; 0
    pop r13 ; 0
    pop r12 ; 0
    pop rbx ; 0
    pop rbp ; 0

    ; Setup the 64-bit System V calling argument rule
    ; We copy the argument pointer from R15 into RDI so task_trampoline can read it.
    mov rdi, r15

    ; Jump out directly into task_trampoline
    ret
