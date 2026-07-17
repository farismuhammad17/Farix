"""
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
"""

from fxtools.core import printer
from fxtools.core.build import proc_run

from fxtools.vars import emulation

def run():
    proc_run(f"qemu-img create -f raw {emulation.DISK_PATH} 64M")
    proc_run(f"mkfs.fat -F 32 -n FARIX {emulation.DISK_PATH}")
    proc_run(f"mmd -i {emulation.DISK_PATH} ::/system")
    printer.success(f"Created disk at {emulation.DISK_PATH}")

def help():
    return {
        "USAGE": "fx disk",
        "DESCRIPTION": "Initializes an empty, formatted 64-megabyte raw storage container image\n"
                       "(disk.img) configured with a FAT32 filesystem layout for kernel emulation."
    }
