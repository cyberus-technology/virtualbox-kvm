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
  xmlns:str="http://xsltsl.org/string"
  >

  <xsl:import href="../docbook-refentry-link-replacement-xsl-gen.xsl"/>

  <!-- Translated strings -->
  <xsl:variable name="sChapter" select="'глава'"/>
  <xsl:variable name="sSection" select="'секция'"/>
  <xsl:variable name="sOfManual" select="'руководства пользователя'"/>
  <xsl:variable name="sInManual" select="'руководства пользователя'"/>

</xsl:stylesheet>

