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

#include <string>
#include <string.h>

#include "drivers/ata.h"
#include "memory/heap.h"
#include "fs/vfs.h"

#include "fs/fat32.h"

static uint8_t      header_buffer[512];
static FAT32Header* disk_info = nullptr;

FileOperations fat32_ops = {
    .read   = fat32_read,
    .write  = fat32_write,
    .create = fat32_create,
    .mkdir  = fat32_mkdir,
    .remove = fat32_remove,
    .get    = fat32_get,
    .getall = fat32_getall
};

uint32_t get_lba_from_cluster(uint32_t cluster, FAT32Header* header) {
    uint32_t root_dir_start = header->reserved_sectors + (header->fat_count * header->sectors_per_fat);
    return root_dir_start + ((cluster - 2) * header->sectors_per_cluster);
}

uint32_t get_file_cluster(FAT32File* file) {
    return (uint32_t)file->cluster_low | ((uint32_t)file->cluster_high << 16);
}

uint32_t get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = disk_info->reserved_sectors + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    uint8_t buffer[512];
    ata_read_sector(fat_sector, buffer);

    // Use uint32_t pointer arithmetic on the byte offset
    uint32_t next_cluster = *(uint32_t*) &buffer[ent_offset];
    return next_cluster & 0x0FFFFFFF;
}

uint32_t find_free_fat_entry() {
    uint8_t fat_buffer[512];
    for (uint32_t s = 0; s < disk_info->sectors_per_fat; s++) {
        ata_read_sector(disk_info->reserved_sectors + s, fat_buffer);
        uint32_t* fat = (uint32_t*)fat_buffer;

        for (int i = 0; i < 128; i++) {
            // Cluster 0 and 1 are reserved, so skip them in the very first sector
            if (s == 0 && i < 2) continue;

            uint32_t entry = fat[i] & 0x0FFFFFFF;
            if (entry == 0x0000000) {
                return (s * 128) + i; // This is the absolute Cluster Number
            }
        }
    }
    return 0; // Disk Full
}

void format_to_83(const std::string& name, uint8_t* out_name) {
    kmemset(out_name, ' ', 11);

    size_t dot_pos = name.find_last_of('.');
    size_t name_len = (dot_pos == std::string::npos) ? name.length() : dot_pos;

    // Copy name part
    for(size_t i = 0; i < name_len && i < 8; i++) {
        out_name[i] = toupper(name[i]);
    }

    // Copy extension part
    if(dot_pos != std::string::npos) {
        for(size_t k = 0; k < 3 && (dot_pos + 1 + k) < name.length(); k++) {
            out_name[8 + k] = toupper(name[dot_pos + 1 + k]);
        }
    }
}

bool compare_fat_names(uint8_t* fat_name, const std::string& user_name) {
    uint8_t formatted_user_name[11];
    format_to_83(user_name, formatted_user_name);

    for (int i = 0; i < 11; i++) {
        if (fat_name[i] != formatted_user_name[i]) return false;
    }
    return true;
}

void update_fat_entry(uint32_t cluster, uint32_t value) {
    uint8_t fat_buffer[512];

    uint32_t fat_sector_offset = (cluster * 4) / 512;
    uint32_t lba = disk_info->reserved_sectors + fat_sector_offset;

    ata_read_sector(lba, fat_buffer);

    uint32_t* fat = (uint32_t*)fat_buffer;
    uint32_t entry_index = cluster % 128;
    fat[entry_index] = value;

    ata_write_sector(lba, fat_buffer);
}

uint32_t find_entry_in_cluster(uint32_t directory_cluster, std::string name) {
    uint32_t current_cluster = directory_cluster;
    uint8_t buffer[512];

    // Follow the cluster chain of the directory
    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

        // A cluster can have multiple sectors
        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*)buffer;

            for (int i = 0; i < 16; i++) {
                // 0x00 means end of directory, stop searching
                if (entries[i].name[0] == 0x00) return 0;
                // 0xE5 is a deleted file, skip it
                if (entries[i].name[0] == 0xE5) continue;

                if (compare_fat_names(entries[i].name, name)) {
                    return get_file_cluster(&entries[i]);
                }
            }
        }
        // Move to the next cluster in the directory's chain
        current_cluster = get_next_cluster(current_cluster);
    }

    return 0; // Not found
}

