# Farix Kernel Licencing Policy & Additional Permissions

**The "project owner", throughout the project repository, is the individual that owns and maintains the "Farix" project in the "project repository" in [github.com/farismuhammad17/farix](https://github.com/farismuhammad17/farix) or its official successors, and whose cryptographic identity can be verified by the master public key defined in [legal/AUTH.md](./AUTH.md), unless further notices, signed by the master public key, were made in the aforementioned repository.**

The public repository source code for the Farix project is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0). As the sole Copyright Holder of this software, the project owner provides this source code subject to the following binding definitions and clarifications regarding derivative works and distribution boundaries. The goal is to ensure that the project remains open-source and free of cost to every casual user using Farix acquired from the project repository.

## Part A: Additional Licence Permissions (AGPLv3 Section 7)

* **Section 0–3 (Permissions):** You have the explicit right to run, copy, and modify this software for personal, educational, or open-source projects.
* **Section 4–5 (Source & Version Integrity):** You can distribute your modifications, provided you keep all original copyright notices and clearly mark the dates of your changes.
* **Section 6 (Binary Distribution):** If you distribute compiled binaries, you must provide a straightforward, free way for users to download the corresponding source code.
* **Section 7 (Architectural Boundary & Physical Hardware Driver Exception):**
  - ***Combined Derivative Work:*** Except as explicitly permitted under the "Proprietary Hardware Driver Exception" below, any third-party software component that executes in a privileged hardware execution level(s), maps into or directly manipulates the Farix kernel address space(s), operates within a shared execution context, or links against, includes, or replicates data structures from Farix kernel headers constitutes a Combined Derivative Work and is strictly subject to the AGPLv3 copyleft terms.
  - ***Proprietary Hardware Driver Exception:*** To foster hardware compatibility, third-party manufacturers may develop and distribute proprietary, closed-source kernel modules or drivers that execute in a privileged hardware execution level(s), provided they strictly comply with **ALL** of the following conditions:-
    1. The module serves solely to enable communication with a physical hardware component by implementing and populating the kernel's existing, built-in device abstraction structures provided for hardware initialisation and utilisation.
    2. The module interfaces with the core kernel exclusively by registering its populated hardware structures via the kernel's internal device registration and management routines, utilising only pre-defined, built-in device type classifications.
    3. The module interacts with other system components solely via official internal kernel device APIs or standard system calls, as defined in the source code of the project repository, and does not alter, intercept, or extend core kernel execution logic.
    4. The compiled binary is made fully available to the general public completely free of charge, without functional restriction, activation locks, or tied monetisation mechanisms.
    *Any module that introduces new unapproved execution paths, bypasses the standard device registration framework, or fails to meet these conditions is an infringing derivative work. If an exception is desired, contact the project owner for further discussion.*
  - ***User-Space Applications:*** Standard user-space applications interacting with the kernel solely via public system calls (syscalls), as defined in the source code of the project repository, are completely exempted from copyleft requirements.
* **Section 13 (The Network Rule):** If any modification is made to run the program on a server for users to interact with over a network, the modified source code **must** be made publicly available to the users free of charge.
* **Section 15–17 (No Warranty):** The software is provided "as-is." The author is not liable for any damages, system failures, or data loss that occur during deployment.

## Part B: Commercial Terms & Proprietary Deployment

*The project owner holds the sole copyright and rights to this project via contributor agreements. The AGPLv3 terms and the additional permissions in the earlier "Part A: Additional Licence Permissions (AGPLv3 Section 7)" section apply strictly to open-source and casual distribution.*

* **Commercial Exception:** You must secure a private commercial licence from the project owner to bypass any of these terms if:-
  1. ***Product Integration & Commercial Use:*** You intend to incorporate Farix into a proprietary product, device, commercial operating system distribution, or hardware solution intended for commercial sale, lease, distribution, or revenue-generating internal business operations.
  2. ***Licence Compliance:*** You are unable or unwilling to comply with the AGPLv3 licence terms or the conditional exceptions stated herein.

---

**Contact Email:** `farismuhd172009@gmail.com`  
**Subject Line Standard:** `Farix Licencing - <Organisation Name>`
