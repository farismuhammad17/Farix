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

void format_to_83(string name, uint8_t* out_name) {
    // Initialize with spaces (FAT32 requirement)
    for(int i = 0; i < 11; i++) out_name[i] = ' ';

    // Create an uppercase version of the name to work with
    string* upper_name = name.upper();

    // Find the dot position in the uppercase string
    int dot_pos = -1;
    for(int j = 0; j < upper_name->length(); j++) {
        if((*upper_name)[j] == '.') {
            dot_pos = j;
            break;
        }
    }

    // Copy the name part (up to 8 chars)
    int name_len = (dot_pos == -1) ? upper_name->length() : dot_pos;
    for(int i = 0; i < name_len && i < 8; i++) {
        out_name[i] = (*upper_name)[i];
    }

    // Copy the extension part (up to 3 chars)
    if(dot_pos != -1) {
        for(int k = 0; k < 3 && (dot_pos + 1 + k) < upper_name->length(); k++) {
            out_name[8 + k] = (*upper_name)[dot_pos + 1 + k];
        }
    }

    delete upper_name;
}

bool compare_fat_names(uint8_t* fat_name, string user_name) {
    uint8_t formatted_user_name[11];

    format_to_83(user_name, formatted_user_name);

    for (int i = 0; i < 11; i++) {
        if (fat_name[i] != formatted_user_name[i]) {
            return false;
        }
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

uint32_t find_entry_in_cluster(uint32_t directory_cluster, string name) {
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
uint32_t find_cluster_for_path(string path) {
    size_t  parts_count = 0;
    string* parts = path.split('/', 0, parts_count);

    uint32_t current_cluster = disk_info->root_cluster;

    for (size_t i = 0; i < parts_count; i++) {
        if (parts[i].length() == 0) continue; // Skip empty parts from "//"

        // Search current_cluster for parts[i]
        uint32_t next = find_entry_in_cluster(current_cluster, parts[i]);
        if (next == 0) {
            delete[] parts;
            return 0; // Path not found
        }
        current_cluster = next;
    }

    delete[] parts;
    return current_cluster;
}

void init_fat32() {
    uint8_t boot_buf[512];
    ata_read_sector(0, boot_buf);

    if (boot_buf[510] != 0x55 || boot_buf[511] != 0xAA) return;

    // Allocate persistent memory so disk_info doesn't vanish
    disk_info = (FAT32Header*)malloc(sizeof(FAT32Header));
    memcpy(disk_info, boot_buf, sizeof(FAT32Header));
}

bool fat32_read(string name, void* buffer, size_t size) {
    // Resolve the parent directory cluster
    size_t part_count = 0;
    string* parts = name.split('/', 0, part_count);

    uint32_t parent_cluster = disk_info->root_cluster;
    string filename = parts[part_count - 1];

    if (part_count > 1) {
        // Find the cluster of the directory containing the file
        for (size_t i = 0; i < part_count - 1; i++) {
            parent_cluster = find_entry_in_cluster(parent_cluster, parts[i]);
            if (parent_cluster == 0) { delete[] parts; return false; }
        }
    }

    // Search for the file inside the parent_cluster
    uint32_t lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t sector_buffer[512];
    uint8_t* out_ptr = (uint8_t*)buffer;

    // Search the parent directory (simplified to 1 sector here)
    ata_read_sector(lba, sector_buffer);
    FAT32File* files = (FAT32File*)sector_buffer;

    for (int i = 0; i < 16; i++) {
        if (files[i].name[0] == 0x00) { delete[] parts; return false; }

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
                    memcpy(out_ptr, data_temp, chunk);

                    out_ptr += chunk;
                    bytes_to_read -= chunk;
                }

                // Follow the chain to the next cluster
                current_cluster = get_next_cluster(current_cluster);
            }

            delete[] parts;
            return true;
        }
    }

    delete[] parts;
    return false;
}

bool fat32_write(string name, const void* buffer, size_t size) {
    // Resolve path to find the parent directory cluster

    size_t part_count = 0;
    string* parts = name.split('/', 0, part_count);

    if (part_count == 0) return false;

    uint32_t parent_cluster = disk_info->root_cluster;
    string filename = parts[part_count - 1];

    if (part_count > 1) {
        for (size_t i = 0; i < part_count - 1; i++) {
            parent_cluster = find_entry_in_cluster(parent_cluster, parts[i]);
            if (parent_cluster == 0) {
                delete[] parts;
                return false;
            }
        }
    }

    // Search for the file entry in that parent directory
    uint32_t parent_lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t dir_buf[512];
    ata_read_sector(parent_lba, dir_buf);
    FAT32File* entries = (FAT32File*)dir_buf;

    for (int i = 0; i < 16; i++) {
        if (compare_fat_names(entries[i].name, filename)) {
            uint32_t current_cluster = get_file_cluster(&entries[i]);
            uint32_t bytes_left = size;
            const uint8_t* write_ptr = (const uint8_t*)buffer;

            // Write data to the file's clusters
            while (bytes_left > 0 && current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

                for (int s = 0; s < disk_info->sectors_per_cluster && bytes_left > 0; s++) {
                    uint8_t temp_block[512] = {0};
                    uint32_t to_write = (bytes_left > 512) ? 512 : bytes_left;

                    memcpy(temp_block, (void*)write_ptr, to_write);
                    ata_write_sector(lba + s, temp_block);

                    write_ptr += to_write;
                    bytes_left -= to_write;
                }

                if (bytes_left > 0) {
                    current_cluster = get_next_cluster(current_cluster);
                }
            }

            // Update metadata in the parent directory sector
            entries[i].size = size;
            ata_write_sector(parent_lba, dir_buf);

            delete[] parts;
            return true;
        }
    }

    delete[] parts;
    return false;
}

