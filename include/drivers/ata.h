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

#ifndef ATA_H
#define ATA_H

/*

ATA (Advanced Technology Attachment)

Directly communicating with storage drives using raw electrical signals is
complex and error-prone. To standardize how software talks to hard disks and
solid-state drives, we use the ATA interface. It provides a consistent protocol
for the kernel to move data between physical disk sectors and system memory.

The ATA controller allows the kernel to issue commands—like "read" or "write" to
specific disk locations called LBA (Logical Block Addressing). Instead of the CPU
waiting idly for the slow mechanical disk to finish, we use status flags like BSY
(Busy) and DRQ (Data Request) to synchronize the high-speed CPU with the slower
storage hardware.

Since disk controller registers and I/O ports are specific to the motherboard's
bus architecture, the implementation is organized by platform:

x86_32: src/arch/x86_32/ata.c

void init_ata:
    Initializes the disk controller, detects connected drives, and prepares
    the bus for data transfer.

void ata_read_sector / ata_write_sector:
    These functions translate high-level requests into low-level commands sent
    to the disk. They handle the transfer of 512-byte chunks of data (sectors)
    between the hardware and a provided memory buffer.

void ata_wait_ready:
    A helper that polls the status register. It ensures the drive is not
    busy and is ready to accept the next command or data packet before
    the kernel proceeds.

*/

#define STATUS_BSY 0x80
#define STATUS_DRQ 0x08

#include <stdint.h>

void init_ata();
void ata_read_sector(uint32_t lba, uint8_t* buffer);
void ata_write_sector(uint32_t lba, uint8_t* buffer);
void ata_wait_ready();

#endif
