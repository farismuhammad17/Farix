/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include <stddef.h>
#include <stdint.h>

#include "klib/string.h"

#include "drivers/terminal.h"
#include "fs/vfs.h"
#include "memory/heap.h"

#include "fs/ramdisk.h"

typedef struct {
    File* file;
    File* next;
} RamdiskFile;

static RamdiskFile* files_table[RAMDISK_HASH_SIZE];

VFS ramdisk_vfs = {
    .name  = "RAMDISK",

    .read   = ramdisk_read,
    .write  = ramdisk_write,
    .create = ramdisk_create,
    .mkdir  = ramdisk_mkdir,
    .remove = ramdisk_remove,
    .get    = ramdisk_get,
    .getall = ramdisk_getall,

    .check_write_safety = NULL
};

/*
djb2 hash
Source: https://www.cse.yorku.ca/~oz/hash.html

It is short, efficient, and fast, hence why it was chosen, but any hash will
suffice here as well. The code does not rely on the workings of this specific
hash, simply because I haven't spent the time to understand the hash to see
if there are ways to utilise it any more than a mere hashing algorithm.
*/
static unsigned long hash(const char* name) {
    unsigned long hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/*
Get RamdiskFile provided the File's name. The back parameter just records an
offset like value, in the sense if back = 0, then it returns the RamdiskFile
with the File of the given name, if it's 1, then it returns the one before it,
if 2, it returns the one two before it, and so on.
*/
static RamdiskFile* get_ramfile(const char* name, size_t back) {
    unsigned long index = hash(name) % RAMDISK_HASH_SIZE;

    RamdiskFile* t_ramfile = files_table[index];
    if (unlikely(!t_ramfile)) return NULL;

    while (t_ramfile != NULL && back != 0) {
        t_ramfile = t_ramfile->next;
        back--;
    }
    if (unlikely(back != 0)) return NULL;

    RamdiskFile* ramfile = files_table[index];

    // If t_ramfile is NULL, that means there isn't enough files
    // in the linked list to go back that far, so we just return
    // the root node of the linked list.
    if (unlikely(!t_ramfile)) return ramfile;

    while (strcmp(t_ramfile->file->name, name) != 0 && t_ramfile != NULL) {
        ramfile = ramfile->next;
        t_ramfile = t_ramfile->next;
    }

    // If we moved through the linked list, and the number of files
    // was more than the back value provided, yet there was no file
    // with the given name, don't bother giving the ramfile back of
    // the end.
    if (unlikely(t_ramfile == NULL)) return NULL;

    return ramfile;
}

/*
Creating a file and creating a folder require the exact same procedure, hence
it is dry to repeat this twice. This function simple creates a new file, does all
the ramfile allocations necessary, and returns the File object back for the rest
of the actual function.
*/
static File* create_new_ramdisk_file(const char* name) {
    uint32_t index = hash(name) % RAMDISK_HASH_SIZE;

    RamdiskFile* new_ramfile = (RamdiskFile*) kmalloc(sizeof(RamdiskFile));

    if (unlikely(!new_ramfile)) {
        err_printf("ramdisk_create: Out of memory for ramfile creating file %d", name);
        return NULL;
    }

    memset(new_ramfile, 0, sizeof(RamdiskFile));

    new_ramfile->next = NULL;

    RamdiskFile* current_ramfile_at_index = files_table[index];

    if (likely(current_ramfile_at_index == NULL)) {
        files_table[index] = new_ramfile;
    } else {
        while (current_ramfile_at_index->next != NULL) {
            current_ramfile_at_index = current_ramfile_at_index->next;
        }
        current_ramfile_at_index->next = new_ramfile;
    }

    File* new_file = (File*) kmalloc(sizeof(File));

    if (unlikely(!new_file)) {
        err_printf("ramdisk_create: Out of memory for file creating file %d", name);
        return NULL;
    }

    memset(new_file, 0, sizeof(File));

    new_ramfile->file = new_file;

    return new_file;
}

/* Initialises RAMDISK by setting everything in the files_table to NULL */
void init_ramdisk() {
    for (int i = 0; i < RAMDISK_HASH_SIZE; i++) {
        files_table[i] = NULL;
    }
}

/* Read file from offset to offset+size */
int ramdisk_read(const char* name, void* buffer, size_t size, uint32_t offset) {
    File* file = ramdisk_get(name);

    if (unlikely(!file || !file->data || file->is_directory)) return -1;
    if (unlikely(offset >= file->size)) return 0; // End of file

    // Ensure we don't read past the end of the file
    size_t bytes_to_read = size;
    if (offset + bytes_to_read > file->size) {
        bytes_to_read = file->size - offset;
    }

    // Copy starting from the offset pointer
    memcpy(buffer, (uint8_t*) file->data + offset, bytes_to_read);

    return (int) bytes_to_read;
}

/* Write from offset to offset+size */
int ramdisk_write(const char* name, const void* buffer, size_t size, uint32_t offset) {
    File* file = ramdisk_get(name);

    if (unlikely(!file || file->is_directory)) return -1;

    // If we are writing beyond current capacity, we need more RAM
    if (unlikely(offset + size > file->size)) {
        uint8_t* new_data = (uint8_t*) kmalloc(offset + size);
        if (unlikely(!new_data)) {
            err_printf("ramdisk_write: Out of memory writing to file %s", name);
            return -1;
        }

        // If there was old data, preserve it
        if (likely(file->data)) {
            memcpy(new_data, file->data, file->size);
            kfree(file->data);
        }

        file->data = new_data;
        file->size = offset + size;
    }

    // Now that we're sure the buffer is big enough, copy to the offset
    memcpy((uint8_t*) file->data + offset, buffer, size);

    return (int) size;
}

/* Create new file */
int ramdisk_create(const char* name) {
    File* new_file = create_new_ramdisk_file(name);
    if (unlikely(!new_file)) return -1;

    new_file->name = (char*) strdup(name);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->is_directory = false;

    return 1;
}

/* Create new folder */
int ramdisk_mkdir(const char* name) {
    File* new_file = create_new_ramdisk_file(name);
    if (unlikely(!new_file)) return -1;

    new_file->name = (char*) strdup(name);
    new_file->data = NULL;
    new_file->size = 0;
    new_file->is_directory = true;

    return 1;
}

/* Delete file/folder */
int ramdisk_remove(const char* name) {
    RamdiskFile* prev_ramfile = get_ramfile(name, 1);
    if (unlikely(!prev_ramfile)) return 0;

    RamdiskFile* ramfile = prev_ramfile->next;

    // If there were multiple ramfiles in the linked list chain,
    // we could find the ramfile right before, but, only in the
    // case that there was 1 ramfile, then our ramfile canot be
    // found. In this case, ramfile is simply what we though of
    // as prev_ramfile.
    // Only exception to this logic is if I'm stupid and wrong,
    // while sleep depraved.
    if (unlikely(!ramfile)) {
        ramfile = prev_ramfile;
        files_table[hash(name) % RAMDISK_HASH_SIZE] = NULL;
    }

    // ramfile cannot exist without the file,
    // hence we do not check if it's NULL
    File* file = ramfile->file;

    // Stitch linked list
    prev_ramfile->next = ramfile->next;

    if (likely(file->data != NULL)) kfree(file->data);
    if (likely(file->name != NULL)) kfree((void*) file->name);
    kfree(file);
    kfree(ramfile);

    return 1;
}

/* Get file object at path */
File* ramdisk_get(const char* name) {
    RamdiskFile* ramfile = get_ramfile(name, 0);
    if (unlikely(!ramfile)) return NULL;
    return ramfile->file;
}

/*
Get all contents of directory
NOTE: Files inside each node MUST be freed after use, else, it would cause
memory leaks. The files cannot be freed here itself, since you need to get
the files, obviously.
*/
FileNode* ramdisk_getall(const char* path) {
    FileNode* head  = NULL;

    const char* search_path = (strcmp(path, "/") == 0) ? "" : path;
    size_t search_len = strlen(search_path);

    for (int i = 0; i < RAMDISK_HASH_SIZE; i++) {
        RamdiskFile* ramfile = files_table[i];
        if (unlikely(ramfile == NULL)) continue;

        while (ramfile) {
            const char* name = ramfile->file->name;

            if (strncmp(name, search_path, search_len) == 0 && strcmp(name, search_path) != 0) {
                const char* relative = name + search_len;

                if (relative[0] == '/') relative++;

                if (!strchr(relative, '/')) {
                    if (strlen(relative) == 0) continue;

                    FileNode* newNode = (FileNode*) kmalloc(sizeof(FileNode));

                    if (unlikely(!newNode)) {
                        err_print("ramdisk_getall: Out of memory");
                        return head;
                    }

                    memset(newNode, 0, sizeof(FileNode));

                    newNode->file.size = ramfile->file->size;
                    newNode->file.is_directory = ramfile->file->is_directory;
                    newNode->file.name = ramfile->file->name;

                    newNode->next = head;
                    head = newNode;
                }
            }

            ramfile = ramfile->next;
        }
    }

    return head;
}
