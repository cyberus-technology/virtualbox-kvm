## @file
# Makefile for a sample/testcase using the 'glue' Java API bindings

#
# Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#
# User serviceable parts: adjust the following lines appropriately
#

# The host OS: linux, solaris, win, darwin, freebsd
HOSTOS=linux
# Absolute (!) path to the VirtualBox install directory
VBOX_BIN = /opt/VirtualBox
# Path to the sdk directory of the unpacked VirtualBox SDK
VBOX_SDK = ../../..
# On Windows, if you want to use the COM API: install directory of Jacob lib
JACOB_DIR =
# Extra classpath entries needed for compiling/running the sample
CLASSPATH =
# Java compiler to use
JAVAC = javac
# Java VM to use
JAVA = java


#
# No changes should be necessary after this point.
#

ifeq ($(HOSTOS),win)
 JACOB_JAR=$(JACOB_DIR)/jacob.jar
 VBOX_JAR=$(VBOX_SDK)/bindings/mscom/java/vboxjmscom.jar
 SEP=\;
 JAVA_COM_ARGS += -Djava.library.path=$(JACOB_DIR)
 CLASSPATH += $(JACOB_JAR)$(SEP)
else
 VBOX_JAR=$(VBOX_SDK)/bindings/xpcom/java/vboxjxpcom.jar
 SEP=:
 JAVA_COM_ARGS += -Dvbox.home=$(VBOX_BIN)
endif

VBOX_JAR_WS=$(VBOX_SDK)/bindings/webservice/java/jax-ws/vboxjws.jar


all: ws/TestVBox.class com/TestVBox.class

test: ws/test-TestVBox com/test-TestVBox

com/TestVBox.class:
	@mkdir com 2>/dev/null || true
	$(JAVAC) -d com -cp $(VBOX_JAR)$(SEP)$(CLASSPATH) TestVBox.java

com/test-TestVBox: com/TestVBox.class
	$(JAVA) $(JAVA_COM_ARGS) -cp com$(SEP)$(VBOX_JAR)$(SEP)$(CLASSPATH) TestVBox

ws/TestVBox.class:
	@mkdir ws 2>/dev/null || true
	$(JAVAC) -d ws -cp $(VBOX_JAR_WS)$(SEP)$(CLASSPATH) TestVBox.java

ws/test-TestVBox: ws/TestVBox.class
	$(JAVA) $(JAVA_WS_ARGS) -cp ws$(SEP)$(VBOX_JAR_WS)$(SEP)$(CLASSPATH) TestVBox -w -url http://localhost:18083/
