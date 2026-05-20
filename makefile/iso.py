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

import makefile.globals as m

def create_iso():
    if os.path.exists("farix.iso"):
        os.remove("farix.iso")

    if not os.path.exists("farix.bin"):
        print(f"\x1b[31mfarix.bin not found (use 'm')\x1b[0m")
        return

    iso_dir = "build/iso_root"
    os.makedirs(f"{iso_dir}/boot/grub", exist_ok=True)

    shutil.copy("farix.bin", f"{iso_dir}/boot/farix.bin")
    with open(f"{iso_dir}/boot/grub/grub.cfg", "w") as f:
        f.write(
            "set timeout=5\n"
            "set default=0\n\n"
            "menuentry 'Farix OS' {\n"
            "    multiboot /boot/farix.bin\n"
            "    boot\n"
            "}\n"
        )

    subprocess.run(["grub-mkrescue", "-o", "farix.iso", iso_dir], check=True)
