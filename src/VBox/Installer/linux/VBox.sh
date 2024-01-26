#!/bin/sh
## @file
# Oracle VM VirtualBox startup script, Linux hosts.
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

PATH="/usr/bin:/bin:/usr/sbin:/sbin"

# The below is GNU-specific.  See slightly further down for a version which
# works on Solaris and OS X.
TARGET=`readlink -e -- "${0}"` || exit 1
MY_DIR="${TARGET%/[!/]*}"

# (
#     path="${0}"
#     while test -n "${path}"; do
#         # Make sure we have at least one slash and no leading dash.
#         expr "${path}" : / > /dev/null || path="./${path}"
#         # Filter out bad characters in the path name.
#         expr "${path}" : ".*[*?<>\\]" > /dev/null && exit 1
#         # Catch embedded new-lines and non-existing (or path-relative) files.
#         # $0 should always be absolute when scripts are invoked through "#!".
#         test "`ls -l -d "${path}" 2> /dev/null | wc -l`" -eq 1 || exit 1
#         # Change to the folder containing the file to resolve relative links.
#         folder=`expr "${path}" : "\(.*/\)[^/][^/]*/*$"` || exit 1
#         path=`expr "x\`ls -l -d "${path}"\`" : "[^>]* -> \(.*\)"`
#         cd "${folder}"
#         # If the last path was not a link then we are in the target folder.
#         test -n "${path}" || pwd
#     done
# )

if test -f /usr/lib/virtualbox/VirtualBox &&
    test -x /usr/lib/virtualbox/VirtualBox; then
    INSTALL_DIR=/usr/lib/virtualbox
elif test -f "${MY_DIR}/VirtualBox" && test -x "${MY_DIR}/VirtualBox"; then
    INSTALL_DIR="${MY_DIR}"
else
    echo "Could not find VirtualBox installation. Please reinstall."
    exit 1
fi

# Note: This script must not fail if the module was not successfully installed
#       because the user might not want to run a VM but only change VM params!

if [ "$1" = "shutdown" ]; then
    SHUTDOWN="true"
elif ! lsmod|grep -q vboxdrv; then
    cat << EOF
WARNING: The vboxdrv kernel module is not loaded. Either there is no module
         available for the current kernel (`uname -r`) or it failed to
         load. Please recompile the kernel module and install it by

           sudo /sbin/vboxconfig

         You will not be able to start VMs until this problem is fixed.
EOF
elif [ ! -c /dev/vboxdrv ]; then
    cat << EOF
WARNING: The character device /dev/vboxdrv does not exist. Try

           sudo /sbin/vboxconfig

         and if that is not successful, try to re-install the package.

         You will not be able to start VMs until this problem is fixed.
EOF
fi

if [ -f /etc/vbox/module_not_compiled ]; then
    cat << EOF
WARNING: The compilation of the vboxdrv.ko kernel module failed during the
         installation for some reason. Starting a VM will not be possible.
         Please consult the User Manual for build instructions.
EOF
fi

SERVER_PID=`ps -U \`whoami\` | grep VBoxSVC | awk '{ print $1 }'`
if [ -z "$SERVER_PID" ]; then
    # Server not running yet/anymore, cleanup socket path.
    # See IPC_GetDefaultSocketPath()!
    if [ -n "$LOGNAME" ]; then
        rm -rf /tmp/.vbox-$LOGNAME-ipc > /dev/null 2>&1
    else
        rm -rf /tmp/.vbox-$USER-ipc > /dev/null 2>&1
    fi
fi

if [ "$SHUTDOWN" = "true" ]; then
    if [ -n "$SERVER_PID" ]; then
        kill -TERM $SERVER_PID
        sleep 2
    fi
    exit 0
fi

APP=`basename $0`
case "$APP" in
    VirtualBox|virtualbox)
        exec "$INSTALL_DIR/VirtualBox" "$@"
        ;;
    VirtualBoxVM|virtualboxvm)
        exec "$INSTALL_DIR/VirtualBoxVM" "$@"
        ;;
    VBoxManage|vboxmanage)
        exec "$INSTALL_DIR/VBoxManage" "$@"
        ;;
    VBoxSDL|vboxsdl)
        exec "$INSTALL_DIR/VBoxSDL" "$@"
        ;;
    VBoxVRDP|VBoxHeadless|vboxheadless)
        exec "$INSTALL_DIR/VBoxHeadless" "$@"
        ;;
    VBoxAutostart|vboxautostart)
        exec "$INSTALL_DIR/VBoxAutostart" "$@"
        ;;
    VBoxBalloonCtrl|vboxballoonctrl)
        exec "$INSTALL_DIR/VBoxBalloonCtrl" "$@"
        ;;
    VBoxBugReport|vboxbugreport)
        exec "$INSTALL_DIR/VBoxBugReport" "$@"
        ;;
    VBoxDTrace|vboxdtrace)
        exec "$INSTALL_DIR/VBoxDTrace" "$@"
        ;;
    VBoxAudioTest|vboxaudiotest|vkat)
        exec "$INSTALL_DIR/VBoxAudioTest" "$@"
        ;;
    vboxwebsrv)
        exec "$INSTALL_DIR/vboxwebsrv" "$@"
        ;;
    *)
        echo "Unknown application - $APP"
        exit 1
        ;;
esac
exit 0
