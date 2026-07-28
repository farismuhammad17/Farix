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

from fxtools.core.editor import init

# im severely ill as i write this
# there are probably bugs everywhere
# because i cant focus on a thing and
# i dont trust chatgpt to write the code
# to write the code.
# ill clean up the entire thing later once
# im better. this also will probably be
# lost in the void or smth, no one gona
# read this ever.

def run():
    init.Editor()

def help():
    return {
        "USAGE": "fx studio",
        "DESCRIPTION": "Launches a TUI editor"
    }
