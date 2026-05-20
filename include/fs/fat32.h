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

#include "fs/vfs.h"

extern VFS fat32_vfs;

void RARE_FUNC init_fat32();

int       fat32_read   (const char* name, void* buffer, size_t size, uint32_t offset);
int       fat32_write  (const char* name, const void* buffer, size_t size, uint32_t offset);
int       fat32_create (const char* name);
int       fat32_mkdir  (const char* name);
int       fat32_remove (const char* name);
File*     fat32_get    (const char* name);
FileNode* fat32_getall (const char* path);

int fat32_check_write_safety(uint32_t lba);

#endif
