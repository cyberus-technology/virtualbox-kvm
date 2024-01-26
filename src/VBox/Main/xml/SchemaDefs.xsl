<?xml version="1.0"?>

<!--
 *  A template to generate a header that will contain some important constraints
 *  extracted from the VirtualBox XML Schema (VirtualBox-settings-*.xsd).
 *  The output file name must be SchemaDefs.h.
 *
 *  This template depends on XML Schema structure (type names and constraints)
 *  and should be reviewed on every Schema change.
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

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
>
<xsl:output method="text"/>

<xsl:strip-space elements="*"/>

<xsl:param name="mode" expr=''/>

<!--
//  helpers
////////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  Extract the specified value and assign it to an enum member with the given
 *  name
-->
<xsl:template name="defineEnumMember">
    <xsl:param name="member"/>
    <xsl:param name="select"/>
    <xsl:if test="$select">
      <xsl:value-of select="concat('        ', $member, ' = ', $select, ',&#x0A;')"/>
    </xsl:if>
</xsl:template>

<!--
//  templates
////////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  shut down all implicit templates
-->
<xsl:template match="*"/>
<xsl:template match="*" mode="declare"/>
<xsl:template match="*" mode="declare.enum"/>
<xsl:template match="*" mode="define"/>

<xsl:template match="/">
  <xsl:choose>
    <xsl:when test="$mode='declare'">
      <xsl:apply-templates select="/" mode="declare"/>
    </xsl:when>
    <xsl:when test="$mode='define'">
      <xsl:apply-templates select="/" mode="define"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
Value '<xsl:value-of select="$mode"/>' of parameter 'mode' is invalid!
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  declare mode (C++ header file)
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-->

<xsl:template match="/" mode="declare">
<xsl:text>
/*
 *  DO NOT EDIT.
 *
 *  This header is automatically generated from the VirtualBox XML Settings
 *  Schema and contains selected schema constraints declared in C++.
 */

#ifndef ____H_SCHEMADEFS
#define ____H_SCHEMADEFS

namespace SchemaDefs
{
    enum
    {
</xsl:text>

  <xsl:apply-templates select="xsd:schema" mode="declare.enum"/>

<xsl:text>        DummyTerminator
    };
</xsl:text>

<xsl:apply-templates select="xsd:schema" mode="declare"/>

<xsl:text>}

#endif // !____H_SCHEMADEFS
</xsl:text>
</xsl:template>

<!--
 *  enumeration values
-->
<xsl:template match="xsd:schema" mode="declare.enum">

  <!-- process include statements -->
  <xsl:for-each select="xsd:include">
    <xsl:apply-templates select="document(@schemaLocation)/xsd:schema" mode="declare.enum"/>
  </xsl:for-each>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MinGuestRAM'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='TMemory']/xsd:attribute[@name='RAMSize']//xsd:minInclusive/@value
    "/>
  </xsl:call-template>
  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MaxGuestRAM'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='TMemory']/xsd:attribute[@name='RAMSize']//xsd:maxInclusive/@value
    "/>
  </xsl:call-template>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MinGuestVRAM'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='TDisplay']/xsd:attribute[@name='VRAMSize']//xsd:minInclusive/@value
    "/>
  </xsl:call-template>
  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MaxGuestVRAM'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='TDisplay']/xsd:attribute[@name='VRAMSize']//xsd:maxInclusive/@value
    "/>
  </xsl:call-template>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MinCPUCount'"/>
    <xsl:with-param name="select" select="
      xsd:simpleType[@name='TCPUCount']//xsd:minInclusive/@value
    "/>
  </xsl:call-template>
  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MaxCPUCount'"/>
    <xsl:with-param name="select" select="
      xsd:simpleType[@name='TCPUCount']//xsd:maxInclusive/@value
    "/>
  </xsl:call-template>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MaxGuestMonitors'"/>
    <xsl:with-param name="select" select="
      xsd:simpleType[@name='TMonitorCount']//xsd:maxInclusive/@value
    "/>
  </xsl:call-template>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'SerialPortCount'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='TUARTPort']/xsd:attribute[@name='slot']//xsd:maxExclusive/@value
    "/>
  </xsl:call-template>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'ParallelPortCount'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='TLPTPort']/xsd:attribute[@name='slot']//xsd:maxExclusive/@value
    "/>
  </xsl:call-template>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'MaxBootPosition'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='TBoot']//xsd:element[@name='Order']//xsd:attribute[@name='position']//xsd:maxInclusive/@value
    "/>
  </xsl:call-template>

  <xsl:call-template name="defineEnumMember">
    <xsl:with-param name="member" select="'DefaultHardwareVersion'"/>
    <xsl:with-param name="select" select="
      xsd:complexType[@name='THardware']/xsd:attribute[@name='version']/@default
    "/>
  </xsl:call-template>

</xsl:template>

<!--
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  define mode (C++ source file)
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-->

<xsl:template match="/" mode="define">
<xsl:text>
/*
 *  DO NOT EDIT.
 *
 *  This source is automatically generated from the VirtualBox XML Settings
 *  Schema and contains selected schema constraints defined in C++.
 */

#include "SchemaDefs.h"

namespace SchemaDefs
{
</xsl:text>

<xsl:apply-templates select="xsd:schema" mode="define"/>

<xsl:text>}
</xsl:text>
</xsl:template>

<!--
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  END
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-->

</xsl:stylesheet>
