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

#include "architecture/io.h"

#include "drivers/ata.h"

void init_ata() {
    outb(0x1F6, 0xA0);    // Select Master drive
    outb(0x1F2, 0);       // Sector count 0
    outb(0x1F3, 0);       // LBA Low
    outb(0x1F4, 0);       // LBA Mid
    outb(0x1F5, 0);       // LBA High
    outb(0x1F7, 0xEC);    // SEND IDENTIFY COMMAND

    uint8_t status = inb(0x1F7);
    if (status == 0) return; // Drive does not exist

    ata_wait_ready();

    // Read 256 words (512 bytes) of identification data
    uint16_t data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = inw(0x1F0);
    }
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    outb(0x1F6, (uint8_t)((lba >> 24) | 0xE0));

    for(int i = 0; i < 4; i++) inb(0x1F7);

    outb(0x1F2, 1);               // Sector count
    outb(0x1F3, (uint8_t)lba);    // LBA Low
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);            // Command: READ

    ata_wait_ready(); // Now wait for data

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(0x1F0);
    }
}

void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    outb(0x1F6, (uint8_t)((lba >> 24) | 0xE0));
    for(int i = 0; i < 4; i++) inb(0x1F7);

    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30); // WRITE command

    ata_wait_ready(); // Wait for DRQ before sending data

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ptr[i]);
    }

    // --- THE CRITICAL PART ---
    // After writing, tell the drive to CACHE FLUSH (0xE7)
    // or at least wait for BSY to clear.
    outb(0x1F7, 0xE7);
    while (inb(0x1F7) & 0x80);
}

void ata_wait_ready() {
    // Wait while the drive is BUSY (BSY bit 7)
    while (inb(0x1F7) & 0x80);

    // Wait until the drive is READY (DRQ bit 3) or has an ERROR (ERR bit 0)
    uint8_t status;
    while (!((status = inb(0x1F7)) & 0x08)) {
        if (status & 0x01) return;
    }
}
