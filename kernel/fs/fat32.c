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

#include "klib/string.h"

#include "drivers/storage/bdl.h"
#include "drivers/terminal.h"
#include "memory/heap.h"

#include "fs/fat32.h"
#include "fs/vfs.h"

// Reference: Microsoft FAT Specification, August 30 2005, 3.1 BPB structure common to FAT12, FAT16, and FAT32 implementations
#define FIXED_MEDIA_TYPE_VALUE     0xF8
#define REMOVABLE_MEDIA_TYPE_VALUE 0xF0

#define FAT32_ERROR_CODE 0

// Reference: https://wiki.osdev.org/FAT#Extended_Boot_Record
#define BOOT_SECTOR_SIG 0xAA55

#define ENTRIES_PER_SECTOR (512 / sizeof(fat32_file_t)) // bytes

typedef struct {
    uint8_t  boot_drive;
    uint8_t  start_chs[3];
    uint8_t  partition_type;
    uint8_t  end_chs[3];
    uint32_t start_lba;
    uint32_t sector_count;
} __attribute__((packed)) mbr_partition_t;

typedef struct {
    uint8_t  boot_code[446];
    mbr_partition_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) mbr_sector_t;

// Reference: Microsoft FAT Specification, August 30 2005, 3.1 BPB structure common to FAT12, FAT16, and FAT32 implementations
typedef struct {
    // Jump instructions and OEM Name
    uint8_t  jmp[3];
    char     oem_name[8];

    // Standard BPB (BIOS Parameter Block)
    uint16_t bytes_per_sector;      // Can only be 512, 1024, 2048, or 4096
    uint8_t  sectors_per_cluster;   // Can only be 1, 2, 4, 8, 16, 32, 64, or 128
    uint16_t reserved_sectors;      // Sectors before the first FAT
    uint8_t  num_fats;              // Almost always 2
    uint16_t root_entry_count;      // 0 for FAT32 (used in FAT12/16)
    uint16_t total_sectors_16;      // 0 for FAT32
    uint8_t  media_type;            // 0xF8 for fixed disks
    uint16_t fat_size_16;           // 0 for FAT32
    uint16_t sectors_per_track;     // Only relevant for media that have a geometry
    uint16_t num_heads;             // Number of heads for interrupt 0x13
    uint32_t hidden_sectors;        // Sectors before the partition
    uint32_t total_sectors_32;      // Total sectors in volume

    // 3.3 Extended BPB structure for FAT32 volumes
    uint32_t sectors_per_fat;       // Size of one FAT table
    uint16_t ext_flags;             // Mirroring info
    uint16_t fs_version;            // Should be 0:0
    uint32_t root_cluster;          // Cluster where / starts (usually 2)
    uint16_t fs_info_sector;        // Sector number of FSINFO (usually 1)
    uint16_t backup_boot_sector;    // Sector of boot copy (usually 6)
    uint8_t  reserved[12];          // Should be 0
    uint8_t  drive_number;          // 0x80 for hard disks
    uint8_t  reserved1;             // For Windows NT
    uint8_t  boot_signature;        // 0x29
    uint32_t volume_id;             // Serial number
    char     volume_label[11];      // "NO NAME    "
    char     fs_type[8];            // "FAT32   "

    // Boot Code and Signature
    uint8_t  boot_code[420];        // The rest of the sector
    uint16_t boot_sector_sig;       // 0xAA55 (Little Endian)
} __attribute__((packed)) fat32_bpb_t;

// Reference: https://wiki.osdev.org/FAT
typedef struct {
    uint8_t  boot_jmp[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // FAT32 Extended Section
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;      // Files' entry point
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;

    // 12 byte padding is to be here, so we steal it
    uint32_t data_lba;              // Start of Cluster 2
    uint32_t total_clusters;        // Max cluster limit
    uint32_t partition_start_lba;

    uint8_t  drive_num;
    uint8_t  reserved1;
    uint8_t  boot_sig;          // 0x28 or 0x29
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];        // Should say "FAT32   " (hopefully)
} __attribute__((packed)) fat32_header_t;

// Reference: https://wiki.osdev.org/FAT
typedef struct {
    uint8_t  name[11];      // 8 chars name, 3 extension
    uint8_t  attributes;    // If folder, read-only, etc.
    uint8_t  reservedNT;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t cluster_high;  // Top 16 bits of the address
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;   // Bottom 16 bits of the address
    uint32_t size;          // File size in bytes
} __attribute__((packed)) fat32_file_t;

