All files related to file systems will go here.

# VFS (Virtual File System)

Provides an abstraction layer between the specific file systems and the rest of the kernel, so that the kernel doesn't need to know which exact file system to use, and leave that upto the VFS to manage.

```c
void vfs_mount(FileOperations* ops);
```

This function takes in a `FileOperations` object, which is defined by each file system, and is just a blueprint of each function. VFS then uses the functions defined here.

```c
int       fs_read   (const char* name, void* buffer, size_t size);
int       fs_write  (const char* name, const void* buffer, size_t size);
bool      fs_create (const char* name);
bool      fs_mkdir  (const char* name);
bool      fs_remove (const char* name);
File*     fs_get    (const char* name);
FileNode* fs_getall (const char* path);
```

These functions are abstractions from the rest of the file system functions. These functions can be ramdisk's if we mounted it, or fat32's, if we mounted it, etc. Note that functions, like read, do not actually return the literal text there, but just set it to the buffer instead.

There is a unique FileNode struct, which is just a linked list of files, which is useful for iterate through.

# RAM Disk

This is the simplest thing to make: it's just a hash-map of *pseudo*-files in memory itself. The files and folders really don't exist, but is useful and fast, occasionally. The specific hash used is the djb2 hash, for accuracy and speed.

```c
void init_ramdisk();
```

Sets the RAM disk hash-map to NULL pointers.

```c
int ramdisk_read(const char* name, void* buffer, size_t size);
```

Just reads the file at the hash-map and read the data stored at it, and saves it to the buffer.

```c
int ramdisk_write(const char* name, const void* buffer, size_t size);
```

Writes the content of buffer into the file. It first finds the hash-map, if data was already there, we free it from memory to write to it, then just mempcy the data in.

```c
bool ramdisk_create(const char* name);
bool ramdisk_mkdir(const char* name);
```

These functions work the exact same, except they are a single value different: the directory value is set to `true` or `false` depending on the function. We just malloc the space for a new space, and just set the values.

```c
bool ramdisk_remove(const char* name);
```

Just frees the file from memory, effectively deleting it, as well as removing it from the hash-map.

```c
File* ramdisk_get(const char* name);
FileNode* ramdisk_getall(const char* path);
```

Returns the hash-map is the specific required way. Unfortunately, these were for efficiency, but are annoying with hash-maps.

# File Allocation Table 32

This was the most annoying thing, but is very reliable, and is set-in-stone. As far as I can tell, this has stood the test of tests, and works perfectly (hopefully).

FAT32 is a persistent file system that relies on a "File Allocation Table" to keep track of how files are scattered across the disk. Instead of files being one big contiguous block, they are broken into **clusters**. These static functions handle the low-level math and disk I/O needed to traverse these clusters and manage file metadata. Since the disk is addressed in sectors (LBA) but FAT32 thinks in clusters, we need translation layers to find where data actually lives.

```c
static uint32_t get_lba_from_cluster(uint32_t cluster, FAT32Header* header);
```

Converts a FAT32 cluster number into a physical sector address on the disk. It skips over the reserved sectors and the FAT tables to find the start of the data area.

```c
static uint32_t get_file_cluster(FAT32File* file);
```

FAT32 splits the starting cluster address of a file into two 16-bit fields (High and Low) in the directory entry. This helper stitches them back together into a single 32-bit address. The File Allocation Table itself is like a linked list where each entry tells you where the next part of the file is.

```c
static uint32_t get_next_cluster(uint32_t cluster);
```

Looks into the FAT table on the disk to find the "pointer" to the next cluster in a chain. If it returns an "End of Chain" marker, the file is finished.

```c
static uint32_t find_free_fat_entry();
```

Scans the FAT table for a zeroed entry. This is how the kernel finds empty space on the disk to store new data.

```c
static void update_fat_entry(uint32_t cluster, uint32_t value);
```

Writes a new value into the FAT table. This is used when extending a file or marking a new cluster as the "End of Chain." FAT32 uses the classic **8.3 filename** format (8 characters for the name, 3 for the extension), which requires some formatting before we can compare what the user typed with what is on the disk.

```c
static void format_to_83(const char* name, uint8_t* out_name);
```

Converts a standard string (like "test.txt") into the padded, uppercase 11-byte format FAT32 expects ("TEST    TXT").

