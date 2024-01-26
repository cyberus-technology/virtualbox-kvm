#
# Makefile for the VirtualBox Linux Host Drivers.
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

ifneq ($(KERNELRELEASE),)

# Building from kBuild (make -C <kernel_directory> M=`pwd`),
# or inside a kernel source tree.

obj-m = vboxdrv/
 ifneq ($(wildcard $(CURDIR)/vboxnetflt/Makefile),)
obj-m += vboxnetflt/
 endif
 ifneq ($(wildcard $(CURDIR)/vboxnetadp/Makefile),)
obj-m += vboxnetadp/
 endif
 ifneq ($(wildcard $(CURDIR)/vboxpci/Makefile),)
obj-m += vboxpci/
 endif

else # ! KERNELRELEASE

# convenience Makefile without KERNELRELEASE

ifndef SUDO
 ifneq ($(shell id -u),0)
SUDO := $(firstword $(wildcard /usr/bin/sudo /bin/sudo /usr/sbin/sudo))
 else
SUDO :=
 endif
endif

KBUILD_VERBOSE ?=
.PHONY: all install clean check unload load \
    vboxdrv vboxnetflt vboxnetadp vboxpci \
    install-vboxdrv install-vboxnetflt install-vboxnetadp install-vboxpci \
    clean-vboxdrv clean-vboxnetflt clean-vboxnetadp clean-vboxpci

all: vboxdrv vboxnetflt vboxnetadp vboxpci

# We want to build on Linux 2.6.18 and later kernels.
KERN_VER ?= $(shell uname -r)
 ifneq ($(filter-out 1.% 2.0.% 2.1.% 2.2.% 2.3.% 2.4.% 2.5.%,$(KERN_VER)),)

vboxdrv:
	@echo "=== Building 'vboxdrv' module ==="
	+@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv
	@cp vboxdrv/vboxdrv.ko .
	@echo

