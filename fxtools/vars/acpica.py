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

from fxtools.core import statejson

data = statejson.get()
arch = data["DEFAULT_ARCH"]

ACPICA_ARCH_INDEPENDANT = "include/drivers/acpi"
ACPICA_ARCH_DEPENDANT = f"arch/{arch}/include/acpi"

ACPICA_SRC = "kernel/drivers/acpi"
ACPICA_CFLAGS = (
    "-ffreestanding -O2 -Wall -Wextra -fno-exceptions "
    "-mno-red-zone "
    "-mno-mmx -mno-sse -mno-sse2 "
    "-fdiagnostics-color=always "
    f"-Iinclude "
    f"-I{ACPICA_ARCH_INDEPENDANT} -I{ACPICA_ARCH_DEPENDANT} "
    "-mcmodel=kernel -fno-pic "
    "-fno-stack-protector -U_FORTIFY_SOURCE "
)
