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

#include "libc/syscalls.h"
#include "process/task.h"

#define BCM2837_IRQ_BASIC_PENDING (0x3F000000 + 0xB200)

typedef struct syscalls_registers_arm32_t {
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
    uint32_t lr;
} syscalls_registers_arm32_t;

// Dump general purpose registers for deeper debugging
static void dump_register_info(syscalls_registers_arm32_t* regs) {
    printf("--- Register values ---\n");
    printf("R0: %08lx  R1: %08lx  R2: %08lx  R3: %08lx\n", regs->r0, regs->r1, regs->r2, regs->r3);
    printf("R4: %08lx  R5: %08lx  R6: %08lx  R7: %08lx\n", regs->r4, regs->r5, regs->r6, regs->r7);
    printf("R8: %08lx  R9: %08lx  R10: %lx   R11: %lx\n", regs->r8, regs->r9, regs->r10, regs->r11);
    printf("R12: %08lx LR: %08lx\n", regs->r12, regs->lr);
}

// Dumps multitasking information in case of race condition errors
static void dump_multitasking_info() {
    printf("--- Multitasking ---\n");
    printf("Name:           %s (%lu)\n", current_task->name, current_task->id);
    printf("Stack Pointer:  0x%08lx\n", current_task->stack_pointer);
    printf("Stack Origin:   %p\n", (void*) current_task->stack_origin);
    printf("Page Directory: %p\n", (void*) current_task->page_directory);
}

void syscall_handler(syscalls_registers_arm32_t* regs) {
    // ARM Convention: r7 = ID, r0 = arg1, r1 = arg2, r2 = arg3
    uint32_t syscall_id = regs->r7;

    switch (syscall_id) {
        case SYS_WRITE:
            // arg1 (r0) = fd, arg2 (r1) = buf, arg3 (r2) = len
            regs->r0 = _write(regs->r0, (char*) regs->r1, regs->r2);
            break;

        case SYS_READ:
            regs->r0 = _read(regs->r0, (char*) regs->r1, regs->r2);
            break;

        case SYS_EXIT:
            _exit(regs->r0); // Doesn't return
            break;

        case SYS_SBRK:
            regs->r0 = (uint32_t) _sbrk(regs->r0);
            break;

        default:
            printf("Unknown syscall: %d\n", syscall_id);
            regs->r0 = -1;
            break;
    }
}

void exception_undef_handler(syscalls_registers_arm32_t* regs) {
    asm volatile("cpsid i");

    // BSOD
    terminal_change_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    terminal_clear();

    // On ARM, the LR points to the instruction AFTER the bad one
    uint32_t faulting_instruction = regs->lr - 4;

    printf("Exception: Undefined Instruction\n");
    printf("Faulting Instruction: %08lx\n", faulting_instruction);
    printf("LR: %08lx  CPSR: [N/A]\n", regs->lr); // CPSR can be added later if you push it

    dump_register_info(regs);
    dump_multitasking_info();

    while(1) asm volatile("wfi");
}

void exception_prefetch_handler(syscalls_registers_arm32_t* regs) {
    asm volatile("cpsid i");

    terminal_change_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    terminal_clear();

    uint32_t ifar, ifsr;
    asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r"(ifar)); // Instruction Fault Address Register (IFAR)
    asm volatile("mrc p15, 0, %0, c5, c0, 1" : "=r"(ifsr)); // Instruction Fault Status Register  (IFSR)

    printf("Exception: PREFETCH ABORT (Instruction Fetch Fault)\n");
    printf("IFSR (Status): %lx\n", ifsr);
    printf("Address CPU tried to execute (IFAR): %lx\n", ifar);
    printf("PC: %lx  LR: %lx\n", regs->lr - 4, regs->lr);

    dump_register_info(regs);
    dump_multitasking_info();

    while(1) asm volatile("wfi");
}

void exception_data_abort_handler(syscalls_registers_arm32_t* regs) {
    asm volatile("cpsid i"); // Disable interrupts

    terminal_change_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    terminal_clear();

    uint32_t dfar, dfsr;
    // MRC p15, 0, <reg>, c6, c0, 0 -> Read Data Fault Address Register (DFAR)
    asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar));
    // MRC p15, 0, <reg>, c5, c0, 0 -> Read Data Fault Status Register (DFSR)
    asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr));

    printf("Exception: DATA ABORT (Memory Access Fault)\n");
    printf("Faulting Address (DFAR): 0x%08lx\n", dfar);
    printf("Status (DFSR): 0x%08lx\n", dfsr);

    // Bit 11 of DFSR indicates if the fault was a Write (1) or Read (0)
    const char* operation = (dfsr & (1 << 11)) ? "WRITE" : "READ";
    printf("Attempted Operation: %s\n", operation);

    printf("PC (Faulting Instruction): 0x%08lx\n", regs->lr);

    dump_register_info(regs);
    dump_multitasking_info();

    while(1) asm volatile("wfi");
}

void irq_handler(syscalls_registers_arm32_t* regs) {
    uint32_t pending = inw(BCM2837_IRQ_BASIC_PENDING);

    // Check if the System Timer (IRQ 1) is pending
    // Note: On Pi 3, the System Timer is usually mapped to bit 1
    // of the GPU pending registers or handled via the Basic Pending.
    if (pending & (1 << 0)) {
        timer_handler();
    }
}
