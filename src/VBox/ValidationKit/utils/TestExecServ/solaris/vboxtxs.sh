#!/bin/sh
## @file
# VirtualBox Test Execution Service Architecture Wrapper for Solaris.
#

#
# Copyright (C) 2010-2023 Oracle and/or its affiliates.
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
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
# in the VirtualBox distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#
# SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
#

# 1. Change directory to the script directory (usually /opt/VBoxTest/).
set -x
MY_DIR=`dirname "$0"`
cd "${MY_DIR}"

# 2. Determine the architecture.
MY_ARCH=`isainfo -k`
case "${MY_ARCH}" in
    amd64)
        MY_ARCH=amd64
        ;;
    i386)
        MY_ARCH=x86
        ;;
    *)
        echo "vboxtxs.sh: Unsupported architecture '${MY_ARCH}' returned by isainfo -k." >&2
        exit 2;
        ;;
esac

# 3. Exec the service.
exec "./${MY_ARCH}/TestExecService" \
    --cdrom="/cdrom/cdrom0/" \
    --scratch="/var/tmp/VBoxTest/" \
    --no-display-output \
    $*
exit 3;

