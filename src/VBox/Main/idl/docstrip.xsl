<?xml version="1.0"?>

<!--
    Copies the source XIDL file to the output, except that all <desc>
    tags are stripped in the process. This is to generate a copy
    of VirtualBox.xidl which is then used as a source for generating
    the COM/XPCOM headers, the webservice files, the Qt bindings and
    others. The idea is that updating the documentation tags in the
    original XIDL should not cause a full recompile of nearly all of
    VirtualBox.
-->
<!--
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
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="xml" indent="yes" encoding="utf-8" />

<!-- copy everything unless there's a more specific template -->
<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

<!-- swallow desc -->
<xsl:template match="desc" />

<!-- swallow all comments -->
<xsl:template match="comment()" />

</xsl:stylesheet>

