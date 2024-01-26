<?xml version="1.0"?>
<!--
    docbook-to-man.xsl:
        XSLT stylesheet that renders a refentry into a troff manpage.
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
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:import href="manpages/docbook.xsl"/>

  <!--
  <xsl:output method="xml" version="1.0" encoding="utf-8" indent="yes"/>
  <xsl:strip-space elements="*"/>
  -->

  <!--
      Extract manual's date from an *info/pubdate node (cf.
      get.refentry.date).  Detect RCS Date keyword.
  -->
  <xsl:template match="pubdate">
    <xsl:choose>
      <!-- careful with that keyword -->
      <xsl:when test="starts-with(text(), concat('$', 'Date:'))">
        <!-- Fetch the ISO 8601 date from inside -->
        <xsl:value-of select="substring(text(), 8, 10)"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- Use as-is -->
        <xsl:value-of select="text()"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>

