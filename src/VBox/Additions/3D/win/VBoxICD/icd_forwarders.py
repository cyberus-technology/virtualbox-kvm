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

def GenerateForwarders():

    # Get list of functions.
    exports_file = open(sys.argv[1], "r")
    if not exports_file:
        print("Error: couldn't open %s file!" % filename)
        sys.exit()

    names = []
    cbArgs = []
    for line in exports_file.readlines():
        line = line.strip()
        if len(line) > 0 and line[0] != ';' and line != 'EXPORTS':
            # Parse 'glAccum = glAccum@8'
            words = line.split('=', 1)

            # Function name
            names.append(words[0].strip())

            # Size of arguments in bytes
            words = words[1].split('@')
            cbArgs.append(words[1].strip())

    exports_file.close()


    #
    # Assembler forwarders
    #
    asm_file = open(sys.argv[2], "w")
    if not asm_file:
        print("Error: couldn't open %s file!" % filename)
        sys.exit()

    asm_file.write('%include "iprt/asmdefs.mac"\n')
    asm_file.write('\n')
    asm_file.write(';;;; Enable ICD_LAZY_LOAD to lazy load the ICD DLL (does not work on Win64)\n')
    asm_file.write('; %define ICD_LAZY_LOAD 1\n')
    asm_file.write('\n')
    asm_file.write('%ifdef RT_ARCH_AMD64\n')
    asm_file.write('%define PTR_SIZE_PREFIX qword\n')
    asm_file.write('%else ; X86\n')
    asm_file.write('%define PTR_SIZE_PREFIX dword\n')
    asm_file.write('%endif\n')
    asm_file.write('\n')
    asm_file.write('%ifdef ICD_LAZY_LOAD\n')
    asm_file.write('extern NAME(VBoxLoadICD)\n')
    asm_file.write('%endif\n')
    asm_file.write('extern NAME(g_hmodICD)\n')

    for index in range(len(names)):
        fn = names[index]
        cbRet = cbArgs[index]
        asm_file.write('\n')
        asm_file.write('BEGINPROC_EXPORTED %s\n' % fn)
        asm_file.write('    extern NAME(pfn_%s)\n' % fn)
        asm_file.write(';    int3\n')
        asm_file.write('%ifdef ICD_LAZY_LOAD\n')
        asm_file.write('    mov   xAX, PTR_SIZE_PREFIX NAME(g_hmodICD)\n')
        asm_file.write('    mov   xAX, [xAX]\n')
        asm_file.write('    or    xAX, xAX\n')
        asm_file.write('    jnz   l_icd_loaded_%s\n' % fn)
        asm_file.write('    call  NAME(VBoxLoadICD)\n')
        asm_file.write('l_icd_loaded_%s:\n' % fn)
        asm_file.write('%endif\n')
        asm_file.write('    mov   xAX, PTR_SIZE_PREFIX NAME(pfn_%s)\n' % fn)
        asm_file.write('    mov   xAX, [xAX]\n')
        asm_file.write('    or    xAX, xAX\n')
        asm_file.write('    jnz   l_jmp_to_%s\n' % fn)
        asm_file.write('%ifdef RT_ARCH_AMD64\n')
        asm_file.write('    ret\n')
        asm_file.write('%else ; X86\n')
        asm_file.write('    ret %s\n' % cbRet)
        asm_file.write('%endif\n')
        asm_file.write('l_jmp_to_%s:\n' % fn)
        asm_file.write('    jmp   xAX\n')
        asm_file.write('ENDPROC %s\n' % fn)

    asm_file.close()

GenerateForwarders()
