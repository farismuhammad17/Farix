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

You should have received a cop of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
"""

class Menubar:
    def __init__(self, width: int, height: int, text: str = ""):
        self.text = text
        self.width = width
        self.height = height

    def update_text(self, text: str) -> None:
        self.text = text

    def render(self) -> str:
        # Jump cursor directly to the bottom row, column 1, then draw the bar
        formatted_text = self.text[:self.width]
        bar_string = f"\x1b[47m\x1b[30m{formatted_text:<{self.width}}\x1b[0m"
        return f"\x1b[{self.height};1H{bar_string}"