static fat32_header_t* disk_info = NULL;

VFS fat32_vfs = {
    .name = "FAT32",

    .read   = fat32_read,
    .write  = fat32_write,
    .create = fat32_create,
    .mkdir  = fat32_mkdir,
    .remove = fat32_remove,
    .get    = fat32_get,
    .getall = fat32_getall,

    .check_write_safety = fat32_check_write_safety
};

/* Get the LBA the given cluster is at */
static inline uint32_t get_lba_from_cluster(uint32_t cluster) {
    return disk_info->partition_start_lba + disk_info->data_lba + ((cluster - 2) * disk_info->sectors_per_cluster);
}

/* Get the cluster the given file is at */
static inline uint32_t get_file_cluster(fat32_file_t* file) {
    return (uint32_t) file->cluster_low | ((uint32_t) file->cluster_high << 16);
}

/* Get the next cluster of the current cluster in the linker list */
static uint32_t get_next_cluster(uint32_t cluster) {
    // Check if the cluster index is physically possible
    if (unlikely(cluster < 2 || cluster >= disk_info->total_clusters + 2)) {
        err_printf("get_next_cluster: Attempted to fetch next cluster for invalid index %d", cluster);
        return FAT32_ERROR_CODE;
    }

    uint32_t fat_sector = disk_info->reserved_sectors + (cluster / 128);
    uint32_t buffer[128];
    bdl_read(disk_info->partition_start_lba + fat_sector, buffer);

    uint32_t next = buffer[cluster % 128] & 0x0FFFFFFF;

    // Detect "Zero Link" corruption (a used file pointing to cluster 0)
    if (unlikely(next == 0)) {
        err_print("get_next_cluster: Corruption detected; file points to cluster 0.");
        return FAT32_ERROR_CODE;
    }

    return next;
}

/* Iterate through the FAT to find the first free entry */
static uint32_t find_free_fat_entry() {
    uint32_t fat_buffer[128];

    for (uint32_t s = 0; s < disk_info->sectors_per_fat; s++) {
        bdl_read(disk_info->partition_start_lba + disk_info->reserved_sectors + s, fat_buffer);

        for (int i = 0; i < 128; i++) {
            if (unlikely(s == 0 && i < 2)) continue; // Skip reserved entries

            if (unlikely((fat_buffer[i] & 0x0FFFFFFF) == 0)) {
                uint32_t cluster = (s * 128) + i;

                // Safety check against volume size
                if (unlikely(cluster >= disk_info->total_clusters + 2)) return FAT32_ERROR_CODE;

                return cluster;
            }
        }
    }

    return FAT32_ERROR_CODE; // Disk full
}

/* Format to 8 char for name and 3 for extension */
static void format_to_83(const char* name, uint8_t* out_name) {
    memset(out_name, ' ', 11);

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

/* Check if fat_name == user_name after truncating to 8 char.3 char */
static inline bool compare_fat_names(uint8_t* fat_name, const char* user_name) {
    uint8_t formatted[11];
    format_to_83(user_name, formatted);
    return memcmp(fat_name, formatted, 11) == 0;
}

/* Update a FAT32 cluster entry and mirror the write to backup FATs if mirroring is enabled. */
static void update_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t fat_buffer[128];
    uint32_t lba = disk_info->reserved_sectors + (cluster / 128);

    bdl_read(disk_info->partition_start_lba + lba, fat_buffer);
    fat_buffer[cluster % 128] = value;
    bdl_write(disk_info->partition_start_lba + lba, fat_buffer);

    // Check if bit 7 is 0 (Mirroring Enabled)
    if (!(disk_info->flags & 0x80)) {
        // Loop starts at 1 because we already wrote to FAT 0
        for (uint32_t i = 1; i < disk_info->fat_count; i++) {
            uint32_t backup_lba = lba + (i * disk_info->sectors_per_fat);
            bdl_write(disk_info->partition_start_lba + backup_lba, fat_buffer);
        }
    }
}

/* Search a directory's cluster chain for an entry by name and return its starting cluster. */
static uint32_t find_entry_in_cluster(uint32_t directory_cluster, const char* name) {
    uint32_t current_cluster = directory_cluster;
    fat32_file_t entries[ENTRIES_PER_SECTOR];

    // Follow the cluster chain of the directory
    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_cluster);

        // A cluster can have multiple sectors
        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(lba + s, entries);

            for (int i = 0; i < 16; i++) {
                // 0x00 means end of directory, stop searching
                if (unlikely(entries[i].name[0] == 0x00)) return FAT32_ERROR_CODE;
                // 0xE5 is a deleted file, skip it
                if (unlikely(entries[i].name[0] == 0xE5)) continue;

                if (compare_fat_names(entries[i].name, name)) {
                    return get_file_cluster(&entries[i]);
                }
            }
        }

        // Move to the next cluster in the directory's chain
        current_cluster = get_next_cluster(current_cluster);
    }

    return FAT32_ERROR_CODE; // Not found
}

