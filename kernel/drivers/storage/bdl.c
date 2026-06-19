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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpu/multicore.h"
#include "cpu/pci.h"
#include "drivers/terminal.h"
#include "fs/vfs.h"

#include "drivers/storage/ahci.h"
#include "drivers/storage/ata.h"

#include "drivers/storage/bdl.h"

static BDLDevice* current_bdl_dev = NULL;

static spinlock bdl_lock = 0;

/*
Looks through the PCI to find storage devices. If the AHCI is found, it would
initialise with it, else, it would initialise with the ATA, calling init_ahci
or init_ata respectively.
*/
void init_storage() {
    pci_device_t* ata_dev = NULL;

    for (int i = 0; i < pci_device_count; i++) {
        pci_device_t* dev = &pci_devices[i];

        if (dev->class_code == PCI_CLASS_CODE_STORAGE) {
            // Check for AHCI first
            if (dev->subclass == PCI_AHCI_SUBCLASS && dev->progif == 0x01) {
                init_ahci(dev);
                return; // AHCI is active, we are done.
            }

            // Keep track of an ATA device in case we don't find AHCI
            if (dev->subclass == PCI_ATA_SUBCLASS) {
                ata_dev = dev;
            }
        }
    }

    // Fallback: only if no AHCI was found
    if (ata_dev) {
        init_ata(ata_dev);
    } else {
        err_print("init_storage: No supported storage device found");
    }
}

/* Change the BDL device */
void bdl_mount(BDLDevice* dev) {
    current_bdl_dev = dev;
}

/* Reads from the given LBA and writes to the given buffer */
void bdl_read(uint32_t lba, void* buf) {
    spin_lock(&bdl_lock);

    if (unlikely(!current_bdl_dev || !current_bdl_dev->read)) {
        err_print("bdl_read: BDL operation not found");
        return;
    }

    current_bdl_dev->read(lba, (uint8_t*) buf);

    spin_unlock(&bdl_lock);
}

/* Writes the given buffer into the given LBA */
void bdl_write(uint32_t lba, void* buf) {
    spin_lock(&bdl_lock);

    if (unlikely(!current_bdl_dev || !current_bdl_dev->write)) {
        err_print("bdl_write: BDL operation not found");
        return;
    }

    if (unlikely(current_vfs && current_vfs->check_write_safety)) {
        int write_safety_res = current_vfs->check_write_safety(lba);

        if (unlikely(write_safety_res != 0)) {
            err_printf("bdl_write: Error %d at LBA %d", write_safety_res, lba);
            return;
        }
    }

    current_bdl_dev->write(lba, (uint8_t*) buf);

    spin_unlock(&bdl_lock);
}
