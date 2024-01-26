#!/bin/sh
## @file
# VirtualBox Test Execution Service Init Script.
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

# chkconfig: 35 35 65
# description: VirtualBox Test Execution Service
#
### BEGIN INIT INFO
# Provides:       vboxtxs
# Required-Start: $network
# Required-Stop:
# Default-Start:  2 3 4 5
# Default-Stop:   0 1 6
# Description:    VirtualBox Test Execution Service
### END INIT INFO

PATH=$PATH:/bin:/sbin:/usr/sbin
SCRIPTNAME=vboxtxs.sh

CDROM_PATH=/media/cdrom
SCRATCH_PATH=/tmp/vboxtxs-scratch

PIDFILE="/var/run/vboxtxs"

# Preamble for Gentoo
if [ "`which $0`" = "/sbin/rc" ]; then
    shift
fi

begin_msg()
{
    test -n "${2}" && echo "${SCRIPTNAME}: ${1}."
    logger -t "${SCRIPTNAME}" "${1}."
}

succ_msg()
{
    logger -t "${SCRIPTNAME}" "${1}."
}

fail_msg()
{
    echo "${SCRIPTNAME}: failed: ${1}." >&2
    logger -t "${SCRIPTNAME}" "failed: ${1}."
}

killproc() {
    kp_binary="${1##*/}"
    pkill "${kp_binary}" || return 0
    sleep 1
    pkill "${kp_binary}" || return 0
    sleep 1
    pkill -9 "${kp_binary}"
    return 0
}

case "`uname -m`" in
    AMD64|amd64|X86_64|x86_64)
        binary=/opt/validationkit/linux/amd64/TestExecService
        ;;

    i386|x86|i486|i586|i686|i787)
        binary=/opt/validationkit/linux/x86/TestExecService
        ;;

    *)
        binary=/opt/validationkit/linux/x86/TestExecService
        ;;
esac

fixAndTestBinary() {
    chmod a+x "$binary" 2> /dev/null > /dev/null
    test -x "$binary" || {
        echo "Cannot run $binary"
        exit 1
    }
}

start() {
    if ! test -f $PIDFILE; then
        begin_msg "Starting VirtualBox Test Execution Service" console
        fixAndTestBinary
        mount /dev/cdrom "${CDROM_PATH}" 2> /dev/null > /dev/null
        $binary --auto-upgrade --scratch="${SCRATCH_PATH}" --cdrom="${CDROM_PATH}" --no-display-output > /dev/null
        RETVAL=$?
        test $RETVAL -eq 0 && sleep 2 && echo `pidof TestExecService` > $PIDFILE
        if ! test -s "${PIDFILE}"; then
            RETVAL=5
        fi
        if test $RETVAL -eq 0; then
            succ_msg "VirtualBox Test Execution service started"
        else
            fail_msg "VirtualBox Test Execution service failed to start"
        fi
    fi
    return $RETVAL
}

stop() {
    if test -f $PIDFILE; then
        begin_msg "Stopping VirtualBox Test Execution Service" console
        killproc $binary
    fi
}

restart() {
    stop && start
}

status() {
    echo -n "Checking for vboxtxs"
    if [ -f $PIDFILE ]; then
        echo " ...running"
    else
        echo " ...not running"
    fi
}

case "$1" in
start)
    start
    ;;
stop)
    stop
    ;;
restart)
    restart
    ;;
status)
    status
    ;;
setup)
    ;;
cleanup)
    ;;
*)
    echo "Usage: $0 {start|stop|restart|status}"
    exit 1
esac

exit $RETVAL
