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

#include <stdarg.h>
#include <stdint.h>

#include "klib/stdio.h"
#include "klib/string.h"

#include "hal.h"

#include "drivers/terminal.h"
#include "memory/vmm.h"
#include "process/task.h"

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

/* Panic shell helper to convert hexadecimal to integer */
static uint64_t hex_to_int(char* s) {
    uint64_t res = 0;
    // Skip "0x" if the user typed it
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;

    while (*s) {
        uint8_t byte = *s;
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F') byte = byte - 'A' + 10; // Handle caps
        else break; // Stop at first non-hex char

        res = (res << 4) | (byte & 0xF);
        s++;
    }
    return res;
}

/* Panic shell printf function */
static void panic_err_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    static char buffer[256];
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    if (len > 0) {
        echo_raw(buffer, len);
    }
}

/* Dump general purpose registers for deeper debugging upon crash */
static inline void dump_register_info(syscalls_registers_x86_64_t* regs) {
    panic_err_printf(
        "--- Register values ---\n"
        "RAX: %llx RBX: %llx RCX: %llx RDX: %llx\n"
        "RDI: %llx RSI: %llx RBP: %llx RSP: %llx\n",
        regs->rax, regs->rbx, regs->rcx, regs->rdx,
        regs->rdi, regs->rsi, regs->rbp, regs->rsp
    );
}

/* Dumps multitasking information in case of race condition errors upon crash */
static inline void dump_multitasking_info() {
    if (unlikely(current_task == NULL)) return;

    panic_err_printf("--- Multitasking ---\n");
    panic_err_printf("Name:  %s (ID:%u)\n", current_task->name, current_task->id);
    panic_err_printf("Stack: 0x%016llx -> 0x%016llx\n", (uint64_t) current_task->stack_origin, current_task->stack_pointer);
    panic_err_printf("Page:  %p (PRIVILEGE:%u)\n", (void*) current_task->page_directory, current_task->privilege);
}

/* Dumps call log upon crash */
static inline void dump_call_log(int funcs_per_line) {
    if (likely(!__DEBUG__)) return;

    panic_err_printf("--- Call log ---\n");
    for (size_t i = 0; i < MAX_LOG_LEN; i++) {
        panic_err_printf("%d:%s ", i, call_log[i]);
        if ((i + 1) % funcs_per_line == 0) {
            panic_err_printf("\n");
        }
    }
    panic_err_printf("(%s at %d)\n", last_call_finished ? "Finished" : "Unfinished", log_index - 1);
}

// TODO: use t_printf instead, but t_print doesn't support colors yet

/* Called upon exception caught by IDT */
void exception_handler(syscalls_registers_x86_64_t* regs) {
    asm volatile("cli");

    // BSOD
    terminal_change_color(0x1F); // Blue background, White foreground
    terminal_clear();

    if (regs->int_no < 32) {
        panic_err_printf("Exception: %d (%s)\n", regs->int_no, exception_messages[regs->int_no]);
    } else {
        panic_err_printf("Exception: %d\n", regs->int_no);
    }

    if (unlikely(__DEBUG__)) {
        panic_err_printf("Logged number: %d | ", logged_num);
    }

    panic_err_printf("Error Code: %llx\n", regs->err_code);
    panic_err_printf("RIP: %llx  CS: %llx  RFLAGS: %llx\n", regs->rip, regs->cs, regs->rflags);

    switch (regs->int_no) {
        case 13: { // GPF
            panic_err_printf("Selector: %llx (%s)\n", regs->err_code & 0xFFFFFFFFFFFFFFF8,
                    (regs->err_code & 0x04) ? "LDT" : "GDT");
            break;
        }

        case 14: { // Page fault
            uint64_t faulting_address;
            asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

            panic_err_printf("Faulting Address (CR2): %llx\n", faulting_address);
            panic_err_printf("Reason: %s, %s, %s\n",
                (regs->err_code & PAGE_PRESENT) ? "Page unaccessible" : "Non-present page",
                (regs->err_code & PAGE_RW)      ? "Write fault" : "Read fault",
                (regs->err_code & PAGE_USER)    ? "User-mode" : "Kernel-mode");

            break;
        }
    }

    dump_call_log(3);
    dump_register_info(regs);
    dump_multitasking_info();

    while (1) system_halt();
}
