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

#include "include/pic.h"

#include "cpu/ints.h"
#include "drivers/acpi/acpi.h"
#include "drivers/terminal.h"
#include "drivers/uart.h"
#include "memory/vmm.h"

#include "cpu/irq.h"

static uint32_t lapic_virt;
static uint32_t ioapic_virt;
static uint8_t  irq0_pin = 2; // Default PIT pin

static ACPI_TABLE_HEADER* ACPI_MADT_P;

static void parse_madt(ACPI_TABLE_MADT* madt);
static void ioapic_set_entry(uint8_t pin, uint64_t data);
static void irq_unmask(uint8_t pin, uint8_t vector);

static inline void lapic_write(uint32_t reg, uint32_t data);
static inline uint32_t lapic_read(uint32_t reg);

static inline void ioapic_write(uintptr_t base, uint32_t reg, uint32_t val);

void init_irq_controller() {
    ACPI_STATUS status = AcpiGetTable("APIC", 1, &ACPI_MADT_P);

    if (unlikely(!ACPI_SUCCESS(status))) {
        uart_print("init_irq_controller: ACPI unable to get APIC table (MADT)");
        t_print("init_irq_controller: ACPI unable to get APIC table (MADT)");
        return;
    }

    uint32_t lapic_phys = (uint32_t) ((ACPI_TABLE_MADT*) ACPI_MADT_P)->Address;
             lapic_virt = PHYSICAL_TO_VIRTUAL(lapic_phys);

    vmm_map_page(kernel_directory, (void*) lapic_phys, (void*) lapic_virt,
             PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);

    // Set the Spurious Interrupt Vector Register (Offset 0xF0)
    // Vector 0xFF, and Bit 8 (0x100) to enable the APIC
    lapic_write(0xF0, lapic_read(0xF0) | 0x1FF);

    parse_madt((ACPI_TABLE_MADT*) ACPI_MADT_P);

    // Map the actual hardware interrupts
    // These must match the IDT vectors exactly
    irq_unmask(irq0_pin, 32);  // PIT
    irq_unmask(1, 33);         // Keyboard
    irq_unmask(12, 44);        // Mouse
}

void irq_send_eoi() {
    lapic_write(0xB0, 0);
}

// Defined in idt.asm
extern void apic_spurious_handler() {
    uart_print("APIC: Called Spurious handler");
    t_print("APIC: Called Spurious handler");
}

// --- Helper functions ---

static void parse_madt(ACPI_TABLE_MADT* madt) {
    uint8_t* ptr = (uint8_t*)(madt + 1);
    uint8_t* end = (uint8_t*)madt + madt->Header.Length;

    while (ptr < end) {
        ACPI_SUBTABLE_HEADER* sub = (ACPI_SUBTABLE_HEADER*) ptr;

        if (sub->Type == ACPI_MADT_TYPE_IO_APIC) {
            ACPI_MADT_IO_APIC* io = (ACPI_MADT_IO_APIC*) sub;

            // Map IO-APIC
            ioapic_virt = PHYSICAL_TO_VIRTUAL(io->Address);
            vmm_map_page(kernel_directory, (void*) io->Address, (void*) ioapic_virt,
                         PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
        }
        else if (sub->Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE) {
            ACPI_MADT_INTERRUPT_OVERRIDE* iso = (ACPI_MADT_INTERRUPT_OVERRIDE*) sub;
            if (iso->SourceIrq == 0) {
                irq0_pin = iso->GlobalIrq;
            }
        }
        ptr += sub->Length;
    }
}

static void ioapic_set_entry(uint8_t pin, uint64_t data) {
    // Each pin has two 32-bit registers starting at 0x10
    // 0x10 + (pin * 2) is the low bits, +1 is the high bits
    ioapic_write(ioapic_virt, 0x10 + (pin * 2), (uint32_t) data);
    ioapic_write(ioapic_virt, 0x10 + (pin * 2) + 1, (uint32_t)(data >> 32));
}

static void irq_unmask(uint8_t pin, uint8_t vector) {
    // 0x00000000000000xx: Just the vector in the low byte
    // Bit 16 (Mask) is 0 by default, meaning "Enabled"
    ioapic_set_entry(pin, (uint64_t) vector);
}

static inline void lapic_write(uint32_t reg, uint32_t data) {
    volatile uint32_t* addr = (volatile uint32_t*)(lapic_virt + reg);
    *addr = data;
}

static inline uint32_t lapic_read(uint32_t reg) {
    volatile uint32_t* addr = (volatile uint32_t*)(lapic_virt + reg);
    return *addr;
}

static inline void ioapic_write(uintptr_t base, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(base + 0x00) = reg;
    *(volatile uint32_t*)(base + 0x10) = val;
}