/* Helper to find the starting cluster of a directory/file path */
static uint32_t find_cluster_for_path(const char* path) {
    uint32_t current_cluster = disk_info->root_cluster;
    const char* start = path;

    if (*start == '/') {
        start++;
    }

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
        memcpy(segment, start, len);
        segment[len] = '\0';

        if (len > 0) {
            uint32_t next = find_entry_in_cluster(current_cluster, segment);
            if (unlikely(next == FAT32_ERROR_CODE)) return FAT32_ERROR_CODE; // Folder/File not found
            current_cluster = next;
        }

        if (unlikely(end == NULL)) break;
        start = end + 1;
    }

    return current_cluster;
}

/* Create a new sub-directory entry, allocate its cluster, and initialize its '.' and '..' links. */
static bool create_directory_entry(uint32_t sector_lba, int index, const char* name, uint32_t parent_cluster) {
    fat32_file_t entries[ENTRIES_PER_SECTOR];
    bdl_read(sector_lba, entries);

    uint32_t new_cluster = find_free_fat_entry();
    if (unlikely(new_cluster == FAT32_ERROR_CODE)) return false;

    format_to_83(name, (uint8_t*) entries[index].name);

    entries[index].attributes   = 0x10;
    entries[index].cluster_low  = (uint16_t)(new_cluster & 0xFFFF);
    entries[index].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);
    entries[index].size = 0;

    bdl_write(sector_lba, entries);
    update_fat_entry(new_cluster, 0x0FFFFFFF);

    // Initialize "." and ".."
    uint8_t folder_data[512] = {0};
    fat32_file_t* dot_entries = (fat32_file_t*) folder_data;

    memcpy(dot_entries[0].name, ".          ", 11);
    dot_entries[0].attributes   = 0x10;
    dot_entries[0].cluster_low  = (uint16_t)(new_cluster & 0xFFFF);
    dot_entries[0].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);

    memcpy(dot_entries[1].name, "..         ", 11);
    dot_entries[1].attributes   = 0x10;
    dot_entries[1].cluster_low  = (uint16_t)(parent_cluster & 0xFFFF);
    dot_entries[1].cluster_high = (uint16_t)((parent_cluster >> 16) & 0xFFFF);

    uint32_t folder_lba = get_lba_from_cluster(new_cluster);

    bdl_write(folder_lba, folder_data);

    return true;
}

/* Finds the folder (path) and the file's name and writes to the pointers */
static void write_path_filename(const char* full_path, char** out_path, size_t* path_len, const char** filename) {
    char* last_slash = strrchr(full_path, '/');

    if (last_slash == NULL) {
        (*out_path)[0] = '\0';
        *path_len = 0;
        *filename = full_path;
    } else {
        *path_len = (size_t)(last_slash - full_path);

        strncpy(*out_path, full_path, *path_len);
        (*out_path)[*path_len] = '\0';

        *filename = last_slash + 1;
    }
}

