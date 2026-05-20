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

# AHCI (Advanced Host Controller Interface)

*Reference: [Serial ATA AHCI: Specification, Rev. 1.3.1
](https://www.intel.com/content/www/us/en/io/serial-ata/serial-ata-ahci-spec-rev1-3-1.html)*

## Initialisation

```c
void init_ahci(pci_device_t* dev);
```

As per the documentation, section 2.1.11 (ABAR – AHCI Base Address), the AHCI is located at ABAR, its base address. Note that in our code, we refer to ABAR as `bar5`. We find it in the program as so:

```c
// Enable PCI Bus Mastering and Memory Space
uint32_t pci_cmd = pci_read(dev->bus, dev->device, dev->function, 0x04);
pci_write(dev->bus, dev->device, dev->function, 0x04, pci_cmd | (1 << 1) | (1 << 2));

// Map BAR5 (ABAR)
uint32_t  bar5 = pci_read(dev->bus, dev->device, dev->function, 0x24);
uintptr_t phys = bar5 & 0xFFFFFFF0;
```

The PDF further continues,

> The ABAR must be allocated to contain enough space for the global AHCI registers, the port specific registers for each port, and any vendor specific space (if needed).

To do this, we just have to use the VMM to map out that memory address, and map `phys` to a predefined base address to use, `AHCI_BASE_VIRT`. Since each page from the PMM is only 4 KB large, it is very possible for it to not be able to fit everything AHCI demands, hence why we map multiple pages.

```c
uint32_t* pd = vmm_get_current_directory();
for (int i = 0; i < AHCI_HBA_PAGES; i++) {
    vmm_map_page(pd, phys + (i * PAGE_SIZE), AHCI_BASE_VIRT + (i * PAGE_SIZE),
                 PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
}
```

We can then use a struct at that mapped address to then map out each individual register we need to use. We also let this be a global variable inside the file.

```c
g_hba = (volatile hba_mem_t*) AHCI_BASE_VIRT;
```

According to section 3.1.10 (CAP2 – HBA Capabilities Extended), the `cap2` register's 0th bit, BOH, indicates whether the AHCI belongs to the BIOS or the OS. To boot into the kernel/OS, the BIOS often kidnaps the AHCI just to load in. Once that is done, the kernel will want the AHCI. Whether or not the AHCI was taken by the BIOS is stored inside the 0th bit of CAP2. We check for this condition in the code:

```c
if (g_hba->cap2 & 0x01) {
    // Move AHCI to OS from BIOS
}
```

As per section 3.1.11 (BOHC – BIOS/OS Handoff Control and Status), if the BOHC register's 1st bit is set, an OS semaphore is set up, to request the ownership of the HBA. Hence, the first step to move the HBA from the BIOS is to set that bit:

```c
g_hba->bohc |= (1 << 1);
```

Then, we check if the BIOS' semaphore has cleared, which would indicate to us that the HBA was handed over from the BIOS. We do this using a while loop that will immediately exit as soon as the BIOS semaphore has cleared. For safety, we also keep a `timeout` variable, just to make sure that the kernel won't get stuck here, just because some error occurred:

```c
int timeout = MAX_TIMEOUT_DURATION;
while ((g_hba->bohc & (1 << 0)) && --timeout) { 
    system_pause();
}
```

Of course, as soon as this happens, the HBA will set a busy bit, to indicate that the BIOS is busy cleaning up the junk it left over from the ownership change. We give it the time for this in a similiar manner as earlier:

```c
timeout = MAX_TIMEOUT_DURATION;
while ((g_hba->bohc & (1 << 4)) && --timeout) {
    system_pause();
}
```

Now, the HBA should have fully transitions from BIOS owned to kernel owned, we have to enable the AHCI, which is mentioned in section 3.1.2 (GHC – Global HBA Control):

> When set, indicates that communication to the HBA shall be via AHCI mechanisms.

Right after this is the main initialisation sequence, which can be found at section 10.1.2 (System Software Specific Initialization), which details it step-by-step. The first step states:

> Indicate that system software is AHCI aware by setting GHC.AE to ‘1’.

```c
g_hba->ghc |= GHC_AE;
timer_stall(1000);
g_hba->ghc |= GHC_HR;
```

A millisecond of time is given between both instructions as per documentation. We then check if the change stuck with the hardware or not, mandatory according to the same section 3.1.2:

```c
int reset_timeout = MAX_TIMEOUT_DURATION;
while ((g_hba->ghc & GHC_HR) && --reset_timeout) {
    system_pause();
}
```

As through the same section, we ought to also set the GHC to IE at the end:

```c
g_hba->ghc |= GHC_AE;    // Re-enable AHCI mode after reset
timer_stall(1000);       // 1ms stall
g_hba->ghc |= GHC_IE;    // Enable Interrupts
```

Back to section 10.1.2, the next step is:

> Determine which ports are implemented by the HBA, by reading the PI register. This bit map value will aid software in determining how many ports are available and which port registers need to be initialized.

This is quite straightforward to make:

```c
uint32_t pi = g_hba->pi;;
```

The `drives_found` is set just to keep track of whether we found any or not. The documentation for the PI register can be found at section 3.1.4 (PI – Ports Implemented). It is a bitmap, $0$ for no drive, and $1$ for existent drive. Since the AHCI supports at most 32 drives, we can just iterate through them and check if the bitmap is true:

```c
for (int i = 0; i < 32; i++) {
    if (pi & (1 << i)) {
        // Register port
    }
}
```

Section 10.1.2 continues with the initialisation steps:

> Ensure that the controller is not in the running state by reading and examining each implemented port’s PxCMD register. If PxCMD.ST, PxCMD.CR, PxCMD.FRE, and PxCMD.FR are all cleared, the port is in an idle state. Otherwise, the port is not idle and should be placed in the idle state prior to manipulating HBA and port specific registers. System software places a port into the idle state by clearing PxCMD.ST and waiting for PxCMD.CR to return ‘0’ when read. Software should wait at least 500 milliseconds for this to occur. If PxCMD.FRE is set to ‘1’, software should clear it to ‘0’ and wait at least 500 milliseconds for PxCMD.FR to return ‘0’ when read. If PxCMD.CR or PxCMD.FR do not clear to ‘0’ correctly, then software may attempt a port reset or a full HBA reset to recover.

We thus need to get the registers of the port, so we just map it through a the `hba_port_t` struct. 'PxCMD' refers to the `cmd` value present in the struct, the documentation of which can be found at section 3.3.7 (PxCMD – Port x Command and Status), which states the following facts we need to know:

| Name | Bit |
| ---- | --- |
| ST   | 0   |
| CR   | 15  |
| FRE  | 4   |
| FR   | 14  |

Since it wants the port to be idle, we have to make sure it is, and if not, make it idle. To check if they are all cleared, which according to the documentation means that they are in an idle state, we simply check the bit:

```c
if (port->cmd & (PX_CMD_ST | PX_CMD_CR | PX_CMD_FRE | PX_CMD_FR) != 0) {
    // Make port idle
}
```

Documentation details what to do if our port is not idle:

* System software places a port into the idle state by clearing PxCMD.ST and waiting for PxCMD.CR to return ‘0’ when read.
* Software should wait at least 500 milliseconds for this to occur
* If PxCMD.FRE is set to ‘1’, software should clear it to ‘0’
* and wait at least 500 milliseconds for PxCMD.FR to return ‘0’ when read.

Since this is quite a repititive function, we can move it into its own function:

```c
static void ahci_stop_port(hba_port_t *port) {
    // Clear ST (Start)
    port->cmd &= ~PX_CMD_ST;

    // Wait for CR to clear
    int timeout = MAX_TIMEOUT_DURATION;
    while ((port->cmd & PX_CMD_CR) && --timeout) {
        system_pause();
    }

    // Clear FRE
    port->cmd &= ~PX_CMD_FRE;

    // Wait for FR to clear
    timeout = MAX_TIMEOUT_DURATION;
    while ((port->cmd & PX_CMD_FR) && --timeout) {
        system_pause();
    }
}
```

Documentation does state to wait 500 milliseconds between these checks, but the while loop *should* give enough time for it, that we can move on exactly when the bit it set. Now, the documentation does state a fail-safe logic:

> If PxCMD.CR or PxCMD.FR do not clear to ‘0’ correctly, then software may attempt a port reset or a full HBA reset to recover.

HBA reset means to reset **all** 32 points, which is overboard, so we should just do a port reset. We can change out the function to tell us if CR and FR did clear:-

```c
static bool ahci_stop_port(hba_port_t *port) {
    // Clear ST (Start)
    port->cmd &= ~PX_CMD_ST;

    // Wait for CR to clear
    int timeout = 1000000;
    while (timeout--) {
        if (!(port->cmd & PX_CMD_CR)) break;
        system_pause();
    }
    if (timeout <= 0) return false; // Failed to stop command engine

    // Clear FRE
    port->cmd &= ~PX_CMD_FRE;

    // Wait for FR to clear
    timeout = 1000000;
    while (timeout--) {
        if (!(port->cmd & PX_CMD_FR)) break;
        system_pause();
    }

    return (timeout > 0);
}
```

This changes out the check for idle section to this:

```c
if (unlikely((port->cmd & (PX_CMD_ST | PX_CMD_CR | PX_CMD_FRE | PX_CMD_FR)) != 0)) {
    bool res = ahci_stop_port(port);

    if (unlikely(!res)) {
        err_printf("init_ahci: Port %d failed to stop (PxCMD: 0x%x)", i, port->cmd);
        continue;
    }
}
```

We continue off of the port since the port is faulty, remains in idle state, and we cannot do anything about it, so we ignore it. Finally, we can move to the fourth step of the initialisation steps at section 10.1.2:

> Determine how many command slots the HBA supports, by reading CAP.NCS.

Section 3.1.1 states that NCS is located from bits 8 to 12. To find it, just bit shift back 8, then mask off the next 5 bits, to get everything from 8 to 12 inclusive. To keep code clean, we can have a macro get it for us:

```c
#define HBA_CAP_NCS(cap) (((cap) >> 8) & 0x1F)
```

Although the documentation mentions this fact after we're already in the loop, we're actually meant to have this value outside the loop.

```c
uint32_t ncs = HBA_CAP_NCS(g_hba->cap);
int slot_count = ncs + 1;

uint32_t pi = g_hba->pi;
bool found_drive = false;

for (int i = 0; i < 32; i++) {
    // Loop through ports and register
}
```

> For each implemented port, system software shall allocate memory for and program...

This step can simply be done by just allocating a new 4 kb page and mapping it, zeroing it out to clear off any junk values, then just setting up the port there:

```c
uint32_t port_phys = (uint32_t) pmm_alloc_page();
uint32_t port_virt = PHYSICAL_TO_VIRTUAL(port_phys);

vmm_map_page(vmm_get_current_directory(), (void*) port_phys, (void*) port_virt,
                PAGE_PRESENT | PAGE_RW | PAGE_PCD);
memset((void*) port_virt, 0, PAGE_SIZE);

port->clb  = (uint32_t)(port_phys & 0xFFFFFFFF);
port->clbu = (uint32_t)(port_phys >> 32);

uint64_t fis_addr = port_phys + 1024;
port->fb  = (uint32_t)(fis_addr & 0xFFFFFFFF);
port->fbu = (uint32_t)(fis_addr >> 32);

port->cmd |= PX_CMD_FRE;
```

The values are only set like this, because the code has to work for both 32 bit and 64 bit machines, so we just mask off the values appropriately. I have no idea how else to explain it. It just works in my head.

> For each implemented port, clear the PxSERR register, by writing ‘1s’ to each implemented bit location.

Translation:

```c
port->serr = 0xFFFFFFFF;
```

> Determine which events should cause an interrupt, and set each implemented port’s PxIE register with the appropriate enables. To enable the HBA to generate interrupts, system software must also set GHC.IE to a ‘1’.
>
> **Note:** Due to the multi-tiered nature of the AHCI HBA’s interrupt architecture, system software must always ensure that the PxIS (clear this first) and IS.IPS (clear this second) registers are cleared to ‘0’ before programming the PxIE and GHC.IE registers. This will prevent any residual bits set in these registers from causing an interrupt to be asserted.

So the order should be:

1. Clear port IS (interrupt status)
2. Clear Global IS
3. Set port IE to 1 to enable interrupts
4. Enable GHC.IE to enable AHCI system interrupts

```c
port->is  = port->is;
g_hba->is = (1 << i);

// Trigger interrupts for DHRE, PSE, DSE, and SDBE
port->ie = 0xF;
```

Section 3.3.7 states that setting PxCMD's bit 0 to true turns it from idle state to running, thus we can end the loop at that point:

```c
        port->cmd |= PX_CMD_ST;
    }
}

// Last part of the last step
g_hba->ghc |= GHC_IE;
```

We also need to know the ports later for reading and writing, so we store them into an array `active_drives` at the end of the for-loop.

```c
// Check if a device is present (0x3 means present and communication established)
if ((port->ssts & 0x0F) == 0x03) {
    active_drives[drives_found++] = port;
}
```

## Reading a sector

```c
void ahci_read_sector(uint32_t lba, uint8_t* buffer);
```

A sector is a block of data that stores something. We need a function that takes in the `lba`, which is sort of the 'n' in nth sector, and store the value there into a given buffer. Since we only get the `lba`, we need to first find the first free port. Section 3.3.8 details how to get the status of a port, which states that bits 0 to 7 hold the status of the port. It also states that if bit 7 is set, the port is busy; if bit 3 is set, a request is already sent to the driver. Of course, if either of those are true, we ought not to disturb the port. Since we have an array of the ports found at `init_ahci`, we can just iterate through it to find the first free port to take. Since this is also probably a repititive task, we'll put it in a function.

```c
static hba_port_t* ahci_find_free_port(size_t max_retry_attempts) {
    for (size_t attempt_i = 0; attempt_i < max_retry_attempts; attempt_i++) {
        for (size_t port_i = 0; port_i < drives_found; port_i++) {
            hba_port_t* port = active_drives[port_i];

            // We want only SATA
            if (port->sig == AHCI_SIG_SATA && !(port->tfd & (PX_TFD_BSY | PX_TFD_DRQ))) {
                return port;
            }
        }

        timer_stall(1000); // 1 ms
    }

    return NULL;
}
```

The idea of the function is quite simple: We have a retry loop, in case all the ports are currently unusable. We then iterate through the ports we found, do the check the documentation inferred to do, and if the port is free, return it, if not, go to the next port. If we find no ports, give a millisecond breathing room, and retry.

Next, we need to find a vacant slot to use. Each port is sort of the "worker", and the slots are the "pending tasks", i.e. a list of queued commands the port has to do. Section 3.3.13 and 3.3.14 detail how we can do that. We're just iterating through the slots and checking for the first free one.

```c
static int ahci_find_free_slot(hba_port_t* port) {
    // If a bit is set in either register, the slot is occupied
    uint32_t slots = (port->ci | port->sact);

    for (int i = 0; i < 32; i++) {
        if (!((slots >> i) & 1)) {
            return i;
        }
    }

    return -1; // All 32 slots are currently full
}
```

Next, in the read function, I ripped off the struct from the OSDev wiki, and I just have to use it straight from the CLB register. I also need the header, which is located at $base + (slot \cross 32)$. Since the `hba_cmd_header_t` is exact 32 bytes large, C does the math.

```c
void ahci_read_sector(uint32_t lba, uint8_t* buffer) {
    hba_port_t* port = ahci_find_free_port();

    if (unlikely(!port)) {
        err_print("ahci_read_sector: No port found");
        return;
    }

    int port_slot = ahci_find_free_slot(port);

    if (unlikely(port_slot == -1)) {
        err_print("ahci_read_sector: No free slot found on port");
        return;
    }

    hba_cmd_header_t* cmd = (hba_cmd_header_t*) PHYSICAL_TO_VIRTUAL(port->clb);
    hba_cmd_header_t* header = &cmd[port_slot];

    // Send actual read command
}
```

For safety, it is recommended to `memset` everything to 0 in the same section. Without the zeroing step, we are gambling that the HBA ignores the "Reserved" bits, which it explicitly says it might not do.

```c
memset(header, 0, sizeof(hba_cmd_header_t));
```

Now, regarding what to do next, the documentation is quite useless. You have to go back to section 5.1 to just find this paragraph:

> Software posts new commands received by the OS to empty slots in the list, and sets the corresponding slot bit in the PxCI register. The HBA continuously looks at PxCI to determine if there are commands to transmit to the device.

This means we must end the function with:

```c
port->ci = (1 << port_slot);
```

Next is the FIS: The port is the device, the slot is the queued commands, and the command itself is wrapped in a package called the FIS. Since the command header's `ctba` is the base address for the command table, which we need next, we can just yoink the command table struct from OSDev Wiki and use it directly:

```c
hba_cmd_table_t* table = (hba_cmd_table_t*) PHYSICAL_TO_VIRTUAL(header->ctba);
```

With the command table, we can make the FIS, by just using this command table to acquire the `cfis` (command FIS). For anything to be done via the AHCI, you need to go port, slot, command header, command table, FIS & PRDT.

```c
fis->fis_type = FIS_TYPE_REG_H2D;
fis->c = 1;
fis->command = FIS_CMD_READ;
fis->device  = FIS_SATA_LBA_MODE;
```

The source to the sorcery seem to be buried around the documentation. Specifically around section 5 and 10, and it seems I can't find where I got the values from. I just copied the macros straight from the OSDev Wiki. Then we just set up the LBA values of the FIS and then tell it that we only need one sector:

```c
fis->lba0 = (uint8_t) lba;
fis->lba1 = (uint8_t)(lba >> 8);
fis->lba2 = (uint8_t)(lba >> 16);
fis->lba3 = (uint8_t)(lba >> 24);
fis->lba4 = 0;
fis->lba5 = 0;

fis->countl = 1;
```

After the FIS, we must follow it up with the PRDT, which tells the AHCI driver where to dump the data, which we do as per section 5.4.2.
