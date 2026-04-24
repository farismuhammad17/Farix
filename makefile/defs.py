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
import re

ROOT_DIR = "."

# Regex for declarations/definitions
FUNC_DECL_PATTERN = re.compile(r'^\s*[a-zA-Z_][\w\s\*]+\s+([a-zA-Z_]\w*)\s*\([^;\{]*\)')
# Regex for usage
FUNC_USAGE_PATTERN = re.compile(r'\b([a-zA-Z_]\w*)\s*\(')

data = {}

def scan_files():
    print(f"Deep scanning \x1b[32m{os.path.abspath(ROOT_DIR)}... \x1b[90m(Please wait)\x1b[0m")

    file_count = 0
    for root, _, filenames in os.walk(ROOT_DIR):
        for file in filenames:
            if not file.endswith(('.c', '.h')):
                continue

            path = os.path.join(root, file)

            # \r moves to start of line; \x1b[K clears the line to avoid leftover text
            # We truncate the path slightly if it's too long for a standard terminal
            display_path = (path[:75] + '..') if len(path) > 77 else path
            print(f"\r\x1b[KScanning: \x1b[94m{display_path}\x1b[0m", end="", flush=True)

            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    file_count += 1

                    # Definitions
                    for line in content.splitlines():
                        match = FUNC_DECL_PATTERN.search(line)
                        if match:
                            name = match.group(1)
                            if name not in data:
                                data[name] = {"headers": set(), "bodies": set(), "used_in": set()}

                            if file.endswith('.h'):
                                data[name]["headers"].add(path)
                            else:
                                data[name]["bodies"].add(path)

                    # Usages
                    for match in FUNC_USAGE_PATTERN.finditer(content):
                        name = match.group(1)
                        if name in data:
                            if path not in data[name]["headers"] and path not in data[name]["bodies"]:
                                data[name]["used_in"].add(path)
            except Exception:
                continue

    print(f"\r\x1b[KIndex complete: {file_count} files processed, {len(data)} functions mapped.\n")

def query_loop():
    while True:
        try:
            query = input("\x1b[1;34mFunction > \x1b[0m").strip()

            if query.lower() in ("quit", "exit", "q"):
                break

            if not query:
                continue

            query = query.replace("()", "")

            if query in data:
                info = data[query]
                print(f"\n\x1b[1;32m{query}:\x1b[0m")

                for h in sorted(info["headers"]):
                    print(f"\x1b[1;33m[Header]\x1b[0m {h}")
                for b in sorted(info["bodies"]):
                    print(f"\x1b[1;36m[Body]  \x1b[0m {b}")

                print("\x1b[1m[Used in]\x1b[0m")
                if info["used_in"]:
                    for u in sorted(info["used_in"]):
                        print(f"  {u}")
                else:
                    print("  No usages found.")

                print()
            else:
                print(f"\x1b[91m'{query}' not found in the codebase.\x1b[0m\n")
        except KeyboardInterrupt:
            break
