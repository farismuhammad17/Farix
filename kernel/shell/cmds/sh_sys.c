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

#include "shell/commands.h"
#include "shell/shell.h"

/* Outputs the current VFS */
void cmd_vfs(UNUSED_ARG const char* args) {
    sh_print("%s\n", current_vfs->name);
}

/* Shutdown command */
void cmd_shutdown(UNUSED_ARG const char* args) {
    sh_print("Shutting down...");
    system_set_power_state(5);
}

/* Reboot/Restart command */
void cmd_reboot(UNUSED_ARG const char* args) {
    sh_print("Rebooting...");
    system_reboot();
}
