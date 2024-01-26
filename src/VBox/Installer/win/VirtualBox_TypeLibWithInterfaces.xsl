<?xml version="1.0"?>

<!--
    Generates a wiX include files with Interface elements for
    the stuff in the proxy stub DLLs.
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

<xsl:output method="xml"
            version="1.0"
            encoding="utf-8"
            indent="yes"/>

<xsl:strip-space elements="*"/>

<xsl:param name="a_sTarget">all</xsl:param>
<xsl:param name ="a_sWithSDS" select="no"/>
<xsl:param name="a_sProxyStubClsid">{0BB3B78C-1807-4249-5BA5-EA42D66AF0BF}</xsl:param>
<xsl:variable name="g_sProxyStubClsid" select="translate($a_sProxyStubClsid,'abcdef','ABCDEF')"/>


<!--
//  templates
/////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  header
-->
<xsl:template match="/idl">
  <xsl:comment>

    DO NOT EDIT! This is a generated file.

    WiX include script for the VirtualBox Type Library
    generated from XIDL (XML interface definition).

    Source    : src/VBox/Main/idl/VirtualBox.xidl
    Generator : src/VBox/Installer/win/VirtualBox_Interfaces.xsl
    Arguments : a_sTarget=<xsl:value-of select="$a_sTarget"/>
                a_sProxyStubClsid=<xsl:value-of select="$a_sProxyStubClsid"/>

  </xsl:comment>
  <xsl:apply-templates/>
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Key for more efficiently looking up of parent interfaces.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>

<!--
* filters to skip VBoxSDS class and interfaces if a VBOX_WITH_SDS is not defined in kmk
-->
    <xsl:template match="application[@uuid='ec0e78e8-fa43-43e8-ac0a-02c784c4a4fa']">
        <xsl:if test="$a_sWithSDS='yes'" >
            <xsl:call-template name="application_template" />
        </xsl:if>
    </xsl:template>

<!--
  Libraries.
-->
<xsl:template match="library">
  <Include>
    <TypeLib>
      <xsl:attribute name="Id"><xsl:value-of select="@uuid"/></xsl:attribute>
      <xsl:attribute name="Advertise">yes</xsl:attribute>
      <xsl:attribute name="MajorVersion"><xsl:value-of select="substring(@version,1,1)"/></xsl:attribute>
      <xsl:attribute name="MinorVersion"><xsl:value-of select="substring(@version,3)"/></xsl:attribute>
      <xsl:attribute name="Language">0</xsl:attribute>
      <xsl:attribute name="Description"><xsl:value-of select="@name"/></xsl:attribute>
      <xsl:attribute name="HelpDirectory"><xsl:text>msm_VBoxApplicationFolder</xsl:text></xsl:attribute>
      <xsl:apply-templates select="application | if[@target='midl']/application" />
    </TypeLib>
  </Include>
</xsl:template>

<!--
Applications.
-->
<xsl:template match="application" name="application_template">
    <AppId>
        <xsl:attribute name="Id">
            <xsl:value-of select="@uuid"/>
        </xsl:attribute>
        <xsl:attribute name="Description">
            <xsl:value-of select="@name"/> Application
        </xsl:attribute>
        <!--
            The name of windows service should be defined as module name in .xidl.
            It's viable for correct registration of COM windows service.
        -->
        <xsl:if test="module/@context = 'LocalService'">
            <xsl:attribute name="LocalService" >
                <xsl:value-of select="module/@name"/>
            </xsl:attribute>
        </xsl:if>
        <xsl:choose>
            <xsl:when test="$a_sTarget = 'VBoxClient-x86'">
                <xsl:apply-templates select="module[@name='VBoxC']/class"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:apply-templates select="module/class"/>
            </xsl:otherwise>
        </xsl:choose>
    </AppId>
    <xsl:apply-templates select="interface | if/interface">
        <xsl:sort select="translate(@uuid,'abcdef','ABCDEF')"/>
    </xsl:apply-templates>
</xsl:template>

