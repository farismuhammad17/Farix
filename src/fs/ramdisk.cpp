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

#include <map>
#include <string>
#include <string.h>

#include "fs/vfs.h"
#include "memory/heap.h"

#include "fs/ramdisk.h"

static std::map<std::string, File*>* files = nullptr;

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
    files = new std::map<std::string, File*>();
}

bool ramdisk_read(std::string& name, void* buffer, size_t size) {
    File* file = ramdisk_get(name);

    if (!file || !file->data || file->is_directory) return false;

    size_t bytes_to_read = (size < file->size) ? size : file->size;

    memcpy(buffer, file->data, bytes_to_read);

    return true;
}

bool ramdisk_write(std::string& name, const void* buffer, size_t size) {
    File* file = ramdisk_get(name);
    if (!file || file->is_directory) return false;

    if (file->data) kfree(file->data);

    file->data = (uint8_t*) kmalloc(size);
    if (!file->data) return false;

    memcpy(file->data, buffer, size);

    file->size = size;
    return true;
}

bool ramdisk_create(std::string& name) {
    if (files->find(name) != files->end()) return false; // File already exists

    File* new_file = new File();
    new_file->name = name;
    new_file->data = nullptr; // Empty file
    new_file->size = 0;
    new_file->is_directory = false;

    (*files)[name] = new_file;
    return true;
}

bool ramdisk_mkdir(std::string& name) {
    if (files->find(name) != files->end()) return false;

    File* new_dir = new File();
    new_dir->name = name;
    new_dir->data = nullptr;
    new_dir->size = 0;
    new_dir->is_directory = true;

    (*files)[name] = new_dir;
    return true;
}

bool ramdisk_remove(std::string& name) {
    File* file = ramdisk_get(name);

    if (file == nullptr) return false;
    if (file->data != nullptr) kfree(file->data);

    delete file;
    files->erase(name);

    return true;
}

File* ramdisk_get(std::string& name) {
    if (files == nullptr) return nullptr;

    auto it = files->find(name);
    if (it == files->end()) return nullptr;

    return it->second;
}

FileNode* ramdisk_getall(std::string& path) {
    FileNode* head = nullptr;

    std::string search_path = path;
    if (search_path == "/") search_path = "";

    for (auto it = files->begin(); it != files->end(); ++it) {
        const std::string& name = it->first;
        File* file_ptr = it->second;

        if (name.find(search_path) == 0 && name != search_path) {
            std::string relative = name.substr(search_path.length());

            if (!relative.empty() && relative[0] == '/') {
                relative = relative.substr(1);
            }

            if (relative.find('/') == std::string::npos) {
                if (relative.empty()) continue;

                FileNode* newNode = new FileNode();
                if (!newNode) return head;

                newNode->file.size = file_ptr->size;
                newNode->file.is_directory = file_ptr->is_directory;
                newNode->file.name = name;

                newNode->next = head;
                head = newNode;
            }
        }
    }

    return head;
}
