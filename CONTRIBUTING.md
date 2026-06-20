# Contributing 

A rare specimen *(noun)*: An individual willing to improve the Farix source code. If you are indeed one of these, please do note that upon opening a Pull Request, submitting a patch, or contributing code to this repository:-

1. **Code**
  * **Style:** The code you provide must pass through the linter. Documentation specifies required coding style.
2. **License**
  * **Grant of Rights:** You grant the owner of the project, Faris Muhammad, a perpetual, worldwide, royalty-free, irrevocable license to use, modify, and distribute your contributions. This allows the project to remain free of cost, while still maintaining the dual license.
  * **Dual-License Support:** You agree that your code can be distributed under the project's open-source license (GNU AGPLv3) AND included in separate, paid commercial licenses sold to enterprises.
  * **Original Work:** You certify that the code you are submitting is entirely your own original work and does not infringe on any third-party copyrights or proprietary licenses.
  * **License header:** Every file unique to the project must begin with the license header, using appropriate commenting, as follows:-
  ```c
  /*
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
  */
  ```
3. **Source integrity:** To guarantee that the core kernel subsystems have not been tampered with or modified to bypass licensing boundaries, the Farix project tracks deterministic tree checksums for every major development milestone. It is required to pass this hash for every commit at the bottom of the changelog, and the latest hash for the log be at the top.
