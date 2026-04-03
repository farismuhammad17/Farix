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

#include "arch/kernel.h"
#include "arch/stubs.h"
#include "arch/x86/multiboot.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#define PIC1    		0x20
#define PIC1_COMMAND	PIC1
#define PIC1_DATA   (PIC1+1)
#define PIC2	    	0xA0
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define GDT_ACCESS_PRESENT    0x80 // Must be 1 for valid segments
#define GDT_ACCESS_RING0      0x00 // Kernel privilege
#define GDT_ACCESS_RING3      0x60 // User privilege
#define GDT_ACCESS_CODE_DATA  0x10 // Must be 1 for code/data segments
#define GDT_ACCESS_EXECUTABLE 0x08 // 1 for Code ; 0 for Data
#define GDT_ACCESS_WRITABLE   0x02 // Readable for code, Writable for data
#define GDT_ACCESS_ACCESSED   0x01  // Set by CPU when segment is used

#define GDT_GRAN_4K    0x80 // Limit is in 4KB blocks
#define GDT_GRAN_32BIT 0x40 // 32-bit protected mode
#define GDT_GRAN_64BIT 0x20 // Not used in i386

typedef struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) GDTEntry;

typedef struct GDTPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) GDTPointer;

typedef struct TSSEntry {
    uint32_t prev_tss;
    uint32_t esp0;     // Kernel stack pointer
    uint32_t ss0;      // Kernel stack segment
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed)) TSSEntry;

GDTEntry   gdt[6];
GDTPointer gdt_ptr;
TSSEntry   tss_entry;

// Defined in asm/gdt_flush.asm
extern void gdt_flush(uint32_t gdt_ptr_addr);
extern void load_tss();

static void pic_remap();
static void init_tss(uint32_t idx, uint32_t kss, uint32_t kesp);
static void init_gdt();
static void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

multiboot_info* mbi = NULL;

void arch_kmain(uint32_t magic, multiboot_info* _mbi) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("OS Error: Invalid Multiboot Magic Number");
        while(1) asm volatile("hlt");
    }

    mbi = _mbi;

    pic_remap();

    init_pmm();
    init_vmm();

    init_gdt();

    kernel_main();
}

static void pic_remap() {
    // ICW1
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2
    outb(PIC1_DATA, 0x20); // Master -> 32
    outb(PIC2_DATA, 0x28); // Slave -> 40

    // ICW3
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    // ICW4
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* For understanding:
     *
     * 0 = Enabled | 1 = Disabled
     *
     * In the Master PIC (PIC1):
     * Bit | IRQ | Hardware      | Current  | Description
     * 0   | 0   | Timer         | Enabled  | Important for timing stuff, multitasking, etc.
     * 1   | 1   | Keyboard      | Enabled  |
     * 2   | 2   | Slave cascade | Enabled  | Bridge to second PIC.
     * 3   | 3   | COM2          | Disabled | Serial port 2; ancient way to connect modems or link two PCs.
     * 4   | 4   | COM1          | Disabled | Serial port 1.
     * 5   | 5   | LPT2          | Disabled | Parallel port 1; used for very old printers or scanners.
     * 6   | 6   | Floppy disk   | Disabled |
     * 7   | 7   | LPT1          | Disabled | Parallel port 2; often fires randomly if the hardware is noisy.
     *
     * In the Slave PIC (PIC2):
     * Bit | IRQ | Hardware      | Current  | Description
     * 0   | 8   | RTC           | Disabled | Real time clock; important for system clock, date, etc.
     * 1   | 9   | ACPI          | Disabled | Handles power buttons and "Sleep/Wake" signals.
     * 2   | 10  | PCI Device    | Disabled | Usually assigned to network cards, sound cards, etc.
     * 3   | 11  | PCI Device    | Disabled | Extra slot for more hardware.
     * 4   | 12  | PS/2 mouse    | Enabled  |
     * 5   | 13  | FPU           | Disabled | Math co-processor.
     * 6   | 14  | Primary ATA   | Disabled | Hard Drive Controller.
     * 7   | 15  | Secondary ATA | Disabled | Second hard drive or CD-ROM controller.
     */
    outb(PIC1_DATA, 0b11111000);
    outb(PIC2_DATA, 0b11101111);

    outb(0x64, 0xAE); // Enable keyboard
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
}

static void init_tss(uint32_t idx, uint32_t kss, uint32_t kesp) {
    uint32_t base = (uint32_t) PHYSICAL_TO_VIRTUAL(&tss_entry);
    uint32_t limit = sizeof(TSSEntry);

    // Access: 0x89 (Present, Executable, Accessed, Ring 0)
    gdt_set_entry(idx, base, limit, 0x89, 0x00);

    kmemset(&tss_entry, 0, sizeof(TSSEntry));

    tss_entry.ss0  = kss;     // Usually 0x10 (Kernel Data)
    tss_entry.esp0 = kesp;

    // Set the I/O map base to the size of the TSS to disable it
    tss_entry.iomap_base = sizeof(TSSEntry);
}

static void init_gdt() {
    gdt_ptr.limit = (sizeof(GDTEntry) * 6) - 1;
    gdt_ptr.base  = (uint32_t) PHYSICAL_TO_VIRTUAL(&gdt);

    // 0x00: Null segment
    gdt_set_entry(0, 0, 0, 0, 0);

    // Common flags for all our segments
    uint8_t base_access = GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA;
    uint8_t base_gran   = GDT_GRAN_4K | GDT_GRAN_32BIT;

    // 0x08: Kernel Code
    gdt_set_entry(1, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING0 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F); // 0x0F represents the high 4 bits of the 0xFFFFF limit

    // 0x10: Kernel Data
    gdt_set_entry(2, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING0 | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F);

    // 0x18: User Mode Code
    gdt_set_entry(3, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING3 | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F);

    // 0x20: User Mode Data
    gdt_set_entry(4, 0, 0xFFFFFFFF,
        base_access | GDT_ACCESS_RING3 | GDT_ACCESS_WRITABLE,
        base_gran | 0x0F);

    uint32_t current_esp;
    asm volatile("mov %%esp, %0" : "=r"(current_esp));
    init_tss(5, 0x10, current_esp);

    // Defined in gdt_flush.asm
    gdt_flush((uint32_t) &gdt_ptr);
    load_tss();
}

static void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Descriptor base address
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    // Descriptor limits
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    // Granularity and access flags
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access       = access;
}

// Defined in stubs.h:
// I placed the abstraction for this one function here,
// though it is quite ugly, it prevents me having to
// create a header file for the TSS.
void set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}
