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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WIDTH   80
#define HEIGHT  25
#define MEMORY  0xB8000

#define PIC1    		0x20
#define PIC1_COMMAND	PIC1
#define PIC1_DATA   (PIC1+1)
#define PIC2	    	0xA0
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define KBD_LEN 58

extern "C" {
    void default_handler_stub();
    void keyboard_handler_stub();
}

size_t    cursor_x;
size_t    cursor_y;
uint8_t   terminal_color;
uint16_t* terminal_buffer = (uint16_t*) MEMORY;

bool shift_pressed = false;

unsigned char kbd[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',

    // Shifted
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

struct idt_entry {
    uint16_t base_low;    // Lower 16 bits of the handler address
    uint16_t sel;         // Kernel Segment Selector (0x08)
    uint8_t  always0;     // This byte must be 0
    uint8_t  flags;       // That "Permission Slip" byte (0x8E)
    uint16_t base_high;   // Upper 16 bits of the handler address
} __attribute__((packed));
struct idt_entry idt[256];

struct idt_ptr {
    uint16_t limit; // 2 bytes
    uint32_t base;  // 4 bytes
} __attribute__((packed));
struct idt_ptr idtp;

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}
uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void update_cursor(size_t x, size_t y) {
    uint16_t pos = y * WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (base & 0xFFFF);        // Lower 16 bits
    idt[num].base_high = (base >> 16) & 0xFFFF;  // Upper 16 bits

    idt[num].sel     = sel;    // Usually 0x08 (Kernel Code Segment)
    idt[num].always0 = 0;      // Always 0
    idt[num].flags   = flags;  // 0x8E (Present, Ring 0, Interrupt Gate)
}

void init_terminal() {
	cursor_x = 0;
	cursor_y = 0;

	update_cursor(0, 0);

	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
		const size_t index = y * WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;  // Defines the IDT pointer's size how x86 likes
    idtp.base  = (uint32_t)&idt;                        // The memory address of the IDT array

    for (int i = 0; i < 256; i++) {
        uint32_t base = (uint32_t) default_handler_stub;
        idt_set_gate(i, base, 0x08, 0x8E);
    }

    // Register Keyboard Handler at index 33
    idt_set_gate(33, (uint32_t) keyboard_handler_stub, 0x08, 0x8E);

    asm volatile("lidt %0" : : "m"(idtp));
}

void pic_remap() {
    // ICW1
    outb(0x20, 0x11);
    outb(0x80, 0); // io_wait
    outb(0xA0, 0x11);
    outb(0x80, 0);

    // ICW2
    outb(0x21, 0x20); // Master -> 32
    outb(0x80, 0);
    outb(0xA1, 0x28); // Slave -> 40
    outb(0x80, 0);

    // ICW3
    outb(0x21, 0x04);
    outb(0x80, 0);
    outb(0xA1, 0x02);
    outb(0x80, 0);

    // ICW4
    outb(0x21, 0x01);
    outb(0x80, 0);
    outb(0xA1, 0x01);
    outb(0x80, 0);

    // Use 0xFD to ONLY enable Keyboard (IRQ 1). Disable Timer (IRQ 0) for now.
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    outb(0x64, 0xAE); // Enable keyboard
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
}

void echo_at(char c, uint8_t color, size_t x, size_t y) {
	terminal_buffer[y * WIDTH + x] = vga_entry(c, color);
}

void echo_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        }
        else if (cursor_y > 0) {
            cursor_y--;

            cursor_x = WIDTH - 1;
            while (cursor_x > 0) {
                uint16_t entry = terminal_buffer[cursor_y * WIDTH + cursor_x];
                unsigned char char_at_pos = (unsigned char)(entry & 0xFF);

                if (char_at_pos != ' ') {
                    if (cursor_x < WIDTH - 1) cursor_x++;
                    break;
                }

                cursor_x--;
            }
        }

        echo_at(' ', terminal_color, cursor_x, cursor_y);
    }
    else {
        echo_at(c, terminal_color, cursor_x, cursor_y);
        if (++cursor_x == WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    update_cursor(cursor_x, cursor_y);

    if (cursor_y >= HEIGHT) {
        cursor_y = 0;
    }
}

void echo(const char* data) {
    size_t size = strlen(data);
	for (size_t i = 0; i < size; i++) {
	    echo_char(data[i]);
	}
	echo_char('\n');
}

void echo(const char* data, char end) {
    size_t size = strlen(data);
	for (size_t i = 0; i < size; i++) {
	    echo_char(data[i]);
	}
	echo_char(end);
}

extern "C" {
    void keyboard_handler() {
        uint8_t scancode = inb(0x60);

        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = true;
        } else if (scancode == 0xAA || scancode == 0xB6) {
            shift_pressed = false;
        } else if (!(scancode & 0x80)) {
            size_t offset = 0;
            if (shift_pressed) {
                offset += KBD_LEN;
            }

            char c = kbd[scancode + offset];

            if (c > 0) {
                echo_char(c);
            }
        }

        outb(0x20, 0x20);
    }

    void kernel_main() {
        init_terminal();
        echo("Initialized Terminal");

        init_idt();
        echo("Initialized IDT");

        pic_remap();
        echo("Remapped PIC");

        asm volatile("sti");
        echo("Interrupts enabled. Waiting for keypress...");

        // The OS must NEVER die.
        // Interrupts take back control from this loop whenever
        // they are called, so the OS is never stuck in the
        // while loop forever.
        while (1) {};
    }
}
