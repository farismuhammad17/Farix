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

#ifndef IDT_H
#define IDT_H

#define IDT_GATE_KERNEL 0x8E  // 1000 1110: Present, Ring 0, Interrupt Gate
#define IDT_GATE_USER   0xEE  // 1110 1110: Present, Ring 3, Interrupt Gate

#include <stdint.h>

extern "C" {
    void default_handler_stub();
    void timer_handler_stub();
    void keyboard_handler_stub();
    void mouse_handler_stub();
    void syscall_handler_stub();
    void isr0();  void isr1();  void isr2();  void isr3();
    void isr4();  void isr5();  void isr6();  void isr7();
    void isr8();  void isr9();  void isr10(); void isr11();
    void isr12(); void isr13(); void isr14(); void isr15();
    void isr16(); void isr17(); void isr18(); void isr19();
    void isr20(); void isr21(); void isr22(); void isr23();
    void isr24(); void isr25(); void isr26(); void isr27();
    void isr28(); void isr29(); void isr30(); void isr31();
}

struct idt_entry {
    uint16_t base_low;    // Lower 16 bits of the handler address
    uint16_t sel;         // Kernel Segment Selector (0x08)
    uint8_t  always0;     // This byte must be 0
    uint8_t  flags;       // That "Permission Slip" byte (0x8E)
    uint16_t base_high;   // Upper 16 bits of the handler address
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit; // 2 bytes
    uint32_t base;  // 4 bytes
} __attribute__((packed));

void init_idt();

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif
