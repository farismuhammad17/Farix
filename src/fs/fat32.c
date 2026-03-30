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

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "drivers/ata.h"
#include "fs/vfs.h"
#include "memory/heap.h"

#include "fs/fat32.h"

static FAT32Header* disk_info = NULL;

FileOperations fat32_ops = {
    .read   = fat32_read,
    .write  = fat32_write,
    .create = fat32_create,
    .mkdir  = fat32_mkdir,
    .remove = fat32_remove,
    .get    = fat32_get,
    .getall = fat32_getall
};

static uint32_t get_lba_from_cluster(uint32_t cluster, FAT32Header* header) {
    uint32_t root_dir_start = header->reserved_sectors + (header->fat_count * header->sectors_per_fat);
    return root_dir_start + ((cluster - 2) * header->sectors_per_cluster);
}

static uint32_t get_file_cluster(FAT32File* file) {
    return (uint32_t) file->cluster_low | ((uint32_t) file->cluster_high << 16);
}

static uint32_t get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster << 2;
    uint32_t fat_sector = disk_info->reserved_sectors + (fat_offset >> 9);
    uint32_t ent_offset = fat_offset % 512;

    uint8_t buffer[512];
    ata_read_sector(fat_sector, buffer);

    // Use uint32_t pointer arithmetic on the byte offset
    uint32_t next_cluster = *(uint32_t*) &buffer[ent_offset];
    return next_cluster & 0x0FFFFFFF;
}

static uint32_t find_free_fat_entry() {
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

static void format_to_83(const char* name, uint8_t* out_name) {
    kmemset(out_name, ' ', 11);

    const char* dot_ptr = strrchr(name, '.');

    size_t name_len;
    if (dot_ptr == NULL) {
        name_len = strlen(name);
    } else {
        name_len = (size_t)(dot_ptr - name);
    }

    for (size_t i = 0; i < name_len && i < 8; i++) {
        out_name[i] = (uint8_t) toupper((unsigned char) name[i]);
    }

    if (dot_ptr != NULL) {
        const char* ext = dot_ptr + 1;
        for (size_t k = 0; k < 3 && ext[k] != '\0'; k++) {
            out_name[8 + k] = (uint8_t) toupper((unsigned char) ext[k]);
        }
    }
}

static bool compare_fat_names(uint8_t* fat_name, const char* user_name) {
    uint8_t formatted_user_name[11];
    format_to_83(user_name, formatted_user_name);

    for (int i = 0; i < 11; i++) {
        if (fat_name[i] != formatted_user_name[i]) return false;
    }
    return true;
}

static void update_fat_entry(uint32_t cluster, uint32_t value) {
    uint8_t fat_buffer[512];

    uint32_t fat_sector_offset = cluster >> 7; // Equivalent to * 4 / 512
    uint32_t lba = disk_info->reserved_sectors + fat_sector_offset;

    ata_read_sector(lba, fat_buffer);

    uint32_t* fat = (uint32_t*) fat_buffer;
    uint32_t entry_index = cluster % 128;
    fat[entry_index] = value;

    ata_write_sector(lba, fat_buffer);
}

static uint32_t find_entry_in_cluster(uint32_t directory_cluster, const char* name) {
    uint32_t current_cluster = directory_cluster;
    uint8_t buffer[512];

    // Follow the cluster chain of the directory
    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

        // A cluster can have multiple sectors
        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*) buffer;

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
static uint32_t find_cluster_for_path(const char* path) {
    uint32_t current_cluster = disk_info->root_cluster;
    const char* start = path;

    if (*start == '/') start++;

    while (*start != '\0') {
        const char* end = strchr(start, '/');
        char segment[13]; // FAT 8.3 names are max 12 chars (8 + 1 + 3)
        size_t len;

        if (end == NULL) {
            // Last part of the path (the filename)
            len = strlen(start);
        } else {
            // Middle part of the path (a folder name)
            len = (size_t)(end - start);
        }

        if (len > 12) len = 12;
        kmemcpy(segment, start, len);
        segment[len] = '\0';

        if (len > 0) {
            uint32_t next = find_entry_in_cluster(current_cluster, segment);
            if (next == 0) return 0; // Folder/File not found
            current_cluster = next;
        }

        if (end == NULL) break;
        start = end + 1;
    }

    return current_cluster;
}

static bool create_directory_entry(uint32_t sector_lba, int index, const char* name, uint32_t parent_cluster) {
    uint8_t buffer[512];
    ata_read_sector(sector_lba, buffer);

    FAT32File* entries = (FAT32File*) buffer;

    uint32_t new_cluster = find_free_fat_entry();
    if (new_cluster == 0) return false;

    format_to_83(name, (uint8_t*)entries[index].name);

    entries[index].attributes   = 0x10;
    entries[index].cluster_low  = (uint16_t)(new_cluster & 0xFFFF);
    entries[index].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);
    entries[index].size = 0;

    ata_write_sector(sector_lba, buffer);
    update_fat_entry(new_cluster, 0x0FFFFFFF);

    // Initialize "." and ".."
    uint8_t folder_data[512] = {0};
    FAT32File* dot_entries = (FAT32File*)folder_data;

    kmemcpy(dot_entries[0].name, ".          ", 11);
    dot_entries[0].attributes   = 0x10;
    dot_entries[0].cluster_low  = (uint16_t)(new_cluster & 0xFFFF);
    dot_entries[0].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);

    kmemcpy(dot_entries[1].name, "..         ", 11);
    dot_entries[1].attributes   = 0x10;
    dot_entries[1].cluster_low  = (uint16_t)(parent_cluster & 0xFFFF);
    dot_entries[1].cluster_high = (uint16_t)((parent_cluster >> 16) & 0xFFFF);

    uint32_t folder_lba = get_lba_from_cluster(new_cluster, disk_info);

    ata_write_sector(folder_lba, folder_data);

    return true;
}

void init_fat32() {
    uint8_t boot_buf[512];
    ata_read_sector(0, boot_buf);

    if (boot_buf[510] != 0x55 || boot_buf[511] != 0xAA) return;

    disk_info = (FAT32Header*) kmalloc(sizeof(FAT32Header));
    if (disk_info != NULL) kmemcpy(disk_info, boot_buf, sizeof(FAT32Header));
}

int fat32_read(const char* name, void* buffer, size_t size) {
    char* filename = strrchr(name, '/');
    char* path     = (char*) name;

    if (filename == NULL) {
        filename = (char*) name;
        path = "";
    } else {
        *filename = '\0';
        filename++;
    }

    uint32_t parent_cluster = disk_info->root_cluster;

    if (path[0] != '\0') {
        parent_cluster = find_cluster_for_path(path);
        if (parent_cluster == 0) return -1;
    }

    uint8_t  sector_buffer[512];
    uint8_t* out_ptr = (uint8_t*) buffer;

    uint32_t current_dir_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_dir_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, sector_buffer);
            FAT32File* files = (FAT32File*) sector_buffer;

            for (int i = 0; i < 16; i++) {
                if (files[i].name[0] == 0x00) return -1;
                if (files[i].name[0] == 0xE5 || files[i].attributes == 0x0F) continue;

                if (compare_fat_names(files[i].name, filename)) {
                    uint32_t current_cluster = get_file_cluster(&files[i]);

                    uint32_t file_size       = files[i].size;
                    uint32_t bytes_remaining = (size < file_size) ? size : file_size;

                    uint32_t total_to_read   = bytes_remaining;

                    while (current_cluster >= 2 &&
                           current_cluster < 0x0FFFFFF8 &&
                           bytes_remaining > 0) {
                        uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

                        for (int sec = 0; sec < disk_info->sectors_per_cluster && bytes_remaining > 0; sec++) {
                            ata_read_sector(lba + sec, sector_buffer);

                            uint32_t to_copy = (bytes_remaining > 512) ? 512 : bytes_remaining;
                            kmemcpy(out_ptr, sector_buffer, to_copy);

                            out_ptr += to_copy;
                            bytes_remaining -= to_copy;
                        }

                        current_cluster = get_next_cluster(current_cluster);
                    }

                    return (int)(total_to_read - bytes_remaining);
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);
    }

    return -1;
}

int fat32_write(const char* name, const void* buffer, size_t size) {
    char* filename = strrchr(name, '/');
    char* path     = (char*) name;

    if (filename == NULL) {
        filename = (char*) name;
        path = "";
    } else {
        *filename = '\0';
        filename++;
    }

    uint32_t parent_cluster = disk_info->root_cluster;
    if (path[0] != '\0' && strcmp(path, "/") != 0)
        parent_cluster = find_cluster_for_path(path);

    if (parent_cluster == 0) return -1;

    uint8_t  dir_buf[512];
    uint32_t current_dir_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_dir_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, dir_buf);
            FAT32File* entries = (FAT32File*) dir_buf;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00) return -1;  // End of directory
                if (entries[i].name[0] == 0xE5 || entries[i].attributes == 0x0F) continue;

                if (compare_fat_names(entries[i].name, filename)) {
                    uint32_t current_cluster = get_file_cluster(&entries[i]);

                    uint32_t bytes_left = size;
                    const uint8_t* write_ptr = (const uint8_t*) buffer;

                    while (bytes_left > 0 && current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                        uint32_t data_lba = get_lba_from_cluster(current_cluster, disk_info);

                        for (int sec = 0; sec < disk_info->sectors_per_cluster && bytes_left > 0; sec++) {
                            uint8_t temp_block[512];
                            ata_read_sector(data_lba + sec, temp_block);

                            uint32_t to_write = (bytes_left > 512) ? 512 : bytes_left;
                            kmemcpy(temp_block, write_ptr, to_write);

                            ata_write_sector(data_lba + sec, temp_block);

                            write_ptr += to_write;
                            bytes_left -= to_write;
                        }

                        if (bytes_left > 0) current_cluster = get_next_cluster(current_cluster);
                    }

                    // Update size
                    entries[i].size = size;

                    // Write back this directory sector
                    ata_write_sector(lba + s, dir_buf);

                    return size - bytes_left;
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);
    }

    return -1;
}