// Helper to find the starting cluster of a directory/file path
uint32_t find_cluster_for_path(const std::string& path) {
    uint32_t current_cluster = disk_info->root_cluster;
    size_t start = 0;

    while (start < path.length()) {
        size_t end = path.find('/', start);
        size_t len = (end == std::string::npos) ? (path.length() - start) : (end - start);

        std::string segment = path.substr(start, len);

        if (!segment.empty()) {
            uint32_t next = find_entry_in_cluster(current_cluster, segment);
            if (next == 0) return 0;
            current_cluster = next;
        }

        if (end == std::string::npos) break;
        start = end + 1;
    }
    return current_cluster;
}

void init_fat32() {
    uint8_t boot_buf[512];
    ata_read_sector(0, boot_buf);

    if (boot_buf[510] != 0x55 || boot_buf[511] != 0xAA) return;

    disk_info = (FAT32Header*) kmalloc(sizeof(FAT32Header));
    if (disk_info != nullptr) kmemcpy(disk_info, boot_buf, sizeof(FAT32Header));
}

bool fat32_read(std::string& name, void* buffer, size_t size) {
    size_t last_slash = name.find_last_of('/');
    std::string path = (last_slash == std::string::npos) ? "" : name.substr(0, last_slash);
    std::string filename = (last_slash == std::string::npos) ? name : name.substr(last_slash + 1);

    uint32_t parent_cluster = disk_info->root_cluster;

    // If there is a path (e.g., "dir1/dir2"), we need to walk it
    if (!path.empty() && path != "/") {
        // This helper should walk the path and return the cluster of the last directory
        parent_cluster = find_cluster_for_path(path);
        if (parent_cluster == 0) return false;
    }

    // Search for the file inside the parent_cluster
    uint32_t lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t sector_buffer[512];
    uint8_t* out_ptr = (uint8_t*)buffer;

    // Read the directory sector
    ata_read_sector(lba, sector_buffer);
    FAT32File* files = (FAT32File*) sector_buffer;

    for (int i = 0; i < 16; i++) {
        if (files[i].name[0] == 0x00) return false;

        if (compare_fat_names(files[i].name, filename)) {
            uint32_t current_cluster = get_file_cluster(&files[i]);
            uint32_t bytes_to_read = (size < files[i].size) ? size : files[i].size;

            while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8 && bytes_to_read > 0) {
                uint32_t file_lba = get_lba_from_cluster(current_cluster, disk_info);

                for (int sec = 0; sec < disk_info->sectors_per_cluster && bytes_to_read > 0; sec++) {
                    uint8_t data_temp[512];
                    ata_read_sector(file_lba + sec, data_temp);

                    // Calculate the chunk for this specific sector
                    uint32_t chunk = (bytes_to_read > 512) ? 512 : bytes_to_read;
                    kmemcpy(out_ptr, data_temp, chunk);

                    out_ptr += chunk;
                    bytes_to_read -= chunk;
                }

                // Follow the chain to the next cluster
                current_cluster = get_next_cluster(current_cluster);
            }

            return true;
        }
    }

    return false;
}

bool fat32_write(std::string& name, const void* buffer, size_t size) {
    size_t last_slash = name.find_last_of('/');
    std::string path = (last_slash == std::string::npos) ? "/" : name.substr(0, last_slash);
    std::string filename = (last_slash == std::string::npos) ? name : name.substr(last_slash + 1);

    uint32_t parent_cluster = (path == "/" || path.empty()) ? disk_info->root_cluster : find_cluster_for_path(path);
    if (parent_cluster == 0) return false;

    uint32_t parent_lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t dir_buf[512];
    ata_read_sector(parent_lba, dir_buf);
    FAT32File* entries = (FAT32File*)dir_buf;

    for (int i = 0; i < 16; i++) {
        if (compare_fat_names(entries[i].name, filename)) {
            uint32_t current_cluster = get_file_cluster(&entries[i]);
            uint32_t bytes_left = size;
            const uint8_t* write_ptr = (const uint8_t*)buffer;

            while (bytes_left > 0 && current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

                for (int s = 0; s < disk_info->sectors_per_cluster && bytes_left > 0; s++) {
                    uint8_t temp_block[512] = {0};
                    uint32_t to_write = (bytes_left > 512) ? 512 : bytes_left;

                    kmemcpy(temp_block, write_ptr, to_write);
                    ata_write_sector(lba + s, temp_block);

                    write_ptr += to_write;
                    bytes_left -= to_write;
                }

                if (bytes_left > 0) {
                    uint32_t next = get_next_cluster(current_cluster);
                    current_cluster = next;
                }
            }

            entries[i].size = size;
            ata_write_sector(parent_lba, dir_buf);

            return true;
        }
    }

    return false; // File not found
}

