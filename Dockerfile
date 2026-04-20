# -----------------------------------------------------------------------
# Copyright (C) 2026 Faris Muhammad

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
binutils \
build-essential \
dosfstools \
gcc-arm-none-eabi \
gcc-i686-linux-gnu \
git \
grub-common \
grub-pc-bin \
make \
mtools \
nasm \
python3 \
qemu-system-arm \
qemu-system-x86 \
xorriso \
&& apt-get clean

RUN echo '#!/bin/sh\npython3 /root/make.py "$@"' > /usr/bin/m && chmod +x /usr/bin/m

WORKDIR /root
