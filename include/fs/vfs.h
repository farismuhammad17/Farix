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

#ifndef VFS_H
#define VFS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct File {
    const char* name;
    uint8_t*    data; // Pointer to the actual content in RAM
    size_t      size;
    bool        is_directory;
} File;

typedef struct FileNode {
    File file;
    struct FileNode* next;
} FileNode;

typedef struct FileOperations {
    int       (*read)   (const char* name, void* buffer, size_t size, uint32_t offset);
    int       (*write)  (const char* name, const void* buffer, size_t size, uint32_t offset);
    int       (*create) (const char* name);
    int       (*mkdir)  (const char* name);
    int       (*remove) (const char* name);
    File*     (*get)    (const char* name);
    FileNode* (*getall) (const char* path);
} FileOperations;

void RARE_FUNC vfs_mount (FileOperations* ops);

int       fs_read   (const char* name, void* buffer, size_t size, uint32_t offset);
int       fs_write  (const char* name, const void* buffer, size_t size, uint32_t offset);
int       fs_create (const char* name);
int       fs_mkdir  (const char* name);
int       fs_remove (const char* name);
File*     fs_get    (const char* name);
FileNode* fs_getall (const char* path);

#endif
