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
import subprocess

def farix_iso():
    if not os.path.exists("bootloader/farix.bin"):
        print("\x1b[31mfarix.bin not found\x1b[0m")
        return

    if os.path.exists("farix.iso"):
        os.remove("farix.iso")

    cmd = [
        "grub-mkrescue",
        "-o", "farix.iso",
        "bootloader"
    ]

    try:
        subprocess.run(cmd, check=True)
        print("\x1b[1;32mfarix.iso generated successfully!\x1b[0m")
    except FileNotFoundError:
        print("\x1b[31mError: 'grub-mkrescue' or 'xorriso' is not installed on your system.\x1b[0m")
    except subprocess.CalledProcessError as e:
        print(f"\x1b[31mISO Generation failed: {e}\x1b[0m")