bool fat32_create(std::string& path) {
    size_t last_slash = path.find_last_of('/');
    std::string parent_path = (last_slash == std::string::npos) ? "/" : path.substr(0, last_slash);
    std::string filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);

    uint32_t parent_cluster = (parent_path == "/" || parent_path.empty())
                              ? disk_info->root_cluster
                              : find_cluster_for_path(parent_path);

    if (parent_cluster == 0) return false;

    uint32_t current_dir_cluster = parent_cluster;
    uint8_t buffer[512];

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_dir_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*)buffer;

            for (int i = 0; i < 16; i++) {
                // 0x00 = End of directory, 0xE5 = Deleted entry (Both are free slots)
                if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
                    uint32_t free_cluster = find_free_fat_entry();
                    if (free_cluster == 0) return false; // Disk Full

                    format_to_83(filename, (uint8_t*)entries[i].name);
                    entries[i].attributes = 0x20; // Archive attribute
                    entries[i].cluster_low  = (uint16_t)(free_cluster & 0xFFFF);
                    entries[i].cluster_high = (uint16_t)((free_cluster >> 16) & 0xFFFF);
                    entries[i].size = 0;

                    // Write updated directory sector back to disk
                    ata_write_sector(lba + s, buffer);

                    // Mark cluster as EOF in the FAT
                    update_fat_entry(free_cluster, 0x0FFFFFFF);

                    return true;
                }
            }
        }
        current_dir_cluster = get_next_cluster(current_dir_cluster);
    }

    return false; // No space in directory
}

bool fat32_mkdir(std::string& name) {
    if (fat32_get(name) != nullptr) return false;

    size_t last_slash = name.find_last_of('/');
    std::string parent_path = (last_slash == std::string::npos) ? "/" : name.substr(0, last_slash);
    std::string folder_name = (last_slash == std::string::npos) ? name : name.substr(last_slash + 1);

    uint32_t parent_cluster = (parent_path == "/" || parent_path.empty())
                              ? disk_info->root_cluster
                              : find_cluster_for_path(parent_path);

    if (parent_cluster == 0) return false;

    uint32_t parent_lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t dir_buf[512];
    ata_read_sector(parent_lba, dir_buf);
    FAT32File* entries = (FAT32File*) dir_buf;

    int slot = -1;
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return false; // No space in parent sector

    uint32_t new_cluster = find_free_fat_entry();
    if (new_cluster == 0) return false;

    uint8_t folder_data[512];
    memset(folder_data, 0, 512);

    FAT32File* dot_entries = (FAT32File*)folder_data;

    // Setup "." (Points to itself)
    kmemcpy(dot_entries[0].name, ".          ", 11);
    dot_entries[0].attributes = 0x10; // Directory attribute
    dot_entries[0].cluster_low = (uint16_t)(new_cluster & 0xFFFF);
    dot_entries[0].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);

    // Setup ".." (Points to parent)
    kmemcpy(dot_entries[1].name, "..         ", 11);
    dot_entries[1].attributes = 0x10;
    dot_entries[1].cluster_low = (uint16_t)(parent_cluster & 0xFFFF);
    dot_entries[1].cluster_high = (uint16_t)((parent_cluster >> 16) & 0xFFFF);

    // Write "." and ".." to the disk
    uint32_t folder_lba = get_lba_from_cluster(new_cluster, disk_info);
    ata_write_sector(folder_lba, folder_data);

    format_to_83(folder_name, (uint8_t*)entries[slot].name);
    entries[slot].attributes = 0x10;
    entries[slot].cluster_low = (uint16_t)(new_cluster & 0xFFFF);
    entries[slot].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);
    entries[slot].size = 0;

    ata_write_sector(parent_lba, dir_buf);
    update_fat_entry(new_cluster, 0x0FFFFFFF); // Mark as EOF in FAT

    return true;
}

bool fat32_remove(std::string& name) {
    size_t last_slash = name.find_last_of('/');
    std::string path = (last_slash == std::string::npos) ? "/" : name.substr(0, last_slash);
    std::string target_name = (last_slash == std::string::npos) ? name : name.substr(last_slash + 1);

    uint32_t parent_cluster = (path == "/" || path.empty())
                              ? disk_info->root_cluster
                              : find_cluster_for_path(path);

    if (parent_cluster == 0) return false;

    uint32_t parent_lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t dir_buf[512];
    ata_read_sector(parent_lba, dir_buf);
    FAT32File* entries = (FAT32File*)dir_buf;

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) break;    // End of directory
        if (entries[i].name[0] == 0xE5) continue; // Deleted

        if (compare_fat_names(entries[i].name, target_name)) {
            uint32_t current_cluster = get_file_cluster(&entries[i]);

            while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                uint32_t next_cluster = get_next_cluster(current_cluster);
                update_fat_entry(current_cluster, 0x00000000); // 0x00 = FREE
                current_cluster = next_cluster;
            }

            entries[i].name[0] = 0xE5;
            ata_write_sector(parent_lba, dir_buf);

            return true;
        }
    }

    return false; // File or folder not found
}

