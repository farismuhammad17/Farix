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

import os

import makefile.sysmods

import makefile.globals as m

def disk_img():
    if m.OS == "Darwin":
        m.run(f"qemu-img create -f raw {m.DISK_PATH} 64M")
        m.run(f'hdiutil create -size 64m -fs "MS-DOS FAT32" -volname "FARIX" -type UDIF -layout NONE {m.DISK_PATH}')
        os.rename(f"{m.DISK_PATH}.dmg", m.DISK_PATH)
    else:
        m.run(f"qemu-img create -f raw {m.DISK_PATH} 64M")
        m.run(f"mkfs.fat -F 32 -n FARIX {m.DISK_PATH}")

    m.run(f"mmd -i {m.DISK_PATH} ::/system")

    print(f"\x1b[32mCreated disk at {m.DISK_PATH}\x1b[0m")

    makefile.sysmods.build_sysmods()
