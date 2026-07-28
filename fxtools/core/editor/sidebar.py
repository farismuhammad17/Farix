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

class Sidebar:
    def __init__(self, height: int, width: int = 30, root_dir: str = "."):
        self.width = width
        self.height = height

        self.pointer = 0
        self.visible = False

        # Navigation state
        self.current_path = os.path.abspath(root_dir)
        self.content = []
        self.history = []        # Stack of (path, pointer_position) tuples for going back

        self.load_directory(self.current_path)

    def load_directory(self, path: str) -> None:
        """Loads contents of a directory into the sidebar."""
        try:
            self.current_path = path
            entries = os.listdir(path)

            # Sort: directories first, then files, alphabetically
            dirs = sorted([e for e in entries if os.path.isdir(os.path.join(path, e))])
            files = sorted([e for e in entries if not os.path.isdir(os.path.join(path, e))])

            self.content = dirs + files
            self.pointer = 0

        except OSError:
            self.content = ["<Permission Denied>"]
            self.pointer = 0

    def enable(self) -> None:
        self.visible = True

    def disable(self) -> None:
        self.visible = False

    def toggle(self) -> None:
        self.visible = not self.visible

    def handle_input(self, data: tuple) -> None:
        if data == ("ARROW_UP",):
            if self.content and self.pointer > 0:
                self.pointer -= 1
        elif data == ("ARROW_DOWN",):
            if self.content and self.pointer < len(self.content) - 1:
                self.pointer += 1
        elif data == ("ARROW_RIGHT",):
            # Open directory if item selected is a folder
            if self.content:
                selected_item = self.content[self.pointer]
                target_path = os.path.join(self.current_path, selected_item)

                if os.path.isdir(target_path):
                    # Save current state onto history stack before entering
                    self.history.append((self.current_path, self.pointer))
                    self.load_directory(target_path)
        elif data == ("ARROW_LEFT",):
            # Go back up a directory using history stack if available, else parent dir
            if self.history:
                prev_path, prev_pointer = self.history.pop()
                self.load_directory(prev_path)
                self.pointer = prev_pointer
            else:
                # Fallback to system parent directory
                parent_path = os.path.dirname(self.current_path)
                if parent_path and parent_path != self.current_path:
                    self.load_directory(parent_path)

    def render(self) -> list[str]:
        rendered_lines = []

        for r in range(self.height):
            if not self.visible:
                line = " " * self.width
            else:
                if self.content:
                    self.pointer = max(0, min(self.pointer, len(self.content) - 1))
                else:
                    self.pointer = 0

                if r < len(self.content):
                    item = self.content[r]
                    # Append indicator if it's a directory
                    full_path = os.path.join(self.current_path, item)
                    is_dir = os.path.isdir(full_path)
                    display_suffix = "/" if is_dir else ""
                    display_str = (item + display_suffix)[:self.width - 2]

                    padded_str = f" {display_str} ".ljust(self.width - 1)

                    if r == self.pointer:
                        line = f"\x1b[42m\x1b[30m{padded_str}\x1b[0m"
                    else:
                        color_code = "\x1b[33m" if is_dir else "\x1b[36m" # Yellow for folders, Cyan for files
                        line = f" {color_code}{display_str:<{self.width-2}}\x1b[0m"
                else:
                    line = " " * (self.width - 1)

            border = "\x1b[90m│\x1b[0m" if self.visible else " "
            line += border
            rendered_lines.append(line)

        return rendered_lines