bool fat32_create(const char* path) {
    char* filename = strrchr(path, '/');
    char* dir_path = (char*) path;

    if (filename == NULL) {
        filename = (char*) path;
        dir_path = "";
    } else {
        *filename = '\0';
        filename++;
    }

    uint32_t parent_cluster = disk_info->root_cluster;
    if (dir_path[0] != '\0' && strcmp(dir_path, "/") != 0)
        parent_cluster = find_cluster_for_path(dir_path);

    if (parent_cluster == 0) return false;

    uint8_t buffer[512];
    uint32_t current_dir_cluster = parent_cluster;
    uint32_t last_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        last_cluster = current_dir_cluster;
        uint32_t lba = get_lba_from_cluster(current_dir_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*) buffer;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
                    uint32_t file_cluster = find_free_fat_entry();
                    if (file_cluster == 0) return false;

                    format_to_83(filename, (uint8_t*)entries[i].name);
                    entries[i].attributes = 0x20; // Archive attribute
                    entries[i].cluster_low  = (uint16_t)(file_cluster & 0xFFFF);
                    entries[i].cluster_high = (uint16_t)((file_cluster >> 16) & 0xFFFF);
                    entries[i].size = 0;

                    ata_write_sector(lba + s, buffer);
                    update_fat_entry(file_cluster, 0x0FFFFFFF); // Mark EOF in FAT

                    return true;
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);
    }

    // If we reach here, the directory is full, so we grow it.
    uint32_t new_cluster = find_free_fat_entry();
    if (new_cluster == 0) return false;

    update_fat_entry(last_cluster, new_cluster);
    update_fat_entry(new_cluster, 0x0FFFFFFF);

    // Zero out the new directory cluster
    uint32_t new_lba = get_lba_from_cluster(new_cluster, disk_info);
    uint8_t zero_block[512] = {0};
    for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
        ata_write_sector(new_lba + s, zero_block);
    }

    // Place the new file entry in the first slot of the new cluster
    uint32_t file_cluster = find_free_fat_entry();
    if (file_cluster == 0) return false;

    // Use the first slot [0] of the freshly zeroed cluster
    FAT32File new_entry = {0};
    format_to_83(filename, (uint8_t*)new_entry.name);
    new_entry.attributes = 0x20;
    new_entry.cluster_low  = (uint16_t)(file_cluster & 0xFFFF);
    new_entry.cluster_high = (uint16_t)((file_cluster >> 16) & 0xFFFF);
    new_entry.size = 0;

    // Write the new entry to the start of the new cluster
    kmemcpy(buffer, &new_entry, sizeof(FAT32File));
    ata_write_sector(new_lba, buffer);
    update_fat_entry(file_cluster, 0x0FFFFFFF);

    return true;
}

bool fat32_mkdir(const char* path) {
    char* folder_name = strrchr(path, '/');
    char* parent_path = (char*) path;

    if (folder_name == NULL) {
        folder_name = (char*) path;
        parent_path = "";
    } else {
        *folder_name = '\0';
        folder_name++;
    }

    uint32_t parent_cluster = disk_info->root_cluster;
    if (parent_path[0] != '\0' && strcmp(parent_path, "/") != 0)
        parent_cluster = find_cluster_for_path(parent_path);

    if (parent_cluster == 0) return false;

    uint8_t buffer[512];
    uint32_t current_dir_cluster = parent_cluster;
    uint32_t last_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        last_cluster = current_dir_cluster;
        uint32_t lba = get_lba_from_cluster(current_dir_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*) buffer;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
                    return create_directory_entry(lba + s, i, folder_name, parent_cluster);
                }
            }
        }
        current_dir_cluster = get_next_cluster(current_dir_cluster);
    }

    uint32_t new_parent_ext_cluster = find_free_fat_entry();
    if (new_parent_ext_cluster == 0) return false;

    update_fat_entry(last_cluster, new_parent_ext_cluster);
    update_fat_entry(new_parent_ext_cluster, 0x0FFFFFFF);

    uint32_t new_lba = get_lba_from_cluster(new_parent_ext_cluster, disk_info);
    uint8_t zero_block[512] = {0};

    // Initialize the new sector with zeros
    for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
        ata_write_sector(new_lba + s, zero_block);
    }

    // Use slot 0 of the newly allocated directory cluster
    return create_directory_entry(new_lba, 0, folder_name, parent_cluster);
}

