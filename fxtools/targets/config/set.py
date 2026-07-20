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
from fxtools.core import statejson

def run(field: str, to: str):
    data = statejson.get()

    if not data[field]:
        printer.error(f"'{field}' is not a valid field")
        return

    data[field] = eval(to)

    statejson.save(data)
    statejson.flush()

def help():
    return {
        "USAGE": "fx config <-field> <-to>",
        "DESCRIPTION": "Set a field in the current state JSON file"
    }
