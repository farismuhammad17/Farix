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

from fxtools.core import printer

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

def run():
    try:
        # Query git using -z to get raw, unquoted paths separated by null bytes (\0)
        result = subprocess.run(
            ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'],
            capture_output=True,
            check=True
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        printer.error("Error: git failed")
        return

    raw_paths = result.stdout.split(b'\x00') if result.stdout else []

    file_paths = []
    for path_bytes in raw_paths:
        if not path_bytes:
            continue
        try:
            path_str = path_bytes.decode('utf-8').strip()
            if path_str.endswith(extensions):
                file_paths.append(path_str)
        except UnicodeDecodeError:
            continue

    # Sort ascending to guarantee cross-platform deterministic order
    file_paths.sort()

    hasher = hashlib.sha256()

    for path in file_paths:
        # Check physical existence first to handle deleted unstaged files consistently
        if not os.path.exists(path):
            continue

        # Normalize the structural path representation
        normalized_path = path.replace(os.sep, '/')
        hasher.update(normalized_path.encode('utf-8'))

        # Process the contents in binary mode to neutralize CRLF/LF variations
        with open(path, 'rb') as f:
            while chunk := f.read(8192):
                normalized_chunk = chunk.replace(b'\r\n', b'\n')
                hasher.update(normalized_chunk)

    print(hasher.hexdigest())

def help():
    return {
        "USAGE": "fx checksum",
        "DESCRIPTION": "Generates a deterministic SHA-256 cryptographic fingerprint of the Farix source tree.\n"
                       "This unique ID maps directly to the current state of the code and legal boundaries,\n"
                       "ensuring absolute tamper-verification."
    }
