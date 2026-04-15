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

#include "fs/vfs.h"

static FileOperations* current_fs_ops = NULL;

void vfs_mount(FileOperations* ops) {
    current_fs_ops = ops;
}

int fs_read(const char* name, void* buffer, size_t size, uint32_t offset) {
    if (!current_fs_ops || !current_fs_ops->read) return false;
    return current_fs_ops->read(name, buffer, size, offset);
}

int fs_write(const char* name, const void* buffer, size_t size, uint32_t offset) {
    if (!current_fs_ops || !current_fs_ops->write) return false;
    return current_fs_ops->write(name, buffer, size, offset);
}

bool fs_create(const char* name) {
    if (!current_fs_ops || !current_fs_ops->create) return false;
    return current_fs_ops->create(name);
}

bool fs_mkdir(const char* name) {
    if (!current_fs_ops || !current_fs_ops->mkdir) return false;
    return current_fs_ops->mkdir(name);
}

bool fs_remove(const char* name) {
    if (!current_fs_ops || !current_fs_ops->remove) return false;
    return current_fs_ops->remove(name);
}

File* fs_get(const char* name) {
    if (!current_fs_ops || !current_fs_ops->get) return NULL;
    return current_fs_ops->get(name);
}

FileNode* fs_getall(const char* path) {
    if (!current_fs_ops || !current_fs_ops->getall) return NULL;
    return current_fs_ops->getall(path);
}
