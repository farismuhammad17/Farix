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

#include <stddef.h>
#include <stdint.h>

#include "drivers/terminal.h"

#include "fs/vfs.h"

VFS* current_vfs = NULL;

void vfs_mount(VFS* ops) {
    current_vfs = ops;
}

int fs_read(const char* name, void* buffer, size_t size, uint32_t offset) {
    if (unlikely(!current_vfs || !current_vfs->read)) {
        err_print("fs_read: File operation not found");
        return 0;
    }

    return current_vfs->read(name, buffer, size, offset);
}

int fs_write(const char* name, const void* buffer, size_t size, uint32_t offset) {
    if (unlikely(!current_vfs || !current_vfs->write)) {
        err_print("fs_write: File operation not found");
        return 0;
    }

    return current_vfs->write(name, buffer, size, offset);
}

int fs_create(const char* name) {
    if (unlikely(!current_vfs || !current_vfs->create)) {
        err_print("fs_create: File operation not found");
        return 0;
    }

    return current_vfs->create(name);
}

int fs_mkdir(const char* name) {
    if (unlikely(!current_vfs || !current_vfs->mkdir)) {
        err_print("fs_mkdir: File operation not found");
        return 0;
    }

    return current_vfs->mkdir(name);
}

int fs_remove(const char* name) {
    if (unlikely(!current_vfs || !current_vfs->remove)) {
        err_print("fs_remove: File operation not found");
        return 0;
    }

    return current_vfs->remove(name);
}

File* fs_get(const char* name) {
    if (unlikely(!current_vfs || !current_vfs->get)) {
        err_print("fs_get: File operation not found");
        return NULL;
    }

    return current_vfs->get(name);
}

FileNode* fs_getall(const char* path) {
    if (unlikely(!current_vfs || !current_vfs->getall)) {
        err_print("fs_getall: File operation not found");
        return NULL;
    }

    return current_vfs->getall(path);
}
