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

from fxtools.core import printer
from fxtools.core import env
from fxtools.core import statejson

data = statejson.get()
arch = data["DEFAULT_ARCH"]

_cached_tools = None

def get_tools() -> dict[str, str]:
    global _cached_tools
    if _cached_tools is not None:
        return _cached_tools

    PREFIX = None

    compilers = ()
    if arch == "x86_64":
        compilers = ("x86_64-linux-gnu-gcc", "x86_64-elf-gcc")
    for comp in compilers:
        if env.shell_which(comp):
            PREFIX = comp[:-3]
            break
    if not PREFIX:
        printer.error("Found not supported compiler")
        return {}

    _cached_tools = {
        "PREFIX": PREFIX,
        "CC": f"{PREFIX}gcc",
        "AS": f"{PREFIX}as",
    }

    return _cached_tools
