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

#include "cpu/idt.h"

struct idt_entry idt[256];
struct idt_ptr   idtp;

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;  // Defines the IDT pointer's size how x86 likes
    idtp.base  = (uint32_t)&idt;                        // The memory address of the IDT array

    for (int i = 0; i < 256; i++) {
        uint32_t base = (uint32_t) default_handler_stub;
        idt_set_gate(i, base, 0x08, 0x8E);
    }

    // Register Timer Handler at index 32 (IRQ 0)
    idt_set_gate(32, (uint32_t) timer_handler_stub, 0x08, 0x8E);

    // Register Keyboard Handler at index 33
    idt_set_gate(33, (uint32_t) keyboard_handler_stub, 0x08, 0x8E);

    asm volatile("lidt %0" : : "m"(idtp));
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (base & 0xFFFF);        // Lower 16 bits
    idt[num].base_high = (base >> 16) & 0xFFFF;  // Upper 16 bits

    idt[num].sel     = sel;    // Usually 0x08 (Kernel Code Segment)
    idt[num].always0 = 0;      // Always 0
    idt[num].flags   = flags;  // 0x8E (Present, Ring 0, Interrupt Gate)
}