<!--
  Classes.
-->
<xsl:template match="library//module/class">
  <Class>
    <xsl:attribute name="Id"><xsl:value-of select="@uuid"/></xsl:attribute>
    <xsl:attribute name="Description"><xsl:value-of select="@name"/> Class</xsl:attribute>
    <xsl:attribute name="Server">
      <xsl:choose>
        <xsl:when test="$a_sTarget = 'VBoxClient-x86' and ../@name = 'VBoxC'"><xsl:text>VBoxClient_x86</xsl:text></xsl:when>
        <xsl:otherwise><xsl:value-of select="../@name"/></xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:attribute name="Context">
      <xsl:choose>
        <xsl:when test="../@context='InprocServer'">InprocServer32</xsl:when>
        <xsl:when test="../@context='LocalServer'" >LocalServer32</xsl:when>
        <xsl:when test="../@context='LocalService'">LocalServer32</xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:value-of select="concat(../../@name,'::',../@name,': ')"/>
            <xsl:text>module context </xsl:text>
            <xsl:value-of select="concat('&quot;',../@context,'&quot;')"/>
            <xsl:text> is invalid!</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:if test="../@context='InprocServer'">
      <xsl:variable name="tmodel" select="(./@threadingModel | ../@threadingModel)[last()]"/>
      <xsl:attribute name="ThreadingModel">
        <xsl:choose>
          <xsl:when test="$tmodel='Apartment'">apartment</xsl:when>
          <xsl:when test="$tmodel='Free'">free</xsl:when>
          <xsl:when test="$tmodel='Both'">both</xsl:when>
          <xsl:when test="$tmodel='Neutral'">neutral</xsl:when>
          <xsl:when test="$tmodel='Single'">single</xsl:when>
          <xsl:when test="$tmodel='Rental'">rental</xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes">
              <xsl:value-of select="concat(../../@name,'::',@name,': ')"/>
              <xsl:text>class (or module) threading model </xsl:text>
              <xsl:value-of select="concat('&quot;',$tmodel,'&quot;')"/>
              <xsl:text> is invalid!</xsl:text>
            </xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>
    <ProgId>
      <xsl:attribute name="Id">
        <xsl:value-of select="concat(//library/@name,'.',@name,'.1')"/>
      </xsl:attribute>
      <xsl:attribute name="Description"><xsl:value-of select="@name"/> Class</xsl:attribute>
      <ProgId>
        <xsl:attribute name="Id">
          <xsl:value-of select="concat(//library/@name,'.',@name)"/>
        </xsl:attribute>
        <xsl:attribute name="Description"><xsl:value-of select="@name"/> Class</xsl:attribute>
      </ProgId>
    </ProgId>
  </Class>
</xsl:template>


<!--
  Interfaces.
-->
<xsl:template match=" library/application/interface
                    | library/application/if[@target='midl']/interface
                    | library/if[@target='midl']/application/interface
                    ">
  <Interface>
<!-- Interface Id="{00C8F974-92C5-44A1-8F3F-702469FDD04B}" Name="IDHCPServer" ProxyStubClassId32="{0BB3B78C-1807-4249-5BA5-EA42D66AF0BF}" NumMethods="33" -->
    <xsl:attribute name="Id">
      <xsl:text>{</xsl:text>
      <xsl:value-of select="translate(@uuid,'abcdef','ABCDEF')"/>
      <xsl:text>}</xsl:text>
    </xsl:attribute>
    <xsl:attribute name="Name"><xsl:value-of select="@name"/></xsl:attribute>
    <xsl:attribute name="ProxyStubClassId32"><xsl:value-of select="$g_sProxyStubClsid"/></xsl:attribute>
    <xsl:attribute name="NumMethods"><xsl:call-template name="fnCountMethods"/></xsl:attribute>
  </Interface>
</xsl:template>



<!--
  Count the number of methods.  This must match what midl.exe initializes
  CInterfaceStubVtbl::header::DispatchTableCount with in VirtualBox_p.c!
  -->
