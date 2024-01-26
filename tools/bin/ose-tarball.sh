#!/bin/sh
# $Id$
## @file
# Use this script in conjunction with snapshot-ose.sh
#

#
# Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

vboxdir=`pwd`
if [ ! -r "$vboxdir/Config.kmk" -o ! -r "$vboxdir/Doxyfile.Core" ]; then
  echo "Is $vboxdir really a VBox tree?"
  exit 1
fi
if [ -r "$vboxdir/src/VBox/RDP/server/server.cpp" ]; then
  echo "Found RDP stuff, refused to build OSE tarball!"
  exit 1
fi
vermajor=`grep "^VBOX_VERSION_MAJOR *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verminor=`grep "^VBOX_VERSION_MINOR *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verbuild=`grep "^VBOX_VERSION_BUILD *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verpre=`grep "^VBOX_VERSION_PRERELEASE *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verpub=`grep "^VBOX_BUILD_PUBLISHER *=" "$vboxdir/Version.kmk"|sed -e "s|.*= *\(.*\)|\1|g"`
verstr="$vermajor.$verminor.$verbuild"
[ -n "$verpre" ] && verstr="$verstr"_"$verpre"
[ -n "$verpub" ] && verstr="$verstr$verpub"
rootpath=`cd ..;pwd`
rootname="VirtualBox-$verstr"
if [ $# -eq 1 ]; then
    tarballname="$1"
else
    tarballname="$rootpath/$rootname.tar.bz2"
fi
rm -f "$rootpath/$rootname"
ln -s `basename "$vboxdir"` "$rootpath/$rootname"
if [ $? -ne 0 ]; then
  echo "Cannot create root directory link!"
  exit 1
fi
tar \
  --create \
  --bzip2 \
  --dereference \
  --owner 0 \
  --group 0 \
  --totals \
  --exclude=.svn \
  --exclude="$rootname/out" \
  --exclude="$rootname/env.sh" \
  --exclude="$rootname/configure.log" \
  --exclude="$rootname/build.log" \
  --exclude="$rootname/AutoConfig.kmk" \
  --exclude="$rootname/LocalConfig.kmk" \
  --exclude="$rootname/prebuild" \
  --directory "$rootpath" \
  --file "$tarballname" \
  "$rootname"
echo "Successfully created $tarballname"
rm -f "$rootpath/$rootname"
