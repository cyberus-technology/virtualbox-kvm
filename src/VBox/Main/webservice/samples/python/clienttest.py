#!/usr/bin/python

__copyright__ = \
"""
Copyright (C) 2012-2023 Oracle and/or its affiliates.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
"""


# Things needed to be set up before running this sample:
# - Install Python and verify it works (2.7.2 will do, 3.x is untested yet)
# - On Windows: Install the PyWin32 extensions for your Python version
#   (see http://sourceforge.net/projects/pywin32/)
# - If not already done, set the environment variable "VBOX_INSTALL_PATH"
#   to point to your VirtualBox installation directory (which in turn must have
#   the "sdk" subfolder")
# - Install the VirtualBox Python bindings by doing a
#   "[python] vboxapisetup.py install"
# - Run this sample with "[python] clienttest.py"

import os,sys
import traceback

#
# Converts an enumeration to a printable string.
#
def enumToString(constants, enum, elem):
    all = constants.all_values(enum)
    for e in all.keys():
        if str(elem) == str(all[e]):
            return e
    return "<unknown>"

def main(argv):

    from vboxapi import VirtualBoxManager
    # This is a VirtualBox COM/XPCOM API client, no data needed.
    mgr = VirtualBoxManager(None, None)

    # Get the global VirtualBox object
    vbox = mgr.getVirtualBox()

    print "Running VirtualBox version %s" %(vbox.version)

    # Get all constants through the Python manager code
    vboxConstants = mgr.constants

    # Enumerate all defined machines
    for mach in mgr.getArray(vbox, 'machines'):

        try:
            # Be prepared for failures - the VM can be inaccessible
            vmname = '<inaccessible>'
            try:
                vmname = mach.name
            except Exception, e:
                None
            vmid = '';
            try:
                vmid = mach.id
            except Exception, e:
                None

            # Print some basic VM information even if there were errors
            print "Machine name: %s [%s]" %(vmname,vmid)
            if vmname == '<inaccessible>' or vmid == '':
                continue

            # Print some basic VM information
            print "    State:           %s" %(enumToString(vboxConstants, "MachineState", mach.state))
            print "    Session state:   %s" %(enumToString(vboxConstants, "SessionState", mach.sessionState))

            # Do some stuff which requires a running VM
            if mach.state == vboxConstants.MachineState_Running:

                # Get the session object
                session = mgr.getSessionObject()

                 # Lock the current machine (shared mode, since we won't modify the machine)
                mach.lockMachine(session, vboxConstants.LockType_Shared)

                # Acquire the VM's console and guest object
                console = session.console
                guest = console.guest

                # Retrieve the current Guest Additions runlevel and print
                # the installed Guest Additions version
                addRunLevel = guest.additionsRunLevel
                print "    Additions State: %s" %(enumToString(vboxConstants, "AdditionsRunLevelType", addRunLevel))
                if addRunLevel != vboxConstants.AdditionsRunLevelType_None:
                    print "    Additions Ver:   %s"  %(guest.additionsVersion)

                # Get the VM's display object
                display = console.display

                # Get the VM's current display resolution + bit depth + position
                screenNum = 0 # From first screen
                (screenW, screenH, screenBPP, screenX, screenY, _) = display.getScreenResolution(screenNum)
                print "    Display (%d):     %dx%d, %d BPP at %d,%d"  %(screenNum, screenW, screenH, screenBPP, screenX, screenY)

                # We're done -- don't forget to unlock the machine!
                session.unlockMachine()

        except Exception, e:
            print "Errror [%s]: %s" %(mach.name, str(e))
            traceback.print_exc()

    # Call destructor and delete manager
    del mgr

if __name__ == '__main__':
    main(sys.argv)
