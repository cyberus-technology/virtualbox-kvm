#!/bin/sh
#
# Oracle VM VirtualBox
# VirtualBox linux installation script

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

set -e

# Usage:
USAGE_MESSAGE=\
'Usage: `basename ${0}` [-h|--help|
    --test [<uname -r output> rpm|dpkg [<base expected> <versioned expected>]]]
This script tests whether a Linux system is set up to build kernel modules,
and if not prints a guess as to which distribution packages need to be
installed to do so, and what commands to use.  It uses the output of the
"uname -r" command to guess the packages and searches for packaging tools
by checking for packaging databases.

For testing you can specify the output of "uname -r" which will be used
instead of the real one and the package type, and optionally the expected
output.  --test without parameters will test a number of known inputs.'

# General theory of operation (valid Nov 19 2016): we check whether this is an
# rpm or dpkg system by checking for the respective non-empty database directory
# (we assert that only one is present), and based on that we map uname -r output
# to a kernel package name.  Here is a textual description of known mappings
# (not all of these are currently implemented) for both the general package name
# and the version-specific package name.  Keeping this here as it took some time
# and pain to research.
#
# Debian/Ubuntu (dpkg): <version>-<flavour> -> linux-headers-<flavour>,
#                                              linux-headers-<version>-<flavour>
DEBIAN_FLAVOURS="generic lowlatency virtual 486 686-pae amd64 rt-686-pae \
    rt-amd64 i386 586 grsec-686-pae grsec-amd64 686"
# SUSE (rpm): <version>-<flavour> -> kernel-<flavour>-devel,
#                                    kernel-<flavour>-devel-<version>
SUSE_FLAVOURS="debug default ec2 pae trace vanilla vmi xen"
# OL/RHEL/CentOS (rpm): <version><flavour>[.i686|.x86_64] -> kernel-<flavour>-devel,
#   kernel-<flavour>-devel-<`uname -r`>, where <version> ends in el*.
EL_FLAVOURS="uek xen"
# Fedora (rpm): <version> -> kernel-devel, kernel-devel-<version>, where <version>
#   ends in fc*.
#
# OUTPUT NOT YET TESTED ON REAL SYSTEMS
#
# Mageia (rpm): <version>-*.mga* -> kernel-linus-latest,
#                                   kernel-linus-<`uname -r`>
#   <version>-<flavour>-*.mga* -> kernel-<flavour>-latest,
#                                 kernel-<flavour>-<`uname -r`>
MAGEIA_FLAVOURS="desktop desktop586 server tmb-desktop"
# PCLinuxOS (dpkg): <version>-pclos* -> kernel-devel, kernel-devel-<`uname -r`>

PATH=$PATH:/bin:/sbin:/usr/sbin

HAVE_TOOLS=
HAVE_HEADERS=
PACKAGE_TYPE=
UNAME=
BASE_PACKAGE=
VERSIONED_PACKAGE=
TOOLS="gcc make perl"
TEST=
UNIT_TEST=

