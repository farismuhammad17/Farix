"""
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
"""

import os
import json

import makefile.globals

def config_mjson():
    file_path = makefile.globals.MAKE_CONF_JSON

    while True:
        with open(file_path, 'r') as f:
            data = json.load(f)

        # Clear screen and reset cursor
        print("\033[H\033[J", end="")
        print("\x1b[1;36m--- Farix Configuration ---\x1b[0m")

        # Display current data
        keys = list(data.keys())
        for i, key in enumerate(keys, 1):
            print(f"  \x1b[1;32m{i}.\x1b[0m \x1b[1m{key}\x1b[0m: {data[key]}")

        print(f"\n\x1b[90m(Type 'exit' to return)\x1b[0m")
        choice = input("\x1b[1;36m>>> \x1b[0m").strip()

        if choice.lower() in ['exit', 'quit']:
            break

        # Resolve key from index or string
        key = None
        if choice.isdigit() and 1 <= int(choice) <= len(keys):
            key = keys[int(choice) - 1]
        elif choice in data:
            key = choice

        if not key:
            continue

        # Handle Value Input
        current_val = data[key]
        print(f"\n\x1b[1m{key}:\x1b[0m \x1b[90m{current_val} ->\x1b[0m ", end="")

        new_val = input("").strip()

        # Update and save
        if new_val: # Only update if input isn't empty
            data[key] = new_val
            with open(file_path, 'w') as f:
                json.dump(data, f, indent=4)