/*
Reads the MBR to find the BPB, from which, we initialise the FAT32 system, after
a set of checks. We then cache the data into `disk_info` to use.
*/
void init_fat32() {
    mbr_sector_t mbr;
    bdl_read(0, &mbr);

    uint32_t partition_base = mbr.partitions[0].start_lba;

    fat32_bpb_t bpb;
    bdl_read(partition_base, &bpb);

    // Verify boot signature
    if (unlikely(bpb.boot_sector_sig != BOOT_SECTOR_SIG)) {
        err_printf("init_fat32: Invalid boot signature: %d", bpb.boot_sector_sig);
        return;
    }

    // Verify Sectors Per Cluster is a power of 2
    else if (unlikely(bpb.sectors_per_cluster == 0 || (bpb.sectors_per_cluster & (bpb.sectors_per_cluster - 1)) != 0)) {
        err_printf("init_fat32: Invalid sectors per cluster: %d", bpb.sectors_per_cluster);
        return;
    }

    // For FAT32, fat_size_16 must be 0
    else if (unlikely(bpb.fat_size_16 != 0)) {
        err_printf("init_fat32: Not a FAT32 volume (FAT12/16 detected): %d", bpb.fat_size_16);
        return;
    }

    // For fat32, root_entry_count must be 0
    else if (unlikely(bpb.root_entry_count != 0)) {
        err_printf("init_fat32: Invalid root entry count for FAT32: %d", bpb.root_entry_count);
        return;
    }

    // If the boot sector says there are 0 sectors
    else if (unlikely(bpb.total_sectors_32 == 0)) {
        err_print("init_fat32: Volume reports 0 sectors");
        return;
    }

    // If the sectors per fat is reported to be 0
    else if (unlikely(bpb.sectors_per_fat == 0)) {
        err_print("init_fat32: FAT size is 0");
        return;
    }

    // FAT32 volume version number must be 0x0
    else if (unlikely(bpb.fs_version != 0x0)) {
        err_printf("init_fat32: Version no 0x0, rather: %d", bpb.fs_version);
        return;
    }

    disk_info = (fat32_header_t*) kmalloc(sizeof(fat32_header_t));

    if (unlikely(!disk_info)) {
        err_print("init_fat32: Out of memory");
        return;
    }

    memcpy(disk_info->boot_jmp, bpb.jmp, 3);
    memcpy(disk_info->oem_name, bpb.oem_name, 8);

    disk_info->bytes_per_sector    = bpb.bytes_per_sector;
    disk_info->sectors_per_cluster = bpb.sectors_per_cluster;
    disk_info->reserved_sectors    = bpb.reserved_sectors;
    disk_info->fat_count           = bpb.num_fats;
    disk_info->media_type          = bpb.media_type;
    disk_info->sectors_per_track   = bpb.sectors_per_track;
    disk_info->head_count          = bpb.num_heads;
    disk_info->hidden_sectors      = bpb.hidden_sectors;
    disk_info->total_sectors_32    = bpb.total_sectors_32;

    disk_info->sectors_per_fat     = bpb.sectors_per_fat;
    disk_info->flags               = bpb.ext_flags;
    disk_info->version             = bpb.fs_version;
    disk_info->root_cluster        = bpb.root_cluster;
    disk_info->fs_info_sector      = bpb.fs_info_sector;
    disk_info->backup_boot_sector  = bpb.backup_boot_sector;

    disk_info->drive_num = bpb.drive_number;
    disk_info->reserved1 = bpb.reserved1;
    disk_info->boot_sig  = bpb.boot_signature;
    disk_info->volume_id = bpb.volume_id;

    memcpy(disk_info->fs_type, bpb.fs_type, 8);
    memcpy(disk_info->volume_label, bpb.volume_label, 11);
    disk_info->volume_label[11] = '\0';

    // Legacy FAT12/FAT16
    disk_info->root_dir_entries = 0;
    disk_info->total_sectors_16 = 0;
    disk_info->fat_size_16      = 0;

    // Pre-calculated values
    uint32_t relative_data_area = bpb.reserved_sectors + (bpb.num_fats * bpb.sectors_per_fat);
    disk_info->data_lba = partition_base + relative_data_area;
    disk_info->total_clusters = (bpb.total_sectors_32 - relative_data_area) / bpb.sectors_per_cluster;
    disk_info->partition_start_lba = partition_base;
}

