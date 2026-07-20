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
from cryptography.hazmat.primitives.asymmetric import ed25519
from cryptography.hazmat.primitives import serialization

from fxtools.core import printer
from fxtools.core import statejson

def run():
    keys_dir = os.path.expanduser("~/.farix")

    priv_path = os.path.join(keys_dir, ".private_key")
    pub_path = os.path.join(keys_dir, ".public_key")

    # If the keys already exist, just read them
    if os.path.exists(priv_path) and os.path.exists(pub_path):
        with open(pub_path, "r", encoding="utf-8") as f:
            pub_key_str = f.read().strip()
        print(f"Public key:\n{pub_key_str}")
        return

    os.makedirs(keys_dir, exist_ok=True)

    private_key = ed25519.Ed25519PrivateKey.generate()
    public_key = private_key.public_key()

    # Save the private key file safely
    private_bytes = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )
    with open(priv_path, "wb") as f:
        f.write(private_bytes)
    os.chmod(priv_path, 0o600) # Only the user can read/write this file

    # Save the public key file cleanly
    public_bytes = public_key.public_bytes(
        encoding=serialization.Encoding.OpenSSH,
        format=serialization.PublicFormat.OpenSSH
    )
    pub_key_str = public_bytes.decode('utf-8')
    with open(pub_path, "w", encoding="utf-8") as f:
        f.write(pub_key_str)

    data = statejson.get()
    data["USER_SIGN_FILES"] = keys_dir
    statejson.save(data)
    statejson.flush()

    printer.success(f"Private key saved to '{printer.F_GREEN(priv_path)}' (public key is saved alongside)")
    print(f"Public key:\n{pub_key_str}")

def help():
    return {
        "USAGE": "fx sign setup",
        "DESCRIPTION": "Generate your unique public and private key, and store them in your disk.",
        "NOTES": [
            "For safety reasons, this command does not create the files into the project folder.",
            "Do NOT lose EITHER of your keys; back them up, and don't let anyone steal them."
        ]
    }