File* fat32_get(std::string& name) {
    uint32_t cluster = find_cluster_for_path(name);
    if (cluster == 0 && name != "/") return nullptr;

    size_t last_slash = name.find_last_of('/');
    std::string path = (last_slash == std::string::npos) ? "/" : name.substr(0, last_slash);
    std::string filename = (last_slash == std::string::npos) ? name : name.substr(last_slash + 1);

    uint32_t parent_cluster = (path == "/" || path.empty()) ? disk_info->root_cluster : find_cluster_for_path(path);

    uint8_t sector_buf[512];
    FAT32File found_entry;
    bool entry_found = false;

    uint32_t current_dir_cluster = parent_cluster;
    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_dir_cluster, disk_info);
        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, sector_buf);
            FAT32File* entries = (FAT32File*)sector_buf;
            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00) goto search_done;
                if (entries[i].name[0] == 0xE5) continue;

                if (compare_fat_names(entries[i].name, filename)) {
                    kmemcpy(&found_entry, &entries[i], sizeof(FAT32File));
                    entry_found = true;
                    goto search_done;
                }
            }
        }
        current_dir_cluster = get_next_cluster(current_dir_cluster);
    }

search_done:
    if (!entry_found) return nullptr;

    File* f = new File();
    f->name = name;
    f->is_directory = (found_entry.attributes & 0x10);
    f->size = found_entry.size;

    if (f->is_directory) {
        f->data = nullptr;
    } else {
        f->data = (uint8_t*) kmalloc(f->size);
        if (!f->data) return f;

        uint32_t current_cluster = get_file_cluster(&found_entry);
        uint32_t bytes_read = 0;

        while (bytes_read < f->size && current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
            uint32_t start_lba = get_lba_from_cluster(current_cluster, disk_info);

            for (int s = 0; s < disk_info->sectors_per_cluster && bytes_read < f->size; s++) {
                uint32_t remaining = f->size - bytes_read;
                if (remaining >= 512) {
                    ata_read_sector(start_lba + s, (uint8_t*)(f->data + bytes_read));
                    bytes_read += 512;
                } else {
                    uint8_t bounce[512];
                    ata_read_sector(start_lba + s, bounce);
                    kmemcpy((uint8_t*)(f->data + bytes_read), bounce, remaining);
                    bytes_read += remaining;
                }
            }
            current_cluster = get_next_cluster(current_cluster);
        }
    }

    return f;
}

FileNode* fat32_getall(std::string& path) {
    uint32_t dir_cluster = find_cluster_for_path(path);

    // Handle root vs specific path
    if (path == "" || path == "/") {
        dir_cluster = disk_info->root_cluster;
    } else if (dir_cluster == 0) {
        return nullptr;
    }

    FileNode* head = nullptr;
    uint32_t current_cluster = dir_cluster;

    // Using a stack-based buffer is faster/safer than kmalloc for 512 bytes
    uint8_t buffer[512];

    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*) buffer;

            for (int i = 0; i < 16; i++) {
                // 0x00: End of entries
                if (entries[i].name[0] == 0x00) return head;

                // 0xE5: Deleted, 0x08: Volume Label, 0x0F: LFN entry (skip for now)
                if (entries[i].name[0] == 0xE5 || (entries[i].attributes & 0x08)) continue;

                // Skip "." and ".." to keep 'ls' clean, or include them if you prefer
                if (entries[i].name[0] == '.') continue;

                FileNode* newNode = new FileNode();
                if (!newNode) return head;

                newNode->file.size = entries[i].size;
                newNode->file.is_directory = (entries[i].attributes & 0x10);

                // --- Format the 8.3 name into a readable string ---
                std::string name_str = "";
                // Filename (8 chars)
                for (int k = 0; k < 8; k++) {
                    if (entries[i].name[k] != ' ') name_str += (char)entries[i].name[k];
                }
                // Extension (3 chars)
                if (entries[i].name[8] != ' ') {
                    name_str += '.';
                    for (int k = 8; k < 11; k++) {
                        if (entries[i].name[k] != ' ') name_str += (char)entries[i].name[k];
                    }
                }

                newNode->file.name = name_str;

                // Prepend to list
                newNode->next = head;
                head = newNode;
            }
        }
        current_cluster = get_next_cluster(current_cluster);
    }

    return head;
}
