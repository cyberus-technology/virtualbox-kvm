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
  xmlns:str="http://xsltsl.org/string"
  >

  <xsl:import href="string.xsl"/>
  <xsl:import href="common-formatcfg.xsl"/>

  <xsl:output method="text" version="1.0" encoding="utf-8" indent="yes"/>
  <xsl:strip-space elements="*"/>

  <xsl:param name="g_fDebugText" select="0"/>

  <xsl:variable name="g_sUnderlineRefSect1">
    <xsl:text>===================================================================================================================</xsl:text>
  </xsl:variable>
  <xsl:variable name="g_sUnderlineRefSect2">
    <xsl:text>-------------------------------------------------------------------------------------------------------------------</xsl:text>
  </xsl:variable>
  <xsl:variable name="g_sNewLine"><xsl:value-of select="'&#10;'" /></xsl:variable>

  <!-- Sub-command style command (true) or single command (false). -->
  <xsl:variable name="g_fSubCommands" select="not(not(//refsect2[@id]))" />

  <!-- Translatable strings -->
  <xsl:variable name="sUsage"           select="'Usage'"/>
  <xsl:variable name="sUsageUnderscore" select="'====='"/>


  <!-- Default action, do nothing. -->
  <xsl:template match="node()|@*"/>

  <!--
    main() - because we need to order the output in a specific manner
             that is contrary to the data flow in the refentry, this is
             going to look a bit more like a C program than a stylesheet.
    -->
  <xsl:template match="refentry">
    <!-- Assert refetry expectations. -->
    <xsl:if test="not(./refsynopsisdiv)">
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>refentry must have a refsynopsisdiv</xsl:message>
    </xsl:if>
    <xsl:if test="not(./refentryinfo/title)">
      <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>refentry must have a refentryinfo with title</xsl:message>
    </xsl:if>
    <xsl:if test="not(./refmeta/refentrytitle)">
      <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>refentry must have a refentryinfo with title</xsl:message>
    </xsl:if>
    <xsl:if test="./refmeta/refentrytitle != ./refnamediv/refname">
      <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>The refmeta/refentrytitle and the refnamediv/refname must be identical</xsl:message>
    </xsl:if>
    <xsl:if test="not(./refsect1/title)">
      <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>refentry must have a refsect1 with title</xsl:message>
    </xsl:if>
    <xsl:if test="not(@id) or @id = ''">
      <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>refentry must have an id attribute</xsl:message>
    </xsl:if>

    <!-- variables -->
    <xsl:variable name="sBaseId" select="@id"/>
    <xsl:variable name="sDataBaseSym" select="concat('g_', translate(@id, '-', '_'))"/>


    <!--
      Convert the refsynopsisdiv into REFENTRY::Synopsis data.
      -->
    <xsl:text>

static const RTMSGREFENTRYSTR </xsl:text><xsl:value-of select="$sDataBaseSym"/><xsl:text>_synopsis[] =
{</xsl:text>
    <xsl:for-each select="./refsynopsisdiv/cmdsynopsis">
      <!-- Assert synopsis expectations -->
      <xsl:if test="not(@id) or substring-before(@id, '-') != 'synopsis'">
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>The refsynopsisdiv/cmdsynopsis elements must have an id starting with 'synopsis-'.</xsl:message>
      </xsl:if>
      <xsl:if test="not(starts-with(substring-after(@id, '-'), $sBaseId))">
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>The refsynopsisdiv/cmdsynopsis elements @id is expected to include the refentry @id.</xsl:message>
      </xsl:if>
      <xsl:if test="not(../../refsect1/refsect2[@id=./@id]) and $g_fSubCommands">
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>No refsect2 with id="<xsl:value-of select="@id"/>" found.</xsl:message>
      </xsl:if>

      <!-- Do the work. -->
      <xsl:apply-templates select="."/>

    </xsl:for-each>
    <xsl:text>
};</xsl:text>


    <!--
      Convert the whole manpage to help text.
      -->
    <xsl:text>
