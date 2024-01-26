#!/bin/sh
## @file
#
# VirtualBox postinstall script for FreeBSD.
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

PATH_TMP="/tmp"
PATH_TMP_MODS="$PATH_TMP/vbox_mods"
PATH_INST="/usr/local/lib/virtualbox"
PATH_KERN_SRC="/usr/src/sys"
FILE_VBOXDRV="$PATH_INST/vboxdrv.tar.gz"


if [ ! -f $PATH_KERN_SRC/Makefile ]; then
    echo "Kernel sources are not installed. Please install them and reinstall VirtualBox"
    exit 1
fi

echo "Compiling kernel modules, please wait..."

# Create temporary directory
mkdir -p $PATH_TMP_MODS

# Unpack archive
tar -C $PATH_TMP_MODS -xf  $FILE_VBOXDRV

# Compile
cd $PATH_TMP_MODS
make

# Check if we succeeded
if [ $? != 0 ]; then
    echo "Compiling kernel modules failed."
    cd ..
    rm -rf $PATH_TMP_MODS
    exit 1
fi

# Copy the modules to /boot/kernel
echo "Installing kernel modules to /boot/kernel, please wait..."
cp $PATH_TMP_MODS/vboxdrv.ko /boot/kernel
cp $PATH_TMP_MODS/vboxnetflt.ko /boot/kernel
cp $PATH_TMP_MODS/vboxnetadp.ko /boot/kernel
kldxref -R /boot

# Load them now, unloading old modules
make load

if [ $? != 0 ]; then
    echo "Loading kernel modules failed"
    cd ..
    rm -rf $PATH_TMP_MODS
    exit 1
fi

echo "Kernel modules successfully installed."
echo "To load them on every boot put them into /boot/loader.conf"

# Cleanup
cd ..
rm -rf $PATH_TMP_MODS

exit 0

