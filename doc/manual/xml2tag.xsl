<?xml version="1.0"?>

<!--
    xml2tag.xsl:
        XSLT stylesheet that extracts just the tags of an XML document.
-->
<!--
    Copyright (C) 2018-2023 Oracle and/or its affiliates.

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

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:output method="text"/>

    <xsl:strip-space elements="*"/>

    <xsl:template match="*">
        <xsl:value-of select="name()"/>
        <xsl:text>&#10;</xsl:text>
        <xsl:apply-templates select="*"/>
    </xsl:template>

</xsl:stylesheet>