static const RTMSGREFENTRYSTR </xsl:text><xsl:value-of select="$sDataBaseSym"/><xsl:text>_full_help[] =
{</xsl:text>
    <!-- We start by combining the refentry title and the refpurpose into a short description. -->
    <xsl:text>
    {   </xsl:text><xsl:call-template name="calc-scope-for-refentry"/><xsl:text>,
        "</xsl:text>
        <xsl:apply-templates select="./refentryinfo/title/node()"/>
        <xsl:text> -- </xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="text">
            <xsl:apply-templates select="./refnamediv/refpurpose/node()"/>
          </xsl:with-param>
        </xsl:call-template>
        <xsl:text>." },
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "" },</xsl:text>

    <!-- The follows the usage (synopsis) section. -->
    <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_GLOBAL,
        "</xsl:text><xsl:value-of select="$sUsage"/><xsl:text>" },
    {   RTMSGREFENTRYSTR_SCOPE_SAME,
        "</xsl:text><xsl:value-of select="$sUsageUnderscore"/><xsl:text>" },</xsl:text>
        <xsl:apply-templates select="./refsynopsisdiv/node()"/>

    <!-- Then comes the description and other refsect1 -->
    <xsl:for-each select="./refsect1">
      <xsl:if test="name(*[1]) != 'title'"><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Expected title as the first element in refsect1.</xsl:message></xsl:if>
      <xsl:if test="text()"><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>No text supported in refsect1.</xsl:message></xsl:if>
      <xsl:if test="not(./remark[@role='help-skip'])">
        <xsl:variable name="sTitle">
          <xsl:apply-templates select="./title/node()"/>
        </xsl:variable>
        <xsl:text>
    {   </xsl:text><xsl:call-template name="calc-scope-refsect1"/><xsl:text>, "" },
    {   RTMSGREFENTRYSTR_SCOPE_SAME,
        "</xsl:text><xsl:value-of select="$sTitle"/><xsl:text>" },
    {   RTMSGREFENTRYSTR_SCOPE_SAME,
        "</xsl:text>
        <xsl:value-of select="substring($g_sUnderlineRefSect1, 1, string-length($sTitle))"/>
        <xsl:text>" },</xsl:text>

        <xsl:apply-templates select="./*[name() != 'title']"/>

        <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "" },</xsl:text>
      </xsl:if>
    </xsl:for-each>

    <xsl:text>
};</xsl:text>

    <!--
      Generate the refentry structure.
      -->
    <xsl:text>
