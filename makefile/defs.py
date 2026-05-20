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
import difflib

ROOT_DIR = "."

# Regex for functions, macros, variables
FUNC_DECL_PATTERN = re.compile(r'^\s*[a-zA-Z_][\w\s\*]+\s+([a-zA-Z_]\w*)\s*\([^;\{]*\)')
MACRO_PATTERN = re.compile(r'^\s*#\s*define\s+([a-zA-Z_]\w*)')
VAR_PATTERN = re.compile(r'^\s*(?:static\s+|extern\s+)?(?:[a-zA-Z_]\w*\s*\*?\s+)+([a-zA-Z_]\w*)\s*[:;=]')

# Regex for usage
USAGE_PATTERN = re.compile(r'\b([a-zA-Z_]\w*)\b')

data = {}

def scan_files():
    print(f"Deep scanning \x1b[32m{os.path.abspath(ROOT_DIR)} ... \x1b[90m(Please wait)\x1b[0m")

    file_count = 0
    pending_usages = []

    for root, _, filenames in os.walk(ROOT_DIR):
        for file in filenames:
            if not file.endswith(('.c', '.h')):
                continue

            path = os.path.join(root, file)
            display_path = (path[:75] + '..') if len(path) > 77 else path
            print(f"\r\x1b[K[0] Scanning: \x1b[94m{display_path}\x1b[0m", end="", flush=True)

            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    file_count += 1
                    pending_usages.append((path, content))

                    # Definitions
                    for line in content.splitlines():
                        match = FUNC_DECL_PATTERN.search(line) or MACRO_PATTERN.search(line) or VAR_PATTERN.search(line)
                        if match:
                            name = match.group(1)
                            if name not in data:
                                data[name] = {"headers": set(), "bodies": set(), "used_in": set()}

                            if file.endswith('.h'):
                                data[name]["headers"].add(path)
                            else:
                                data[name]["bodies"].add(path)
            except Exception:
                continue

    # Final pass for usages
    for path, content in pending_usages:
        display_path = (path[:75] + '..') if len(path) > 77 else path
        print(f"\r\x1b[K[1] Indexing: \x1b[94m{display_path}\x1b[0m", end="", flush=True)

        for match in USAGE_PATTERN.finditer(content):
            name = match.group(1)
            if name in data:
                data[name]["used_in"].add(path)

    print(f"\r\x1b[KIndex complete: {file_count} files processed, {len(data)} items mapped.\n")

def query_loop():
    while True:
        try:
            query = input("\x1b[1;34mIdentifier > \x1b[0m").strip()

            if query.lower() in ("quit", "exit", "q"):
                break

            if not query:
                continue

            query = query.replace("()", "")

            # Wildcard logic
            if "*" in query:
                print(f"\n\x1b[90mPatterns matching '{query}'\x1b[0m")

                # Convert glob-style * to regex
                # escape other chars but keep * as .*
                pattern_str = "^" + re.escape(query).replace(r"\*", ".*") + "$"
                pattern = re.compile(pattern_str)

                matches = [name for name in data.keys() if pattern.match(name)]

                if matches:
                    for m in sorted(matches):
                        print(f" * \x1b[1m{m}\x1b[0m")
                else:
                    print("\x1b[91mNo matches found.\x1b[0m")
                print()
                continue

            # Exact match logic (Original)
            if query in data:
                info = data[query]

                print()
                for h in sorted(info["headers"]):
                    print(f"\x1b[1;33m[Header]\x1b[0m {h}")
                for b in sorted(info["bodies"]):
                    print(f"\x1b[1;36m[Body]  \x1b[0m {b}")

                print("\x1b[1m[Used in]\x1b[0m")
                if info["used_in"]:
                    for u in sorted(info["used_in"]):
                        status = "(internal)" if (u in info["headers"] or u in info["bodies"]) else ""
                        print(f"  {u} {status}")
                else:
                    print("  No usages found.")

                print()
            else:
                suggestions = difflib.get_close_matches(query, data.keys(), n=5, cutoff=0.6)
                if not suggestions:
                    suggestions = [name for name in data.keys() if name.startswith(query) or name.endswith(query)]
                    suggestions = sorted(suggestions)[:5]

                print(f"\x1b[91m'{query}' not found.\x1b[0m\n")
                if suggestions:
                    suggestions = [s for s in suggestions if s != query]
                    print(f"\x1b[33mDid you mean:\x1b[0m")
                    for s in suggestions:
                        print(f" * {s}")
                print()
        except KeyboardInterrupt:
            break
