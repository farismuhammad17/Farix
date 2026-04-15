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
#include <string.h>

#include "fs/vfs.h"
#include "memory/heap.h"

#include "fs/ramdisk.h"

// TODO
// The hash map currently doesn't have a solution for
// hash collisions; will implement linked list form to
// fix this, but I am very lazy, and don't really need
// ramdisk yet.

// djb2 hash
// Source: https://www.cse.yorku.ca/~oz/hash.html
static unsigned long hash(const char* name) {
    unsigned long hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static File* files_table[RAMDISK_HASH_SIZE];

FileOperations ramdisk_ops = {
    .read   = ramdisk_read,
    .write  = ramdisk_write,
    .create = ramdisk_create,
    .mkdir  = ramdisk_mkdir,
    .remove = ramdisk_remove,
    .get    = ramdisk_get,
    .getall = ramdisk_getall
};

void init_ramdisk() {
    for (int i = 0; i < RAMDISK_HASH_SIZE; i++) {
        files_table[i] = NULL;
    }
}

int ramdisk_read(const char* name, void* buffer, size_t size, uint32_t offset) {
    File* file = ramdisk_get(name);

    if (!file || !file->data || file->is_directory) return -1;
    if (offset >= file->size) return 0; // End of file

    // Ensure we don't read past the end of the file
    size_t bytes_to_read = size;
    if (offset + bytes_to_read > file->size) {
        bytes_to_read = file->size - offset;
    }

    // Copy starting from the offset pointer
    kmemcpy(buffer, (uint8_t*) file->data + offset, bytes_to_read);

    return (int) bytes_to_read;
}

int ramdisk_write(const char* name, const void* buffer, size_t size, uint32_t offset) {
    File* file = ramdisk_get(name);
    if (!file || file->is_directory) return -1;

    // If we are writing beyond current capacity, we need more RAM
    if (offset + size > file->size) {
        uint8_t* new_data = (uint8_t*) kmalloc(offset + size);
        if (!new_data) return -1;

        // If there was old data, preserve it
        if (file->data) {
            kmemcpy(new_data, file->data, file->size);
            kfree(file->data);
        }

        file->data = new_data;
        file->size = offset + size;
    }

    // Now that we're sure the buffer is big enough, copy to the offset
    kmemcpy((uint8_t*) file->data + offset, buffer, size);

    return (int) size;
}

int ramdisk_create(const char* name) {
    uint32_t index = hash(name) % RAMDISK_HASH_SIZE;
    if (files_table[index] != NULL) return 0;

    File* new_file = (File*) kmalloc(sizeof(File));
    kmemset(new_file, 0, sizeof(File));

    new_file->name = (char*) strdup(name);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->is_directory = false;

    files_table[index] = new_file;

    return 1;
}

int ramdisk_mkdir(const char* name) {
    uint32_t index = hash(name) % RAMDISK_HASH_SIZE;
    if (files_table[index] != NULL) return 0;

    File* new_file = (File*) kmalloc(sizeof(File));
    kmemset(new_file, 0, sizeof(File));

    new_file->name = (char*) strdup(name);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->is_directory = true;

    files_table[index] = new_file;

    return 1;
}

int ramdisk_remove(const char* name) {
    uint32_t index = hash(name) % RAMDISK_HASH_SIZE;
    File* file = files_table[index];

    if (file == NULL || strcmp(file->name, name) != 0) {
        return 0;
    }

    if (file->data != NULL) {
        kfree(file->data);
    }

    if (file->name != NULL) {
        kfree((void*)file->name);
    }

    kfree(file);
    files_table[index] = NULL;

    return 1;
}

File* ramdisk_get(const char* name) {
    return files_table[hash(name) % RAMDISK_HASH_SIZE];
}

FileNode* ramdisk_getall(const char* path) {
    FileNode* head  = NULL;

    const char* search_path = (strcmp(path, "/") == 0) ? "" : path;
    size_t search_len = strlen(search_path);

    for (int i = 0; i < RAMDISK_HASH_SIZE; i++) {
        File* file_ptr = files_table[i];
        if (file_ptr == NULL) continue;

        const char* name = file_ptr->name;

        if (strncmp(name, search_path, search_len) == 0 && strcmp(name, search_path) != 0) {
            const char* relative = name + search_len;

            if (relative[0] == '/') relative++;

            if (!strchr(relative, '/')) {
                if (strlen(relative) == 0) continue;

                FileNode* newNode = (FileNode*) kmalloc(sizeof(FileNode));
                if (!newNode) return head;
                kmemset(newNode, 0, sizeof(FileNode));

                newNode->file.size = file_ptr->size;
                newNode->file.is_directory = file_ptr->is_directory;
                newNode->file.name = file_ptr->name;

                newNode->next = head;
                head = newNode;
            }
        }
    }

    return head;
}
