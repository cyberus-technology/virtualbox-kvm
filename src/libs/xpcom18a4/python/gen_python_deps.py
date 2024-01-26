#!/usr/bin/python

"""
Copyright (C) 2009-2023 Oracle and/or its affiliates.

This file is part of VirtualBox base platform packages, as
available from https://www.virtualbox.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, in version 3 of the
License.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <https://www.gnu.org/licenses>.

SPDX-License-Identifier: GPL-3.0-only
"""

from __future__ import print_function
import os,sys
from distutils.version import StrictVersion

versions = ["2.6", "2.7", "3.1", "3.2", "3.2m", "3.3", "3.3m", "3.4", "3.4m", "3.5", "3.5m", "3.6", "3.6m", "3.7", "3.7m", "3.8", "3.9", "3.10", "3.11", "3.12" ]
prefixes = ["/usr", "/usr/local", "/opt", "/opt/local"]
known = {}

def checkPair(p, v, dllpre, dllsuff, bitness_magic):
    incdir = os.path.join(p, "include", "python"+v)
    incfile = os.path.join(incdir, "Python.h")
    if not os.path.isfile(incfile):
        return None

    lib = os.path.join(p, "lib/i386-linux-gnu", dllpre+"python"+v+dllsuff)
    if not os.path.isfile(lib):
        lib = os.path.join(p, "lib", dllpre+"python"+v+dllsuff)
        if not os.path.isfile(lib):
            lib = None

    if bitness_magic == 1:
        lib64 = os.path.join(p, "lib", "64", dllpre+"python"+v+dllsuff)
        if not os.path.isfile(lib64):
            lib64 = None
    elif bitness_magic == 2:
        lib64 = os.path.join(p, "lib/x86_64-linux-gnu", dllpre+"python"+v+dllsuff)
        if not os.path.isfile(lib64):
            lib64 = os.path.join(p, "lib64", dllpre+"python"+v+dllsuff)
            if not os.path.isfile(lib64):
                lib64 = os.path.join(p, "lib", dllpre+"python"+v+dllsuff)
                if not os.path.isfile(lib64):
                    lib64 = None
    else:
        lib64 = None

    if lib is None and lib64 is None:
        return None
    else:
        return [incdir, lib, lib64]

def print_vars(vers, known, sep, bitness_magic):
    print("VBOX_PYTHON%s_INC=%s%s" %(vers, known[0], sep))
    if bitness_magic > 0:
        if known[2]:
            print("VBOX_PYTHON%s_LIB=%s%s" %(vers, known[2], sep))
        if known[1]:
            print("VBOX_PYTHON%s_LIB_X86=%s%s" %(vers, known[1], sep))
    else:
        print("VBOX_PYTHON%s_LIB=%s%s" %(vers, known[1], sep))


def main(argv):
    global prefixes
    global versions

    dllpre = "lib"
    dllsuff = ".so"
    bitness_magic = 0

    if len(argv) > 1:
        target = argv[1]
    else:
        target = sys.platform

    if len(argv) > 2:
        arch = argv[2]
    else:
        arch = "unknown"

    if len(argv) > 3:
        multi = int(argv[3])
    else:
        multi = 1

    if multi == 0:
        prefixes = ["/usr"]
        versions = [str(sys.version_info[0])+'.'+str(sys.version_info[1]),
                    str(sys.version_info[0])+'.'+str(sys.version_info[1])+'m']

    if target == 'darwin':
        ## @todo Pick up the locations from VBOX_PATH_MACOSX_SDK_10_*.
        prefixes = ['/Developer/SDKs/MacOSX10.4u.sdk/usr',
                    '/Developer/SDKs/MacOSX10.5.sdk/usr',
                    '/Developer/SDKs/MacOSX10.6.sdk/usr',
                    '/Developer/SDKs/MacOSX10.7.sdk/usr']
        dllsuff = '.dylib'

    if target == 'solaris' and arch == 'amd64':
        bitness_magic = 1

    if target == 'linux' and arch == 'amd64':
        bitness_magic = 2

    for v in versions:
        if v.endswith("m"):
            realversion = v[:-1]
        else:
            realversion = v
        if StrictVersion(realversion) < StrictVersion('2.6'):
            continue
        for p in prefixes:
            c = checkPair(p, v, dllpre, dllsuff, bitness_magic)
            if c is not None:
                known[v] = c
                break
    keys = list(known.keys())
    # we want default to be the lowest versioned Python
    keys.sort()
    d = None
    # We need separator other than newline, to sneak through $(shell)
    sep = "|"
    for k in keys:
        if d is None:
            d = k
        vers = k.replace('.', '').upper()
        print_vars(vers, known[k], sep, bitness_magic)
    if d is not None:
        print_vars("DEF", known[d], sep, bitness_magic)
    else:
        print(argv[0] + ": No Python development package found!", file=sys.stderr)

if __name__ == '__main__':
    main(sys.argv)