/* Read a file and write the data into the buffer, reading from offset to offset+size */
int fat32_read(const char* name, void* buffer, size_t size, uint32_t offset) {
    char* path;
    size_t path_len;
    const char* filename;

    write_path_filename(name, &path, &path_len, &filename);

    uint32_t parent_cluster = disk_info->root_cluster;

    if (path[0] != '\0') {
        parent_cluster = find_cluster_for_path(path);

        if (unlikely(parent_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_read: Parent cluster not found");
            return FAT32_ERROR_CODE;
        }
    }

    static uint8_t sector_buffer[512];
    uint32_t current_dir_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_dir_cluster);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(lba + s, sector_buffer);

            fat32_file_t* files = (fat32_file_t*) sector_buffer;

            for (int i = 0; i < 16; i++) {
                if (unlikely(files[i].name[0] == 0x00)) return FAT32_ERROR_CODE;
                if (unlikely(files[i].name[0] == 0xE5 || files[i].attributes == 0x0F)) continue;

                if (compare_fat_names(files[i].name, filename)) {
                    uint32_t current_cluster = get_file_cluster(&files[i]);
                    uint32_t file_size       = files[i].size;

                    if (unlikely(offset >= file_size)) return 0;
                    if (offset + size > file_size) size = file_size - offset;

                    // Fast-forward to the cluster where 'offset' lives
                    uint32_t clusters_to_skip = offset / (disk_info->sectors_per_cluster << 9);
                    for (uint32_t i = 0; i < clusters_to_skip; i++) {
                        current_cluster = get_next_cluster(current_cluster);

                        if (unlikely(current_cluster >= 0x0FFFFFF8 || current_cluster == FAT32_ERROR_CODE)) {
                            err_printf("fat32_read: Could not get next cluster while skipping, tried %x", current_cluster);
                            return FAT32_ERROR_CODE;
                        }
                    }

                    // Calculate internal offsets
                    uint32_t cluster_offset = offset % (disk_info->sectors_per_cluster << 9);
                    uint32_t sector_in_cluster = cluster_offset >> 9;
                    uint32_t byte_in_sector = cluster_offset % 512;

                    uint32_t bytes_read = 0;

                    while (bytes_read < size && current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                        uint32_t lba = get_lba_from_cluster(current_cluster);

                        // Start from sector_in_cluster on the first cluster, then 0 for others
                        for (int sec = sector_in_cluster; sec < disk_info->sectors_per_cluster && bytes_read < size; sec++) {
                            bdl_read(lba + sec, sector_buffer);

                            // How much can we take from this sector
                            uint32_t available = 512 - byte_in_sector;
                            uint32_t to_copy = (size - bytes_read > available) ? available : (size - bytes_read);

                            memcpy((uint8_t*) buffer + bytes_read, sector_buffer + byte_in_sector, to_copy);

                            bytes_read += to_copy;
                            byte_in_sector = 0; // After the first partial read, we start at byte 0
                        }

                        sector_in_cluster = 0; // After the first partial cluster, we start at sector 0
                        current_cluster = get_next_cluster(current_cluster);

                        if (unlikely(current_cluster == FAT32_ERROR_CODE)) {
                            err_print("fat32_read: Could not get next cluster");
                            return FAT32_ERROR_CODE;
                        }
                    }

                    return bytes_read;
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);

        if (unlikely(current_dir_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_read: Could not get next directory cluster");
            return FAT32_ERROR_CODE;
        }
    }

    return FAT32_ERROR_CODE;
}

/* Write to the given file from offset to offset+size */
int fat32_write(const char* name, const void* buffer, size_t size, uint32_t offset) {
    char* path;
    size_t path_len;
    const char* filename;

    write_path_filename(name, &path, &path_len, &filename);

    uint32_t parent_cluster = disk_info->root_cluster;
    if (path[0] != '\0' && strcmp(path, "/") != 0) {
        parent_cluster = find_cluster_for_path(path);

        if (unlikely(parent_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_write: Parent cluster not found");
            return FAT32_ERROR_CODE;
        }
    }

    uint8_t  dir_buf[512];
    uint32_t current_dir_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_dir_cluster);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(lba + s, dir_buf);

            fat32_file_t* entries = (fat32_file_t*) dir_buf;

            for (int i = 0; i < 16; i++) {
                if (unlikely(entries[i].name[0] == 0x00)) return FAT32_ERROR_CODE;  // End of directory
                if (unlikely(entries[i].name[0] == 0xE5 || entries[i].attributes == 0x0F)) continue;

                if (compare_fat_names(entries[i].name, filename)) {
                    uint32_t current_cluster = get_file_cluster(&entries[i]);

                    uint32_t bytes_per_cluster = disk_info->sectors_per_cluster << 9;
                    uint32_t skip = offset / bytes_per_cluster;
                    for (uint32_t j = 0; j < skip; j++) {
                        current_cluster = get_next_cluster(current_cluster);

                        if (unlikely(current_cluster >= 0x0FFFFFF8 || current_cluster == FAT32_ERROR_CODE)) {
                            err_printf("fat32_write: Could not get next cluster while skipping, tried %x", current_cluster);
                            return FAT32_ERROR_CODE;
                        }
                    }

                    // Calculate starting points within that cluster
                    uint32_t cluster_off = offset % bytes_per_cluster;
                    uint32_t sector_in_cluster = cluster_off >> 9;
                    uint32_t byte_in_sector = cluster_off % 512;

                    uint32_t bytes_written = 0;
                    const uint8_t* write_ptr = (const uint8_t*) buffer;

                    while (bytes_written < size && current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                        uint32_t data_lba = get_lba_from_cluster(current_cluster);

                        for (int sec = sector_in_cluster; sec < disk_info->sectors_per_cluster && bytes_written < size; sec++) {
                            uint8_t temp_block[512];

                            // Read current sector to preserve data we aren't overwriting
                            bdl_read(data_lba + sec, temp_block);

                            uint32_t available = 512 - byte_in_sector;
                            uint32_t to_write = (size - bytes_written > available) ? available : (size - bytes_written);

                            // Copy new data into the specific offset of the sector buffer
                            memcpy(temp_block + byte_in_sector, write_ptr + bytes_written, to_write);

                            bdl_write(data_lba + sec, temp_block);

                            bytes_written += to_write;
                            byte_in_sector = 0; // After first sector, we start at byte 0
                        }

                        sector_in_cluster = 0; // After first cluster, we start at sector 0

                        if (bytes_written < size) {
                            current_cluster = get_next_cluster(current_cluster);

                            if (unlikely(current_cluster == FAT32_ERROR_CODE)) {
                                err_print("fat32_write: Could not get next cluster");
                                return FAT32_ERROR_CODE;
                            }
                        }
                    }

                    // Update directory entry size ONLY if file grew
                    if (offset + size > entries[i].size) {
                        entries[i].size = offset + size;
                        bdl_write(lba + s, dir_buf);
                    }

                    return bytes_written;
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);

        if (unlikely(current_dir_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_write: Could not get next directory cluster");
            return FAT32_ERROR_CODE;
        }
    }

    return FAT32_ERROR_CODE;
}

/* Create a new file at given path */
int fat32_create(const char* path) {
    char* dir_path;
    size_t path_len;
    const char* filename;

    write_path_filename(path, &dir_path, &path_len, &filename);

    uint32_t parent_cluster = disk_info->root_cluster;
    if (dir_path[0] != '\0' && strcmp(dir_path, "/") != 0) {
        parent_cluster = find_cluster_for_path(dir_path);

        if (unlikely(parent_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_create: Parent cluster not found");
            return FAT32_ERROR_CODE;
        }
    }

    uint8_t  buffer[512];
    uint32_t current_dir_cluster = parent_cluster;
    uint32_t last_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        last_cluster = current_dir_cluster;
        uint32_t lba = get_lba_from_cluster(current_dir_cluster);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(lba + s, buffer);
            fat32_file_t* entries = (fat32_file_t*) buffer;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
                    uint32_t file_cluster = find_free_fat_entry();
                    if (unlikely(file_cluster == FAT32_ERROR_CODE)) return 0;

                    format_to_83(filename, (uint8_t*)entries[i].name);
                    entries[i].attributes = 0x20; // Archive attribute
                    entries[i].cluster_low  = (uint16_t)(file_cluster & 0xFFFF);
                    entries[i].cluster_high = (uint16_t)((file_cluster >> 16) & 0xFFFF);
                    entries[i].size = 0;

                    bdl_write(lba + s, buffer);
                    update_fat_entry(file_cluster, 0x0FFFFFFF); // Mark EOF in FAT

                    return 1;
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);

        if (unlikely(current_dir_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_create: Could not get next directory cluster");
            return FAT32_ERROR_CODE;
        }
    }

    // If we reach here, the directory is full, so we grow it.
    uint32_t new_cluster = find_free_fat_entry();
    if (unlikely(new_cluster == FAT32_ERROR_CODE)) return 0;

    update_fat_entry(last_cluster, new_cluster);
    update_fat_entry(new_cluster, 0x0FFFFFFF);

    // Zero out the new directory cluster
    uint32_t new_lba = get_lba_from_cluster(new_cluster);
    uint8_t zero_block[512] = {0};
    for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
        bdl_write(new_lba + s, zero_block);
    }

    // Place the new file entry in the first slot of the new cluster
    uint32_t file_cluster = find_free_fat_entry();
    if (unlikely(file_cluster == FAT32_ERROR_CODE)) return 0;

    // Use the first slot [0] of the freshly zeroed cluster
    fat32_file_t new_entry = {0};
    format_to_83(filename, (uint8_t*)new_entry.name);
    new_entry.attributes = 0x20;
    new_entry.cluster_low  = (uint16_t)(file_cluster & 0xFFFF);
    new_entry.cluster_high = (uint16_t)((file_cluster >> 16) & 0xFFFF);
    new_entry.size = 0;

    // Write the new entry to the start of the new cluster
    memcpy(buffer, &new_entry, sizeof(fat32_file_t));
    bdl_write(new_lba, buffer);
    update_fat_entry(file_cluster, 0x0FFFFFFF);

    return 1;
}

/* Create a new folder at the given path */
int fat32_mkdir(const char* path) {
    char* parent_path;
    size_t path_len;
    const char* folder_name;

    write_path_filename(path, &parent_path, &path_len, &folder_name);

    uint32_t parent_cluster = disk_info->root_cluster;
    if (parent_path[0] != '\0' && strcmp(parent_path, "/") != 0) {
        parent_cluster = find_cluster_for_path(parent_path);

        if (unlikely(parent_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_mkdir: Parent cluster not found");
            return FAT32_ERROR_CODE;
        }
    }

    uint8_t  buffer[512];
    uint32_t current_dir_cluster = parent_cluster;
    uint32_t last_cluster = parent_cluster;

    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        last_cluster = current_dir_cluster;
        uint32_t lba = get_lba_from_cluster(current_dir_cluster);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(lba + s, buffer);
            fat32_file_t* entries = (fat32_file_t*) buffer;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) {
                    return create_directory_entry(lba + s, i, folder_name, parent_cluster);
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);

        if (unlikely(current_dir_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_mkdir: Could not get next directory cluster");
            return FAT32_ERROR_CODE;
        }
    }

    uint32_t new_parent_ext_cluster = find_free_fat_entry();
    if (unlikely(new_parent_ext_cluster == FAT32_ERROR_CODE)) return 0;

    update_fat_entry(last_cluster, new_parent_ext_cluster);
    update_fat_entry(new_parent_ext_cluster, 0x0FFFFFFF);

    uint32_t new_lba = get_lba_from_cluster(new_parent_ext_cluster);
    uint8_t zero_block[512] = {0};

    // Initialize the new sector with zeros
    for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
        bdl_write(new_lba + s, zero_block);
    }

    // Use slot 0 of the newly allocated directory cluster
    return create_directory_entry(new_lba, 0, folder_name, parent_cluster);
}

