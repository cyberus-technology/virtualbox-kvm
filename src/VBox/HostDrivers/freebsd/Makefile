#
# Makefile for the VirtualBox FreeBSD Host Drivers.
#

#
# Copyright (C) 2008-2023 Oracle and/or its affiliates.
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
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
# in the VirtualBox distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#
# SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
#

KBUILD_VERBOSE =

all:
	@echo "=== Building 'vboxdrv' module ==="
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv
	@cp vboxdrv/vboxdrv.ko .
	@echo
	@if [ -d vboxnetflt ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxnetflt; \
	    fi; \
	    echo "=== Building 'vboxnetflt' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetflt; \
	    cp vboxnetflt/vboxnetflt.ko .; \
	    echo; \
	fi
	@if [ -d vboxnetadp ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxnetadp; \
	    fi; \
	    echo "=== Building 'vboxnetadp' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetadp; \
	    cp vboxnetadp/vboxnetadp.ko .; \
	    echo; \
	fi


install:
	@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv install
	@if [ -d vboxnetflt ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetflt install; \
	fi
	@if [ -d vboxnetadp ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetadp install; \
	fi

clean:
	@$(MAKE) -C vboxdrv clean
	@if [ -d vboxnetflt ]; then \
	    $(MAKE) -C vboxnetflt clean; \
	fi
	@if [ -d vboxnetadp ]; then \
	    $(MAKE) -C vboxnetadp clean; \
	fi
	rm -f vboxdrv.ko vboxnetflt.ko vboxnetadp.ko

load:
	@for module in vboxnetadp vboxnetflt vboxdrv; do \
		if kldstat -n "$$module" >/dev/null; then \
			echo "Removing previously installed $$module module"; \
			/sbin/kldunload $$module; \
		fi; \
	done
	@for module in vboxdrv vboxnetflt vboxnetadp; do \
		if test -f $$module.ko; then \
			echo "Installing $$module module"; \
			/sbin/kldload ./$$module.ko; \
		fi; \
	done
