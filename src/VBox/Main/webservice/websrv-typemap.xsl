<?xml version="1.0"?>

<!--
    websrv-typemap.xsl:
        XSLT stylesheet that generates a typemap file from
        VirtualBox.xidl for use with the gSOAP compilers.
        See webservice/Makefile.kmk for an overview of all the things
        generated for the webservice.
-->
<!--
    Copyright (C) 2006-2023 Oracle and/or its affiliates.

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
  xmlns:xsd="http://www.w3.org/2001/XMLSchema">

  <xsl:output method="text"/>

  <xsl:strip-space elements="*"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_xsltFilename" select="'websrv-typemap.xsl'" />

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<!-- - - - - - - - - - - - - - - - - - - - - - -
  root match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="/idl">
  <xsl:text><![CDATA[
# DO NOT EDIT! This is a generated file.
# Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
# Generator: src/VBox/Main/webservice/websrv-typemap.xsl

# forces typedefs:
xsd__int = | long
xsd__unsignedInt = | unsigned long

# xsd__short =| int16_t
# xsd__unsignedShort =| uint16_t
# xsd__int =| int32_t
# xsd__unsignedInt =| uint32_t
# xsd__long =| int64_t
# xsd__unsignedLong =| uint64_t

# Main namespace (which is mapped to vbox__ prefixes):
]]></xsl:text>
  <xsl:value-of select="concat('vbox = &quot;', $G_targetNamespace, '&quot;')" />
  <xsl:text>

# Namespaces for the interfaces in xidl that need to be mapped according to their wsmap attribs:
</xsl:text>
  <xsl:apply-templates />
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  if
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
 *  ignore all |if|s except those for WSDL target
-->
<xsl:template match="if">
  <xsl:if test="@target='wsdl'">
    <xsl:apply-templates/>
  </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  cpp
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="cpp">
<!--  ignore this -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  library
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="library">
  <xsl:apply-templates />
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  class
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="module/class">
<!--  TODO swallow for now -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  enum
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="enum">
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  const
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
<xsl:template match="const">
  <xsl:apply-templates />
</xsl:template>
-->

<!-- - - - - - - - - - - - - - - - - - - - - - -
  desc
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="desc">
<!--  TODO swallow for now -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  note
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="note">
<!--  TODO -->
  <xsl:apply-templates />
</xsl:template>

<xsl:template match="interface | collection">
  <!-- remember the interface name in local variables -->
  <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
  <xsl:variable name="wsmap"><xsl:value-of select="@wsmap" /></xsl:variable>
  <xsl:choose>
    <xsl:when test="$wsmap='struct'" />
    <xsl:when test="$wsmap='suppress'" />
    <xsl:otherwise>
      <xsl:value-of select="concat($ifname, ' = ', $G_targetNamespace, $G_targetNamespaceSeparator,
                                   $ifname, $G_bindingSuffix, $G_sNewLine)" />
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
