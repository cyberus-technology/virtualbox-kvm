#!/bin/sh

#
# Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#
# Wrapper for the per user autostart daemon. Gets a list of all users
# and starts the VMs.
#

function vboxStartStopAllUserVms()
{
    # Go through the list and filter out all users without a shell and a
    # non existing home.
    for user in `dscl . -list /Users`
    do
        HOMEDIR=`dscl . -read /Users/"${user}" NFSHomeDirectory | sed 's/NFSHomeDirectory: //g'`
        USERSHELL=`dscl . -read /Users/"${user}" UserShell | sed 's/UserShell: //g'`

        # Check for known home directories and shells for daemons
        if [[   "${HOMEDIR}" == "/var/empty" || "${HOMEDIR}" == "/dev/null" || "${HOMEDIR}" == "/var/root"
             || "${USERSHELL}" == "/usr/bin/false" || "${USERSHELL}" == "/dev/null" || "${USERSHELL}" == "/usr/sbin/uucico" ]]
        then
            continue
        fi

        case "${1}" in
            start)
                # Start the daemon
                su "${user}" -c "/Applications/VirtualBox.app/Contents/MacOS/VBoxAutostart --quiet --start --background --config ${CONFIG}"
                ;;
            stop)
                # Stop the daemon
                su "${user}" -c "/Applications/VirtualBox.app/Contents/MacOS/VBoxAutostart --quiet --stop --config ${CONFIG}"
                ;;
               *)
                echo "Usage: start|stop"
                exit 1
        esac
    done
}

function vboxStopAllUserVms()
{
    vboxStartStopAllUserVms "stop"
}

CONFIG=${1}
vboxStartStopAllUserVms "start"
trap vboxStopAllUserVms HUP KILL TERM


