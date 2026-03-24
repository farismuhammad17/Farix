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

#ifndef RAMDISK_H
#define RAMDISK_H

#define RAMDISK_HASH_SIZE 64

#include "fs/vfs.h"

extern FileOperations ramdisk_ops;

void      init_ramdisk();

int       ramdisk_read   (const char* name, void* buffer, size_t size);
int       ramdisk_write  (const char* name, const void* buffer, size_t size);
bool      ramdisk_create (const char* name);
bool      ramdisk_mkdir  (const char* name);
bool      ramdisk_remove (const char* name);
File*     ramdisk_get    (const char* name);
FileNode* ramdisk_getall (const char* path);

#endif
