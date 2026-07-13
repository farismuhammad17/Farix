# Contributor License Agreement (CLA)

### 1. Purpose

This Agreement governs all contributions to the Farix project. By submitting a contribution, you agree to these terms to ensure Farix remains sustainable and capable of dual-licensing (AGPLv3 and private commercial terms).

### 2. Grant of Rights

You grant the Project Owner a perpetual, worldwide, irrevocable, royalty-free, and sublicensable license to:
* Use, modify, reproduce, and distribute your contribution.
* **Dual-Licensing:** Explicitly distribute your contribution under both the GNU AGPLv3 and any private commercial licenses negotiated by the Project Owner.

### 3. Originality and Warranty
* **Ownership:** You certify that your contribution is your original work and you possess all necessary rights to grant the licenses herein.
* **Third-Party Code:** Contributions containing third-party code must be documented with all required attributions in [`/legal/ATTRIBUTIONS.txt`](./ATTRIBUTIONS.txt).
* **Encumbrances:** You certify your contribution is free of any third-party claims or conflicting intellectual property obligations.
* **No Warranty:** Contributions are provided "as-is" without warranty.

### 4. Contribution Standards
* **License Header:** Every new file must begin with the following header (with appropriate commenting):
    ```
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
    ```
* **Source Integrity:** To ensure kernel stability and licensing boundary integrity, the project tracks deterministic checksums. Every commit message must include the checksum of the development milestone in the changelog footer.
* **Sign-Off:** To signal your agreement to this CLA, every commit message must end with:
    ```
    Contributor: [Your Name] <[Your Email]>
    ```