<xsl:template name="fnCountMethods">
  <xsl:variable name="sParent" select="@extends"/>

  <!-- Count immediate methods and attributes by kind. -->
  <xsl:variable name="cMethods"
    select="count(child::method)
          + count(child::if[@target='midl']/method)"/>
  <xsl:variable name="cReadOnlyAttributes"
    select="count(child::attribute[@readonly='yes'])
          + count(child::if[@target='midl']/attribute[@readonly='yes'])"/>
  <xsl:variable name="cReadWriteAttributes"
    select="count(child::attribute[not(@readonly) or not(@readonly='yes')])
          + count(child::if[@target = 'midl']/attribute[not(@readonly) or not(@readonly='yes')])"/>
  <xsl:variable name="cReservedMethods">
    <xsl:choose>
      <xsl:when test="not(@reservedMethods)">0</xsl:when>
      <xsl:otherwise><xsl:value-of select="@reservedMethods"/></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="cReservedAttributes">
    <xsl:choose>
      <xsl:when test="not(@reservedAttributes)">0</xsl:when>
      <xsl:otherwise><xsl:value-of select="@reservedAttributes"/></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- Recursively figure out the parent -->
  <xsl:variable name="cParent">
    <xsl:choose>
    <xsl:when test="@extends = '$unknown'">
      <xsl:value-of select='7'/>
    </xsl:when>
    <xsl:when test="@extends = '$errorinfo'">
      <xsl:value-of select='8'/> <!-- check this one! -->
    </xsl:when>
    <xsl:otherwise>
        <xsl:if test="count(key('G_keyInterfacesByName', $sParent)) != 1">
          <xsl:message terminate="yes">Couldn't find parent (<xsl:value-of select="$sParent"/>) to <xsl:value-of select="@name"/></xsl:message>
        </xsl:if>
        <xsl:for-each select="key('G_keyInterfacesByName', $sParent)">
          <xsl:call-template name="fnCountMethods"/>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- Compute the result. -->
  <xsl:variable name="cMethodsTotal"
    select="$cMethods + $cReservedMethods + $cReadOnlyAttributes
          + ($cReadWriteAttributes * 2) + $cReservedAttributes + $cParent"/>
  <xsl:value-of select="$cMethodsTotal"/>

  <!-- For debugging! -->
  <xsl:if test="0 or $cMethodsTotal > 256">
    <xsl:message terminate="no">
      <xsl:text>Debug: cMethods=</xsl:text><xsl:value-of select="$cMethods"/>
      <xsl:text> cReadOnlyAttributes=</xsl:text><xsl:value-of select="$cReadOnlyAttributes"/>
      <xsl:text> cReadWriteAttributes=</xsl:text><xsl:value-of select="$cReadWriteAttributes"/>
      <xsl:text> cReservedMethods=</xsl:text><xsl:value-of select="$cReservedMethods"/>
      <xsl:text> cReservedAttributes=</xsl:text><xsl:value-of select="$cReservedAttributes"/>
      <xsl:text> cParent=</xsl:text><xsl:value-of select="$cParent"/>
      <xsl:text> name=</xsl:text><xsl:value-of select="@name"/>
      <xsl:text> parent=</xsl:text><xsl:value-of select="$sParent"/>
    </xsl:message>
    <xsl:if test="$cMethodsTotal > 256">
      <xsl:message terminate="yes">
        <xsl:text>
Fatal xidl error: Interface </xsl:text><xsl:value-of select="@name"/>
        <xsl:text> has </xsl:text><xsl:value-of select="$cMethodsTotal"/>
        <xsl:text>! The maximum that older windows allows for proxy stubs is 256.
                  Please try adjust the number of reserved methods or attributes,
                  though it's clearly time to consider splitting up this monster interface.

</xsl:text>
      </xsl:message>
    </xsl:if>
  </xsl:if>
</xsl:template>



<!--
  Eat everything else not explicitly matched.
-->
<xsl:template match="*">
</xsl:template>


</xsl:stylesheet>

