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

# Internal tool handlers
from fxtools.core import printer
from fxtools.core import statejson
from fxtools.core import build
from fxtools.core import tools
from fxtools.core import env
from fxtools.core import help_manager

__all__ = [
    "printer",
    "statejson",
    "build",
    "tools",
    "env",
    "help_manager",
]

__author__          = "Faris Muhammad"
__copyright__       = "Copyright (C) 2026  Faris Muhammad"
__license__         = "GNU Affero General Public License v3.0"
__description__     = "Fx utility tool for the Farix Operating System"
__url__             = "https://github.com/farismuhammad17/Farix"
