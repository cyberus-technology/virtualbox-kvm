<?xml version="1.0"?>

<!--
    Generate a list of dependencies from a wixobj file.
-->
<!--
    Copyright (C) 2015-2023 Oracle and/or its affiliates.

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
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:wix="http://schemas.microsoft.com/wix/2006/objects"
    >
<xsl:output method="text" encoding="utf-8"/>

<xsl:strip-space elements="*"/>

<xsl:include href="../../Main/idl/typemap-shared.inc.xsl"/>


<xsl:template name="output-file">
    <xsl:param name="name"/>
    <xsl:if test="1 or substring($name, 2, 1) = ':'">
        <xsl:text>    </xsl:text>
        <xsl:value-of select="translate($name, '\', '/')"/>
        <xsl:text> \</xsl:text>
        <xsl:call-template name="xsltprocNewlineOutputHack"/>
    </xsl:if>
</xsl:template>

<xsl:template match="wix:table[@name = 'Binary' or @name = 'Icon']">
    <xsl:for-each select="wix:row/wix:field[2]">
        <xsl:call-template name="output-file">
            <xsl:with-param name="name" select="normalize-space(.)"/>
        </xsl:call-template>
    </xsl:for-each>
</xsl:template>

<xsl:template match="wix:table[@name = 'WixFile']">
    <xsl:for-each select="wix:row/wix:field[7]">
        <xsl:call-template name="output-file">
            <xsl:with-param name="name" select="normalize-space(.)"/>
        </xsl:call-template>
    </xsl:for-each>
</xsl:template>

<xsl:template match="wix:wixObject">
    <xsl:apply-templates/>
</xsl:template>

<xsl:template match="wix:section">
    <xsl:apply-templates/>
</xsl:template>

<!-- Eat everything that's unmatched. -->
<xsl:template match="*">
</xsl:template>

</xsl:stylesheet>

