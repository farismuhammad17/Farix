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

#include "sysmods/devices.h"
#include "sysmods/interface.h"

#include "drivers/input.h"

#define KEY_UP               0x11
#define KEY_DOWN             0x12
#define KBD_LEN                58

// Ports
#define PS2_DATA_PORT        0x60
#define PS2_STATUS_PORT      0x64
#define PS2_COMMAND_PORT     0x64

// Status Register Flags
#define PS2_STATUS_OUT_READY 0x01
#define PS2_STATUS_IN_BUSY   0x02

// Commands
#define PS2_CMD_READ_CB      0x20
#define PS2_CMD_WRITE_CB     0x60
#define PS2_CMD_ENABLE_PORT1 0xAE
#define PS2_CMD_RESET        0xFF

// Command Byte Bits
#define PS2_CB_KBD_IRQ       (1 << 0)
#define PS2_CB_MOUSE_IRQ     (1 << 1)
#define PS2_CB_TRANSLATION   (1 << 6)

// Expected Responses
#define PS2_ACK              0xFA
#define PS2_SELF_TEST_OK     0xAA

static kernel_api_t* k_api = NULL;
static input_dev_t*  dev   = NULL;

static bool shift_pressed = false;
static bool is_extended   = false;

static uint64_t base_addr = 0;

static unsigned char kbd[128] = {
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
static const char* kbd_ptr = NULL;

static void interrupt_handler() {
    uint8_t status = k_api->inb(PS2_STATUS_PORT);

    if (unlikely(!(status & PS2_STATUS_OUT_READY) || (status & PS2_STATUS_IN_BUSY))) {
        k_api->irq_send_eoi();
        return;
    }

    uint8_t scancode = k_api->inb(PS2_DATA_PORT);

    if (scancode == 0xE0) {
        is_extended = true;
    } else if (is_extended) {
        if (scancode == 0x48) {
            if (dev->on_event) dev->on_event((void*)(uintptr_t) KEY_UP);
        } else if (scancode == 0x50) {
            if (dev->on_event) dev->on_event((void*)(uintptr_t) KEY_DOWN);
        }
        is_extended = false;
    } else {
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = true;
        } else if (scancode == 0xAA || scancode == 0xB6) {
            shift_pressed = false;
        } else if (!(scancode & 0x80)) {
            size_t offset = shift_pressed ? KBD_LEN : 0;
            unsigned char c = kbd_ptr[scancode + offset];

            if (likely(c > 0 && dev->on_event)) {
                // Pack the literal ASCII value directly into the pointer slot.
                // uintptr_t ensures no sign-extension or pointer truncation warnings.
                dev->on_event((void*)(uintptr_t) c);
            }
        }
    }

    k_api->irq_send_eoi();
}

static int init_keyboard(kernel_api_t* api, uint64_t b_addr) {
    k_api = api;
    base_addr = b_addr;

    kbd_ptr = (char*) SYSMOD_TO_KERNEL(kbd);

    // Prepare Command Byte
    while (k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_IN_BUSY);
    k_api->outb(PS2_COMMAND_PORT, PS2_CMD_READ_CB);

    while (!(k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_READY));
    uint8_t cb = k_api->inb(PS2_DATA_PORT);

    // Enable IRQs and Scancode Translation
    cb |= (PS2_CB_KBD_IRQ | PS2_CB_MOUSE_IRQ | PS2_CB_TRANSLATION);

    while (k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_IN_BUSY);
    k_api->outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_CB);

    while (k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_IN_BUSY);
    k_api->outb(PS2_DATA_PORT, cb);

    // Initialize Hardware
    while (k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_READY) k_api->inb(PS2_DATA_PORT);

    while (k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_IN_BUSY);
    k_api->outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_PORT1);

    while (k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_IN_BUSY);
    k_api->outb(PS2_DATA_PORT, PS2_CMD_RESET);

    // Verify Response
    while (!(k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_READY));
    if (unlikely(k_api->inb(PS2_DATA_PORT) != PS2_ACK)) {
        k_api->err_print("Keyboard: Reset failed (NACK)");
        return 1;
    }

    while (!(k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_READY));
    if (unlikely(k_api->inb(PS2_DATA_PORT) != PS2_SELF_TEST_OK)) {
        k_api->err_print("Keyboard: Self-test failed");
        return 1;
    }

    dev = k_api->kmalloc(sizeof(input_dev_t));
    dev->id = KEYBOARD_PS2_ID;
    dev->type = DEV_INPUT;

    dev->on_event = NULL;

    k_api->register_device(DEV_INPUT, (void*) dev);

    k_api->register_interrupt(33, (void*) SYSMOD_TO_KERNEL(interrupt_handler));

    return 0;
}

static int exit_keyboard() {
    // Disable the Keyboard Port on the controller
    // This ensures no more IRQs hit our handler while we clean up
    while (k_api->inb(PS2_STATUS_PORT) & PS2_STATUS_IN_BUSY);
    k_api->outb(PS2_COMMAND_PORT, 0xAD); // 0xAD = Disable 1st PS/2 port

    k_api->unregister_interrupt(33);
    k_api->unregister_device(DEV_INPUT, (void*) dev);

    k_api->kfree(dev);

    return 0;
}

SYSMOD_HEADER sysmod_t test_module_entry = {
    .name = "KEYBOARD/PS2",
    .init = init_keyboard,
    .exit = exit_keyboard
};
