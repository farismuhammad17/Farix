"""
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
"""

import os
import shutil
import subprocess

import makefile.globals

def disk_img():
    if makefile.globals.OS == "Darwin":
        makefile.globals.run(f"qemu-img create -f raw {makefile.globals.DISK_PATH} 64M")
        makefile.globals.run(f'hdiutil create -size 64m -fs "MS-DOS FAT32" -volname "FARIX" -type UDIF -layout NONE {makefile.globals.DISK_PATH}')
        os.rename(f"{makefile.globals.DISK_PATH}.dmg", makefile.globals.DISK_PATH)
    else:
        makefile.globals.run(f"qemu-img create -f raw {makefile.globals.DISK_PATH} 64M")
        makefile.globals.run(f"mkfs.fat -F 32 -n FARIX {makefile.globals.DISK_PATH}")

    print(f"\x1b[32mCreated disk at {makefile.globals.DISK_PATH}\x1b[0m")