bool fat32_create(string path) {
    size_t part_count = 0;
    string* parts = path.split('/', 0, part_count);

    if (part_count == 0) return false;

    // Determine the parent directory cluster
    uint32_t parent_cluster = disk_info->root_cluster;
    string filename = parts[part_count - 1];

    if (part_count > 1) {
        // Find the cluster for the directory containing the file
        // We navigate to the second-to-last part
        for (size_t i = 0; i < part_count - 1; i++) {
            parent_cluster = find_entry_in_cluster(parent_cluster, parts[i]);
            if (parent_cluster == 0) {
                delete[] parts;
                return false; // Parent folder doesn't exist!
            }
        }
    }

    // Now search for an empty slot in the parent_cluster
    uint32_t parent_lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t buffer[512];

    // For simplicity, we search the first sector of the parent directory
    ata_read_sector(parent_lba, buffer);
    FAT32File* entries = (FAT32File*)buffer;

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
            uint32_t free_cluster = find_free_fat_entry();
            if (free_cluster == 0) { delete[] parts; return false; }

            // Write the entry
            format_to_83(filename, (uint8_t*)entries[i].name);
            entries[i].attributes = 0x20;
            entries[i].cluster_low  = (uint16_t)(free_cluster & 0xFFFF);
            entries[i].cluster_high = (uint16_t)((free_cluster >> 16) & 0xFFFF);
            entries[i].size = 0;

            ata_write_sector(parent_lba, buffer);
            update_fat_entry(free_cluster, 0x0FFFFFFF);

            delete[] parts;
            return true;
        }
    }

    delete[] parts;
    return false;
}

bool fat32_mkdir(string name) {
    // Check if it exists
    if (fat32_get(name) != nullptr) return false;

    // Find a slot in Root (or parent)
    uint32_t root_lba = get_lba_from_cluster(disk_info->root_cluster, disk_info);
    uint8_t dir_buf[512];
    ata_read_sector(root_lba, dir_buf);
    FAT32File* entries = (FAT32File*) dir_buf;

    int slot = -1;
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return false;

    // Allocate a cluster for the folder itself
    uint32_t new_cluster = find_free_fat_entry();
    if (new_cluster == 0) return false;

    // Initialize the new cluster with "." and ".."
    uint8_t folder_data[512] = {0};
    FAT32File* dot_entries = (FAT32File*)folder_data;

    // Setup "."
    memcpy(dot_entries[0].name, ".          ", 11);
    dot_entries[0].attributes = 0x10;
    dot_entries[0].cluster_low = (uint16_t)(new_cluster & 0xFFFF);
    dot_entries[0].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);

    // Setup ".." (pointing to root for now)
    memcpy(dot_entries[1].name, "..         ", 11);
    dot_entries[1].attributes = 0x10;
    dot_entries[1].cluster_low = (uint16_t)(disk_info->root_cluster & 0xFFFF);
    dot_entries[1].cluster_high = (uint16_t)((disk_info->root_cluster >> 16) & 0xFFFF);

    // Write the new folder's base entries to disk
    uint32_t folder_lba = get_lba_from_cluster(new_cluster, disk_info);
    ata_write_sector(folder_lba, folder_data);

    // Link the folder into the parent directory
    format_to_83(name, (uint8_t*)entries[slot].name);
    entries[slot].attributes = 0x10; // DIRECTORY
    entries[slot].cluster_low = (uint16_t)(new_cluster & 0xFFFF);
    entries[slot].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);
    entries[slot].size = 0; // Folders technically have 0 size in FAT32 metadata

    ata_write_sector(root_lba, dir_buf);
    update_fat_entry(new_cluster, 0x0FFFFFFF);

    return true;
}

