## @file
#  Apply fixup to VTF binary image for FFS Raw section
#
#  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import sys


d = open(sys.argv[1], 'rb').read()
c = ((len(d) + 4 + 7) & ~7) - 4
if c > len(d):
    c -= len(d)
    # VBox begin
    # Original: f = open(sys.argv[1], 'wb'), changed to:
    f = open(sys.argv[2], 'wb')
    # VBox end
    f.write('\x90' * c)
    f.write(d)
    f.close()
