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

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

struct FAT32Header {
    uint8_t  boot_jmp[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // FAT32 Extended Section
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;      // Files' entry point
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_num;
    uint8_t  reserved1;
    uint8_t  boot_sig;          // 0x28 or 0x29
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];        // Should say "FAT32   " (hopefully)
} __attribute__((packed));

struct FAT32File {
    uint8_t  name[11];      // 8 chars name, 3 extension
    uint8_t  attributes;    // If folder, read-only, etc.
    uint8_t  reservedNT;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t cluster_high;  // Top 16 bits of the address
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;   // Bottom 16 bits of the address
    uint32_t size;          // File size in bytes
} __attribute__((packed));

extern FileOperations fat32_ops;

void      init_fat32();

bool      fat32_read   (string name, void* buffer, size_t size);
bool      fat32_write  (string name, const void* buffer, size_t size);
bool      fat32_create (string name);
bool      fat32_mkdir  (string name);
bool      fat32_remove (string name);
File*     fat32_get    (string name);
FileNode* fat32_getall (string path);

#endif
