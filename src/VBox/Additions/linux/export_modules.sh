#!/bin/sh
# $Id$
## @file
# Create a tar archive containing the sources of the Linux guest kernel modules.
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

export LC_ALL=C

# The below is GNU-specific.  See VBox.sh for the longer Solaris/OS X version.
TARGET=`readlink -e -- "${0}"` || exit 1
MY_DIR="${TARGET%/[!/]*}"

if [ -z "${1}" ] || { [ "x${1}" = x--folder ] && [ -z "${2}" ]; }; then
    echo "Usage: $0 <filename.tar.gz>"
    echo "  Export VirtualBox kernel modules to <filename.tar.gz>."
    echo "Usage: $0 --folder <folder>"
    echo "  Copy VirtualBox kernel module source to <folder>."
    exit 1
fi

if test "x${1}" = x--folder; then
    PATH_OUT="${2}"
else
    PATH_OUT="`cd \`dirname $1\`; pwd`/.vbox_modules"
    FILE_OUT="`cd \`dirname $1\`; pwd`/`basename $1`"
fi
PATH_ROOT="`cd ${MY_DIR}/../../../..; pwd`"
PATH_LOG=/tmp/vbox-export-guest.log
PATH_LINUX="$PATH_ROOT/src/VBox/Additions/linux"
PATH_VBOXGUEST="$PATH_ROOT/src/VBox/Additions/common/VBoxGuest"
PATH_VBOXSF="$PATH_ROOT/src/VBox/Additions/linux/sharedfolders"
PATH_VBOXVIDEO="$PATH_ROOT/src/VBox/Additions/linux/drm"

VBOX_VERSION_MAJOR=`sed -e "s/^ *VBOX_VERSION_MAJOR *= \+\([0-9]\+\)/\1/;t;d" $PATH_ROOT/Version.kmk`
VBOX_VERSION_MINOR=`sed -e "s/^ *VBOX_VERSION_MINOR *= \+\([0-9]\+\)/\1/;t;d" $PATH_ROOT/Version.kmk`
VBOX_VERSION_BUILD=`sed -e "s/^ *VBOX_VERSION_BUILD *= \+\([0-9]\+\)/\1/;t;d" $PATH_ROOT/Version.kmk`
VBOX_SVN_CONFIG_REV=`sed -e 's/^ *VBOX_SVN_REV_CONFIG_FALLBACK *:= \+\$(patsubst *%:,, *\$Rev: *\([0-9]\+\) *\$ *) */\1/;t;d' $PATH_ROOT/Config.kmk`
VBOX_SVN_VERSION_REV=`sed -e 's/^ *VBOX_SVN_REV_VERSION_FALLBACK *:= \+\$(patsubst *%:,, *\$Rev: *\([0-9]\+\) *\$ *) */\1/;t;d' $PATH_ROOT/Version.kmk`
if [ "$VBOX_SVN_CONFIG_REV" -gt "$VBOX_SVN_VERSION_REV" ]; then
    VBOX_SVN_REV=$VBOX_SVN_CONFIG_REV
else
    VBOX_SVN_REV=$VBOX_SVN_VERSION_REV
fi
VBOX_VENDOR=`sed -e 's/^ *VBOX_VENDOR *= \+\(.\+\)/\1/;t;d' $PATH_ROOT/Config.kmk`
VBOX_VENDOR_SHORT=`sed -e 's/^ *VBOX_VENDOR_SHORT *= \+\(.\+\)/\1/;t;d' $PATH_ROOT/Config.kmk`
VBOX_PRODUCT=`sed -e 's/^ *VBOX_PRODUCT *= \+\(.\+\)/\1/;t;d' $PATH_ROOT/Config.kmk`
VBOX_C_YEAR=`date +%Y`

. $PATH_VBOXGUEST/linux/files_vboxguest
. $PATH_VBOXSF/files_vboxsf
. $PATH_VBOXVIDEO/files_vboxvideo_drv

# Temporary path for creating the modules, will be removed later
mkdir -p $PATH_OUT || exit 1

# Create auto-generated version file, needed by all modules
echo "#ifndef ___version_generated_h___" > $PATH_OUT/version-generated.h
echo "#define ___version_generated_h___" >> $PATH_OUT/version-generated.h
echo "" >> $PATH_OUT/version-generated.h
echo "#define VBOX_VERSION_MAJOR $VBOX_VERSION_MAJOR" >> $PATH_OUT/version-generated.h
echo "#define VBOX_VERSION_MINOR $VBOX_VERSION_MINOR" >> $PATH_OUT/version-generated.h
echo "#define VBOX_VERSION_BUILD $VBOX_VERSION_BUILD" >> $PATH_OUT/version-generated.h
echo "#define VBOX_VERSION_STRING_RAW \"$VBOX_VERSION_MAJOR.$VBOX_VERSION_MINOR.$VBOX_VERSION_BUILD\"" >> $PATH_OUT/version-generated.h
echo "#define VBOX_VERSION_STRING \"$VBOX_VERSION_MAJOR.$VBOX_VERSION_MINOR.$VBOX_VERSION_BUILD\"" >> $PATH_OUT/version-generated.h
echo "#define VBOX_API_VERSION_STRING \"${VBOX_VERSION_MAJOR}_${VBOX_VERSION_MINOR}\"" >> $PATH_OUT/version-generated.h
echo "#define VBOX_PRIVATE_BUILD_DESC \"Private build with export_modules\"" >> $PATH_OUT/version-generated.h
echo "" >> $PATH_OUT/version-generated.h
echo "#endif" >> $PATH_OUT/version-generated.h

