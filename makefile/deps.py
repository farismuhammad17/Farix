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
import sys
import subprocess

import makefile.globals as m

def get_deps():
    if not os.path.exists("musl"):
        print("\x1b[33mInstalling musl...\x1b[0m")
        m.run("git clone --depth 1 https://github.com/hadean-mirrors/musl.git",
            capture_output=False)
        print("\x1b[32mInstalled musl\x1b[0m")

    print("\x1b[1;32mProcess completed.\x1b[0m")
