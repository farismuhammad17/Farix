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

from typing import Any

from fxtools.core import printer

def print_format_help(data: dict[str, Any]) -> None:
    print()

    if "USAGE" in data:
        print(f"{printer.F_BLUE('USAGE')} {data['USAGE']}")
        print()

    if "DESCRIPTION" in data:
        print(data["DESCRIPTION"])
        print()

    if "ARGS" in data and isinstance(data["ARGS"], dict):
        print(f"{printer.F_BLUE('ARGS')}:")
        max_len = max(len(arg) for arg in data["ARGS"].keys()) if data["ARGS"] else 0
        for arg, desc in data["ARGS"].items():
            padded_arg = arg.ljust(max_len)
            print(f"  {printer.F_GREEN(padded_arg)}  {desc}")
        print()

    if "VARIABLES" in data and isinstance(data["VARIABLES"], dict):
        print(f"{printer.F_BLUE('VARIABLES')}:")
        for cmd, desc in data["VARIABLES"].items():
            print(f"  {printer.F_GREEN(cmd)}  {desc}")
        print()

    if "NOTES" in data and isinstance(data["NOTES"], list):
        print(f"{printer.F_YELLOW('NOTES')}:")
        for note in data["NOTES"]:
            print(f"  {note}")
        print()
