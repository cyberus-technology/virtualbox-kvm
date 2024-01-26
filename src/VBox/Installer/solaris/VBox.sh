#!/bin/sh
## @file
# Oracle VM VirtualBox startup script, Solaris hosts.
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

CURRENT_ISA=`isainfo -k`
if test "$CURRENT_ISA" = "amd64"; then
    INSTALL_DIR="/opt/VirtualBox/amd64"
else
    INSTALL_DIR="/opt/VirtualBox/i386"
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
    VBoxBugReport|vboxbugreport)
        exec "$INSTALL_DIR/VBoxBugReport" "$@"
        ;;
    VBoxBalloonCtrl|vboxballoonctrl)
        exec "$INSTALL_DIR/VBoxBalloonCtrl" "$@"
        ;;
    VBoxAutostart|vboxautostart)
        exec "$INSTALL_DIR/VBoxAutostart" "$@"
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
    VBoxQtconfig)
        exec "$INSTALL_DIR/VBoxQtconfig" "$@"
        ;;
    vbox-img)
        exec "$INSTALL_DIR/vbox-img" "$0"
        ;;
    *)
        echo "Unknown application - $APP"
        exit 1
        ;;
esac
exit 0

