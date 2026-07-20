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

from fxtools.core import printer
from fxtools.core import statejson

def run(f: str):
    if not os.path.exists(f):
        printer.error(f"File not found: {f}")
        return

    # Resolve the private key path using your state storage configuration
    state_data = statejson.get()
    keys_dir = state_data.get("USER_SIGN_FILES")

    if not keys_dir or not os.path.exists(keys_dir):
        printer.error("Keys could not be found")
        print("set valid 'USER_SIGN_FILES' in state JSON or generate using 'fx sign setup'")
        return

    priv_path = os.path.join(keys_dir, ".private_key")

    if not os.path.exists(priv_path):
        printer.error(f"Private key missing at {priv_path}. Run 'fx sign --identify' first.")
        print("set valid 'USER_SIGN_FILES' in state JSON or generate using 'fx sign setup'")
        return

    # Load the private key from disk
    with open(priv_path, "rb") as private_key_file:
        private_key = serialization.load_pem_private_key(
            private_key_file.read(),
            password=None
        )

    # Read the target file's raw data contents entirely as binary bytes
    with open(f, "rb") as target_file:
        file_bytes = target_file.read()

    # Generate the cryptographic signature block from the file bytes
    signature_bytes = private_key.sign(file_bytes)

    # Write the signature bytes out to a companion .sig file
    sig_path = f"{os.path.basename(f)}.sig"
    with open(sig_path, "wb") as signature_file:
        signature_file.write(signature_bytes)

    print(f"Saved signature to '{sig_path}'")

def help():
    return {
        "USAGE": "fx sign file <-f>",
        "DESCRIPTION": "Creates a unique signature (.sig) file for the given file using the unique keys of the executor.",
        "ARGS": {
            "f": "Path to the file you want to generate the signature file for"
        }
    }
