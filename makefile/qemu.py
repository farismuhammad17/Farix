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
    suffix = "-full-screen" if fullscreen else ""

    cmd = f"{makefile.globals.QEMU_BIN} -kernel farix.bin {makefile.globals.QEMU_FLAGS} {suffix}"
    os.system(cmd)
