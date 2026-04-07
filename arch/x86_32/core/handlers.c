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

#include <stdint.h>
#include <stdio.h>

#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "libc/syscalls.h"
#include "memory/vmm.h"
#include "process/task.h"

typedef struct syscalls_registers_x86_32_t {
    uint32_t ds;                                           // Data segment (pushed by us)
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                             // Pushed in stub
    uint32_t eip, cs, eflags, useresp, ss;                 // Pushed by CPU
} syscalls_registers_x86_32_t;

static const char* exception_messages[] = {
    "Division By Zero",             // 0
    "Debug",                        // 1
    "Non Maskable Interrupt",       // 2
    "Breakpoint",                   // 3
    "Into Detected Overflow",       // 4
    "Out of Bounds",                // 5
    "Invalid Opcode",               // 6
    "No Coprocessor",               // 7
    "Double Fault",                 // 8
    "Coprocessor Segment Overrun",  // 9
    "Bad TSS",                      // 10
    "Segment Not Present",          // 11
    "Stack Fault",                  // 12
    "General Protection Fault",     // 13
    "Page Fault",                   // 14
    "Unknown Interrupt",            // 15
    "Floating Point Error",         // 16
    "Alignment Check",              // 17
    "Machine Check",                // 18
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Dump general purpose registers for deeper debugging
static void dump_register_info(syscalls_registers_x86_32_t* regs) {
    printf("--- Register values ---\n");
    printf("EAX: %lx EBX: %lx ECX: %lx EDX: %lx\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("EDI: %lx ESI: %lx EBP: %lx ESP: %lx\n", regs->edi, regs->esi, regs->ebp, regs->esp_dummy);
}

// Dumps multitasking information in case of race condition errors
static void dump_multitasking_info() {
    if (current_task == NULL) return;

    printf("--- Multitasking ---\n");
    printf("Name:           %s (%lu)\n", current_task->name, current_task->id);
    printf("Stack Pointer:  0x%08lx\n", current_task->stack_pointer);
    printf("Stack Origin:   %p\n", (void*) current_task->stack_origin);
    printf("Page Directory: %p (%lu)\n", (void*) current_task->page_directory, (uint32_t) current_task->privilege);
}

void syscall_handler(syscalls_registers_x86_32_t* regs) {
    switch (regs->eax) {
        case SYS_WRITE:
            regs->eax = _write(regs->ebx, (char*) regs->ecx, regs->edx);
            break;

        case SYS_READ:
            regs->eax = _read(regs->ebx, (char*) regs->ecx, regs->edx);
            break;

        case SYS_EXIT:
            _exit(regs->ebx);
            break;

        case SYS_SBRK:
            regs->eax = (uint32_t) _sbrk(regs->ebx);
            break;

        default:
            printf("Unknown syscall: %ld\n", regs->eax);
            regs->eax = -1;
            break;
    }
}

void exception_handler(syscalls_registers_x86_32_t* regs) {
    asm volatile("cli");

    uart_printf("Exception: %ld (%s)\n", regs->int_no, exception_messages[regs->int_no]);

    // BSOD
    terminal_change_color(0x1F); // Blue background, White foreground
    terminal_clear();

    if (regs->int_no < 32) {
        printf("Exception: %ld (%s)\n", regs->int_no, exception_messages[regs->int_no]);
    } else {
        printf("Unknown Exception: %ld\n", regs->int_no);
    }

    printf("Error Code: %lx\n", regs->err_code);
    printf("EIP: %lx  CS: %lx  EFLAGS: %lx\n", regs->eip, regs->cs, regs->eflags);

    // Specifically for Page Faults
    if (regs->int_no == 14) {
        uint32_t faulting_address;
        asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
        printf("Faulting Address (CR2): %lx\n", faulting_address);

        printf("Reason: %s, %s, %s\n",
            (regs->err_code & PAGE_PRESENT) ? "Page-level protection" : "Non-present page",
            (regs->err_code & PAGE_RW)      ? "Write" : "Read",
            (regs->err_code & PAGE_USER)    ? "User mode" : "Kernel mode");
    }

    dump_register_info(regs);
    dump_multitasking_info();

    while(1) asm volatile("hlt");
}
