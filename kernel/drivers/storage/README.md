# ATA (Advanced Technology Attachment)

Directly communicating with storage drives using raw electrical signals is complex and error-prone. To standardize how software talks to hard disks and solid-state drives, we use the ATA interface. It provides a consistent protocol for the kernel to move data between physical disk sectors and system memory.

The ATA controller allows the kernel to issue commands—like "read" or "write" to specific disk locations called LBA (Logical Block Addressing). Instead of the CPU waiting idly for the slow mechanical disk to finish, we use status flags like BSY (Busy) and DRQ (Data Request) to synchronize the high-speed CPU with the slower storage hardware.

```c
void init_ata();
```

Initializes the disk controller, detects connected drives, and prepares the bus for data transfer. This function uses the PCI to find the ATA, and if no ATA is found, or the ATA is found to be legacy, then it fallbacks to the default values for the ATA driver. If the ATA found by the PCI is a new version, then the ATA's addresses and constants are changed accordingly.

```c
void ata_read_sector(uint32_t lba, uint8_t* buffer);
void ata_write_sector(uint32_t lba, uint8_t* buffer);
```

These functions translate high-level requests into low-level commands sent to the disk. They handle the transfer of 512-byte chunks of data (sectors) between the hardware and a provided memory buffer.

```c
int ata_wait_ready();
```

A helper that polls the status register. It ensures the drive is not busy and is ready to accept the next command or data packet before the kernel proceeds. If the kernel does not return anything by some defined `timeout`, it would just assume failure, and return 1.
