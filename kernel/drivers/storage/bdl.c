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

#include "cpu/pci.h"
#include "drivers/terminal.h"

#include "drivers/uart.h" // TODO REM

#include "drivers/storage/ahci.h"
#include "drivers/storage/ata.h"

#include "drivers/storage/bdl.h"

static BDLDevice* current_bdl_dev = NULL;

void init_storage() {
    bool found = false;

    for (int i = 0; i < pci_device_count; i++) {
        pci_device_t* dev = &pci_devices[i];

        if (dev->class_code == PCI_CLASS_CODE_STORAGE) {
            if (dev->subclass == PCI_AHCI_SUBCLASS) {
                init_ahci(dev);
                found = true;
                break; // Found the modern one, stop here
            }

            else if (dev->subclass == PCI_ATA_SUBCLASS) {
                init_ata(dev);
                found = true;
                // Don't break yet, keep looking for AHCI
            }
        }
    }

    if (!found) {
        t_print("init_storage: Storage device not found");
        return;
    }
}

void bdl_mount(BDLDevice* dev) {
    current_bdl_dev = dev;
}

void bdl_read(uint32_t lba, void* buf) {
    if (unlikely(!current_bdl_dev || !current_bdl_dev->read)) {
        t_print("bdl_read: BDL operation not found");
        return;
    }

    return current_bdl_dev->read(lba, buf);
}

void bdl_write(uint32_t lba, void* buf) {
    if (unlikely(!current_bdl_dev || !current_bdl_dev->write)) {
        t_print("bdl_write: BDL operation not found");
        return;
    }

    return current_bdl_dev->write(lba, buf);
}
