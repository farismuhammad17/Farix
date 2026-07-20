# Project Authentication

The following OpenSSH Ed25519 public key is the absolute cryptographic signature for the project owner of the Farix project, i.e. the "master key". Any claim of official distribution, legal relicensing, or authentic project ownership must be validated against this key:

```
ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIJgSdS+p4OAUc5tWpnr5VEROMYAMA1GJVpwLPTocBdgv
```

Any version of this codebase lacking verification against this root key, or attempting to alter this key without an authenticated upstream chain of custody, constitutes an unofficial fork and cannot claim status as the official project repository under the Farix Kernel Licensing Policy.

## Verification

The `fx sign` utility provides the security verification layer for Farix. Use the following operations to interact with the cryptographic subsystem.

```bash
# To verify a file:
fx sign verify file <-f> <-sig> <-key>
```

```bash
# To verify a commit:
fx sign verify commit <-sig> <-key>
```

Use internal documentation for `fx` for futher information.
