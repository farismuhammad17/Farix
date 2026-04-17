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

def get_deps():
    if not os.path.exists("newlib-cygwin"):
        print("Installing newlib...")
        makefile.globals.run("git clone --depth 1 https://sourceware.org/git/newlib-cygwin.git")

    print("\x1b[33mInstalled dependencies.\x1b[0m")
