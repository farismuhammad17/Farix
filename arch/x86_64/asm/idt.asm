; -----------------------------------------------------------------------
; Farix Operating System
; Copyright (C) 2026  Faris Muhammad
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU Affero General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU Affero General Public License for more details.
;
; You should have received a copy of the GNU Affero General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; -----------------------------------------------------------------------

[BITS 64]
section .text

global timer_handler_stub
global keyboard_handler_stub
global mouse_handler_stub
global syscall_handler_stub
global ahci_interrupt_handler_stub
global apic_spurious_handler_stub
global load_idt

extern timer_handler
extern keyboard_handler
extern mouse_handler
extern syscall_handler
extern exception_handler
extern ahci_interrupt_handler
extern apic_spurious_handler

; Helper macro to save all 64-bit general purpose registers
; This has to match the top half of syscalls_registers_x86_64_t
%macro PUSHALL 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

; Actual POP macro with correct register override mapping
%macro POPALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

timer_handler_stub:
    PUSHALL
    call timer_handler
    POPALL
    iretq

keyboard_handler_stub:
    PUSHALL
    call keyboard_handler
    POPALL
    iretq

mouse_handler_stub:
    PUSHALL
    call mouse_handler
    POPALL
    iretq

syscall_handler_stub:
    push 0              ; err_code dummy placeholder
    push 128            ; int_no (Syscall vector)
    PUSHALL

    ; Pass pointer to the register structure via RDI (1st parameter)
    mov rdi, rsp
    call syscall_handler

    POPALL
    add rsp, 16         ; Clean up int_no and err_code stack pushes
    iretq               ; Return back down to User-space Ring 3

ahci_interrupt_handler_stub:
    PUSHALL
    call ahci_interrupt_handler
    POPALL
    iretq

apic_spurious_handler_stub:
    PUSHALL
    call apic_spurious_handler
    POPALL
    iretq

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0              ; Push fake error code
    push %1             ; Push interrupt exception identification vector number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1             ; CPU already pushed error code, just push exception number
    jmp isr_common_stub
%endmacro

; Generate the CPU Exception ISR entries
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

isr_common_stub:
    PUSHALL

    ; System V Calling Convention: Pass stack pointer struct to C via RDI
    mov rdi, rsp
    call exception_handler

    POPALL
    add rsp, 16         ; Clean up exception number and error code stack variables
    iretq               ; Final return from interrupt execution context
