<?xml version="1.0"?>
<!--
    docbook-refentry-to-manual-sect1.xsl:
        XSLT stylesheet for transforming a refentry (manpage)
        to a sect1 for the user manual.
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
  xmlns:str="http://xsltsl.org/string"
  >

  <xsl:import href="string.xsl"/>

  <xsl:output method="xml" version="1.0" encoding="utf-8" indent="yes"/>
  <xsl:strip-space elements="*"/>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->



<!-- - - - - - - - - - - - - - - - - - - - - - -
  base operation is to copy.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="node()|@*">
  <xsl:copy>
     <xsl:apply-templates select="node()|@*"/>
  </xsl:copy>
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -

 - - - - - - - - - - - - - - - - - - - - - - -->

<!-- rename refentry to sect1 -->
<xsl:template match="refentry">
  <xsl:element name="sect1">
    <xsl:attribute name="condition">refentry</xsl:attribute>
    <xsl:apply-templates select="node()|@*"/>
  </xsl:element>
</xsl:template>

<!-- Remove refentryinfo, keeping the title element. -->
<xsl:template match="refentryinfo">
  <xsl:if test="./title">
    <xsl:copy-of select="./title"/>
  </xsl:if>
</xsl:template>

<!-- Morph refnamediv into a brief description. -->
<xsl:template match="refnamediv">
  <xsl:element name="para">
    <xsl:call-template name="capitalize">
      <xsl:with-param name="text" select="normalize-space(./refpurpose)"/>
    </xsl:call-template>
    <xsl:text>.</xsl:text>
  </xsl:element>
</xsl:template>

<!-- Morph the refsynopsisdiv part into a synopsis section. -->
<xsl:template match="refsynopsisdiv">
  <xsl:if test="name(*[1]) != 'cmdsynopsis'"><xsl:message terminate="yes">Expected refsynopsisdiv to start with cmdsynopsis</xsl:message></xsl:if>
  <xsl:if test="title"><xsl:message terminate="yes">No title element supported in refsynopsisdiv</xsl:message></xsl:if>

  <xsl:element name="sect2">
    <xsl:attribute name="role">not-in-toc</xsl:attribute>
    <xsl:attribute name="condition">refsynopsisdiv</xsl:attribute>
    <xsl:element name="title">
    <xsl:text>Synopsis</xsl:text>
    </xsl:element>
    <xsl:apply-templates />
  </xsl:element>

</xsl:template>

<!-- refsect1 -> sect2 -->
<xsl:template match="refsect1">
  <xsl:if test="not(title)"><xsl:message terminate="yes">refsect1 requires title</xsl:message></xsl:if>
  <xsl:element name="sect2">
    <xsl:attribute name="role">not-in-toc</xsl:attribute>
    <xsl:attribute name="condition">refsect1</xsl:attribute>
    <xsl:if test="@id">
      <xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
    </xsl:if>
    <xsl:apply-templates />
  </xsl:element>
</xsl:template>

<!-- refsect2 -> sect3 -->
<xsl:template match="refsect2">
  <xsl:if test="not(title)"><xsl:message terminate="yes">refsect2 requires title</xsl:message></xsl:if>
  <xsl:element name="sect3">
    <xsl:attribute name="role">not-in-toc</xsl:attribute>
    <xsl:attribute name="condition">refsect2</xsl:attribute>
    <xsl:if test="@id">
      <xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
    </xsl:if>
    <xsl:apply-templates />
  </xsl:element>
</xsl:template>

<!-- refsect3 -> sect4 -->
<xsl:template match="refsect3">
  <xsl:if test="not(title)"><xsl:message terminate="yes">refsect3 requires title</xsl:message></xsl:if>
  <xsl:element name="sect4">
    <xsl:attribute name="role">not-in-toc</xsl:attribute>
    <xsl:attribute name="condition">refsect3</xsl:attribute>
    <xsl:if test="@id">
      <xsl:attribute name="id"><xsl:value-of select="@id"/></xsl:attribute>
    </xsl:if>
    <xsl:apply-templates />
  </xsl:element>
</xsl:template>


<!-- Remove refmeta. -->
<xsl:template match="refmeta"/>

<!--
 remark extensions:
 -->
<!-- Default: remove all remarks. -->
<xsl:template match="remark"/>

<!-- help-manual - stuff that should only be included in the manual. -->
<xsl:template match="remark[@role = 'help-manual']">
  <xsl:apply-templates/>
</xsl:template>

<!-- help-copy-synopsis - used in refsect2 to copy synopsis from the refsynopsisdiv. -->
<xsl:template match="remark[@role = 'help-copy-synopsis']">
  <xsl:if test="not(parent::refsect2)">
    <xsl:message terminate="yes">The help-copy-synopsis remark is only applicable in refsect2.</xsl:message>
  </xsl:if>
  <xsl:variable name="sSrcId" select="concat('synopsis-',../@id)"/>
  <xsl:if test="not(/refentry/refsynopsisdiv/cmdsynopsis[@id = $sSrcId])">
    <xsl:message terminate="yes">Could not find any cmdsynopsis with id=<xsl:value-of select="$sSrcId"/> in refsynopsisdiv.</xsl:message>
  </xsl:if>
  <xsl:element name="cmdsynopsis">
    <xsl:copy-of select="/refentry/refsynopsisdiv/cmdsynopsis[@id = $sSrcId]/node()"/>
  </xsl:element>
</xsl:template>

<!--
 Captializes the given text.
 -->
<xsl:template name="capitalize">
  <xsl:param name="text"/>
  <xsl:call-template name="str:to-upper">
    <xsl:with-param name="text" select="substring($text,1,1)"/>
  </xsl:call-template>
  <xsl:value-of select="substring($text,2)"/>
</xsl:template>

</xsl:stylesheet>