bool fat32_remove(string name) {
    size_t part_count = 0;
    string* parts = name.split('/', 0, part_count);

    if (part_count == 0) return false;

    // 1. Resolve parent directory
    uint32_t parent_cluster = disk_info->root_cluster;
    string target_name = parts[part_count - 1];

    if (part_count > 1) {
        for (size_t i = 0; i < part_count - 1; i++) {
            parent_cluster = find_entry_in_cluster(parent_cluster, parts[i]);
            if (parent_cluster == 0) {
                delete[] parts;
                return false;
            }
        }
    }

    // 2. Search for the target in the parent directory
    uint32_t parent_lba = get_lba_from_cluster(parent_cluster, disk_info);
    uint8_t dir_buf[512];
    ata_read_sector(parent_lba, dir_buf);
    FAT32File* entries = (FAT32File*)dir_buf;

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) break;
        if (entries[i].name[0] == 0xE5) continue;

        if (compare_fat_names(entries[i].name, target_name)) {
            // 3. Free the Cluster Chain in the FAT
            uint32_t current_cluster = get_file_cluster(&entries[i]);
            while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                uint32_t next_cluster = get_next_cluster(current_cluster);
                update_fat_entry(current_cluster, 0x00000000); // Mark FREE
                current_cluster = next_cluster;
            }

            // 4. Mark directory entry as deleted
            entries[i].name[0] = 0xE5;
            ata_write_sector(parent_lba, dir_buf); // Save changes to the parent dir

            delete[] parts;
            return true;
        }
    }

    delete[] parts;
    return false;
}

File* fat32_get(string name) {
    size_t part_count = 0;
    string* parts = name.split('/', 0, part_count);

    uint32_t current_dir_cluster = disk_info->root_cluster;
    FAT32File found_entry;
    bool found = false;

    // Path Resolution Loop
    for (size_t p = 0; p < part_count; p++) {
        if (parts[p].length() == 0) continue;

        found = false;
        uint32_t lba = get_lba_from_cluster(current_dir_cluster, disk_info);
        uint8_t dir_buf[512];
        ata_read_sector(lba, dir_buf);
        FAT32File* entries = (FAT32File*)dir_buf;

        for (int i = 0; i < 16; i++) {
            if (entries[i].name[0] == 0x00) break;
            if (entries[i].name[0] == 0xE5) continue;

            if (compare_fat_names(entries[i].name, parts[p])) {
                memcpy(&found_entry, &entries[i], sizeof(FAT32File));
                current_dir_cluster = get_file_cluster(&found_entry);
                found = true;
                break;
            }
        }

        if (!found) {
            delete[] parts;
            return nullptr;
        }
    }

    delete[] parts;

    // Prepare the VFS File Object
    File* f = new File();
    f->is_directory = (found_entry.attributes & 0x10);
    f->size = found_entry.size;

    if (f->is_directory) {
        f->data = nullptr;
    } else {
        f->data = (uint8_t*)malloc(f->size);

        uint32_t current_cluster = get_file_cluster(&found_entry);
        uint32_t bytes_read = 0;

        // File reading logic
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
                    memcpy((uint8_t*) (f->data + bytes_read), bounce, remaining);
                    bytes_read += remaining;
                }
            }
            current_cluster = get_next_cluster(current_cluster);
        }
    }

    return f;
}

FileNode* fat32_getall(string path) {
    uint32_t dir_cluster = find_cluster_for_path(path);

    // Handle root vs specific path
    if (path == "" || path == "/") dir_cluster = disk_info->root_cluster;
    else if (dir_cluster == 0) return nullptr;

    FileNode* head = nullptr;
    uint32_t current_cluster = dir_cluster;
    uint8_t* buffer = (uint8_t*) malloc(512);

    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*)buffer;

            for (int i = 0; i < 16; i++) {
                // 0x00: End of entries in this directory
                if (entries[i].name[0] == 0x00) return head;
                // 0xE5: Deleted, 0x08: Volume Label
                if (entries[i].name[0] == 0xE5 || (entries[i].attributes & 0x08)) continue;

                // Create the node
                FileNode* newNode = (FileNode*) malloc(sizeof(FileNode));
                if (!newNode) {
                    free(buffer);
                    return head;
                }

                newNode->file.size = entries[i].size;
                newNode->file.is_directory = (entries[i].attributes & 0x10);

                // Format the 8.3 name
                char raw_name[13];
                int p = 0;
                for(int k=0; k<8; k++) if(entries[i].name[k] != ' ') raw_name[p++] = entries[i].name[k];
                if(entries[i].name[8] != ' ') {
                    raw_name[p++] = '.';
                    for(int k=8; k<11; k++) if(entries[i].name[k] != ' ') raw_name[p++] = entries[i].name[k];
                }
                raw_name[p] = '\0';
                newNode->file.name = string(raw_name);

                // Prepend to list: The New Node points to the current head
                newNode->next = head;
                head = newNode;
            }
        }
        current_cluster = get_next_cluster(current_cluster);
    }

    free(buffer);

    return head;
}
