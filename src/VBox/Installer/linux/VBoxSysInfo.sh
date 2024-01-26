#!/bin/sh

# @file
#
# Installer (Unix-like)
# Information about the host system/Linux distribution

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

# Print information about a Linux system
# @param distribution name of the distribution
# @param version      version of the distribution
print_linux_info () {
    # The following regex is not quite correct for an e-mail address, as
    # the local part may not start or end with a dot.  Please correct if
    # this upsets you.
    kern_ver=`cat /proc/version | sed -e 's/ ([a-zA-Z0-9.!#$%*/?^{}\`+=_-]*@[a-zA-Z0-9.-]*)//'`
    echo "Distribution: $1 | Version: $2 | Kernel: $kern_ver"
}

# Determine the distribution name and release for a Linux system and print
# send the information to stdout using the print_linux_info function.
# For practical reasons (i.e. lack of time), this function only gives
# information for distribution releases considered "of interest" and reports
# others as unknown.  It can be extended later if other distributions are
# found to be "of interest".
get_linux_info () {
    if which lsb_release > /dev/null 2>&1
    then
        # LSB-compliant system
        print_linux_info `lsb_release -i -s` `lsb_release -r -s`
    elif [ -r /etc/debian_version ]
    then
        # Debian-based system
        release=`cat /etc/debian_version`
        print_linux_info "Debian" $release
    elif [ -r /etc/mandriva-release ]
    then
        # Mandriva-based system
        release=`cat /etc/mandriva-release | sed -e 's/[A-Za-z ]* release //'`
        print_linux_info "Mandriva" $release
    elif [ -r /etc/fedora-release ]
    then
        # Fedora-based
        release=`cat /etc/fedora-release | sed -e 's/[A-Za-z ]* release //'`
        print_linux_info "Fedora" $release
    elif [ -r /etc/SuSE-release ]
    then
        # SUSE-based.
        release=`cat /etc/SuSE-release | grep "VERSION" | sed -e 's/VERSION = //'`
        if grep openSUSE /etc/SuSE-release
        then
            # Is it worth distinguishing here?  I did it mainly to prevent
            # confusion with the version number
            print_linux_info "openSUSE" $release
        else
            print_linux_info "SUSE" $release
        fi
    elif [ -r /etc/gentoo-release ]
    then
        # Gentoo-based
        release=`cat /etc/gentoo-release | sed -e 's/[A-Za-z ]* release //'`
        print_linux_info "Gentoo" $release
    elif [ -r /etc/slackware-version ]
    then
        # Slackware
        release=`cat /etc/slackware-version | sed -e 's/Slackware //'`
        print_linux_info "Slackware" $release
    elif [ -r /etc/arch-release ]
    then
        # Arch Linux
        print_linux_info "Arch Linux" "none"
    elif [ -r /etc/redhat-release ]
    then
        # Redhat-based.  This should come near the end, as it other
        # distributions may give false positives.
        release=`cat /etc/redhat-release | sed -e 's/[A-Za-z ]* release //'`
        print_linux_info "Redhat" $release
    else
        print_linux_info "unknown" "unknown"
    fi
}

# Print information about a Solaris system.  FIXME.
get_solaris_info () {
    kernel=`uname -v`
    echo "Kernel: $kernel"
}

# Print information about a MacOS system.  FIXME.
get_macos_info () {
    machine=`uname -m`
    kernel=`uname -v`
    echo "Machine: $machine | Kernel: $kernel"
}

# Print information about a FreeBSD system.  FIXME.
get_freebsd_info () {
    kernel=`uname -v`
    echo "Kernel: $kernel"
}

system=`uname -s`
case "$system" in
Linux|linux)
    get_linux_info
    ;;
SunOS)
    get_solaris_info
    ;;
Darwin)
    get_macos_info
    ;;
FreeBSD)
    get_freebsd_info
    ;;
*)
    echo "System unknown"
    exit 1
    ;;
esac
exit 0
