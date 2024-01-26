# $Id: qt-Q_OBJECT.sed 3558 2022-03-02 00:47:18Z bird $
## @file
# Qt unit - sed script for checking for Q_OBJECT in a file.
#
# This is not very sophisticated, but it helps avoid generating
# files we don't need. It outputs '1' when Q_OBJECT is found
# and then quits, allowing us to do $(if $(shell ...),moc_...).
# is
#

#
# Copyright (c) 2008-2010 knut st. osmundsen <bird-kBuild-spamx@anduin.net>
#
# This file is part of kBuild.
#
# kBuild is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# kBuild is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with kBuild; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
#

/Q_OBJECT/!b skip
s/^.*$/1/
q 0
:skip
d
