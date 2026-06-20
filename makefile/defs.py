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
            if not file.endswith(('.c', '.h', '.asm', '.s')):
                continue

            path = os.path.join(root, file)
            display_path = (path[:75] + '..') if len(path) > 77 else path
            print(f"\r\x1b[K[0] Scanning: \x1b[94m{display_path}\x1b[0m", end="", flush=True)

            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    file_count += 1
                    pending_usages.append((path, content))

                    for line_idx, line in enumerate(content.splitlines()):
                        match = FUNC_DECL_PATTERN.search(line) or MACRO_PATTERN.search(line) or VAR_PATTERN.search(line)
                        if match:
                            name = match.group(1)
                            if name not in data:
                                data[name] = {"headers": set(), "bodies": set(), "used_in": set(), "lines": {}}

                            data[name]["lines"][path] = line_idx

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

def extract_docstring(path, line_idx):
    if not os.path.exists(path):
        return None

    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()

        # Move up from the declaration line, skipping empty space
        idx = line_idx - 1
        while idx >= 0 and not lines[idx].strip():
            idx -= 1

        if idx < 0 or "*/" not in lines[idx]:
            return None

        # Gather comment lines walking up until we hit the opening block marker
        comment_lines = []
        while idx >= 0:
            current_line = lines[idx]
            comment_lines.append(current_line)
            if "/*" in current_line:
                break
            idx -= 1

        if idx >= 0:
            comment_lines.reverse()
            # Clean up padding stars and formatting artifacts
            cleaned_lines = []
            for line in comment_lines:
                cleaned = line.strip().lstrip('/*').rstrip('*/').strip().lstrip('*').strip()
                if cleaned or len(cleaned_lines) > 0:  # Avoid trailing/leading whitespace blocks
                    cleaned_lines.append(cleaned)
            return "\n".join(cleaned_lines).strip()
    except Exception:
        return None
    return None

def fetch_and_print_docs(query, info):
    docstring = None
    resolved_source = ""

    # Priority 1: Check source bodies (.c)
    for path in info["bodies"]:
        if path in info["lines"]:
            docstring = extract_docstring(path, info["lines"][path])
            if docstring:
                resolved_source = path
                break

    # Priority 2: Check definitions inside header files (.h)
    if not docstring:
        for path in info["headers"]:
            if path in info["lines"]:
                docstring = extract_docstring(path, info["lines"][path])
                if docstring:
                    resolved_source = path
                    break

    print(f"\x1b[1;32m[Documentation] \x1b[0;90m{os.path.basename(resolved_source)}\x1b[0m")
    if docstring:
        # Indent docstring text cleanly for terminal alignment
        print("\n".join(f" | {line}" for line in docstring.splitlines()))
    else:
        print("  \x1b[90mNo docstring found\x1b[0m")

def query_loop():
    while True:
        try:
            query = input("\x1b[1;34m>>> \x1b[0m").strip()

            if not query:
                continue

            query = query.replace("()", "")

            # Wildcard logic
            if "*" in query:
                print(f"\n\x1b[90mPatterns matching '{query}'\x1b[0m")
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

            # Exact match logic
            if query in data:
                info = data[query]
                print()

                # Fetch documentation blocks first
                fetch_and_print_docs(query, info)
                print()

                for h in sorted(info["headers"]):
                    print(f"\x1b[1;33m[Header]\x1b[0m {h}")
                for b in sorted(info["bodies"]):
                    print(f"\x1b[1;36m[Body]\x1b[0m   {b}")

                print("\x1b[1m[Used in]\x1b[0m")
                if info["used_in"]:
                    for u in sorted(info["used_in"]):
                        status = "(internal)" if (u in info["headers"] or u in info["bodies"]) else ""
                        print(f" | {u} \x1b[90m{status}\x1b[0m")
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
            print("\n")
            break
