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

#include <string>

#include "fs/vfs.h"

extern FileOperations ramdisk_ops;

void      init_ramdisk();

bool      ramdisk_read   (std::string& name, void* buffer, size_t size);
bool      ramdisk_write  (std::string& name, const void* buffer, size_t size);
bool      ramdisk_create (std::string& name);
bool      ramdisk_mkdir  (std::string& name);
bool      ramdisk_remove (std::string& name);
File*     ramdisk_get    (std::string& name);
FileNode* ramdisk_getall (std::string& path);

#endif