bool fat32_remove(const char* name) {
    char* target_name = strrchr(name, '/');
    char* path = (char*) name;

    if (target_name == NULL) {
        target_name = (char*) name;
        path = "";
    } else {
        *target_name = '\0';
        target_name++;
    }

    uint32_t parent_cluster = disk_info->root_cluster;
    if (path[0] != '\0' && strcmp(path, "/") != 0)
        parent_cluster = find_cluster_for_path(path);

    if (parent_cluster == 0) return false;

    uint32_t cluster = parent_cluster;
    uint8_t dir_buf[512];

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint32_t first_lba = get_lba_from_cluster(cluster, disk_info);

        for (uint32_t s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(first_lba + s, dir_buf);
            FAT32File* entries = (FAT32File*) dir_buf;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00) return false; // Not found
                if (entries[i].name[0] == 0xE5) continue;

                if (compare_fat_names(entries[i].name, target_name)) {
                    uint32_t current_file_cluster = get_file_cluster(&entries[i]);

                    while (current_file_cluster >= 2 && current_file_cluster < 0x0FFFFFF8) {
                        uint32_t next_cluster = get_next_cluster(current_file_cluster);
                        update_fat_entry(current_file_cluster, 0x00000000); // Free cluster
                        current_file_cluster = next_cluster;
                    }

                    // Mark directory entry as deleted
                    entries[i].name[0] = 0xE5;
                    ata_write_sector(first_lba + s, dir_buf);

                    return true;
                }
            }
        }

        cluster = get_next_cluster(cluster);
    }

    return false;
}

File* fat32_get(const char* name) {
    uint32_t cluster = find_cluster_for_path(name);
    if (cluster == 0 && strcmp(name, "/") != 0) return NULL;

    char* last_slash_ptr = strrchr(name, '/');

    char path[256];
    char filename[256];

    if (last_slash_ptr == NULL) {
        strcpy(path, "/");
        strcpy(filename, name);
    } else {
        size_t path_len = last_slash_ptr - name;
        if (path_len == 0) {
            strcpy(path, "/");
        } else {
            strncpy(path, name, path_len);
            path[path_len] = '\0';
        }

        strcpy(filename, last_slash_ptr + 1);
    }

    uint32_t parent_cluster;
    if (strcmp(path, "/") == 0 || path[0] == '\0') {
        parent_cluster = disk_info->root_cluster;
    } else {
        parent_cluster = find_cluster_for_path(path);
    }

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
    if (!entry_found) return NULL;

    File* f = (File*) kmalloc(sizeof(File));
    kmemset(f, 0, sizeof(File));

    f->name = name;
    f->is_directory = (found_entry.attributes & 0x10);
    f->size = found_entry.size;

    if (f->is_directory) {
        f->data = NULL;
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

FileNode* fat32_getall(const char* path) {
    uint32_t dir_cluster = find_cluster_for_path(path);

    if (path[0] == '\0' || strcmp(path, "/") == 0) {
        dir_cluster = disk_info->root_cluster;
    } else if (dir_cluster == 0) {
        return NULL;
    }

    FileNode* head = NULL;
    uint32_t current_cluster = dir_cluster;
    uint8_t buffer[512];

    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_cluster, disk_info);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            ata_read_sector(lba + s, buffer);
            FAT32File* entries = (FAT32File*) buffer;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00) return head;
                if (entries[i].name[0] == 0xE5 || (entries[i].attributes & 0x08)) continue;
                if (entries[i].name[0] == '.') continue;

                FileNode* newNode = (FileNode*) kmalloc(sizeof(FileNode));
                if (!newNode) return head;
                kmemset(newNode, 0, sizeof(FileNode));

                newNode->file.size = entries[i].size;
                newNode->file.is_directory = (entries[i].attributes & 0x10);

                // We need 13 bytes max (8 + 1 dot + 3 ext + 1 null)
                char* formatted_name = (char*) kmalloc(13);
                int p = 0;

                // Copy Filename
                for (int k = 0; k < 8; k++) {
                    if (entries[i].name[k] != ' ') {
                        formatted_name[p++] = (char)entries[i].name[k];
                    }
                }

                // Copy Extension
                if (entries[i].name[8] != ' ') {
                    formatted_name[p++] = '.';
                    for (int k = 8; k < 11; k++) {
                        if (entries[i].name[k] != ' ') {
                            formatted_name[p++] = (char)entries[i].name[k];
                        }
                    }
                }
                formatted_name[p] = '\0';

                newNode->file.name = formatted_name;

                // Prepend to linked list
                newNode->next = head;
                head = newNode;
            }
        }

        current_cluster = get_next_cluster(current_cluster);
    }

    return head;
}
