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

from fxtools import statejson
from fxtools import tools

data = statejson.get()
arch = data["DEFAULT_ARCH"]

TOOLS = tools.get_tools()

TARGET_DIR = TOOLS["PREFIX"].rstrip("-")
LIBC_INSTALL_DIR = os.path.join(os.getcwd(), f"libc_build_{arch}")
LIBC_DIR = os.path.join(LIBC_INSTALL_DIR, TARGET_DIR)
LIBC_INC = os.path.join(LIBC_INSTALL_DIR, "include")
LIBC_LIB = os.path.join(LIBC_INSTALL_DIR, "lib")
