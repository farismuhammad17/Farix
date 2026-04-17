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

import makefile.globals

JUNK = (
    "build",
    "farix.bin",
    "bochsout.txt",
    "bochsrc.txt",
    "copy.txt",
    "snapshot.txt"
)

def clean(args):
    todelete = []

    if "apps" in args:
        todelete = [makefile.globals.USER_BUILD_DIR]
    else:
        todelete = list(JUNK)
        for nonjunk in args:
            if nonjunk in todelete:
                todelete.remove(nonjunk)

    for path in todelete:
        if os.path.isdir(path): shutil.rmtree(path)
        elif os.path.exists(path): os.remove(path)
