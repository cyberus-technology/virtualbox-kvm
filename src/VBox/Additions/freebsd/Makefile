#
# Makefile for the VirtualBox FreeBSD Guest Drivers.
#

#
# Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

KBUILD_VERBOSE =

all:
	@echo "=== Building 'vboxguest' module ==="
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxguest
	@if [ -f vboxguest/vboxguest.ko ]; then \
	    cp vboxguest/vboxguest.ko .; \
	 fi
	@echo
	@if [ -d vboxvfs ]; then \
	    echo "=== Building 'vboxvfs' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxvfs; \
	    if [ -f vboxvfs/vboxvfs.ko ]; then \
	        cp vboxvfs/vboxvfs.ko .; \
	    fi; \
	fi


install:
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxguest install
	@if [ -d vboxvfs ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxvfs install; \
	fi

clean:
	@$(MAKE) -C vboxguest clean
	@if [ -d vboxvfs ]; then \
	    $(MAKE) -C vboxvfs clean; \
	fi
	rm -f vboxguest.*o vboxvfs.*o

load:
	@/sbin/kldunload vboxvfs || true
	@/sbin/kldunload vboxguest || true
	@/sbin/kldload ./vboxguest.ko
	@if [ -f vboxvfs.ko ]; then /sbin/kldload ./vboxvfs.ko; fi
