/*
-----------------------------------------------------------------------
Farix Operating System
Copyright (C) 2026  Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#include "klib/stdio.h"
#include "klib/stdlib.h"

#include "drivers/vfs.h"
#include "syshw/power.h"

#include "sysmods/interface.h"
#include "sysmods/loader.h"

#include "shell/commands.h"

/* Outputs the current VFS */
void cmd_vfs(UNUSED_ARG const char* args) {
    printf("%s\n", vfs->name);
}

/* Shutdown command */
void cmd_shutdown(UNUSED_ARG const char* args) {
    printf("Shutting down...");
    system_set_power_state(5);
}

/* Sleep command */
void cmd_sleep(UNUSED_ARG const char* args) {
    printf("Sleeping...");
    system_set_power_state(3);
}

/* Reboot/Restart command */
void cmd_reboot(UNUSED_ARG const char* args) {
    printf("Rebooting...");
    system_reboot();
}

/* List all opened system modules */
void cmd_drivers(UNUSED_ARG const char* args) {
    for (size_t i = 0; i < MAX_SYSMODS; i++) {
        loaded_sysmod_t* reg = &sysmods_registry[i];

        // Can't terminate at first NULL since registry is not
        // linear, i.e. there will be gaps of NULL in between
        // valid devices
        if (!reg || !reg->interface) continue;

        printf("%2d %-16s   %p (%u)\n", i,
            reg->interface->name,
            reg->base_address,
            reg->size);
    }
}

/* Load system module */
void cmd_drv_load(const char* args) {
    load_sysmod(args);
}

/* Unload system module */
void cmd_drv_unload(const char* args) {
    unload_sysmod(atoi(args));
}
