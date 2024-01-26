#!/bin/sh
## @file
#
# VirtualBox postdeinstall script for FreeBSD.
#

#
# Copyright (C) 2007-2023 Oracle and/or its affiliates.
#
# This file is part of VirtualBox base platform packages, as
# available from https://www.virtualbox.org.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, in version 3 of the
# License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses>.
#
# SPDX-License-Identifier: GPL-3.0-only
#

echo "Removing kernel modules, please wait..."

kldunload vboxnetadp
kldunload vboxnetflt
kldunload vboxdrv
rm /boot/kernel/vboxnetflt.ko
rm /boot/kernel/vboxnetadp.ko
rm /boot/kernel/vboxdrv.ko
kldxref -R /boot

echo "Kernel modules successfully removed."

exit 0

