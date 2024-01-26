"""
Copyright (C) 2018-2023 Oracle and/or its affiliates.

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

import sys

def GeneratePfns():

    # Get list of functions.
    exports_file = open(sys.argv[1], "r")
    if not exports_file:
        print("Error: couldn't open %s file!" % filename)
        sys.exit()

    names = []
    for line in exports_file.readlines():
        line = line.strip()
        if len(line) > 0 and line[0] != ';' and line != 'EXPORTS':
            # Parse 'glAccum = glAccum@8'
            words = line.split('=')

            # Function name
            names.append(words[0].strip())

    exports_file.close()


    #
    # C loader data
    #
    c_file = open(sys.argv[2], "w")
    if not c_file:
        print("Error: couldn't open %s file!" % filename)
        sys.exit()

    c_file.write('#include <iprt/win/windows.h>\n')
    c_file.write('#include <VBoxWddmUmHlp.h>\n')
    c_file.write('\n')

    for index in range(len(names)):
        fn = names[index]
        c_file.write('FARPROC pfn_%s;\n' % fn)
    c_file.write('\n')

    c_file.write("struct VBOXWDDMDLLPROC aIcdProcs[] =\n")
    c_file.write('{\n')
    for index in range(len(names)):
        fn = names[index]
        c_file.write('    { "%s",  &pfn_%s },\n' % (fn, fn) )
    c_file.write('    { NULL, NULL }\n')
    c_file.write('};\n')

    c_file.close()

GeneratePfns()