/* Delete whatever was at the given absolute name */
int fat32_remove(const char* name) {
    const char* path;
    size_t path_len;
    const char* target_name;

    write_path_filename(name, &path, &path_len, &target_name);

    uint32_t parent_cluster = disk_info->root_cluster;
    if (path[0] != '\0' && strcmp(path, "/") != 0) {
        parent_cluster = find_cluster_for_path(path);

        if (unlikely(parent_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_remove: Parent cluster not found");
            return FAT32_ERROR_CODE;
        }
    }

    uint32_t cluster = parent_cluster;
    uint8_t  dir_buf[512];

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint32_t first_lba = get_lba_from_cluster(cluster);

        for (uint32_t s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(first_lba + s, dir_buf);
            fat32_file_t* entries = (fat32_file_t*) dir_buf;

            for (int i = 0; i < 16; i++) {
                if (unlikely(entries[i].name[0] == 0x00)) return 0; // Not found
                if (unlikely(entries[i].name[0] == 0xE5)) continue;

                if (compare_fat_names(entries[i].name, target_name)) {
                    uint32_t current_file_cluster = get_file_cluster(&entries[i]);

                    while (current_file_cluster >= 2 && current_file_cluster < 0x0FFFFFF8) {
                        uint32_t next_cluster = get_next_cluster(current_file_cluster);

                        if (unlikely(next_cluster == FAT32_ERROR_CODE)) {
                            err_print("fat32_remove: Could not get next cluster");
                            return FAT32_ERROR_CODE;
                        }

                        update_fat_entry(current_file_cluster, 0x00000000); // Free cluster
                        current_file_cluster = next_cluster;
                    }

                    // Mark directory entry as deleted
                    entries[i].name[0] = 0xE5;
                    bdl_write(first_lba + s, dir_buf);

                    return 1;
                }
            }
        }

        cluster = get_next_cluster(cluster);

        if (unlikely(cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_remove: Could not get cluster");
            return FAT32_ERROR_CODE;
        }
    }

    return 0;
}

/* Get the file object at the given absolute name */
File* fat32_get(const char* name) {
    uint32_t cluster = find_cluster_for_path(name);
    if (unlikely(cluster == 0 && strcmp(name, "/") != 0)) return NULL;

    char* path;
    size_t path_len;
    const char* filename;

    write_path_filename(name, &path, &path_len, &filename);

    uint32_t parent_cluster = disk_info->root_cluster;
    if (path[0] != '\0' && strcmp(path, "/") != 0) {
        parent_cluster = find_cluster_for_path(path);

        if (unlikely(parent_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_get: Parent cluster not found");
            return NULL;
        }
    }

    uint8_t sector_buf[512];
    fat32_file_t found_entry;
    bool entry_found = false;

    uint32_t current_dir_cluster = parent_cluster;
    while (current_dir_cluster >= 2 && current_dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_dir_cluster);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(lba + s, sector_buf);
            fat32_file_t* entries = (fat32_file_t*) sector_buf;
            for (int i = 0; i < 16; i++) {
                if (unlikely(entries[i].name[0] == 0x00)) goto search_done;
                if (unlikely(entries[i].name[0] == 0xE5)) continue;

                if (compare_fat_names(entries[i].name, filename)) {
                    memcpy(&found_entry, &entries[i], sizeof(fat32_file_t));
                    entry_found = true;

                    goto search_done;
                }
            }
        }

        current_dir_cluster = get_next_cluster(current_dir_cluster);

        if (unlikely(current_dir_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_get: Could not get next directory cluster");
            return NULL;
        }
    }

search_done:
    if (unlikely(!entry_found)) return NULL;

    File* f = (File*) kmalloc(sizeof(File));
    memset(f, 0, sizeof(File));

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
            uint32_t start_lba = get_lba_from_cluster(current_cluster);

            for (int s = 0; s < disk_info->sectors_per_cluster && bytes_read < f->size; s++) {
                uint32_t remaining = f->size - bytes_read;
                if (remaining >= 512) {
                    bdl_read(start_lba + s, (uint8_t*)(f->data + bytes_read));
                    bytes_read += 512;
                } else {
                    uint8_t bounce[512];
                    bdl_read(start_lba + s, bounce);
                    memcpy((uint8_t*)(f->data + bytes_read), bounce, remaining);
                    bytes_read += remaining;
                }
            }

            current_cluster = get_next_cluster(current_cluster);

            if (unlikely(current_cluster == FAT32_ERROR_CODE)) {
                err_print("fat32_get: Could not get next cluster");
                return NULL;
            }
        }
    }

    return f;
}

