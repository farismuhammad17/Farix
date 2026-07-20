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
from cryptography.hazmat.primitives import serialization
from cryptography.exceptions import InvalidSignature

from fxtools.core import printer

def run(f: str, sig: str, key: str):
    if not os.path.exists(f):
        printer.error(f"Could not find '{f}'")
        return
    if not os.path.exists(sig):
        printer.error(f"Could not find '{sig}'")
        return

    try:
        # Load the public key string directly
        public_bytes = key.strip().encode("utf-8")
        public_key = serialization.load_ssh_public_key(public_bytes)

        # Read the local target file's raw content
        with open(f, "rb") as file:
            file_bytes = file.read()

        with open(sig, "rb") as sig_file:
            sig_bytes = sig_file.read()

        public_key.verify(sig_bytes, file_bytes)

        printer.success("Valid signature")
    except (InvalidSignature, Exception):
        printer.error("Invalid signature")

def help():
    return {
        "USAGE": "fx sign verify file <-f> <-sig> <-key>",
        "DESCRIPTION": "Verify that the file and the corresponding .sig file matches the given key.",
        "ARGS": {
            "f": "Path to the literal file to check",
            "sig": "Path to the corresponding .sig file",
            "key": "Public key of the user you wish to check the signature against"
        }
    }
