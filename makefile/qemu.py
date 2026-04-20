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

import makefile.globals

def run_qemu(fullscreen=False):
    if makefile.globals.is_in_docker():
        print("\x1b[31mCannot emulate QEMU in docker. Run on native machine instead.\x1b[0m")
        print("\x1b[90mWe set up Docker to only help someone who just cloned the repository to",
            "compile the kernel, and get the farix.bin or farix.iso file. Unfortunately, docker",
            "is an emulation, and thus, it is not possible to run an emulation through it.\x1b[0m", sep='\n')
        return

    suffix = "-full-screen" if fullscreen else ""

    cmd = f"{makefile.globals.QEMU_BIN} -kernel farix.bin {makefile.globals.QEMU_FLAGS} {suffix}"
    os.system(cmd)