# Create auto-generated revision file, needed by all modules
echo "#ifndef __revision_generated_h__" > $PATH_OUT/revision-generated.h
echo "#define __revision_generated_h__" >> $PATH_OUT/revision-generated.h
echo "" >> $PATH_OUT/revision-generated.h
echo "#define VBOX_SVN_REV $VBOX_SVN_REV" >> $PATH_OUT/revision-generated.h
echo "" >> $PATH_OUT/revision-generated.h
echo "#endif" >> $PATH_OUT/revision-generated.h

# Create auto-generated product file, needed by all modules
echo "#ifndef ___product_generated_h___" > $PATH_OUT/product-generated.h
echo "#define ___product_generated_h___" >> $PATH_OUT/product-generated.h
echo "" >> $PATH_OUT/product-generated.h
echo "#define VBOX_VENDOR \"$VBOX_VENDOR\"" >> $PATH_OUT/product-generated.h
echo "#define VBOX_VENDOR_SHORT \"$VBOX_VENDOR_SHORT\"" >> $PATH_OUT/product-generated.h
echo "" >> $PATH_OUT/product-generated.h
echo "#define VBOX_PRODUCT \"$VBOX_PRODUCT\"" >> $PATH_OUT/product-generated.h
echo "#define VBOX_C_YEAR \"$VBOX_C_YEAR\"" >> $PATH_OUT/product-generated.h
echo "" >> $PATH_OUT/product-generated.h
echo "#endif" >> $PATH_OUT/product-generated.h

# vboxguest (VirtualBox guest kernel module)
mkdir $PATH_OUT/vboxguest || exit 1
for f in $FILES_VBOXGUEST_NOBIN; do
    install -D -m 0644 `echo $f|cut -d'=' -f1` "$PATH_OUT/vboxguest/`echo $f|cut -d'>' -f2`"
done
for f in $FILES_VBOXGUEST_BIN; do
    install -D -m 0755 `echo $f|cut -d'=' -f1` "$PATH_OUT/vboxguest/`echo $f|cut -d'>' -f2`"
done

# vboxsf (VirtualBox guest kernel module for shared folders)
mkdir $PATH_OUT/vboxsf || exit 1
for f in $FILES_VBOXSF_NOBIN; do
    install -D -m 0644 `echo $f|cut -d'=' -f1` "$PATH_OUT/vboxsf/`echo $f|cut -d'>' -f2`"
done
for f in $FILES_VBOXSF_BIN; do
    install -D -m 0755 `echo $f|cut -d'=' -f1` "$PATH_OUT/vboxsf/`echo $f|cut -d'>' -f2`"
done

# vboxvideo (VirtualBox guest kernel module for drm support)
mkdir $PATH_OUT/vboxvideo || exit 1
for f in $FILES_VBOXVIDEO_DRM_NOBIN; do
    install -D -m 0644 `echo $f|cut -d'=' -f1` "$PATH_OUT/vboxvideo/`echo $f|cut -d'>' -f2`"
done
for f in $FILES_VBOXVIDEO_DRM_BIN; do
    install -D -m 0755 `echo $f|cut -d'=' -f1` "$PATH_OUT/vboxvideo/`echo $f|cut -d'>' -f2`"
done
sed -f $PATH_VBOXVIDEO/indent.sed -i $PATH_OUT/vboxvideo/*.[ch]

# convenience Makefile
install -D -m 0644 $PATH_LINUX/Makefile "$PATH_OUT/Makefile"

# Only temporary, omit from archive
rm $PATH_OUT/version-generated.h
rm $PATH_OUT/revision-generated.h
rm $PATH_OUT/product-generated.h

# If we are exporting to a folder then stop now.
test "x${1}" = x--folder && exit 0

# Do a test build
echo Doing a test build, this may take a while.
make -C $PATH_OUT > $PATH_LOG 2>&1 &&
    make -C $PATH_OUT clean >> $PATH_LOG 2>&1 ||
    echo "Warning: test build failed.  Please check $PATH_LOG"

# Create the archive
tar -czf $FILE_OUT -C $PATH_OUT . || exit 1

# Remove the temporary directory
rm -r $PATH_OUT

