#!/usr/bin/env kmk_ash
# $Id: retry.sh $
## @file
# Script for retrying a command 5 times.
#

#
# Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

# First try (quiet).
"$@"
EXITCODE=$?
if [ "${EXITCODE}" = "0" ]; then
    exit 0;
fi

# Retries.
for RETRY in 2 3 4 5;
do
    echo "retry.sh: retry$((${RETRY} - 1)): exitcode=${EXITCODE};  retrying: $*"
    "$@"
    EXITCODE=$?
    if [ "${EXITCODE}" = "0" ]; then
        echo "retry.sh: Success after ${RETRY} tries: $*!"
        exit 0;
    fi
done
echo "retry.sh: Giving up: exitcode=${EXITCODE}  command: $@"
exit ${EXITCODE};

