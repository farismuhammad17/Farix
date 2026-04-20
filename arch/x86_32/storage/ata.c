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

// Registers
int REG_DATA     = 0x1F0; // Data port: 16-bit I/O
int REG_ERROR    = 0x1F1; // Error register (Read) / Features (Write)
int REG_FEATURES = 0x1F1;
int REG_SEC_CNT  = 0x1F2; // Sector count (0-255, 0 = 256)
int REG_LBA_LO   = 0x1F3; // LBA bits 0-7
int REG_LBA_MI   = 0x1F4; // LBA bits 8-15
int REG_LBA_HI   = 0x1F5; // LBA bits 16-23
int REG_DRV_SEL  = 0x1F6; // Drive select / LBA bits 24-27
int REG_COMMAND  = 0x1F7; // Command register (Write)
int REG_STATUS   = 0x1F7; // Status register (Read)
int REG_CONTROL  = 0x3F6; // Used for software resets.
int REG_ALT_STAT = 0x3F6; // Same as REG_CONTROL, but read-only

// Commands
int CMD_READ     = 0x20;  // PIO Read with retry
int CMD_WRITE    = 0x30;  // PIO Write with retry
int CMD_IDENTIFY = 0xEC;  // Identify Drive
int CMD_FLUSH    = 0xE7;  // Cache Flush

// Status register bits
int SR_BSY       = 0x80;  // Busy: Drive is processing
int SR_DRDY      = 0x40;  // Drive Ready
int SR_DF        = 0x20;  // Drive Fault: Hardware failure
int SR_DSC       = 0x10;  // Seek Complete
int SR_DRQ       = 0x08;  // Data Request: Ready to move bytes
int SR_CORR      = 0x04;  // Corrected data
int SR_IDX       = 0x02;  // Index (Legacy)
int SR_ERR       = 0x01;  // Error: Check Error Register

// Controls
int CTRL_RESET   = 0x04;  // SRST: Software Reset bit
int CTRL_NORMAL  = 0x00;  // Normal operation (Clear Reset)
int LBA_MASTER   = 0xE0;  // LBA Mode, Master Drive
int LBA_SLAVE    = 0xF0;  // LBA Mode, Slave Drive

// Drives
int DRIVE_MASTER = 0xA0;  // Select Master Drive (CHS/Legacy)
int DRIVE_SLAVE  = 0xB0;  // Select Slave Drive (CHS/Legacy)
int LBA_MODE     = 0x40;  // Bit to enable Logical Block Addressing

// Constants
int CTRL_LEGACY  = 0x3F4; // Legacy Base address for Control Block