vboxnetflt: vboxdrv
	+@if [ -d vboxnetflt ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxnetflt; \
	    fi; \
	    echo "=== Building 'vboxnetflt' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) KBUILD_EXTRA_SYMBOLS=$(abspath vboxnetflt/Module.symvers) -C vboxnetflt || exit 1; \
	    cp vboxnetflt/vboxnetflt.ko .; \
	    echo; \
	fi

vboxnetadp: vboxdrv
	+@if [ -d vboxnetadp ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxnetadp; \
	    fi; \
	    echo "=== Building 'vboxnetadp' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) KBUILD_EXTRA_SYMBOLS=$(abspath vboxnetadp/Module.symvers) -C vboxnetadp || exit 1; \
	    cp vboxnetadp/vboxnetadp.ko .; \
	    echo; \
	fi

vboxpci: vboxdrv
	+@if [ -d vboxpci ]; then \
	    if [ -f vboxdrv/Module.symvers ]; then \
	        cp vboxdrv/Module.symvers vboxpci; \
	    fi; \
	    echo "=== Building 'vboxpci' module ==="; \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) KBUILD_EXTRA_SYMBOLS=$(abspath vboxpci/Module.symvers) -C vboxpci || exit 1; \
	    cp vboxpci/vboxpci.ko .; \
	    echo; \
	fi

install-vboxdrv:
	+@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv install

install-vboxnetflt:
	+@if [ -d vboxnetflt ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetflt install; \
	fi

install-vboxnetadp:
	+@if [ -d vboxnetadp ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxnetadp install; \
	fi

install-vboxpci:
	+@if [ -d vboxpci ]; then \
	    $(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxpci install; \
	fi

install: install-vboxdrv install-vboxnetflt install-vboxnetadp install-vboxpci

# Look for wrapper modules, sorting them so vmmr0 is first.
VBOX_WRAPPER_DIRS := $(notdir $(wildcard $(CURDIR)/vbox_*))
  ifneq ($(VBOX_WRAPPER_DIRS),)
VBOX_WRAPPER_DIRS := $(filter vbox_vmmr0,$(VBOX_WRAPPER_DIRS)) $(sort $(filter-out vbox_vmmr0,$(VBOX_WRAPPER_DIRS)))
  endif
  define wrapper_template
$(wrapper): $(subst $(wrapper),,vbox_vmmr0)
	+$$(MAKE) KBUILD_VERBOSE=$$(KBUILD_VERBOSE) -C $(wrapper)/

load-$(wrapper): $(subst load-$(wrapper),,load-vbox_vmmr0)
	@if ! grep -q "^$(wrapper) " /proc/modules; then \
       	echo "Loading $(wrapper)..."; \
		$(SUDO) /sbin/insmod $(wrapper)/$(wrapper).ko; \
	else \
       	echo "Skipping loading $(wrapper) module (already loaded)."; \
	fi

unload-$(wrapper):
	@if grep -q "^$(wrapper) " /proc/modules; then \
       	echo "Unloading $(wrapper)..."; \
		$(SUDO) /sbin/rmmod $(wrapper)/$(wrapper).ko; \
	fi
  endef
$(foreach wrapper,$(VBOX_WRAPPER_DIRS), $(eval $(wrapper_template)))

wrappers: 	$(VBOX_WRAPPER_DIRS)
wrappers-load: $(addprefix load-,$(VBOX_WRAPPER_DIRS))
wrappers-unload:
	@for module in $(filter-out vbox_vmmr0,$(VBOX_WRAPPER_DIRS)) $(filter vbox_vmmr0,$(VBOX_WRAPPER_DIRS)); \
	do \
		if grep -q "^$${module} " /proc/modules; then \
			echo "Unloading $${module}..."; \
			$(SUDO) /sbin/rmmod "$${module}"; \
		fi \
	done
wrappers-reload: wrappers-unload
	+$(MAKE) -f $(lastword $(MAKEFILE_LIST)) --no-print-directory wrappers-load

buildid:
	@for module in $(foreach module,vboxdrv vboxnetflt vboxnetadp $(VBOX_WRAPPER_DIRS),$(module)/$(module).ko); \
	do \
		buildid=`readelf -n $${module} | sed -ne 's/^.*Build ID: *\([[:xdigit:]][[:xdigit:]]\)\(.*\)$$/\1\/\2/p' `; \
		if [ -n "$${buildid}" ]; then \
			mkdir -p ~/.debug/.build-id/`dirname $${buildid}`; \
			ln -sfn -- "$(CURDIR)/$${module}" ~/.debug/.build-id/$${buildid}; \
		else \
			echo "warning: No build ID for $${module}"; \
		fi \
	done

.PHONY: wrappers wrappers-load wrappers-unload buildid $(VBOX_WRAPPER_DIRS) \
	$(addprefix load-,$(VBOX_WRAPPER_DIRS)) $(addprefix unload-,$(VBOX_WRAPPER_DIRS))

 else # Too old:

VBOX_WRAPPER_DIRS :=

vboxdrv:
vboxnetflt:
vboxnetadp:
vboxpci:
install:
wrappers:
wrappers-load:
wrappers-unload:
buildid:

 endif

clean-vboxdrv:
	+@$(MAKE) -C vboxdrv clean
	rm -rf vboxdrv.ko

clean-vboxnetflt:
	+@if [ -d vboxnetflt ]; then \
	    $(MAKE) -C vboxnetflt clean; \
	fi
	rm -rf vboxnetflt.ko

clean-vboxnetadp:
	+@if [ -d vboxnetadp ]; then \
	    $(MAKE) -C vboxnetadp clean; \
	fi
	rm -rf vboxnetadp.ko

clean-vboxpci:
	+@if [ -d vboxpci ]; then \
	    $(MAKE) -C vboxpci clean; \
	fi
	rm -f vboxpci.ko

clean: clean-vboxdrv clean-vboxnetflt clean-vboxnetadp clean-vboxpci

check:
	+@$(MAKE) KBUILD_VERBOSE=$(KBUILD_VERBOSE) -C vboxdrv check

unload:
	@for module in vboxpci vboxnetadp vboxnetflt vboxdrv; do \
		if grep "^$$module " /proc/modules >/dev/null; then \
			echo "Removing previously installed $$module module"; \
			$(SUDO) /sbin/rmmod $$module; \
		fi; \
	done

load: unload
	@for module in vboxdrv vboxnetflt vboxnetadp vboxpci; do \
		if test -f $$module.ko; then \
			echo "Installing $$module module"; \
			$(SUDO) /sbin/insmod $$module.ko; \
		fi; \
	done

endif # ! KERNELRELEASE

