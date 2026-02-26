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

#include "fs/vfs.h"

FileOperations* current_fs_ops = nullptr;

void vfs_mount(FileOperations* ops) {
    current_fs_ops = ops;
}

bool fs_read(std::string& name, void* buffer, size_t size) {
    if (!current_fs_ops || !current_fs_ops->read) return false;
    return current_fs_ops->read(name, buffer, size);
}

bool fs_write(std::string& name, const void* buffer, size_t size) {
    if (!current_fs_ops || !current_fs_ops->write) return false;
    return current_fs_ops->write(name, buffer, size);
}

bool fs_create(std::string& name) {
    if (!current_fs_ops || !current_fs_ops->create) return false;
    return current_fs_ops->create(name);
}

bool fs_mkdir(std::string& name) {
    if (!current_fs_ops || !current_fs_ops->mkdir) return false;
    return current_fs_ops->mkdir(name);
}

bool fs_remove(std::string& name) {
    if (!current_fs_ops || !current_fs_ops->remove) return false;
    return current_fs_ops->remove(name);
}

File* fs_get(std::string& name) {
    if (!current_fs_ops || !current_fs_ops->get) return nullptr;
    return current_fs_ops->get(name);
}

FileNode* fs_getall(std::string& path) {
    if (!current_fs_ops || !current_fs_ops->getall) return nullptr;
    return current_fs_ops->getall(path);
}
