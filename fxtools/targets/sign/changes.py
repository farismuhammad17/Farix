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
import base64
from cryptography.hazmat.primitives import serialization

from fxtools.core import printer
from fxtools.core import statejson

from fxtools.targets.lint import get_checksum

def generate_repository_signature():
    checksum = get_checksum()

    # Locate the private key directory from your state configuration
    state_data = statejson.get()
    keys_dir = state_data.get("USER_SIGN_FILES")
    if not keys_dir:
        printer.error("Configuration key 'USER_SIGN_FILES' missing from state JSON.")
        return ''

    priv_path = os.path.join(keys_dir, ".private_key")
    if not os.path.exists(priv_path):
        printer.error(f"No private key found at {priv_path}. Run 'fx sign --identify' first")
        return ''

    # Load the private key object from disk
    with open(priv_path, "rb") as f:
        private_key = serialization.load_pem_private_key(
            f.read(),
            password=None
        )

    # Convert the hex checksum string to raw bytes and sign it
    # Ed25519 signs the 64-character hash value directly
    checksum_bytes = checksum.encode("utf-8")
    raw_signature_bytes = private_key.sign(checksum_bytes)

    # Convert the 64 raw binary bytes into a clean, text-safe Base64 string
    base64_signature_string = base64.b64encode(raw_signature_bytes).decode("utf-8")

    return base64_signature_string

def run():
    print(generate_repository_signature())

def help():
    return {
        "USAGE": "fx sign changes",
        "DESCRIPTION": "Uses the current checksum to create a unique signature for the executor"
    }
