<?xml version="1.0"?>

<!--
    A template to generate a C header that will contain all result code
    definitions as entires of the const RTCOMERRMSG array (for use in the
    %Rhrc format specifier) as they are defined in the VirtualBox interface
    definition file (src/VBox/Main/idl/VirtualBox.xidl).
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

    The contents of this file may alternatively be used under the terms
    of the Common Development and Distribution License Version 1.0
    (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
    in the VirtualBox distribution, in which case the provisions of the
    CDDL are applicable instead of those of the GPL.

    You may elect to license modified versions of this file under the
    terms and conditions of either the GPL or the CDDL or both.

    SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
-->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
>
<xsl:output method="text"/>

<xsl:strip-space elements="*"/>

<!--
//  helpers
////////////////////////////////////////////////////////////////////////////////
-->

<!--
//  templates
////////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  shut down all implicit templates
-->
<xsl:template match="*"/>

<xsl:template match="idl">
  <xsl:for-each select="library/application/result">
    <xsl:text>{ "</xsl:text>
    <xsl:choose>
      <xsl:when test="contains(normalize-space(desc/text()), '. ')">
        <xsl:value-of select="normalize-space(substring-before(desc/text(), '. '))"/>
      </xsl:when>
      <xsl:when test="contains(normalize-space(desc/text()), '.')">
        <xsl:value-of select="normalize-space(substring-before(desc/text(), '.'))"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="normalize-space(desc/text())"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>", "</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>", (VBOXSTATUSTYPE)</xsl:text>
    <xsl:value-of select="@value"/>
    <xsl:text>L },&#x0A;</xsl:text>
  </xsl:for-each>
</xsl:template>

<!--
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  END
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-->

</xsl:stylesheet>