/* Get a linked list of all the contents of a given directory */
FileNode* fat32_getall(const char* path) {
    uint32_t parent_cluster = disk_info->root_cluster;
    if (path[0] != '\0' && strcmp(path, "/") != 0) {
        parent_cluster = find_cluster_for_path(path);

        if (unlikely(parent_cluster == FAT32_ERROR_CODE)) {
            err_print("fat32_getall: Parent cluster not found");
            return NULL;
        }
    }

    FileNode* head = NULL;
    uint32_t current_cluster = parent_cluster;
    uint8_t buffer[512];

    while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t lba = get_lba_from_cluster(current_cluster);

        for (int s = 0; s < disk_info->sectors_per_cluster; s++) {
            bdl_read(lba + s, buffer);

            fat32_file_t* entries = (fat32_file_t*) buffer;

            for (int i = 0; i < 16; i++) {
                if (unlikely(entries[i].name[0] == 0x00)) return head;
                if (unlikely(entries[i].name[0] == 0xE5 ||
                            (entries[i].attributes & 0x08) ||
                             entries[i].name[0] == '.')) continue;

                FileNode* newNode = (FileNode*) kmalloc(sizeof(FileNode));
                if (unlikely(!newNode)) return head;
                memset(newNode, 0, sizeof(FileNode));

                newNode->file.size = entries[i].size;
                newNode->file.is_directory = (entries[i].attributes & 0x10);

                // We need 13 bytes max (8 + 1 dot + 3 ext + 1 null)
                char* formatted_name = (char*) kmalloc(13);
                int p = 0;

                // Copy Filename
                for (int k = 0; k < 8; k++) {
                    if (entries[i].name[k] != ' ') {
                        formatted_name[p++] = (char) entries[i].name[k];
                    }
                }

                // Copy Extension
                if (entries[i].name[8] != ' ') {
                    formatted_name[p++] = '.';
                    for (int k = 8; k < 11; k++) {
                        if (entries[i].name[k] != ' ') {
                            formatted_name[p++] = (char) entries[i].name[k];
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

/* Prevent writes to protected LBAs */
int fat32_check_write_safety(uint32_t lba) {
    if (unlikely(!disk_info)) return 1;

    // Never touch the MBR or other partitions
    if (unlikely(lba < disk_info->partition_start_lba)) return 2;

    // Never fly off the edge of the partition
    if (unlikely(lba >= (disk_info->partition_start_lba + disk_info->total_sectors_32))) return 3;

    // Disallow writes to reserved sectors
    if (unlikely(lba < disk_info->reserved_sectors)) return 4;

    return 0;
}
