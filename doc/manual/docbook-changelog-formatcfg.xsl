<?xml version="1.0"?>

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

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- Single html file template -->
<xsl:import href="html/docbook.xsl"/>
<xsl:import href="common-formatcfg.xsl"/>

<!-- Adjust some params -->
<!--
<xsl:param name="draft.mode" select="'no'"/>
<xsl:param name="generate.toc">book nop</xsl:param>
<xsl:param name="generate.index" select="0"></xsl:param>
<xsl:param name="suppress.navigation" select="1"></xsl:param>
<xsl:param name="header.rule" select="0"></xsl:param>
<xsl:param name="abstract.notitle.enabled" select="0"></xsl:param>
<xsl:param name="footer.rule" select="0"></xsl:param>
<xsl:param name="css.decoration" select="1"></xsl:param>
<xsl:param name="html.cleanup" select="1"></xsl:param>
<xsl:param name="css.decoration" select="1"></xsl:param>
-->

<!-- Our hand written css styling -->
<xsl:template name="user.head.content">
 <style type="text/css">
  <xsl:comment>
  <!--
   body
   {
     height: 100%;
     font-family:  Verdana, Sans-serif, Arial, 'Trebuchet MS', 'Times New Roman';
     font-size: small;
     position: absolute;
     margin: 0px 0 0 0;
   }
   h2
   {
     text-decoration: none;
     font-size: 1.2em;
     font-family: Verdana, Sans-serif, Arial, 'Trebuchet MS', 'Times New Roman';
     margin: 5px 0 0;
     padding: 1px 5px 1px;
     border: 1px solid #6b89d4; /* #84C43B; */
     -moz-border-radius: 0.3em;
     background: #e6edff;  /* #d7e9a7; */
   }
   #watermark
   {
     margin: 0;
     position: fixed;
     top: 40%;
     color: #eeeeee;
     width: 100%;
     height: 100%;
     text-align: center;
     vertical-align: middle;
     font-size: 9em;
     font-weight: bold;
     z-index:-1;
   }
   -->
  </xsl:comment>
 </style>
</xsl:template>

<!-- Remove the title page at all -->
<!--
<xsl:template name="book.titlepage">
 -->
 <!-- Doesn't work with Qt, grrr -->
 <!--<xsl:text><div id="watermark">VirtualBox<br />Change Log</div></xsl:text>-->
<!--
</xsl:template>
-->

<!-- Disable any links into the manual -->
<xsl:template match="xref" name="xref">
 <xsl:text>the manual for more information</xsl:text>
</xsl:template>

</xsl:stylesheet>
