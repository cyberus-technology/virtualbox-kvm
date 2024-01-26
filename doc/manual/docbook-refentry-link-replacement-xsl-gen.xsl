<?xml version="1.0"?>
<!--
    docbook-refentry-link-replacement-xsl-gen.xsl:
        XSLT stylesheet for generate a stylesheet that replaces links
        to the user manual in the manpages.
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

  <xsl:output method="text" version="1.0" encoding="utf-8" indent="yes"/>
  <xsl:strip-space elements="*"/>

<xsl:param name="g_sMode" select="not-specified"/>

  <!-- Translatable strings -->
  <xsl:variable name="sChapter" select="'chapter'"/>
  <xsl:variable name="sSection" select="'section'"/>
  <xsl:variable name="sOfManual" select="'of the user manual'"/>
  <xsl:variable name="sInManual" select="'in the user manual'"/>


<!-- Default operation is to supress output -->
<xsl:template match="node()|@*">
  <xsl:apply-templates/>
</xsl:template>


<!-- Remove all remarks. -->
<xsl:template match="remark"/>

<!--
Output header and footer.
-->
<xsl:template match="/">
  <xsl:if test="$g_sMode = 'first'">
    <xsl:text>&lt;?xml version="1.0"?&gt;
&lt;xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" &gt;
&lt;xsl:output method="xml" version="1.0" encoding="utf-8" indent="yes" /&gt;
&lt;xsl:template match="node()|@*"&gt;
  &lt;xsl:copy&gt;
    &lt;xsl:apply-templates select="node()|@*"/&gt;
  &lt;/xsl:copy&gt;
&lt;/xsl:template&gt;

</xsl:text>
  </xsl:if>
  <xsl:apply-templates/>
  <xsl:if test="$g_sMode = 'last'">
    <xsl:text>
&lt;/xsl:stylesheet&gt;
</xsl:text>
  </xsl:if>
</xsl:template>


<!--
Produce the transformation templates:
-->
<xsl:template match="chapter[@id]/title">
  <xsl:text>
&lt;xsl:template match="xref[@linkend='</xsl:text>
  <xsl:value-of select="../@id"/><xsl:text>']"&gt;
  &lt;xsl:text&gt;</xsl:text><xsl:value-of select="$sChapter"/><xsl:text> </xsl:text>
  <xsl:value-of select="count(../preceding-sibling::chapter) + 1"/><xsl:text> &quot;</xsl:text>
  <xsl:value-of select="normalize-space()"/>
  <xsl:text>&quot; </xsl:text><xsl:value-of select="$sInManual"/><xsl:text>&lt;/xsl:text&gt;
&lt;/xsl:template&gt;
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="sect1[@id]/title">
  <xsl:text>&lt;xsl:template match="xref[@linkend='</xsl:text>
  <xsl:value-of select="../@id"/><xsl:text>']"&gt;
  &lt;xsl:text&gt;</xsl:text><xsl:value-of select="$sSection"/><xsl:text> </xsl:text>
  <xsl:value-of select="count(../../preceding-sibling::chapter) + 1"/><xsl:text>.</xsl:text>
  <xsl:value-of select="count(../preceding-sibling::sect1) + 1"/>
  <xsl:text> &quot;</xsl:text>
  <xsl:value-of select="normalize-space()"/><xsl:text>&quot; </xsl:text>
  <xsl:value-of select="$sOfManual"/><xsl:text>&lt;/xsl:text&gt;
&lt;/xsl:template&gt;
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="sect2[@id]/title">
  <xsl:text>&lt;xsl:template match="xref[@linkend='</xsl:text>
  <xsl:value-of select="../@id"/><xsl:text>']"&gt;
  &lt;xsl:text&gt;</xsl:text><xsl:value-of select="$sSection"/><xsl:text> </xsl:text>
  <xsl:value-of select="count(../../../preceding-sibling::chapter) + 1"/><xsl:text>.</xsl:text>
  <xsl:value-of select="count(../../preceding-sibling::sect1) + 1"/><xsl:text>.</xsl:text>
  <xsl:value-of select="count(../preceding-sibling::sect2) + 1"/>
  <xsl:text> &quot;</xsl:text>
  <xsl:value-of select="normalize-space()"/><xsl:text>&quot; </xsl:text>
  <xsl:value-of select="$sOfManual"/><xsl:text>&lt;/xsl:text&gt;
&lt;/xsl:template&gt;
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="sect3[@id]/title">
  <xsl:text>&lt;xsl:template match="xref[@linkend='</xsl:text>
  <xsl:value-of select="../@id"/><xsl:text>']"&gt;
  &lt;xsl:text&gt;</xsl:text><xsl:value-of select="$sSection"/><xsl:text> </xsl:text>
  <xsl:value-of select="count(../../../../preceding-sibling::chapter) + 1"/><xsl:text>.</xsl:text>
  <xsl:value-of select="count(../../../preceding-sibling::sect1) + 1"/><xsl:text>.</xsl:text>
  <xsl:value-of select="count(../../preceding-sibling::sect2) + 1"/><xsl:text>.</xsl:text>
  <xsl:value-of select="count(../preceding-sibling::sect3) + 1"/>
  <xsl:text> &quot;</xsl:text>
  <xsl:value-of select="normalize-space()"/><xsl:text>&quot; </xsl:text>
  <xsl:value-of select="$sOfManual"/><xsl:text>&lt;/xsl:text&gt;
&lt;/xsl:template&gt;
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="preface[@id]/title">
  <xsl:text>&lt;xsl:template match="xref[@linkend='</xsl:text>
  <xsl:value-of select="../@id"/><xsl:text>']"&gt;
  &lt;xsl:text&gt;&quot;</xsl:text>
  <xsl:value-of select="normalize-space()"/><xsl:text>&quot; </xsl:text>
  <xsl:value-of select="$sOfManual"/><xsl:text>&lt;/xsl:text&gt;
&lt;/xsl:template&gt;
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="refentry[@id]/refentryinfo/title">
  <xsl:text>&lt;xsl:template match="xref[@linkend='</xsl:text>
  <xsl:value-of select="../../@id"/><xsl:text>']"&gt;
  &lt;xsl:text&gt; &quot;</xsl:text>
  <xsl:value-of select="normalize-space()"/><xsl:text>&quot;&lt;/xsl:text&gt;
&lt;/xsl:template&gt;
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="refsect2[@id]/title">
  <xsl:text>&lt;xsl:template match="xref[@linkend='</xsl:text>
  <xsl:value-of select="../@id"/><xsl:text>']"&gt;
  &lt;xsl:text&gt;&quot;</xsl:text>
  <xsl:value-of select="normalize-space()"/><xsl:text>&quot;&lt;/xsl:text&gt;
&lt;/xsl:template&gt;
</xsl:text>
  <xsl:apply-templates/>
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

</xsl:stylesheet>

