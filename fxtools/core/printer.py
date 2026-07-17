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

def info(title: str, msg: str):
    print(f"\x1b[34m{title}\x1b[0m {msg}")

def error(msg: str):
    print(f"\x1b[31m{msg}\x1b[0m")

def success(msg: str):
    print(f"\x1b[32m{msg}\x1b[0m")

def warn(msg: str):
    print(f"\x1b[33m{msg}\x1b[0m")

def wait(msg: str):
    print(f"\x1b[34m{msg}\x1b[0m")

# ANSI Formatters

def F_BLACK(s: str) -> str:
    return f"\x1b[30m{s}\x1b[0m"

def F_RED(s: str) -> str:
    return f"\x1b[31m{s}\x1b[0m"

def F_GREEN(s: str) -> str:
    return f"\x1b[32m{s}\x1b[0m"

def F_YELLOW(s: str) -> str:
    return f"\x1b[33m{s}\x1b[0m"

def F_BLUE(s: str) -> str:
    return f"\x1b[34m{s}\x1b[0m"

def F_MAGENTA(s: str) -> str:
    return f"\x1b[35m{s}\x1b[0m"

def F_CYAN(s: str) -> str:
    return f"\x1b[36m{s}\x1b[0m"

def F_WHITE(s: str) -> str:
    return f"\x1b[37m{s}\x1b[0m"

def F_GREY(s: str) -> str:
    return f"\x1b[90m{s}\x1b[0m"
