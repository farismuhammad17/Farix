A kernel is responsible for managing the low-level aspects of a File System.

# VFS (Virtual File System)

The VFS is an abstraction layer that makes it so that you just mount your new file system, and the rest of the kerne would just use it.

```c
int       fs_read   (const char* name, void* buffer, size_t size);
int       fs_write  (const char* name, const void* buffer, size_t size);
bool      fs_create (const char* name);
bool      fs_mkdir  (const char* name);
bool      fs_remove (const char* name);
File*     fs_get    (const char* name);
FileNode* fs_getall (const char* path);
```

These functions are the global, high-level functions to interact with the file system

```c
void vfs_mount (FileOperations* ops);
```

This function is responsible for changing out the functions for the new file system's operations.

# RAM Disk

A non-persistent file system where all files and folders exist in RAM. Implementation wise, it is just a hash-map for the files, and nothing really exists.

# FAT32

File Allocation Table 32 is very broadly used, and almost every file you ever come accross is on it. As a result, I decided it was most important to have this before I tinker any other. It is also a persistent file system, therefore you can reboot, and the files remain where they are.

# ELF

Executable and Linkable Format (ELF) is a type of executable file. It uses the FAT32 read function to load the bytes into memory and load it, and handles the rest of the functionality there.
