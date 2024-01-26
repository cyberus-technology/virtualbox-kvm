<?xml version="1.0"?>
<!--
    docbook-refentry-to-H-help.xsl:
        XSLT stylesheet for generating command and sub-command
        constants header for the built-in help.
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

  <xsl:output method="text" version="1.0" encoding="utf-8" indent="yes"/>
  <xsl:strip-space elements="*"/>

  <xsl:param name="g_sMode" select="not-specified"/>


  <!-- Default action, do nothing. -->
  <xsl:template match="node()|@*"/>

  <!--
    Generate SCOPE enum for a refentry.  We convert the
    cmdsynopsisdiv/cmdsynopsis IDs into constants.
    -->
  <xsl:template match="refentry">
    <xsl:variable name="RefEntry" select="."/>
    <xsl:variable name="sRefEntryId" select="@id"/>
    <xsl:variable name="sBaseNm">
      <xsl:choose>
        <xsl:when test="contains($sRefEntryId, '-')">   <!-- Multi level command. -->
          <xsl:call-template name="str:to-upper">
            <xsl:with-param name="text" select="translate(substring-after($sRefEntryId, '-'), '-', '_')"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>                                 <!-- Simple command. -->
          <xsl:call-template name="str:to-upper">
            <xsl:with-param name="text" select="translate($sRefEntryId, '-', '_')"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:choose>
      <!-- Generate subcommand enums and defines -->
      <xsl:when test="$g_sMode = 'subcmd'">
        <!-- Start enum type and start off with the refentry id. -->
        <xsl:text>
enum
{
#define HELP_SCOPE_</xsl:text>
        <xsl:value-of select="$sBaseNm"/>
        <xsl:value-of select="substring('                                               ',1,56 - string-length($sBaseNm) - 11)"/>
        <xsl:text> RT_BIT_64(HELP_SCOPE_</xsl:text><xsl:value-of select="$sBaseNm"/><xsl:text>_BIT)
        HELP_SCOPE_</xsl:text><xsl:value-of select="$sBaseNm"/><xsl:text>_BIT = 0</xsl:text>

        <!-- Synopsis IDs -->
        <xsl:for-each select="./refsynopsisdiv/cmdsynopsis[@id != concat('synopsis-', $sRefEntryId)]">
          <xsl:variable name="sSubNm">
            <xsl:text>HELP_SCOPE_</xsl:text>
            <xsl:call-template name="str:to-upper">
              <xsl:with-param name="text" select="translate(substring-after(substring-after(@id, '-'), '-'), '-', '_')"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:text>,
#define </xsl:text>
          <xsl:value-of select="$sSubNm"/>
          <xsl:value-of select="substring('                                               ',1,56 - string-length($sSubNm))"/>
          <xsl:text> RT_BIT_64(</xsl:text><xsl:value-of select="$sSubNm"/><xsl:text>_BIT)
        </xsl:text>
          <xsl:value-of select="$sSubNm"/><xsl:text>_BIT</xsl:text>
        </xsl:for-each>

        <!-- Add scoping info for refsect1 and refsect2 IDs that aren't part of the synopsis. -->
        <xsl:for-each select=".//refsect1[@id] | .//refsect2[@id]">
          <xsl:variable name="sThisId" select="@id"/>
          <xsl:if test="not($RefEntry[@id = $sThisId]) and not($RefEntry/refsynopsisdiv/cmdsynopsis[@id = concat('synopsis-', $sThisId)])">
            <xsl:variable name="sSubNm">
              <xsl:text>HELP_SCOPE_</xsl:text>
              <xsl:choose>
                <xsl:when test="contains($sRefEntryId, '-')">   <!-- Multi level command. -->
                  <xsl:call-template name="str:to-upper">
                    <xsl:with-param name="text" select="translate(substring-after(@id, '-'), '-', '_')"/>
                  </xsl:call-template>
                </xsl:when>
                <xsl:otherwise>                                 <!-- Simple command. -->
                  <xsl:call-template name="str:to-upper">
                    <xsl:with-param name="text" select="translate(@id, '-', '_')"/>
                  </xsl:call-template>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:variable>
            <xsl:text>,
#define </xsl:text>
            <xsl:value-of select="$sSubNm"/>
            <xsl:value-of select="substring('                                               ',1,56 - string-length($sSubNm))"/>
            <xsl:text> RT_BIT_64(</xsl:text><xsl:value-of select="$sSubNm"/><xsl:text>_BIT)
        </xsl:text>
            <xsl:value-of select="$sSubNm"/><xsl:text>_BIT</xsl:text>
          </xsl:if>
        </xsl:for-each>

        <!-- Done - complete the enum. -->
        <xsl:text>,
        HELP_SCOPE_</xsl:text><xsl:value-of select="$sBaseNm"/><xsl:text>_END
};
AssertCompile((int)HELP_SCOPE_</xsl:text><xsl:value-of select="$sBaseNm"/><xsl:text>_END &gt;= 1);
AssertCompile((int)HELP_SCOPE_</xsl:text><xsl:value-of select="$sBaseNm"/><xsl:text>_END &lt; 64);
AssertCompile(RT_BIT_64(HELP_SCOPE_</xsl:text><xsl:value-of select="$sBaseNm"/><xsl:text>_END - 1) &amp; RTMSGREFENTRYSTR_SCOPE_MASK);
</xsl:text>
      </xsl:when>

      <!-- Generate command enum value. -->
      <xsl:when test="$g_sMode = 'cmd'">
        <xsl:text>    HELP_CMD_</xsl:text><xsl:value-of select="$sBaseNm"/><xsl:text>,
</xsl:text>
      </xsl:when>

      <!-- Shouldn't happen. -->
      <xsl:otherwise>
        <xsl:message terminate="yes">Bad or missing g_sMode string parameter value.</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>

