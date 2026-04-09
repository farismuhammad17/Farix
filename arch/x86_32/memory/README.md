Memory is almost the exact same throughout the kernel, but all the way until paging is enabled, we have to manually configure it all in the architecture specific files.

# PMM (Physical Memory Manager)

The PMM is the lowest level of memory management in the kernel. Its job is to keep track of every 4KB block of physical RAM (a "page") and decide which are free and which are in use. Because this happens before virtual memory is even fully established, the PMM must be extremely efficient and have a very small footprint.

This implementation uses a **Bitmap**. Each bit in a large array of integers represents one page of physical memory: a `1` means the page is occupied, and a `0` means it is free.

```c
void init_pmm();
```

Initializes the memory registry. It starts by assuming all memory is reserved (marking everything as `1`). It then consults the **Multiboot memory map**, provided by the bootloader, to identify regions of "Available RAM" and marks those as free (`0`). 

Crucially, it then manually re-marks specific regions as "Used" to prevent the kernel from accidentally overwriting itself. This includes:
* **The Kernel:** Protected using the `_kernel_start` and `_kernel_end` symbols from the linker.
* **The Bitmap:** The PMM protects its own tracking array.
* **Multiboot Data:** Essential boot information passed by the bootloader.
* **The First MiB:** Often reserved for BIOS and legacy hardware.

```c
void* pmm_alloc_page();
```

Searches for the first available bit set to `0`. It uses a **First-Fit algorithm**, scanning the `pmm_bitmap` for any integer that isn't `0xFFFFFFFF` (meaning it has at least one free bit). Once found, it marks that bit as `1` and returns the physical address calculated as $page\_number \times 4096$.

```c
void pmm_free_page(void* addr);
```

Releases a previously allocated page back into the pool. It converts the physical address back into a page number by shifting right (dividing by 4096) and clears the corresponding bit in the bitmap.

```c
static void pmm_set_bit(uint32_t page_number);
static void pmm_clear_bit(uint32_t page_number);
```

Internal helper functions that perform the bitwise math required to find the correct array index and specific bit position for a given page number.

# VMM (Virtual Memory Manager)

The Virtual Memory Manager (VMM) implements **Paging**, the mechanism that gives every process the illusion of owning the entire memory space (4GB on x86). It maps **Virtual Addresses** (used by software) to **Physical Addresses** (the actual RAM). This provides memory isolation, preventing one program from crashing another or the kernel.

The x86 architecture uses a two-level paging hierarchy: a **Page Directory** (PD) containing pointers to 1024 **Page Tables** (PT), which in turn point to the actual 4KB physical pages.

```c
void init_vmm();
```

Sets up the initial paging structure. It identity-maps the first few megabytes (where $Virtual == Physical$) so the kernel doesn't crash the moment paging is enabled. It also implements a **Higher-Half Kernel** design, mapping that same physical memory to the `0xC0000000` range (index 768+). Finally, it sets the `PG` bit in the `CR0` register to turn on the MMU.

```c
void vmm_map_page(uint32_t* pd_phys, void* phys, void* virt, uint32_t flags);
```

Creates a new mapping. It breaks the virtual address into a Directory Index and a Table Index. If a Page Table doesn't exist for that range, it allocates one via the PMM. It then links the physical address into the table and uses `invlpg` to flush the **TLB (Translation Lookaside Buffer)**, ensuring the CPU sees the new mapping immediately.

```c
uint32_t* vmm_copy_kernel_directory();
```

When creating a new task, the kernel needs a new Page Directory. This function copies the "Higher-Half" (the kernel's memory) into a new directory while leaving the "Lower-Half" (user space) empty. This ensures the kernel is visible in every process's address space.

```c
void vmm_switch_directory(uint32_t* page_directory);
```

Performs a virtual memory context switch by loading the physical address of a Page Directory into the `CR3` register. This instantly changes the entire memory map of the CPU.

```c
uint32_t vmm_get_phys(uint32_t* pd_phys, void* virt_addr);
```

A "Manual Translation" helper. It traverses the PD and PT structures in software to find what physical address a specific virtual address currently points to. This is vital for the kernel when it needs to access user-provided buffers.
