<?xml version="1.0"?>

<!--
    apiwrap-server-filelist.xsl:

        XSLT stylesheet that generate a makefile include with
        the lists of files that apiwrap-server.xsl produces
        from VirtualBox.xidl.
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
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

<xsl:output method="text"/>

<xsl:strip-space elements="*"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
   XSLT parameters
 - - - - - - - - - - - - - - - - - - - - - - -->

<!-- Whether to generate wrappers for VBoxSDS-->
<xsl:param name="g_fVBoxWithSDS" select="'no'"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_sNewLine">
    <xsl:choose>
        <xsl:when test="$KBUILD_HOST = 'win'">
            <xsl:value-of select="'&#13;&#10;'"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:value-of select="'&#10;'"/>
        </xsl:otherwise>
    </xsl:choose>
</xsl:variable>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  wildcard match, ignore everything which has no explicit match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="*" mode="filelist-even-sources"/>
<xsl:template match="*" mode="filelist-odd-sources"/>
<xsl:template match="*" mode="filelist-headers"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  interface match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="interface" mode="filelist-even-sources">
    <xsl:if test="not(@internal='yes') and not(@autogen='VBoxEvent') and not(@supportsErrorInfo='no') and (position() mod 2) = 0">
        <xsl:value-of select="concat(' \', $G_sNewLine, '&#9;$(VBOX_MAIN_APIWRAPPER_DIR)/', substring(@name, 2), 'Wrap.cpp')"/>
    </xsl:if>
</xsl:template>

<xsl:template match="interface" mode="filelist-odd-sources">
    <xsl:if test="not(@internal='yes') and not(@autogen='VBoxEvent') and not(@supportsErrorInfo='no') and (position() mod 2) = 1">
        <xsl:value-of select="concat(' \', $G_sNewLine, '&#9;$(VBOX_MAIN_APIWRAPPER_DIR)/', substring(@name, 2), 'Wrap.cpp')"/>
    </xsl:if>
</xsl:template>

<xsl:template match="interface" mode="filelist-headers">
    <xsl:if test="not(@internal='yes') and not(@autogen='VBoxEvent') and not(@supportsErrorInfo='no')">
        <xsl:value-of select="concat(' \', $G_sNewLine, '&#9;$(VBOX_MAIN_APIWRAPPER_DIR)/', substring(@name, 2), 'Wrap.h')"/>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  ignore all if tags except those for XPIDL or MIDL target
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="if" mode="filelist-even-sources">
    <xsl:if test="(@target = 'xpidl') or (@target = 'midl')">
        <xsl:apply-templates mode="filelist-even-sources"/>
    </xsl:if>
</xsl:template>

<xsl:template match="if" mode="filelist-odd-sources">
    <xsl:if test="(@target = 'xpidl') or (@target = 'midl')">
        <xsl:apply-templates mode="filelist-odd-sources"/>
    </xsl:if>
</xsl:template>

<xsl:template match="if" mode="filelist-headers">
    <xsl:if test="(@target = 'xpidl') or (@target = 'midl')">
        <xsl:apply-templates mode="filelist-headers"/>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  application match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="application" mode="filelist-even-sources" name="template_app_filelist_even_sources">
    <xsl:apply-templates mode="filelist-even-sources"/>
</xsl:template>

<xsl:template match="application" mode="filelist-odd-sources" name="template_app_filelist_odd_sources">
    <xsl:apply-templates mode="filelist-odd-sources"/>
</xsl:template>

<xsl:template match="application" mode="filelist-headers" name="template_app_filelist_headers">
    <xsl:apply-templates mode="filelist-headers"/>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  library match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="library" mode="filelist-even-sources">
    <xsl:apply-templates mode="filelist-even-sources"/>
</xsl:template>

<xsl:template match="library" mode="filelist-odd-sources">
    <xsl:apply-templates mode="filelist-odd-sources"/>
</xsl:template>

<xsl:template match="library" mode="filelist-headers">
    <xsl:apply-templates mode="filelist-headers"/>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  root match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="/idl">
    <xsl:text>VBOX_MAIN_APIWRAPPER_GEN_SRCS_EVEN := </xsl:text>
    <xsl:apply-templates mode="filelist-even-sources"/>
    <xsl:value-of select="concat($G_sNewLine, $G_sNewLine)"/>

    <xsl:text>VBOX_MAIN_APIWRAPPER_GEN_SRCS_ODD := </xsl:text>
    <xsl:apply-templates mode="filelist-odd-sources"/>
    <xsl:value-of select="concat($G_sNewLine, $G_sNewLine)"/>

    <xsl:text>VBOX_MAIN_APIWRAPPER_GEN_SRCS := $(VBOX_MAIN_APIWRAPPER_GEN_SRCS_EVEN) $(VBOX_MAIN_APIWRAPPER_GEN_SRCS_ODD)</xsl:text>
    <xsl:value-of select="concat($G_sNewLine, $G_sNewLine)"/>

    <xsl:text>VBOX_MAIN_APIWRAPPER_GEN_HDRS := </xsl:text>
    <xsl:apply-templates mode="filelist-headers"/>
    <xsl:value-of select="concat($G_sNewLine, $G_sNewLine)"/>
</xsl:template>


    <xsl:template match="application[@uuid='ec0e78e8-fa43-43e8-ac0a-02c784c4a4fa']" mode="filelist-even-sources" >
        <xsl:if test="$g_fVBoxWithSDS='yes'" >
            <xsl:call-template name="template_app_filelist_even_sources" />
        </xsl:if>
    </xsl:template>

    <xsl:template match="application[@uuid='ec0e78e8-fa43-43e8-ac0a-02c784c4a4fa']" mode="filelist-headers" >
        <xsl:if test="$g_fVBoxWithSDS='yes'" >
            <xsl:call-template name="template_app_filelist_headers" />
        </xsl:if>
    </xsl:template>

    <xsl:template match="application[@uuid='ec0e78e8-fa43-43e8-ac0a-02c784c4a4fa']" mode="filelist-odd-sources" >
        <xsl:if test="$g_fVBoxWithSDS='yes'" >
            <xsl:call-template name="template_app_filelist_odd_sources" />
        </xsl:if>
    </xsl:template>


</xsl:stylesheet>
<!-- vi: set tabstop=4 shiftwidth=4 expandtab: -->

