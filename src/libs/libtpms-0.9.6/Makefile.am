#
# Makefile.am
#
#       The Initial Developer of the Original Code is International
#       Business Machines Corporation. Portions created by IBM
#       Corporation are Copyright (C) 2005, 2011 International Business
#       Machines Corporation. All Rights Reserved.
#
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the Common Public License as published by
#       IBM Corporation; either version 1 of the License, or (at your option)
#       any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       Common Public License for more details.
#
#       You should have received a copy of the Common Public License
#       along with this program; if not, a copy can be viewed at
#       http://www.opensource.org/licenses/cpl1.0.php.
#

SUBDIRS   = \
	include \
	man \
	src \
	tests

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	CHANGES \
	INSTALL \
	LICENSE \
	README \
	bootstrap.sh \
	libtpms.pc.in \
	autogen.sh

pkgconfig_DATA = libtpms.pc

DISTCHECK_CONFIGURE_FLAGS = --with-openssl --with-tpm2