case "${1}" in
"")
    # Return immediately successfully if everything is installed
    type ${TOOLS} >/dev/null 2>&1 && HAVE_TOOLS=yes
    test -d "/lib/modules/`uname -r`/build/include" && HAVE_HEADERS=yes
    test -n "${HAVE_TOOLS}" && test -n "${HAVE_HEADERS}" && exit 0
    UNAME=`uname -r`
    for i in rpm dpkg; do
        for j in /var/lib/${i}/*; do
            test -e "${j}" || break
            if test -z "${PACKAGE_TYPE}"; then
                PACKAGE_TYPE="${i}"
            else
                PACKAGE_TYPE=unknown
            fi
            break
        done
    done
    ;;
-h|--help)
    echo "${USAGE_MESSAGE}"
    exit 0 ;;
*)
    ERROR=""
    UNAME="${2}"
    PACKAGE_TYPE="${3}"
    BASE_EXPECTED="${4}"
    VERSIONED_EXPECTED="${5}"
    test "${1}" = --test || ERROR=yes
    test -n "${UNAME}" && test -n "${PACKAGE_TYPE}" || test -z "${UNAME}" ||
        ERROR=yes
    test -n "${BASE_EXPECTED}" && test -n "${VERSIONED_EXPECTED}" ||
        test -z "${BASE_EXPECTED}" || ERROR=yes
    case "${ERROR}" in ?*)
        echo "${USAGE_MESSAGE}" >&2
        exit 1
    esac
    TEST=yes
    TEST_PARAMS="${2} ${3} ${4} ${5}"
    test -z "${UNAME}" && UNIT_TEST=yes
    ;;
esac

case "${PACKAGE_TYPE}" in
rpm)
    for i in ${SUSE_FLAVOURS}; do
        case "${UNAME}" in *-"${i}")
            BASE_PACKAGE="kernel-${i}-devel"
            VERSIONED_PACKAGE="kernel-${i}-devel-${UNAME%-${i}}"
            break
        esac
    done
    for i in ${EL_FLAVOURS} ""; do
        case "${UNAME}" in *.el5"${i}"|*.el*"${i}".i686|*.el*"${i}".x86_64)
            test -n "${i}" && i="${i}-"  # Hack to handle empty flavour.
            BASE_PACKAGE="kernel-${i}devel"
            VERSIONED_PACKAGE="kernel-${i}devel-${UNAME}"
            break
        esac
    done
    case "${UNAME}" in *.fc*.i686|*.fc*.x86_64)  # Fedora
        BASE_PACKAGE="kernel-devel"
        VERSIONED_PACKAGE="kernel-devel-${UNAME}"
    esac
    for i in ${MAGEIA_FLAVOURS} ""; do  # Mageia
        case "${UNAME}" in *-"${i}"*.mga*)
            if test -z "${i}"; then
                BASE_PACKAGE="kernel-linus-devel"
                VERSIONED_PACKAGE="kernel-linus-devel-${UNAME}"
            else
                BASE_PACKAGE="kernel-${i}-devel"
                VERSIONED_PACKAGE="kernel-${i}-devel-${UNAME%-${i}*}${UNAME#*${i}}"
            fi
            break
        esac
    done
    ;;
dpkg)
    for i in ${DEBIAN_FLAVOURS}; do  # Debian/Ubuntu
        case "${UNAME}" in *-${i})
            BASE_PACKAGE="linux-headers-${i}"
            VERSIONED_PACKAGE="linux-headers-${UNAME}"
            break
        esac
    done
    case "${UNAME}" in *-pclos*)  # PCLinuxOS
        BASE_PACKAGE="kernel-devel"
        VERSIONED_PACKAGE="kernel-devel-${UNAME}"
    esac
esac

case "${UNIT_TEST}${BASE_EXPECTED}" in "")
    echo "This system is currently not set up to build kernel modules." >&2
    test -n "${HAVE_TOOLS}" ||
        echo "Please install the ${TOOLS} packages from your distribution." >&2
    test -n "${HAVE_HEADERS}" && exit 1
    echo "Please install the Linux kernel \"header\" files matching the current kernel" >&2
    echo "for adding new hardware support to the system." >&2
    if test -n "${BASE_PACKAGE}${VERSIONED_PACKAGE}"; then
        echo "The distribution packages containing the headers are probably:" >&2
        echo "    ${BASE_PACKAGE} ${VERSIONED_PACKAGE}" >&2
    fi
    test -z "${TEST}" && exit 1
    exit 0
esac

case "${BASE_EXPECTED}" in ?*)
    case "${BASE_EXPECTED} ${VERSIONED_EXPECTED}" in
        "${BASE_PACKAGE} ${VERSIONED_PACKAGE}")
        exit 0
    esac
    echo "Test: ${TEST_PARAMS}" >&2
    echo "Result: ${BASE_PACKAGE} ${VERSIONED_PACKAGE}"
    exit 1
esac

# Unit test as of here.
# Test expected correct results.
for i in \
    "4.1.12-37.5.1.el6uek.x86_64 rpm kernel-uek-devel kernel-uek-devel-4.1.12-37.5.1.el6uek.x86_64" \
    "2.6.32-642.el6.x86_64 rpm kernel-devel kernel-devel-2.6.32-642.el6.x86_64" \
    "4.1.12-vanilla rpm kernel-vanilla-devel kernel-vanilla-devel-4.1.12" \
    "4.8.8-pclos1 dpkg kernel-devel kernel-devel-4.8.8-pclos1" \
    "4.8.8-desktop-1.mga6 rpm kernel-desktop-devel kernel-desktop-devel-4.8.8-1.mga6" \
    "3.19.8-2.mga5 rpm kernel-linus-devel kernel-linus-devel-3.19.8-2.mga5" \
    "4.8.0-27-generic dpkg linux-headers-generic linux-headers-4.8.0-27-generic"
do
    "${0}" --test ${i} || exit 1
done

# Test argument combinations expected to fail.
for i in \
    "--test NOT_EMPTY" \
    "--test NOT_EMPTY NOT_EMPTY NOT_EMPTY" \
    "--wrong" \
    "--test 4.8.8-pclos1 dpkg kernel-devel kernel-devel-WRONG" \
    "--test 4.8.8-pclos1 dpkg kernel-WRONG kernel-devel-4.8.8-pclos1"
do
    "${0}" ${i} >/dev/null 2>&1 && echo "Bad argument test failed:" &&
        echo "  ${i}" && exit 1
done
exit 0
