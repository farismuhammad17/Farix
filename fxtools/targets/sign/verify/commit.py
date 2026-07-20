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

import base64
from cryptography.hazmat.primitives import serialization
from cryptography.exceptions import InvalidSignature

from fxtools.core import printer

from fxtools.targets.lint import get_checksum

def run(sig: str, key: str):
    try:
        checksum = get_checksum()
        if not checksum:
            printer.error("Could not acquire checksum")
            return False

        # Convert the Base64 signature text back into raw machine bytes
        signature_bytes = base64.b64decode(sig.strip().encode("utf-8"))

        # Load the copy-pasted public key string directly from text
        public_bytes = key.strip().encode("utf-8")
        public_key = serialization.load_ssh_public_key(public_bytes)

        # Authenticate the live checksum matches the signed fingerprint
        public_key.verify(signature_bytes, checksum.encode("utf-8"))

        printer.success("Valid signature")
    except (InvalidSignature, Exception):
        printer.error("Invalid signature")

def help():
    return {
        "USAGE": "fx sign verify commit <-sig> <-key>",
        "DESCRIPTION": "Verifies if a given commit signature is valid for the given key",
        "ARGS": {
            "sig": "Signature given for the commit; usually found in the commit message.",
            "key": "The public key of the user you wish to check the signature against."
        }
    }
