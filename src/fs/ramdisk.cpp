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

#include "types/map.h"
#include "fs/vfs.h"

#include "fs/ramdisk.h"

static map<string, File*>* files = nullptr;

FileOperations ramdisk_ops = {
    .read   = ramdisk_read,
    .write  = ramdisk_write,
    .create = ramdisk_create,
    .remove = ramdisk_remove,
    .get    = ramdisk_get
};

void init_ramdisk() {
    files = new map<string, File*>();
}

bool ramdisk_read(string name, void* buffer, size_t size) {
    File* file = ramdisk_get(name);

    if (!file || !file->data) return false;

    size_t bytes_to_read = (size < file->size) ? size : file->size;

    for (size_t i = 0; i < bytes_to_read; i++) {
        ((uint8_t*) buffer)[i] = file->data[i];
    }

    return true;
}

bool ramdisk_write(string name, const void* buffer, size_t size) {
    File* file = ramdisk_get(name);
    if (!file) return false;

    if (file->data) free(file->data);

    file->data = (uint8_t*)malloc(size);
    if (!file->data) return false;

    for (size_t i = 0; i < size; i++) {
        file->data[i] = ((uint8_t*)buffer)[i];
    }

    file->size = size;
    return true;
}

bool ramdisk_create(string name) {
    if (files->contains(name)) return false; // File already exists

    File* new_file = new File();
    new_file->data = nullptr; // Empty file
    new_file->size = 0;

    files->put(name, new_file);
    return true;
}

bool ramdisk_remove(string name) {
    File* file = ramdisk_get(name);

    if (file == nullptr) return false;

    if (file->data != nullptr) {
        free(file->data);
    }

    delete file;

    files->remove(name);

    return true;
}

File* ramdisk_get(string name) {
    return files->get(name);
}
