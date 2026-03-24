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

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "fs/vfs.h"
#include "memory/heap.h"

#include "fs/ramdisk.h"

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

int ramdisk_read(const char* name, void* buffer, size_t size) {
    File* file = ramdisk_get(name);
    if (!file || !file->data || file->is_directory) return -1;
    size_t bytes_to_read = (size < file->size) ? size : file->size;
    kmemcpy(buffer, file->data, bytes_to_read);
    return (int) bytes_to_read;
}

int ramdisk_write(const char* name, const void* buffer, size_t size) {
    File* file = ramdisk_get(name);

    if (!file || file->is_directory) {
        return -1;
    }

    if (file->data) {
        kfree(file->data);
    }

    file->data = (uint8_t*) kmalloc(size);
    if (!file->data) {
        file->size = 0;
        return -1;
    }

    kmemcpy(file->data, buffer, size);
    file->size = size;

    return (int) size;
}

bool ramdisk_create(const char* name) {
    uint32_t index = hash(name) % RAMDISK_HASH_SIZE;
    if (files_table[index] != NULL) return false;

    File* new_file = (File*) kmalloc(sizeof(File));
    kmemset(new_file, 0, sizeof(File));

    new_file->name = (char*) strdup(name);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->is_directory = false;

    files_table[index] = new_file;

    return true;
}

bool ramdisk_mkdir(const char* name) {
    uint32_t index = hash(name) % RAMDISK_HASH_SIZE;
    if (files_table[index] != NULL) return false;

    File* new_file = (File*) kmalloc(sizeof(File));
    kmemset(new_file, 0, sizeof(File));

    new_file->name = (char*) strdup(name);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->is_directory = true;

    files_table[index] = new_file;

    return true;
}

bool ramdisk_remove(const char* name) {
    uint32_t index = hash(name) % RAMDISK_HASH_SIZE;
    File* file = files_table[index];

    if (file == NULL || strcmp(file->name, name) != 0) {
        return false;
    }

    if (file->data != NULL) {
        kfree(file->data);
    }

    if (file->name != NULL) {
        kfree((void*)file->name);
    }

    kfree(file);
    files_table[index] = NULL;

    return true;
}

File* ramdisk_get(const char* name) {
    return files_table[hash(name) % RAMDISK_HASH_SIZE];
}

FileNode* ramdisk_getall(const char* path) {
    FileNode* head  = NULL;

    const char* search_path = (strcmp(path, "/") == 0) ? "" : path; // TODO CHECK IF THIS WORKS CUZ OF cstring, USE EVERYWHERE IF YES
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
