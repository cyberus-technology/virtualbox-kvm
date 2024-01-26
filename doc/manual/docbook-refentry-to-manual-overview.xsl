<?xml version="1.0"?>
<!--
    docbook-refentry-to-manual-sect1.xsl:
        XSLT stylesheet for nicking the refsynopsisdiv bit of a
        refentry (manpage) for use in the command overview section
        in the user manual.
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
  >

  <xsl:output method="xml" version="1.0" encoding="utf-8" indent="yes"/>
  <xsl:strip-space elements="*"/>


<!-- Base operation is to copy. -->
<xsl:template match="node()|@*">
  <xsl:copy>
     <xsl:apply-templates select="node()|@*"/>
  </xsl:copy>
</xsl:template>

<!--
  The refentry element is the top level one.  We only process the
  refsynopsisdiv sub element within it, since that is all we want.
  -->
<xsl:template match="refentry">
  <xsl:apply-templates select="refsynopsisdiv"/>
</xsl:template>

<!--
  Combine all cmdsynopsis lines into a bunch of commands.
 -->
<xsl:template match="refsynopsisdiv">
  <xsl:if test="not(cmdsynopsis)">
    <xsl:message terminate="yes">What? No cmdsynopsis in the refsynopsisdiv?</xsl:message>
  </xsl:if>
  <xsl:element name="cmdsynopsis">
    <xsl:attribute name="id"><xsl:value-of select="/refentry/@id"/><xsl:text>-overview</xsl:text></xsl:attribute>
    <xsl:for-each select="cmdsynopsis">
      <xsl:copy-of select="node()"/>
    </xsl:for-each>
  </xsl:element>
</xsl:template>

<!-- Remove all remarks (for now). -->
<xsl:template match="remark"/>


</xsl:stylesheet>

