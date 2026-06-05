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

#include "fs/vfs.h"
#include "syshw/power.h"

#include "fs/fat32.h"
#include "fs/ramdisk.h"

#include "shell/commands.h"
#include "shell/shell.h"

/* Outputs the current VFS */
void cmd_vfs(UNUSED_ARG const char* args) {
    if (args[0] == '\0') {
        sh_print("%s\n", current_vfs->name);
    } else if (args[0] == 'F' || args[0] == 'f') {
        vfs_mount(&fat32_vfs);
    } else if (args[0] == 'R' || args[0] == 'r') {
        vfs_mount(&ramdisk_vfs);
    } else {
        sh_print("vfs: Unknown VFS '%s'", args);
    }
}

/* Shutdown command */
void cmd_shutdown(UNUSED_ARG const char* args) {
    sh_print("Shutting down...");
    system_set_power_state(5);
}

/* Sleep command */
void cmd_sleep(UNUSED_ARG const char* args) {
    sh_print("Sleeping...");
    system_set_power_state(3);
}

/* Reboot/Restart command */
void cmd_reboot(UNUSED_ARG const char* args) {
    sh_print("Rebooting...");
    system_reboot();
}
