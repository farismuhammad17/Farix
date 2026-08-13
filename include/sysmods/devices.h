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

#ifndef DEVICES_H
#define DEVICES_H

#define INITBOOT_DEV_ID      0
#define UART_DEV_ID          1
#define PIT_DEV_ID           2
#define KEYBOARD_PS2_DEV_ID  3
#define ATA_DEV_ID           4
#define AHCI_DEV_ID          5
#define FAT32_VFS_ID         6

// DEVELOPER NOTE:
// Every device struct MUST have a next pointer
// to itself placed at the start of the struct,
// except in devices where only one need to be
// at a time (eg. timer, storage device, etc.)

typedef enum {
    DRV_OUTPUT,
    DRV_INPUT,
    DRV_TIMER,
    DRV_STORAGE,
    DRV_VFS,
} driver_type_t;

void register_device(driver_type_t type, void* device);
void unregister_device(driver_type_t type, void* device);

void* get_device(driver_type_t dev_type);

#endif
