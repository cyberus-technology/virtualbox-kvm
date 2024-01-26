<?xml version="1.0"?>

<!--
 *  A template to generate a RGS resource script that contains
 *  registry definitions necessary to properly register
 *  VirtualBox Main API COM components.
-->
<!--
    Copyright (C) 2007-2023 Oracle and/or its affiliates.

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

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text"/>

<xsl:strip-space elements="*"/>

<!--
//  parameters
/////////////////////////////////////////////////////////////////////////////
-->

<!-- Name of the application to generate the RGS script for -->
<xsl:param name="Application"/>
<!-- Name of the module to generate the RGS script for -->
<xsl:param name="Module"/>


<!--
//  templates
/////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  header
-->
<xsl:template match="/idl">
  <xsl:apply-templates/>
</xsl:template>


<!--
 *  libraries
-->
<xsl:template match="library">
  <xsl:apply-templates/>
</xsl:template>


<!--
 *  applications
-->
<xsl:template match="application">
  <xsl:if test="@name=$Application">
    <xsl:variable name="context" select="//module[@name=$Module]/@context"/>
<xsl:text>HKCR
{
  NoRemove AppID
  {
    ForceRemove {</xsl:text><xsl:value-of select="@uuid"/>} = s '<xsl:value-of select="@name"/><xsl:text> </xsl:text>
    <xsl:choose>
      <xsl:when test="$context='LocalService'">
        <xsl:text>Service</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>Application</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>'
</xsl:text>
    <xsl:if test="$context='LocalService'">
      <xsl:text>    {
      val LocalService = s '</xsl:text><xsl:value-of select="$Module"/><xsl:text>'
    }
</xsl:text>
    </xsl:if>
    <xsl:text>    '</xsl:text><xsl:value-of select="$Module"/>
    <xsl:choose>
      <xsl:when test="$context='InprocServer'">
        <xsl:text>.dll</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>.exe</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>'
    {
      val AppID = s '{</xsl:text><xsl:value-of select="//library/application[@name=$Application]/@uuid"/><xsl:text>}'
    }
  }

</xsl:text>
    <xsl:apply-templates select="module[@name=$Module]/class"/>
<xsl:text>}
</xsl:text>
  </xsl:if>
</xsl:template>


<!--
 *  classes
-->
<xsl:template match="library//module/class">
  <xsl:variable name="cname" select="concat(//library/application/@name,'.',@name)"/>
  <xsl:variable name="desc" select="concat(@name,' Class')"/>
  <xsl:text>  </xsl:text>
  <xsl:value-of select="concat($cname,'.1')"/> = s '<xsl:value-of select="$desc"/>'
  {
    CLSID = s '{<xsl:value-of select="@uuid"/>}'
  }
  <xsl:value-of select="$cname"/> = s '<xsl:value-of select="$desc"/>'
  {
    CLSID = s '{<xsl:value-of select="@uuid"/>}'
    CurVer = s '<xsl:value-of select="concat($cname,'.1')"/>'
  }
  NoRemove CLSID
  {
    ForceRemove {<xsl:value-of select="@uuid"/>} = s '<xsl:value-of select="$desc"/>'
    {
      val AppID = s '{<xsl:value-of select="//library/application[@name=$Application]/@uuid"/><xsl:text>}'
</xsl:text>
      <xsl:if test="../@context!='LocalService'">
        <xsl:text>      ProgID = s '</xsl:text><xsl:value-of select="concat($cname,'.1')"/><xsl:text>'
      VersionIndependentProgID = s '</xsl:text><xsl:value-of select="$cname"/><xsl:text>'
      </xsl:text>
        <xsl:choose>
          <xsl:when test="../@context='InprocServer'">InprocServer32</xsl:when>
          <xsl:when test="../@context='LocalServer'">LocalServer32</xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes">
              <xsl:value-of select="concat(../../@name,'::',../@name,': ')"/>
              <xsl:text>module context </xsl:text>
              <xsl:value-of select="concat('&quot;',../@context,'&quot;')"/>
              <xsl:text> is invalid!</xsl:text>
            </xsl:message>
          </xsl:otherwise>
        </xsl:choose><xsl:text> = s '%MODULE%'
</xsl:text>
        <xsl:if test="../@context='InprocServer'">
          <xsl:variable name="tmodel" select="(./@threadingModel | ../@threadingModel)[last()]"/><xsl:text>      {
        val ThreadingModel = s '</xsl:text>
          <xsl:choose>
            <xsl:when test="$tmodel='Apartment'">Apartment</xsl:when>
            <xsl:when test="$tmodel='Free'">Free</xsl:when>
            <xsl:when test="$tmodel='Both'">Both</xsl:when>
            <xsl:when test="$tmodel='Neutral'">Neutral</xsl:when>
            <xsl:when test="$tmodel='Single'">Single</xsl:when>
            <xsl:when test="$tmodel='Rental'">Rental</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../@name,'::',@name,': ')"/>
                <xsl:text>class (or module) threading model </xsl:text>
                <xsl:value-of select="concat('&quot;',$tmodel,'&quot;')"/>
                <xsl:text> is invalid!</xsl:text>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose><xsl:text>'
      }
</xsl:text>
        </xsl:if>
        <xsl:text>      TypeLib = s '{</xsl:text><xsl:value-of select="//library/@uuid"/><xsl:text>}'
</xsl:text>
      </xsl:if>
      <xsl:text>    }
  }
</xsl:text>
</xsl:template>


<!--
 *  eat everything else not explicitly matched
-->
<xsl:template match="*">
</xsl:template>


</xsl:stylesheet>
