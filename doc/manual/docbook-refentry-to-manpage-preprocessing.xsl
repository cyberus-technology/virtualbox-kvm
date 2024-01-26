<?xml version="1.0"?>
<!--
    docbook-refentry-to-manpage-preprocessing.xsl:
        XSLT stylesheet preprocessing remarks elements before
        turning a refentry (manpage) into a unix manual page and
        VBoxManage built-in help.
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

  <xsl:output method="xml" version="1.0" encoding="utf-8" indent="no"/>

  <!--
    The default action is to copy everything.
    -->
  <xsl:template match="node()|@*">
    <xsl:copy>
      <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>

  <!--
    Execute synopsis copy remark (avoids duplication for complicated xml).
    We strip the attributes off it.
    -->
  <xsl:template match="remark[@role = 'help-copy-synopsis']">
    <xsl:choose>
      <xsl:when test="parent::refsect2"/>
      <xsl:otherwise>
        <xsl:message terminate="yes">Misplaced remark/@role=help-copy-synopsis element.
Only supported on: refsect2</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:variable name="sSrcId">
      <xsl:choose>
        <xsl:when test="@condition"><xsl:value-of select="concat('synopsis-', @condition)"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat('synopsis-', ../@id)"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="CmdSynopsis" select="/refentry/refsynopsisdiv/cmdsynopsis[@id = $sSrcId]"/>
    <xsl:if test="not($CmdSynopsis)">
      <xsl:message terminate="yes">Could not find any cmdsynopsis with id=<xsl:value-of select="$sSrcId"/> in refsynopsisdiv.</xsl:message>
    </xsl:if>

    <xsl:element name="cmdsynopsis">
      <xsl:apply-templates select="$CmdSynopsis/node()"/>
    </xsl:element>

  </xsl:template>

  <!--
    Remove bits only intended for the manual.
    -->
  <xsl:template match="remark[@role='help-manual']"/>

  <!--
   Remove remarks without a role.
   These are used for leaving questions and such while working with the documentation team.
  -->
  <xsl:template match="remark[not(@role)]"/>


</xsl:stylesheet>

