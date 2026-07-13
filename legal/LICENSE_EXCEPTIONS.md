# License Exceptions

*Ignorance of licensing terms, repository documentation, or failure to adhere to the dual-license does not establish any liability on the part of the project owner.*

**The "project owner", throughout the project repository, is the individual that owns and maintains the "Farix" project in the "project repository" in [github.com/farismuhammad17/farix](https://github.com/farismuhammad17/farix).**

The public repository source code for the Farix project is licensed under the GNU Affero General Public License v3.0 (AGPL-3.0). As the sole Copyright Holder of this software, the project owner provides this source code subject to the following binding definitions and clarifications regarding derivative works and distribution boundaries. The goal is to ensure that the project remains open-source and free of cost to every casual user using Farix acquired from the project repository.

* **Section 0–3 (Permissions):** You have the explicit right to run, copy, and modify this software for personal, educational, or open-source projects.
* **Section 4–5 (Source & Version Integrity):** You can distribute your modifications, provided you keep all original copyright notices and clearly mark the dates of your changes.
* **Section 6 (Binary Distribution):** If you distribute compiled binaries, you must provide a straightforward, free way for users to download the corresponding source code.
* **Section 7 (Architectural Boundary):**
  - ***Combined Derivative Work:*** Any third-party software that executes in supervisor mode, maps into the Farix kernel address space, or statically links against private kernel headers is a "Combined Derivative Work." Such components must be licensed under the AGPLv3.
  - ***Proprietary drivers:*** To support proprietary software, components that execute in user-space and communicate via documented Standardized Abstraction Interfaces may be distributed as closed-source binaries only if they comply with all of the following:
    1. The binary is made available to the public free of charge.
    2. The binary executes in its own address space without modifying kernel logic.
  - ***Exceptions:*** User-space applications interacting solely via public system calls as defined in the source code of the project repository are excluded from these requirements.
* **Section 13 (The Network Rule):** If any modification is made to run the program on a server for users to interact with over a network, the modified source code **must** be made publicly available to them.
* **Section 15–17 (No Warranty):** The software is provided "as-is." The author is not liable for any damages, system failures, or data loss that occur during deployment.
* **Commercial Exception:** You must secure a private commercial license from the project owner to bypass any of these terms if:-
  1. ***Product Integration:*** You intend to incorporate Farix into a proprietary product, device, or hardware solution intended for commercial sale or distribution.
  2. ***License Compliance:*** You are unable to comply with the AGPLv3 license terms as stated.
