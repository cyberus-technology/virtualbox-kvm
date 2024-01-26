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

#################################################################################
# This program is a port-forwarding configurator supposed to simplify
# port-forwarding for NAT users
# > python VBoxPortForwarding.py --vm winXP -a 1 -p TCP -l 8080 -g 80 -P www
# generates sequence of API calls, equivalent to:
# > VBoxManage setextradata "winXP"
#        "VBoxInternal/Devices/pcnet/0/LUN#0/Config/www/Protocol" TCP
# > VBoxManage setextradata "winXP"
#        "VBoxInternal/Devices/pcnet/0/LUN#0/Config/www/GuestPort" 80
# > VBoxManage setextradata "winXP"
#        "VBoxInternal/Devices/pcnet/0/LUN#0/Config/www/HostPort" 8080
################################################################################

import os,sys
from vboxapi import VirtualBoxManager
import optparse

class OptionParser (optparse.OptionParser):
    def check_required(self, opt):
        option = self.get_option(opt)
        if option.type == "string" and getattr(self.values, option.dest) != None:
            return True
        if option.type == "int" and getattr(self.values, option.dest) != -1:
            return True
        return False

def generate_profile_name(proto, host_port, guest_port):
    return proto + '_' + str(host_port) + '_' + str(guest_port)

def main(argv):

    usage = "usage: %prog --vm winXP -a 1 -p TCP -l 8080 -g 80 -P www"
    parser = OptionParser(usage=usage)
    parser.add_option("-V", "--vm", action="store", dest="vmname", type="string",
                     help="Name or UID of VM to operate on", default=None)
    parser.add_option("-P", "--profile", dest="profile", type="string",
                     default=None)
    parser.add_option("-p", "--ip-proto", dest="proto", type="string",
                     default=None)
    parser.add_option("-l", "--host-port", dest="host_port", type="int",
                     default = -1)
    parser.add_option("-g", "--guest-port", dest="guest_port", type="int",
                     default = -1)
    parser.add_option("-a", "--adapter", dest="adapter", type="int",
                     default=-1)
    (options,args) = parser.parse_args(argv)

    if (not (parser.check_required("-V") or parser.check_required("-G"))):
        parser.error("please define --vm or --guid option")
    if (not parser.check_required("-p")):
        parser.error("please define -p or --ip-proto option")
    if (not parser.check_required("-l")):
        parser.error("please define -l or --host_port option")
    if (not parser.check_required("-g")):
        parser.error("please define -g or --guest_port option")
    if (not parser.check_required("-a")):
        parser.error("please define -a or --adapter option")

    man = VirtualBoxManager(None, None)
    vb = man.getVirtualBox()
    print "VirtualBox version: %s" % vb.version,
    print "r%s" % vb.revision

    vm = None
    try:
        if options.vmname != None:
            vm = vb.findMachine(options.vmname)
        elif options.vmname != None:
            vm = vb.getMachine(options.vmname)
    except:
        print "can't find VM by name or UID:",options.vmname
        del man
        return

    print "vm found: %s [%s]" % (vm.name, vm.id)

    session = man.openMachineSession(vm.id)
    vm = session.machine

    adapter = vm.getNetworkAdapter(options.adapter)

    if adapter.enabled == False:
        print "adapter(%d) is disabled" % adapter.slot
        del man
        return

    name = None
    if (adapter.adapterType == man.constants.NetworkAdapterType_Null):
        print "none adapter type detected"
        return -1
    elif (adapter.adapterType == man.constants.NetworkAdapterType_Am79C970A):
        name = "pcnet"
    elif (adapter.adapterType == man.constants.NetworkAdapterType_Am79C973):
        name = "pcnet"
    elif (adapter.adapterType == man.constants.NetworkAdapterType_I82540EM):
        name = "e1000"
    elif (adapter.adapterType == man.constants.NetworkAdapterType_I82545EM):
        name = "e1000"
    elif (adapter.adapterType == man.constants.NetworkAdapterType_I82543GC):
        name = "e1000"
    print "adapter of '%s' type has been detected" % name

    profile_name = options.profile
    if profile_name == None:
        profile_name = generate_profile_name(options.proto.upper(),
                                            options.host_port,
                                            options.guest_port)
    config = "VBoxInternal/Devices/" + name + "/"
    config = config + str(adapter.slot)  +"/LUN#0/Config/" + profile_name
    proto = config + "/Protocol"
    host_port = config + "/HostPort"
    guest_port = config + "/GuestPort"

    vm.setExtraData(proto, options.proto.upper())
    vm.setExtraData(host_port, str(options.host_port))
    vm.setExtraData(guest_port, str(options.guest_port))


    vm.saveSettings()
    man.closeMachineSession(session)

    del man

if __name__ == "__main__":
    main(sys.argv)
