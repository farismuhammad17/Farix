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
import sys
import termios
import tty
import atexit
import fcntl

from fxtools.core.editor.sidebar import Sidebar
from fxtools.core.editor.menubar import Menubar

class Editor:
    def __init__(self) -> None:
        self.fd = sys.stdin.fileno()
        self.old_settings = termios.tcgetattr(self.fd)

        atexit.register(self.restore_terminal)
        tty.setraw(self.fd)

        width, height = os.get_terminal_size()

        self.sidebar = Sidebar(height=height - 2)
        self.menubar = Menubar(width, height)

        self.mode = "NONE"

        self.buffer = [
            "FX-Studio Workspace (Work-In-Progress)",
            "",
            "Copyright (C) 2026  Faris Muhammad",
            "github.com/farismuhammad17/Farix",
            "",
            "Refer documentation for more information"
        ]

        sys.stdout.write("\x1b[2J\x1b[H") # Clear screen

        # Render initial frame on startup
        sys.stdout.write("\x1b[H")
        sidebar_lines = self.sidebar.render()
        width, height = os.get_terminal_size()
        editor_height = height - 2

        for r in range(editor_height):
            bg_line = self.buffer[r] if r < len(self.buffer) else ""
            if self.sidebar.visible and r < len(sidebar_lines):
                sidebar_line = sidebar_lines[r]
                visible_bg = bg_line[self.sidebar.width:] if len(bg_line) > self.sidebar.width else ""
                combined = f"{sidebar_line}{visible_bg}"
            else:
                combined = f"{bg_line:<{width}}"
            sys.stdout.write(combined + "\r\n")

        self.menubar.update_text(f" {self.mode} | [s] Sidebar | [ESC] Normal | [CTRL+Q] Quit ")
        sys.stdout.write(self.menubar.render() + "\r")

        sys.stdout.flush()

        try:
            self.run()
        finally:
            self.restore_terminal()

    def restore_terminal(self) -> None:
        termios.tcsetattr(self.fd, termios.TCSADRAIN, self.old_settings)

    def read_event(self) -> tuple:
        ch = sys.stdin.read(1)
        if not ch:
            return ("UNKNOWN",)

        ord_ch = ord(ch)

        if ord_ch == 17:
            return ("Q", "CTRL")
        elif ord_ch == 19:
            return ("S", "CTRL")

        elif ch == '\x1b':
            orig_fl = fcntl.fcntl(sys.stdin.fileno(), fcntl.F_GETFL)
            fcntl.fcntl(sys.stdin.fileno(), fcntl.F_SETFL, orig_fl | os.O_NONBLOCK)

            seq = ""
            try:
                # Try to read the rest of the escape sequence bytes if they are buffered
                while len(seq) < 3:
                    try:
                        part = sys.stdin.read(1)
                        if not part:
                            break
                        seq += part
                    except OSError:
                        break
            finally:
                fcntl.fcntl(sys.stdin.fileno(), fcntl.F_SETFL, orig_fl)

            if not seq:
                return ("ESC",)

            if seq == '[A':
                return ("ARROW_UP",)
            elif seq == '[B':
                return ("ARROW_DOWN",)
            elif seq == '[C':
                return ("ARROW_RIGHT",)
            elif seq == '[D':
                return ("ARROW_LEFT",)

            return ("UNKNOWN",)

        return (ch,)

    def run(self) -> None:
        while True:
            data = self.read_event()

            # Exit command
            if data == ("Q", "CTRL"):
                sys.stdout.write("\x1b[2J\x1b[H")
                break

            # Mode switching logic
            if data == ("ESC",):
                self.mode = "NONE"
                self.sidebar.disable()
            elif self.mode == "NONE":
                if data == ('s',) or data == ('S',):
                    self.mode = "SIDEBAR"
                    self.sidebar.enable()
            elif self.mode == "SIDEBAR":
                if data == ('s',) or data == ('S',):
                    self.sidebar.toggle()
                    if not self.sidebar.visible:
                        self.mode = "NONE"
                    else:
                        self.mode = "SIDEBAR"
                else:
                    self.sidebar.handle_input(data)

            # Update status text
            self.menubar.update_text(f" {self.mode} | [s] Sidebar | [ESC] Normal | [CTRL+Q] Quit  ")

            # Render frame update with overlay
            sys.stdout.write("\x1b[H") # Reset cursor to top-left

            sidebar_lines = self.sidebar.render()
            width, height = os.get_terminal_size()
            editor_height = height - 2 # Leave room for menubar

            for r in range(editor_height):
                # 1. Get the background line from the buffer (default to empty string if out of bounds)
                bg_line = self.buffer[r] if r < len(self.buffer) else ""

                if self.sidebar.visible and r < len(sidebar_lines):
                    # Overlay mode: Print sidebar, then slice the background line to start *after* the sidebar width
                    sidebar_line = sidebar_lines[r]
                    # We skip printing the hidden background characters that fall under the sidebar width
                    visible_bg = bg_line[self.sidebar.width:] if len(bg_line) > self.sidebar.width else ""
                    combined = f"{sidebar_line}{visible_bg}"
                else:
                    # Normal full-width render when sidebar is hidden
                    combined = f"{bg_line:<{width}}"

                sys.stdout.write(combined + "\r\n")

            # Draw the menubar at the bottom
            sys.stdout.write(self.menubar.render() + "\r")
            sys.stdout.flush()
