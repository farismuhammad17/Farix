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

#include <stdint.h>

#include "cpu/ints.h"

#define IDT_GATE_KERNEL 0x8E  // 1000 1110: Present, Ring 0, Interrupt Gate
#define IDT_GATE_USER   0xEE  // 1110 1110: Present, Ring 3, Interrupt Gate

typedef struct {
    uint16_t base_low;    // Lower 16 bits of the handler address
    uint16_t sel;         // Kernel Segment Selector (0x08)
    uint8_t  ist;         // Interrupt Stack Table (set to 0 for default stack)
    uint8_t  flags;       // Permission / Attributes byte (0x8E or 0xEE)
    uint16_t base_mid;    // Middle 16 bits of the handler address
    uint32_t base_high;   // Upper 32 bits of the handler address
    uint32_t reserved;    // Always 0
} __attribute__((packed)) idt_entry;

typedef struct {
    uint16_t limit;       // Size of IDT - 1
    uint64_t base;        // 64-bit Base address of the IDT array
} __attribute__((packed)) idt_ptr;

// Defined in idt.asm
void timer_handler_stub();
void keyboard_handler_stub();
void mouse_handler_stub();
void syscall_handler_stub();
void ahci_interrupt_handler_stub();
void apic_spurious_handler_stub();
void isr0();  void isr1();  void isr2();  void isr3();
void isr4();  void isr5();  void isr6();  void isr7();
void isr8();  void isr9();  void isr10(); void isr11();
void isr12(); void isr13(); void isr14(); void isr15();
void isr16(); void isr17(); void isr18(); void isr19();
void isr20(); void isr21(); void isr22(); void isr23();
void isr24(); void isr25(); void isr26(); void isr27();
void isr28(); void isr29(); void isr30(); void isr31();

static idt_entry idt[256];
static idt_ptr   idtp;

/* Set IDT gate at index `num` with given 64-bit function, selector, and flags */
static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (base & 0xFFFF);
    idt[num].base_mid  = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;

    idt[num].sel       = sel;
    idt[num].ist       = 0;
    idt[num].flags     = flags;
    idt[num].reserved  = 0;
}

/*
Initialise the IDT by setting everything to exception 15 (Unknown interrupt),
then setting the interrupts we actually use, so that we can catch stray interrupts.
*/
void init_interrupts() {
    idtp.limit = (sizeof(idt_entry) * 256) - 1;
    idtp.base  = (uint64_t) &idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint64_t) isr15, 0x08, IDT_GATE_KERNEL);
    }

    // Hardware IRQs
    idt_set_gate(32, (uint64_t) timer_handler_stub,    0x08, IDT_GATE_KERNEL);
    idt_set_gate(33, (uint64_t) keyboard_handler_stub, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(44, (uint64_t) mouse_handler_stub,    0x08, IDT_GATE_KERNEL);

    // Storage
    idt_set_gate(46, (uint64_t) ahci_interrupt_handler_stub, 0x08, IDT_GATE_KERNEL);

    // APIC spurious interrupts
    idt_set_gate(255, (uint64_t) apic_spurious_handler_stub, 0x08, IDT_GATE_KERNEL);

    // Syscall
    idt_set_gate(128, (uint64_t) syscall_handler_stub, 0x08, IDT_GATE_USER);

    // CPU Exceptions (0-31)
    idt_set_gate(0,  (uint64_t) isr0,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(1,  (uint64_t) isr1,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(2,  (uint64_t) isr2,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(3,  (uint64_t) isr3,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(4,  (uint64_t) isr4,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(5,  (uint64_t) isr5,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(6,  (uint64_t) isr6,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(7,  (uint64_t) isr7,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(8,  (uint64_t) isr8,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(9,  (uint64_t) isr9,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(10, (uint64_t) isr10, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(11, (uint64_t) isr11, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(12, (uint64_t) isr12, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(13, (uint64_t) isr13, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(14, (uint64_t) isr14, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(15, (uint64_t) isr15, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(16, (uint64_t) isr16, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(17, (uint64_t) isr17, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(18, (uint64_t) isr18, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(19, (uint64_t) isr19, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(20, (uint64_t) isr20, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(21, (uint64_t) isr21, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(22, (uint64_t) isr22, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(23, (uint64_t) isr23, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(24, (uint64_t) isr24, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(25, (uint64_t) isr25, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(26, (uint64_t) isr26, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(27, (uint64_t) isr27, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(28, (uint64_t) isr28, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(29, (uint64_t) isr29, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(30, (uint64_t) isr30, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(31, (uint64_t) isr31, 0x08, IDT_GATE_KERNEL);

    asm volatile("lidt %0" : : "m"(idtp));
}

/*
Exposes idt_set_gate to the rest of the kernel in an architecture independent
way, by assuming sel=0x08, and flag to KERNEL.
*/
void set_interrupt_kernel(uint8_t vector, void* handler) {
    idt_set_gate(vector, (uint64_t) handler, 0x08, IDT_GATE_KERNEL);
}

/*
Exposes idt_set_gate to the rest of the kernel in an architecture independent
way, by assuming sel=0x08, and flag to USER.
*/
void set_interrupt_user(uint8_t vector, void* handler) {
    idt_set_gate(vector, (uint64_t) handler, 0x08, IDT_GATE_USER);
}

/*
Clears out the interrupt at a given vector, by setting it to exception 15,
which is for handling "Unknown interrupts".
*/
void clear_interrupt(uint8_t vector) {
    idt_set_gate(vector, (uint64_t) isr15, 0x08, IDT_GATE_KERNEL);
}
