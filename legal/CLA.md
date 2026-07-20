# Contributor Licence Agreement (CLA)

### 1. Purpose

This Agreement governs all contributions to the Farix project. By submitting a contribution, you agree to these terms to ensure Farix remains sustainable and capable of dual-licencing (AGPLv3 and private commercial terms). This framework ensures the Farix project remains entirely free of cost for all open-source users while establishing the unified intellectual property rights required to maintain the dual-licencing architecture. Acceptance of these terms is a prerequisite for all contributions.

### 2. Grant of Rights

* **Copyright Licence:** You grant the Project Owner a perpetual, worldwide, irrevocable, royalty-free, and sub-licensable licence to use, modify, reproduce, and distribute your contribution.
* **Patent Licence:** You grant the Project Owner a perpetual, worldwide, irrevocable, royalty-free, and sub-licensable patent licence to make, use, sell, offer for sale, or import your contribution, ensuring the project and its users remain free from intellectual property claims.
* **Dual-Licencing:** You explicitly authorise the Project Owner to distribute your contribution under both the GNU AGPLv3 and any private commercial licences negotiated by the Project Owner.

### 3. Originality and Warranty
* **Ownership:** You certify that your contribution is your original work and you possess all necessary rights to grant the licences herein.
* **Third-Party Code:** Contributions containing third-party code must be documented with all required attributions in [`/legal/ATTRIBUTIONS.txt`](./ATTRIBUTIONS.txt).
* **Encumbrances:** You certify your contribution is free of any third-party claims, patent encumbrances, or conflicting intellectual property obligations.
* **No Warranty:** Contributions are provided "as-is" without warranty.

### 4. Contribution Standards
* **Licence Header:** Every new file must begin with the following header (with appropriate commenting):
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
* **Source Integrity:** To ensure kernel stability and licencing boundary integrity, the project tracks deterministic checksums. Every commit message must include the checksum of the development milestone in the changelog footer (which can be acquired via `fx lint`).
* **Sign-Off:** By using `fx commit` to sign and push a contribution, the contributor acknowledges they have read, understood, and agreed to be legally bound by this Contributor Licence Agreement.
