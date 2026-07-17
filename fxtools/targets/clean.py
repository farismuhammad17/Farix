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
import shutil

from fxtools.core import printer

JUNK = (
    "build",
    "bootloader/x86/boot/farix.bin",
    "bootloader/x86/boot/farix_elf32.bin",
    "farix.iso",
    "disk.img",
)

USER_BUILD_DIR = "build/apps"

def run(apps_only: bool = False):
    global JUNK

    if apps_only:
        JUNK = (USER_BUILD_DIR,)

    for path in JUNK:
        if os.path.isdir(path):
            shutil.rmtree(path)
            printer.info("Removed directory:", path)
        elif os.path.exists(path):
            os.remove(path)
            printer.info("Removed file:     ", path)

def help():
    return {
        "USAGE": "fx clean --apps-only",
        "ARGS": {
            "--apps-only": "Removes only the build artifacts of the apps."
        },
        "DESCRIPTION": "Removes the build artifacts from the project."
    }
