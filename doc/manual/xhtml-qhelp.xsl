<?xml version="1.0"?>
<!--
    Copyright (C) 2020-2023 Oracle and/or its affiliates.

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
<xsl:stylesheet  version="1.0"
                 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                 xmlns:xhtml="http://www.w3.org/1999/xhtml">
  <xsl:output method="xml" omit-xml-declaration="no"/>
  <!-- Don't include non matching elements in output -->
  <xsl:template match="text()"/>
  <xsl:strip-space elements="*"/>

  <!-- maybe a bit nicer way of adding a new line to the output -->
  <xsl:variable name="newline"><xsl:text>
  </xsl:text></xsl:variable>

  <xsl:variable name="inputFileName">
    <xsl:text>UserManual.xhtml</xsl:text>
  </xsl:variable>

  <xsl:template match="/">
    <xsl:element name="QtHelpProject">
      <xsl:attribute name="version">
        <xsl:value-of select="format-number(1, '.0')" />
      </xsl:attribute>
      <xsl:value-of select="$newline" />
      <xsl:element name="namespace">org.virtualbox</xsl:element>
      <xsl:value-of select="$newline" />
      <xsl:element name="virtualFolder">manual</xsl:element>
      <xsl:value-of select="$newline" />
      <xsl:element name="filterSection">
        <xsl:value-of select="$newline" />
        <xsl:element name="toc">
          <xsl:apply-templates select="//xhtml:div[@class='toc']//xhtml:span[@class='chapter'] | //xhtml:div[@class='toc']//xhtml:span[@class='preface']"/>
        </xsl:element><!-- toc -->
        <xsl:value-of select="$newline" />
        <!-- <xsl:element name="keywords"></xsl:element> -->
        <xsl:value-of select="$newline" />
        <xsl:element name="files">
          <!-- ======================input html file(s)============================= -->
          <xsl:value-of select="$newline" />
          <!-- ====================chunked html input files========================== -->
          <!-- Process div tag with class='toc'. For each space with class='chapter' -->
          <!-- add a <file>ch(position()).html</file> assuming our docbook chunked html -->
          <!-- files are named in this fashion. -->
          <!-- <xsl:apply-templates select="//xhtml:div[@class='toc']//xhtml:span[@class='chapter']"/> -->
          <!-- ====================single html input file========================== -->
          <xsl:element name="file">
            <xsl:value-of select="$inputFileName" />
          </xsl:element>
          <xsl:value-of select="$newline" />
          <!-- ===================================================================== -->
          <!-- ===================image files======================================= -->
          <xsl:apply-templates select="//xhtml:img"/>
          <!-- ===================================================================== -->

        </xsl:element>
        <xsl:value-of select="$newline" />
      </xsl:element>
      <xsl:value-of select="$newline" />
    </xsl:element>
  </xsl:template>

  <!-- ===================toc related template(s)====================== -->
  <xsl:template match="xhtml:span[@class='chapter'] | xhtml:span[@class='preface']">
    <xsl:element name="section">
      <xsl:attribute name="title">
        <xsl:value-of select="*" />
      </xsl:attribute>
      <xsl:attribute name="ref">
        <xsl:value-of select="$inputFileName" /><xsl:value-of select="xhtml:a/@href" />
      </xsl:attribute>
    </xsl:element>
    <xsl:value-of select="$newline" />
  </xsl:template>
  <!-- ==============================================================   -->

  <xsl:template match="//xhtml:img">
    <xsl:element name="file">
      <xsl:value-of select="@src"/>
    </xsl:element>
    <xsl:value-of select="$newline" />
  </xsl:template>


</xsl:stylesheet>