static const RTMSGREFENTRY </xsl:text><xsl:value-of select="$sDataBaseSym"/><xsl:text> =
{
    /* .idInternal = */   HELP_CMD_</xsl:text>
    <xsl:choose>
      <xsl:when test="contains(@id, '-')">
        <xsl:call-template name="str:to-upper">   <!-- Multi level command. -->
          <xsl:with-param name="text" select="translate(substring-after(@id, '-'), '-', '_')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="str:to-upper">   <!-- Simple command. -->
          <xsl:with-param name="text" select="@id"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>,
    /* .Synopsis   = */   { RT_ELEMENTS(</xsl:text>
    <xsl:value-of select="$sDataBaseSym"/><xsl:text>_synopsis), 0, </xsl:text>
    <xsl:value-of select="$sDataBaseSym"/><xsl:text>_synopsis },
    /* .Help       = */   { RT_ELEMENTS(</xsl:text>
    <xsl:value-of select="$sDataBaseSym"/><xsl:text>_full_help), 0, </xsl:text>
    <xsl:value-of select="$sDataBaseSym"/><xsl:text>_full_help },
    /* pszBrief    = */   "</xsl:text>
    <xsl:apply-templates select="./refnamediv/refpurpose/node()"/>
    <!-- TODO: Add the command name too. -->
    <xsl:text>"
};
</xsl:text>
  </xsl:template>


  <!--
    Convert command synopsis to text.
    -->
  <xsl:template match="cmdsynopsis">
    <xsl:if test="text()"><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>cmdsynopsis with text is not supported.</xsl:message></xsl:if>
    <xsl:if test="position() = 1">
      <xsl:text>
    {   </xsl:text><xsl:call-template name="calc-scope-cmdsynopsis"/><xsl:text> | RTMSGREFENTRYSTR_FLAGS_SYNOPSIS, "" },</xsl:text>
    </xsl:if>
    <xsl:text>
    {   </xsl:text><xsl:call-template name="calc-scope-cmdsynopsis"/><xsl:text> | RTMSGREFENTRYSTR_FLAGS_SYNOPSIS,
        "</xsl:text><xsl:call-template name="emit-indentation"/><xsl:apply-templates select="*|@*"/><xsl:text>" },</xsl:text>
  </xsl:template>

  <xsl:template match="sbr">
    <xsl:text>" },
    {   RTMSGREFENTRYSTR_SCOPE_SAME | RTMSGREFENTRYSTR_FLAGS_SYNOPSIS,
        "    </xsl:text><xsl:call-template name="emit-indentation"/> <!-- hardcoded in VBoxManageHelp.cpp too -->
  </xsl:template>

  <xsl:template match="cmdsynopsis/command">
    <xsl:text>" },
    {   RTMSGREFENTRYSTR_SCOPE_SAME | RTMSGREFENTRYSTR_FLAGS_SYNOPSIS,
        "</xsl:text><xsl:call-template name="emit-indentation"/>
    <xsl:apply-templates select="node()|@*"/>
  </xsl:template>

  <xsl:template match="cmdsynopsis/command[1]" priority="2">
    <xsl:apply-templates select="node()|@*"/>
  </xsl:template>

  <xsl:template match="command|option|computeroutput|literal|emphasis|filename|citetitle|note">
    <xsl:apply-templates select="node()|@*"/>
  </xsl:template>

  <xsl:template match="ulink">
    <xsl:value-of select="@url"/>
  </xsl:template>

  <xsl:template match="replaceable">
    <xsl:choose>
      <xsl:when test="ancestor::arg">
        <xsl:apply-templates />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&lt;</xsl:text>
        <xsl:apply-templates />
        <xsl:text>&gt;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- duplicated in docbook2latex.xsl -->
  <xsl:template match="arg|group">
    <!-- separator char if we're not the first child -->
    <xsl:if test="position() > 1">
      <xsl:choose>
        <xsl:when test="parent::group"><xsl:value-of select="$arg.or.sep"/></xsl:when>
        <xsl:when test="ancestor-or-self::*/@sepchar"><xsl:value-of select="ancestor-or-self::*/@sepchar"/></xsl:when>
        <xsl:otherwise><xsl:text> </xsl:text></xsl:otherwise>
      </xsl:choose>
    </xsl:if>

    <!-- open wrapping -->
    <xsl:variable name="fWrappers" select="not(ancestor::group)"/>
    <xsl:if test="$fWrappers">
      <xsl:choose>
        <xsl:when test="not(@choice) or @choice = ''">  <xsl:value-of select="$arg.choice.def.open.str"/></xsl:when>
        <xsl:when test="@choice = 'opt'">               <xsl:value-of select="$arg.choice.opt.open.str"/></xsl:when>
        <xsl:when test="@choice = 'req'">               <xsl:value-of select="$arg.choice.req.open.str"/></xsl:when>
        <xsl:when test="@choice = 'plain'"/>
        <xsl:otherwise><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Invalid arg choice: "<xsl:value-of select="@choice"/>"</xsl:message></xsl:otherwise>
      </xsl:choose>
    </xsl:if>

    <!-- render the arg (TODO: may need to do more work here) -->
    <xsl:apply-templates />

    <!-- repeat wrapping -->
    <xsl:choose>
      <xsl:when test="@rep = 'norepeat' or not(@rep) or @rep = ''"/>
      <xsl:when test="@rep = 'repeat'">                 <xsl:value-of select="$arg.rep.repeat.str"/></xsl:when>
      <xsl:otherwise><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Invalid rep choice: "<xsl:value-of select="@rep"/>"</xsl:message></xsl:otherwise>
    </xsl:choose>

    <!-- close wrapping -->
    <xsl:if test="$fWrappers">
      <xsl:choose>
        <xsl:when test="not(@choice) or @choice = ''">  <xsl:value-of select="$arg.choice.def.close.str"/></xsl:when>
        <xsl:when test="@choice = 'opt'">               <xsl:value-of select="$arg.choice.opt.close.str"/></xsl:when>
        <xsl:when test="@choice = 'req'">               <xsl:value-of select="$arg.choice.req.close.str"/></xsl:when>
      </xsl:choose>
      <!-- Add a space padding if we're the last element in a repeating arg or group -->
      <xsl:if test="(parent::arg or parent::group) and not(following-sibiling)">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:if>
  </xsl:template>


  <!--
    refsect2
    -->
  <xsl:template match="refsect2">
    <!-- assertions -->
    <xsl:if test="text()"><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>refsect2 shouldn't contain text</xsl:message></xsl:if>
    <xsl:if test="count(./title) != 1"><xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>refsect2 requires a title (<xsl:value-of select="ancestor-or-self::*[@id][1]/@id"/>)</xsl:message></xsl:if>

    <!-- title / command synopsis - sets the scope. -->
    <xsl:variable name="sTitle">
      <xsl:apply-templates select="./title/text()"/>
    </xsl:variable>
    <xsl:text>
    {   </xsl:text><xsl:call-template name="calc-scope-refsect2"/><xsl:text>, "" },
    {   RTMSGREFENTRYSTR_SCOPE_SAME,
        "</xsl:text><xsl:call-template name="emit-indentation"/>
    <xsl:value-of select="$sTitle"/>
    <xsl:text>" },
    {   RTMSGREFENTRYSTR_SCOPE_SAME,
        "</xsl:text><xsl:call-template name="emit-indentation"/>
    <xsl:value-of select="substring($g_sUnderlineRefSect2, 1, string-length($sTitle))"/>
    <xsl:text>" },</xsl:text>

<!--    <xsl:if test="./*[name() != 'title']/following::
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "y" },</xsl:text>  cmdsynopsis -->

    <!-- Format the text in the section -->
    <xsl:for-each select="./*[name() != 'title']">
      <xsl:apply-templates select="."/>
    </xsl:for-each>

    <!-- Add two blank lines, unless we're the last element in this refsect1. -->
    <xsl:if test="position() != last()">
      <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "" },</xsl:text>
    </xsl:if>
  </xsl:template>


  <!--
    para
    -->
  <xsl:template match="para">
    <xsl:if test="position() != 1 or not(parent::listitem)">
      <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "" },</xsl:text>
    </xsl:if>
    <xsl:call-template name="process-mixed"/>
  </xsl:template>


  <!--
    variablelist
    -->
  <xsl:template match="variablelist">
    <xsl:if test="*[not(self::varlistentry)]|text()">
      <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Only varlistentry elements are supported in variablelist </xsl:message>
    </xsl:if>
    <xsl:for-each select="./varlistentry">
      <xsl:if test="not(term) or not(listitem) or count(listitem) > 1">
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Expected one or more term members and exactly one listentry member in varlistentry element.</xsl:message>
      </xsl:if>
      <xsl:if test="(not(@spacing) or @spacing != 'compact') and (position() > 1 or (count(../preceding-sibling::*) - count(../preceding-sibling::title) > 0))">
        <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "" },</xsl:text>
      </xsl:if>
      <xsl:apply-templates select="*"/>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="varlistentry/term">
    <xsl:call-template name="process-mixed"/>
  </xsl:template>

  <xsl:template match="varlistentry/listitem">
    <xsl:call-template name="check-children">
      <xsl:with-param name="UnsupportedNodes" select="*[not(self::para or self::itemizedlist or self::orderedlist or self::variablelist or self::note)]|text()"/>
      <xsl:with-param name="SupportedNames">para, itemizedlist, orderedlist and note</xsl:with-param>
    </xsl:call-template>

    <xsl:apply-templates select="*"/>
  </xsl:template>


  <!--
    itemizedlist and orderedlist
    -->
  <xsl:template match="itemizedlist|orderedlist">
    <xsl:if test="*[not(self::listitem)]|text()">
      <xsl:message terminate="yes">
        <xsl:call-template name="error-prefix"/>Only listitem elements are supported in <xsl:value-of select="name()"/>:
        <xsl:call-template name="list-nodes">
          <xsl:with-param name="Nodes" select="*[not(self::listitem)]|text()"/>
        </xsl:call-template>
        </xsl:message>
    </xsl:if>
    <xsl:if test="parent::para">
      <xsl:message terminate="yes"><xsl:value-of select="name()"/> inside a para is current not supported. <!-- no newline
        -->Close the para before the list, it makes no difference to html and latex/pdf output.</xsl:message>
    </xsl:if>
    <xsl:if test="position() != 1 and (not(@spacing) or @spacing != 'compact')">
      <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "" },</xsl:text>
    </xsl:if>
    <xsl:for-each select="./listitem">
      <xsl:apply-templates select="*"/>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="itemizedlist/listitem|orderedlist/listitem">
    <xsl:if test="*[not(self::para)]|text()">
      <xsl:message terminate="yes">
        <xsl:call-template name="error-prefix"/>Expected <xsl:value-of select="name()"/>/listitem to only contain para elements:
        <xsl:call-template name="list-nodes">
          <xsl:with-param name="Nodes" select="*[not(self::para)]|text()"/>
        </xsl:call-template>
      </xsl:message>
    </xsl:if>

    <xsl:if test="position() != 1 and @spaceing != 'compact'">
      <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME, "" },</xsl:text>
    </xsl:if>
    <xsl:apply-templates select="*"/>
  </xsl:template>


  <!--
    Screen
    -->
  <xsl:template match="screen">
    <xsl:if test="ancestor::para">
      <xsl:text>" },</xsl:text>
    </xsl:if>

    <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME,
        "</xsl:text>

    <xsl:for-each select="node()">
      <xsl:choose>
        <xsl:when test="name() = ''">
          <xsl:call-template name="screen_text_line">
            <xsl:with-param name="sText" select="."/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:if test="*">
            <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Support for elements under screen has not been implemented: <xsl:value-of select="name()"/></xsl:message>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>

    <xsl:if test="not(ancestor::para)">
      <xsl:text>" },</xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template name="screen_text_line">
    <xsl:param name="sText"/>

    <xsl:choose>
      <xsl:when test="contains($sText, '&#x0a;')">
        <xsl:call-template name="escape_fixed_text">
          <xsl:with-param name="sText" select="substring-before($sText,'&#x0a;')"/>
        </xsl:call-template>

        <xsl:if test="substring-after($sText,'&#x0a;')">
          <xsl:text>" },
        {   RTMSGREFENTRYSTR_SCOPE_SAME,
            "</xsl:text>
          <xsl:call-template name="screen_text_line">
            <xsl:with-param name="sText" select="substring-after($sText,'&#x0a;')"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:when>

      <xsl:otherwise> <!-- no newline, so use the whole string -->
        <xsl:call-template name="escape_fixed_text">
          <xsl:with-param name="sText" select="$sText"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Normalizes the current text node taking tailing and leading spaces
       into account (unlike normalize-space which strips them mercilessly). -->
  <xsl:template name="my-normalize-space-current">
    <!-- <xsl:message>dbg0: position=<xsl:value-of select="position()"/> last=<xsl:value-of select="last()"/> .=|<xsl:value-of select="."/>|</xsl:message> -->
    <xsl:if test="(starts-with(.,' ') or starts-with(., $g_sNewLine)) and position() != 1">
      <xsl:value-of select="' '"/>
    </xsl:if>
    <xsl:value-of select="normalize-space(.)"/>
    <xsl:if test="((substring(.,string-length(.)) = ' ') or (substring(.,string-length(.)) = $g_sNewLine)) and position() != last()">
      <xsl:value-of select="' '"/>
    </xsl:if>
  </xsl:template>

  <!--
    Text escaping for C.
    -->
  <xsl:template match="text()" name="escape_text">
    <!-- Leading whitespace hack! -->
    <xsl:if test="(starts-with(.,' ') or starts-with(.,$g_sNewLine)) and position() != 1">
      <xsl:text> </xsl:text>
      <xsl:if test="boolean($g_fDebugText)">
        <xsl:message>text: add lead space</xsl:message>
      </xsl:if>
    </xsl:if>

    <!-- Body of text -->
    <xsl:choose>

      <xsl:when test="contains(., '\') or contains(., '&quot;')">
        <xsl:variable name="sTmp">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="normalize-space(.)"/>
            <xsl:with-param name="replace" select="'\'"/>
            <xsl:with-param name="with" select="'\\'"/>
            <xsl:with-param name="disable-output-escaping" select="yes"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="sTmp2">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$sTmp"/>
            <xsl:with-param name="replace" select="'&quot;'"/>
            <xsl:with-param name="with" select="'\&quot;'"/>
            <xsl:with-param name="disable-output-escaping" select="yes"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="$sTmp2"/>
        <xsl:if test="boolean($g_fDebugText)">
          <xsl:message>text: |<xsl:value-of select="$sTmp2"/>|(1)</xsl:message>
        </xsl:if>
      </xsl:when>

      <xsl:otherwise>
        <xsl:value-of select="normalize-space(.)"/>
        <xsl:if test="boolean($g_fDebugText)">
          <xsl:message>text: |<xsl:value-of select="normalize-space(.)"/>|(2)</xsl:message>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>

    <!-- Trailing whitespace hack! -->
    <xsl:if test="(substring(.,string-length(.)) = ' ' or substring(.,string-length(.)) = $g_sNewLine) and position() != last() and string-length(.) != 1">
      <xsl:text> </xsl:text>
      <xsl:if test="boolean($g_fDebugText)">
        <xsl:message>text: add tail space</xsl:message>
      </xsl:if>
    </xsl:if>

  </xsl:template>

  <!-- Elements producing non-breaking strings (single line). -->
  <xsl:template match="command/text()|option/text()|computeroutput/text()|arg/text()|filename/text()" name="escape_fixed_text">
    <xsl:param name="sText"><xsl:call-template name="my-normalize-space-current"/></xsl:param>
    <xsl:choose>

      <xsl:when test="contains($sText, '\') or contains($sText, '&quot;')">
        <xsl:variable name="sTmp1">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$sText"/>
            <xsl:with-param name="replace" select="'\'"/>
            <xsl:with-param name="with" select="'\\'"/>
            <xsl:with-param name="disable-output-escaping" select="yes"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="sTmp2">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$sTmp1"/>
            <xsl:with-param name="replace" select="'&quot;'"/>
            <xsl:with-param name="with" select="'\&quot;'"/>
            <xsl:with-param name="disable-output-escaping" select="yes"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="sTmp3">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$sTmp2"/>
            <xsl:with-param name="replace" select="' '"/>
            <xsl:with-param name="with" select="'\b'"/>
            <xsl:with-param name="disable-output-escaping" select="yes"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="$sTmp3"/>
        <xsl:if test="boolean($g_fDebugText)">
          <xsl:message>text! |<xsl:value-of select="$sTmp3"/>|</xsl:message>
        </xsl:if>
      </xsl:when>

      <xsl:when test="contains($sText, ' ')">
        <xsl:variable name="sTmp">
          <xsl:call-template name="str:subst">
            <xsl:with-param name="text" select="$sText"/>
            <xsl:with-param name="replace" select="' '"/>
            <xsl:with-param name="with" select="'\b'"/>
            <xsl:with-param name="disable-output-escaping" select="yes"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="$sTmp"/>
        <xsl:if test="boolean($g_fDebugText)">
          <xsl:message>text! |<xsl:value-of select="$sTmp"/>|</xsl:message>
        </xsl:if>
      </xsl:when>

      <xsl:otherwise>
        <xsl:value-of select="$sText"/>
        <xsl:if test="boolean($g_fDebugText)">
          <xsl:message>text! |<xsl:value-of select="$sText"/>|</xsl:message>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!--
    Unsupported elements and elements handled directly.
    -->
  <xsl:template match="synopfragment|synopfragmentref|title|refsect1">
    <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>The <xsl:value-of select="name()"/> element is not supported</xsl:message>
  </xsl:template>

  <xsl:template match="xref">
    <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>The <xsl:value-of select="name()"/> element is not supported, most likely the linkend is not defined or incorrectly processed by docbook-refentry-link-replacement-xsl-gen.xsl</xsl:message>
  </xsl:template>

  <!--
    Fail on misplaced scoping remarks.
    -->
  <xsl:template match="remark[@role = 'help-scope']">
    <xsl:choose>
      <xsl:when test="parent::refsect1"/>
      <xsl:when test="parent::refsect2"/>
      <xsl:when test="parent::cmdsynopsis and ancestor::refsynopsisdiv"/>
      <xsl:otherwise>
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Misplaced remark/@role=help-scope element.
Only supported on: refsect1, refsect2, refsynopsisdiv/cmdsynopsis</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!--
    Execute synopsis copy remark (avoids duplication for complicated xml).
    -->
  <xsl:template match="remark[@role = 'help-copy-synopsis']">
    <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>remark/@role=help-copy-synopsis is not supported by this stylesheet. Must preprocess input!</xsl:message>
  </xsl:template>

  <!--
    Warn about unhandled elements
    -->
  <xsl:template match="*">
    <xsl:message terminate="no">Warning: Unhandled element: <!-- no newline -->
      <xsl:for-each select="ancestor-or-self::*">
        <xsl:text>/</xsl:text>
        <xsl:value-of select="name(.)"/>
        <xsl:if test="@id">
          <xsl:value-of select="concat('[id=', @id ,']')"/>
        </xsl:if>
      </xsl:for-each>
    </xsl:message>
  </xsl:template>


  <!--
    Functions
    Functions
    Functions
    -->

  <!--
    Processes mixed children, i.e. both text and regular elements.
    Normalizes whitespace. -->
  <xsl:template name="process-mixed">
    <xsl:text>
    {   RTMSGREFENTRYSTR_SCOPE_SAME,
        "</xsl:text><xsl:call-template name="emit-indentation"/>

    <xsl:for-each select="node()[not(self::remark)]">
      <xsl:choose>
        <xsl:when test="name() = ''">
          <xsl:call-template name="escape_text"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="."/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>

    <xsl:text>" },</xsl:text>
  </xsl:template>


  <!--
    Element specific scoping.
    -->

  <xsl:template name="calc-scope-for-refentry">
    <xsl:text>HELP_SCOPE_</xsl:text>
    <xsl:choose>
      <xsl:when test="contains(@id, '-')">          <!-- Multi level command. -->
        <xsl:call-template name="str:to-upper">
          <xsl:with-param name="text" select="translate(substring-after(@id, '-'), '-', '_')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>                               <!-- Single command. -->
        <xsl:call-template name="str:to-upper">
          <xsl:with-param name="text" select="@id"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Figures out the scope of a refsect1 element. -->
  <xsl:template name="calc-scope-refsect1">
    <xsl:choose>
      <xsl:when test="title[text() = 'Description']">
        <xsl:text>RTMSGREFENTRYSTR_SCOPE_GLOBAL</xsl:text>
      </xsl:when>
      <xsl:when test="@id or remark[@role='help-scope']">
        <xsl:call-template name="calc-scope-from-remark-or-id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>RTMSGREFENTRYSTR_SCOPE_GLOBAL</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Figures out the scope of a refsect2 element. -->
  <xsl:template name="calc-scope-refsect2">
    <xsl:choose>
      <xsl:when test="@id or remark[@role='help-scope']">
        <xsl:call-template name="calc-scope-from-remark-or-id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>RTMSGREFENTRYSTR_SCOPE_SAME</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Figures out the scope of a refsect1 element. -->
  <xsl:template name="calc-scope-cmdsynopsis">
    <xsl:choose>
      <xsl:when test="ancestor::refsynopsisdiv">
        <xsl:call-template name="calc-scope-from-remark-or-id">
          <xsl:with-param name="sId" select="substring-after(@id, '-')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>RTMSGREFENTRYSTR_SCOPE_SAME</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!--
    Scoping worker functions.
    -->

  <!-- Calculates the current scope from the scope remark or @id.  -->
  <xsl:template name="calc-scope-from-remark-or-id">
    <xsl:param name="sId" select="@id"/>
    <xsl:choose>
      <xsl:when test="remark[@role='help-scope']">
        <xsl:call-template name="calc-scope-consts-from-remark"/>
      </xsl:when>
      <xsl:when test="$sId != ''">
        <xsl:call-template name="calc-scope-const-from-id">
          <xsl:with-param name="sId" select="$sId"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>expected remark child or id attribute.</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Turns a @id into a scope constant.
    Some woodoo taking place here here that chops the everything up to and
    including the first refentry/@id word from all IDs before turning them into
    constants (word delimiter '-'). -->
  <xsl:template name="calc-scope-const-from-id">
    <xsl:param name="sId" select="@id"/>
    <xsl:param name="sAncestorId" select="ancestor::refentry/@id"/>
    <xsl:text>HELP_SCOPE_</xsl:text>
    <xsl:choose>
      <xsl:when test="not($sAncestorId)">           <!-- Sanity check. -->
        <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>calc-scope-const-from-id is invoked without an refentry ancestor with a id. <xsl:call-template name="get-node-path"/> </xsl:message>
      </xsl:when>

      <xsl:when test="contains($sAncestorId, '-')"> <!-- Multi level command. -->
        <xsl:variable name="sPrefix" select="concat(substring-before($sAncestorId, '-'), '-')"/>
        <xsl:if test="not(contains($sId, $sPrefix))">
          <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Expected sId (<xsl:value-of select="$sId"/>) to contain <xsl:value-of select="$sPrefix"/></xsl:message>
        </xsl:if>
        <xsl:call-template name="str:to-upper">
          <xsl:with-param name="text" select="translate(substring-after($sId, $sPrefix), '-', '_')"/>
        </xsl:call-template>
      </xsl:when>

      <xsl:otherwise>                               <!-- Single command. -->
        <xsl:call-template name="str:to-upper">
          <xsl:with-param name="text" select="translate($sId, '-', '_')"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Turns a remark into one or more scope constants. -->
  <xsl:template name="calc-scope-consts-from-remark">
    <xsl:param name="sCondition" select="remark/@condition"/>
    <xsl:variable name="sNormalized" select="concat(normalize-space(translate($sCondition, ',;:|', '    ')), ' ')"/>
    <xsl:if test="$sNormalized = ' ' or $sNormalized = ''">
      <xsl:message terminate="yes"><xsl:call-template name="error-prefix"/>Empty @condition for help-scope remark.</xsl:message>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="substring-before($sNormalized, ' ') = 'GLOBAL'">
        <xsl:text>RTMSGREFENTRYSTR_SCOPE_GLOBAL</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>HELP_SCOPE_</xsl:text><xsl:value-of select="substring-before($sNormalized, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="calc-scope-const-from-remark-worker">
      <xsl:with-param name="sList" select="substring-after($sNormalized, ' ')"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="calc-scope-const-from-remark-worker">
    <xsl:param name="sList"/>
    <xsl:if test="$sList != ''">
      <xsl:choose>
        <xsl:when test="substring-before($sList, ' ') = 'GLOBAL'">
          <xsl:text>| RTMSGREFENTRYSTR_SCOPE_GLOBAL</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text> | HELP_SCOPE_</xsl:text><xsl:value-of select="substring-before($sList, ' ')"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="calc-scope-const-from-remark-worker">
        <xsl:with-param name="sList" select="substring-after($sList, ' ')"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>


  <!--
    Calculates and emits indentation list markup.
    -->
  <xsl:template name="emit-indentation">
    <xsl:variable name="iDepth" select="count(ancestor-or-self::*)"/>
    <xsl:for-each select="ancestor-or-self::*">
      <xsl:choose>

        <xsl:when test="self::refsect1
                      | self::refsect2
                      | self::refsect3
                      | self::refsynopsisdiv">
          <xsl:text>  </xsl:text>
        </xsl:when>

        <xsl:when test="self::term">
           <!-- currently no indent. -->
        </xsl:when>

        <!-- Evidence here (especially with orderedlist) that doing list by for-each
             listitem in the template matching the list type would be easier... -->
        <xsl:when test="self::listitem and parent::itemizedlist and (position() + 1) = $iDepth">
          <xsl:text>  - </xsl:text>
        </xsl:when>

        <xsl:when test="self::listitem and parent::orderedlist and (position() + 1) = $iDepth">
          <xsl:variable name="iNumber" select="count(preceding-sibling::listitem) + 1"/>
          <xsl:if test="$iNumber &lt;= 9">
            <xsl:text> </xsl:text>
          </xsl:if>
          <xsl:value-of select="$iNumber"/>
          <xsl:text>. </xsl:text>
        </xsl:when>

        <xsl:when test="self::listitem">
          <xsl:text>    </xsl:text>
        </xsl:when>

      </xsl:choose>
    </xsl:for-each>
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


  <!--
    Debug/Diagnostics: Return the path to the specified node (by default the current).
    -->
  <xsl:template name="get-node-path">
    <xsl:param name="Node" select="."/>
    <xsl:for-each select="$Node">
      <xsl:for-each select="ancestor-or-self::node()">
        <xsl:choose>
          <xsl:when test="name(.) = ''">
            <xsl:text>text()</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('/', name(.))"/>
            <xsl:choose>
              <xsl:when test="@id">
                <xsl:text>[@id=</xsl:text>
                <xsl:value-of select="@id"/>
                <xsl:text>]</xsl:text>
              </xsl:when>
              <xsl:when test="position() > 1">
                <xsl:text>[</xsl:text><xsl:value-of select="position()"/><xsl:text>]</xsl:text>
              </xsl:when>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:template>

  <!--
    Debug/Diagnostics: Return error message prefix.
    -->
  <xsl:template name="error-prefix">
    <xsl:param name="Node" select="."/>
    <xsl:text>error: </xsl:text>
    <xsl:call-template name="get-node-path">
      <xsl:with-param name="Node" select="$Node"/>
    </xsl:call-template>
    <xsl:text>: </xsl:text>
  </xsl:template>

  <!--
    Debug/Diagnostics: Print list of nodes (by default all children of current node).
    -->
  <xsl:template name="list-nodes">
    <xsl:param name="Nodes" select="node()"/>
    <xsl:for-each select="$Nodes">
      <xsl:if test="position() != 1">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="name(.) = ''">
          <xsl:text>text:text()</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="name(.)"/>
          <xsl:if test="@id">
            <xsl:text>[@id=</xsl:text>
            <xsl:value-of select="@id"/>
            <xsl:text>]</xsl:text>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="check-children">
    <xsl:param name="Node"             select="."/>
    <xsl:param name="UnsupportedNodes" select="*"/>
    <xsl:param name="SupportedNames"   select="'none'"/>
    <xsl:if test="count($UnsupportedNodes) != 0">
      <xsl:message terminate="yes">
        <xsl:call-template name="get-node-path">
          <xsl:with-param name="Node" select="$Node"/>
        </xsl:call-template>
        <!-- -->: error: Only <xsl:value-of select="$SupportedNames"/> are supported as children to <!-- -->
        <xsl:value-of select="name($Node)"/>
        <!-- -->
Unsupported children: <!-- -->
        <xsl:call-template name="list-nodes">
          <xsl:with-param name="Nodes" select="$UnsupportedNodes"/>
        </xsl:call-template>
      </xsl:message>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>

