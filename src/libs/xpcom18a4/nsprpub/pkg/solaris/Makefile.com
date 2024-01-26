#
# Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#ident	"$Id: Makefile.com $"
#

MACH = $(shell mach)

PUBLISH_ROOT = $(DIST)
ifeq ($(MOD_DEPTH),../..)
ROOT = ROOT
else
ROOT = $(subst ../../,,$(MOD_DEPTH))/ROOT
endif

PKGARCHIVE = $(dist_libdir)/pkgarchive
DATAFILES = copyright
FILES = $(DATAFILES) pkginfo prototype

PACKAGE = $(shell basename `pwd`)

PRODUCT_VERSION = $(shell grep PR_VERSION $(dist_includedir)/prinit.h \
		   | sed -e 's/"$$//' -e 's/.*"//' -e 's/ .*//')

LN = /usr/bin/ln

CLOBBERFILES = $(FILES)

include $(topsrcdir)/config/rules.mk

# vim: ft=make
