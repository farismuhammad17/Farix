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

#include <string>

struct File {
    std::string   name;
    uint8_t*      data; // Pointer to the actual content in RAM
    size_t        size;
    bool          is_directory;
};

struct FileNode {
    File file;
    FileNode* next;
};

struct FileOperations {
    bool      (*read)   (std::string& name, void* buffer, size_t size);
    bool      (*write)  (std::string& name, const void* buffer, size_t size);
    bool      (*create) (std::string& name);
    bool      (*mkdir)  (std::string& name);
    bool      (*remove) (std::string& name);
    File*     (*get)    (std::string& name);
    FileNode* (*getall) (std::string& path);
};

void      vfs_mount (FileOperations* ops);

bool      fs_read   (std::string& name, void* buffer, size_t size);
bool      fs_write  (std::string& name, const void* buffer, size_t size);
bool      fs_create (std::string& name);
bool      fs_mkdir  (std::string& name);
bool      fs_remove (std::string& name);
File*     fs_get    (std::string& name);
FileNode* fs_getall (std::string& path);

#endif