```c
static bool compare_fat_names(uint8_t* fat_name, const char* user_name);
```

A helper that formats a user-provided string and checks if it matches a raw 11-byte name found in a directory entry.

```c
static uint32_t find_entry_in_cluster(uint32_t directory_cluster, const char* name);
```

Scans all directory entries within a specific cluster to see if a file or folder exists there. It handles jumping across multiple clusters if a directory is very large.

```c
static uint32_t find_cluster_for_path(const char* path);
```

The "path-walker." It breaks a path like `/home/user/file.txt` into segments and iteratively searches each directory level until it finds the starting cluster of the final target.

```c
static bool create_directory_entry(uint32_t sector_lba, int index, const char* name, uint32_t parent_cluster);
```

Writes a new folder entry to the disk. It allocates a fresh cluster for the new folder and automatically initializes the `.` (current) and `..` (parent) entries so the directory structure remains valid and traversable.

```c
void init_fat32();
```

Reads the very first sector of the disk to find the **BPB (BIOS Parameter Block)**. It verifies the boot signature (`0x55AA`) and copies the header into a global `disk_info` structure, which defines the cluster sizes and FAT locations needed for all subsequent IO.

```c
int fat32_read(const char* name, void* buffer, size_t size);
```

Finds a file by walking its path, then follows its cluster chain to stream data into the provided buffer. It handles the sector-to-cluster math internally and stops once it reaches the requested size or the actual file end.

```c
int fat32_write(const char* name, const void* buffer, size_t size);
```

Locates the file and overwrites its contents. It performs a read-modify-write on a per-sector basis and updates the file's size in its parent directory entry once the write is complete.

```c
bool fat32_create(const char* path);
bool fat32_mkdir(const char* path);
```

Both functions search for an empty slot (marked by `0x00` or `0xE5`) in the parent directory. `create` initializes a standard archive file, while `mkdir` uses the `create_directory_entry` helper to set up the special `.` and `..` records. If a directory is full, these functions will automatically allocate a new cluster to "grow" the directory.

```c
bool fat32_remove(const char* name);
```

Deletes a file by traversing its cluster chain and marking every entry in the FAT as `0x00000000` (free). Finally, it sets the first character of the filename in the directory entry to `0xE5`, signaling to the driver that the slot is available for reuse.

```c
File* fat32_get(const char* name);
```

A convenience function that returns a standalone `File` object. It allocates a buffer, reads the entire file into memory, and populates the metadata (size and attributes). This is essentially a "load file to memory" shortcut.

```c
FileNode* fat32_getall(const char* path);
```

Used for directory listings (like an `ls` command). It iterates through every valid entry in a directory cluster chain and builds a linked list of `FileNode` structures. It also converts the raw 8.3 space-padded names into standard null-terminated strings for the caller.

# ELF Loading and Execution

The executor is responsible for parsing the ELF structure, mapping its segments into virtual memory, and initializing a new task context. It ensures that the code, data, and stack are all in the right place before the CPU begins execution.

```c
bool exec(const char* path);
```

The main entry point for running a program:
* **Header Verification:** Checks the "Magic Numbers" at the start of the file to ensure it is a valid ELF executable and not a random binary or text file.
* **Address Space Creation:** Allocates a fresh page directory for the new process. It clones the kernel’s mapping so the process can still make system calls, but sets up private user-space mappings for the actual program data.
* **Segment Mapping:** Iterates through the ELF **Program Headers**. For every `PT_LOAD` segment, it allocates physical pages and maps them to the specific virtual addresses the compiler expects. This includes zeroing out memory for the `.bss` section (uninitialized data).
* **Stack Initialization:** Allocates and maps a dedicated page for the user stack (`USER_STACK_TOP`), giving the program a place to store local variables and function return addresses.

Once the memory is mapped, the executor creates a new task and prepares it for the scheduler.

```c
void elf_user_trampoline();
```

Since the kernel cannot simply `call` a user-space function (due to privilege level differences), it uses this trampoline to grab the entry point and stack origin from the current task structure.

```c
extern void elf_user_trampoline_stub(uint32_t entry, uint32_t stack);
```

An assembly-level stub that performs the actual privilege switch. It manually sets up the CPU registers, pushes the user-space data and code segment selectors onto the stack, and uses an `iret` or similar instruction to "return" into user-space at the program's `e_entry` address.
