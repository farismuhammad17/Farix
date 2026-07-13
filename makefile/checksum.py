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
import subprocess
import hashlib

extensions = (
    '.c',
    '.h',
    '.asm',
    '.s',
    '.ld',
    '.py',
    '.cmd',
    '.bat',
    '.env',
    'Dockerfile',
    'CONTRIBUTING.md',
    'DOCUMENTATION.md',
    'LICENSE',
    'legal/ATTRIBUTIONS.txt',
    'legal/CLA.md',
    'legal/LICENSE_EXCEPTIONS.md'
)

def print_src_checksum():
    try:
        # Ask git to list all tracked and untracked files, automatically respecting .gitignore
        # --cached: tracked files
        # --others: untracked files
        # --exclude-standard: applies .gitignore, .git/info/exclude, etc.
        # It would be unfair to judge by files not in the source code
        result = subprocess.run(
            ['git', 'ls-files', '--cached', '--others', '--exclude-standard'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("\x1b[31mError: git failed\x1b[0m")
        return

    # Filter by your specified extensions
    file_paths = [
        line.strip() for line in result.stdout.splitlines()
        if line.strip().endswith(extensions)
    ]

    # Sort ascending to guarantee cross-platform deterministic order
    file_paths.sort()

    hasher = hashlib.sha256()

    for path in file_paths:
        # Normalize path strings to use forward slashes (git ls-files usually does this anyway)
        normalized_path = path.replace(os.sep, '/')
        hasher.update(normalized_path.encode('utf-8'))

        # Stream the file contents in binary mode
        if os.path.exists(path):
            with open(path, 'rb') as f:
                while chunk := f.read(8192):
                    hasher.update(chunk)

    print(hasher.hexdigest())
