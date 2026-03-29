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

#include "memory/vmm.h"

#include "cpu/idt.h"

struct idt_entry idt[256];
struct idt_ptr   idtp;

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;  // Defines the IDT pointer's size how x86 likes
    idtp.base  = (uint32_t) PHYSICAL_TO_VIRTUAL(&idt);  // The memory address of the IDT array

    for (int i = 0; i < 256; i++) {
        uint32_t base = (uint32_t) default_handler_stub;
        idt_set_gate(i, base, 0x08, IDT_GATE_KERNEL);
    }

    // Hardware IRQs
    idt_set_gate(32, (uint32_t) timer_handler_stub,    0x08, IDT_GATE_KERNEL);
    idt_set_gate(33, (uint32_t) keyboard_handler_stub, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(44, (uint32_t) mouse_handler_stub,    0x08, IDT_GATE_KERNEL);

    // Syscall
    idt_set_gate(128, (uint32_t) syscall_handler_stub, 0x08, IDT_GATE_USER);

    // CPU Exceptions (0-31)
    idt_set_gate(0,  (uint32_t) isr0,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(1,  (uint32_t) isr1,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(2,  (uint32_t) isr2,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(3,  (uint32_t) isr3,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(4,  (uint32_t) isr4,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(5,  (uint32_t) isr5,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(6,  (uint32_t) isr6,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(7,  (uint32_t) isr7,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(8,  (uint32_t) isr8,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(9,  (uint32_t) isr9,  0x08, IDT_GATE_KERNEL);
    idt_set_gate(10, (uint32_t) isr10, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(11, (uint32_t) isr11, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(12, (uint32_t) isr12, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(13, (uint32_t) isr13, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(14, (uint32_t) isr14, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(15, (uint32_t) isr15, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(16, (uint32_t) isr16, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(17, (uint32_t) isr17, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(18, (uint32_t) isr18, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(19, (uint32_t) isr19, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(20, (uint32_t) isr20, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(21, (uint32_t) isr21, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(22, (uint32_t) isr22, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(23, (uint32_t) isr23, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(24, (uint32_t) isr24, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(25, (uint32_t) isr25, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(26, (uint32_t) isr26, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(27, (uint32_t) isr27, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(28, (uint32_t) isr28, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(29, (uint32_t) isr29, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(30, (uint32_t) isr30, 0x08, IDT_GATE_KERNEL);
    idt_set_gate(31, (uint32_t) isr31, 0x08, IDT_GATE_KERNEL);

    asm volatile("lidt %0" : : "m"(idtp));
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (base & 0xFFFF);        // Lower 16 bits
    idt[num].base_high = (base >> 16) & 0xFFFF;  // Upper 16 bits

    idt[num].sel     = sel;    // Usually 0x08 (Kernel Code Segment)
    idt[num].always0 = 0;      // Always 0
    idt[num].flags   = flags;
}
