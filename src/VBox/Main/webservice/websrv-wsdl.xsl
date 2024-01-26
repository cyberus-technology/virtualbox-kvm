<?xml version="1.0"?>

<!--
    websrv-wsdl.xsl:
        XSLT stylesheet that generates vboxweb.wsdl from
        VirtualBox.xidl. This WSDL file represents our
        web service API..
        See webservice/Makefile.kmk for an overview of all the things
        generated for the webservice.
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

<!--
    A WSDL document describes a web service using these major elements:
    Element     Defines
    <types>     The data types used by the web service, described in XML Schema
                syntax.
    <message>   The messages used by the web service. A message is a function call
                and with it come "parts", which are the parameters.
    <portType>  The operations performed by the web service. A portType can be thought
                of as a class or, in COM terms, as an interface.
    <binding>   The communication protocols used by the web service.

    The root tag is  <definitions>.

    Representing COM interfaces is tricky in WSDL 1.1, which doesn't really have them.
    WSDL only knows about "port types", which are an abstract representation
    of a group of functions. So for each "interface", we need to emit
    a "port type"; in the port type, we declare each "interface method"
    as one "operation". Each operation in turn consists of at least one
    message for the method invocation, which contains all the "in" and
    "inout" arguments. An optional second message for the response contains
    the return value, if one is present in the IDL (called "_return" to
    avoid name clashes), together with all the "out" and "inout" arguments.
    Each of these messages, however, need to be independently declared
    using the "message" element outside of the "port type" declaration.

    As an example: To create this XPCOM IDL:

    void createMachine (
        in wstring baseFolder,
        in wstring name,
        [retval] out IMachine machine
    );

    the following exists in the XIDL:

    <interface name="ifname">
        <method name="createMachine">
            <param name="baseFolder" type="wstring" dir="in" />
            <param name="name" type="wstring" dir="in" />
            <param name="machine" type="IMachine" dir="return" />
        </method>
    </interface>

    So, we have two "in" parameters, and one "out" parameter. The
    operation therefore requires two messages (one for the request,
    with the two "in" parameters, and one for the result with the
    return value). With RPC/encoded style, we end up with this:

    <message name="ifname.methodname_Request">
        <part name="baseFolder" type="xsd:string" />
        <part name="name" type="xsd:string" />
    </message>
    <message name="ifname.methodname_Result">
        <part name="_return" type="IMachine" />
    </message>
    <portType name="ifname">
        <operation name="methodname"
        <input message="ifname.methodname_Request" />
        <output message="ifname.methodname_Result" />
        </operation>
    </portType>

    With document/literal style, things get even more verbose, as
    instead of listing the arguments and return values in the messages,
    we declare a struct-like complexType in the <types> section
    instead and then reference that type in the messages.
-->

<xsl:stylesheet
  version="1.0"
  targetNamespace="http://schemas.xmlsoap.org/wsdl/"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
  xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"
  xmlns:vbox="http://www.virtualbox.org/"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl">

<xsl:param name="G_argDebug" />

<xsl:output
  method="xml"
  version="1.0"
  encoding="utf-8"
  indent="yes"/>

<xsl:strip-space
  elements="*" />

<!--**********************************************************************
 *
 *  global XSLT variables
 *
 **********************************************************************-->

<xsl:variable name="G_xsltFilename" select="'websrv-wsdl.xsl'" />

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<!-- collect all interfaces with "wsmap='suppress'" in a global variable for
     quick lookup -->
<xsl:variable name="G_setSuppressedInterfaces"
              select="//interface[@wsmap='suppress']" />

<!-- this marker is used with WSDL document style to mark that a message
     should have an automatic type that matches a complexType definition;
     use a string that cannot possibly appear in an XIDL interface name -->
<xsl:variable name="G_typeIsGlobalRequestElementMarker"
              select="'&lt;&lt;&lt;&lt;Request'" />
<xsl:variable name="G_typeIsGlobalResponseElementMarker"
              select="'&lt;&lt;&lt;&lt;Response'" />

<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>


<!--**********************************************************************
 *
 *  shared helpers
 *
 **********************************************************************-->

<!--
    function emitConvertedType
    -->
<xsl:template name="emitConvertedType">
  <xsl:param name="ifname" />
  <xsl:param name="methodname" />
  <xsl:param name="type" />
  <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('......emitConvertedType: type=&quot;', $type, '&quot;')" /></xsl:call-template>
  <!-- look up XML Schema type from IDL type from table array in typemap-shared.inc.xsl -->
  <xsl:variable name="xmltypefield" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@xmlname" />
  <xsl:choose>
    <xsl:when test="$type=$G_typeIsGlobalRequestElementMarker"><xsl:value-of select="concat('vbox:', $ifname, $G_classSeparator, $methodname, $G_requestMessageElementSuffix)" /></xsl:when>
    <xsl:when test="$type=$G_typeIsGlobalResponseElementMarker"><xsl:value-of select="concat('vbox:', $ifname, $G_classSeparator, $methodname, $G_responseMessageElementSuffix)" /></xsl:when>
    <!-- if above lookup in table succeeded, use that type -->
    <xsl:when test="string-length($xmltypefield)"><xsl:value-of select="concat('xsd:', $xmltypefield)" /></xsl:when>
    <xsl:when test="$type='$unknown'"><xsl:value-of select="$G_typeObjectRef" /></xsl:when>
    <xsl:when test="$type='global'"><xsl:value-of select="$G_typeObjectRef" /></xsl:when>
    <xsl:when test="$type='managed'"><xsl:value-of select="$G_typeObjectRef" /></xsl:when>
    <xsl:when test="$type='explicit'"><xsl:value-of select="$G_typeObjectRef" /></xsl:when>
    <!-- enums are easy, these are defined in schema at the top of the wsdl -->
    <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0"><xsl:value-of select="concat('vbox:', $type)" /></xsl:when>
    <!-- otherwise test for an interface with this name -->
    <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
      <!-- the type is one of our own interfaces: then it must have a wsmap attr -->
      <xsl:variable name="wsmap" select="key('G_keyInterfacesByName', $type)/@wsmap" />
      <xsl:choose>
        <xsl:when test="$wsmap='struct'"><xsl:value-of select="concat('vbox:', $type)" /></xsl:when>
        <xsl:when test="$wsmap='global'"><xsl:value-of select="$G_typeObjectRef" /></xsl:when>
        <xsl:when test="$wsmap='managed'"><xsl:value-of select="$G_typeObjectRef" /></xsl:when>
        <xsl:when test="$wsmap='explicit'"><xsl:value-of select="$G_typeObjectRef" /></xsl:when>
        <xsl:when test="$wsmap='suppress'">
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('emitConvertedType: Type &quot;', $type, '&quot; in method &quot;', $ifname, '::', $methodname, '&quot; has wsmap=&quot;suppress&quot; attribute in XIDL. This function should have been suppressed as well.')" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('emitConvertedType: Type &quot;', $type, '&quot; used in method &quot;', $ifname, '::', $methodname, '&quot; has unsupported wsmap attribute value &quot;', $wsmap, '&quot;')" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('emitConvertedType: Unknown type &quot;', $type, '&quot; used in method &quot;', $ifname, '::', $methodname, '&quot;.')" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
    function convertTypeAndEmitPartOrElement
    -->
<xsl:template name="convertTypeAndEmitPartOrElement">
  <xsl:param name="ifname" />
  <xsl:param name="methodname" />
  <xsl:param name="name" />
  <xsl:param name="type" />
  <xsl:param name="safearray" />  <!-- "yes" if XIDL has safearray=yes -->
  <xsl:param name="elname" />     <!-- "part" or "element" -->
  <xsl:param name="attrname" />   <!-- attrib of part or element: <part type=...> or <part element=...> or <element type=...> -->

  <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('....convertTypeAndEmitPartOrElement: arg name: ', $name)" /></xsl:call-template>
  <xsl:choose>
    <xsl:when test="$safearray='yes' and $type='octet'">
      <!-- we pass octet arrays as Base64-encoded strings. -->
       <xsl:element name="{$elname}">
         <xsl:attribute name="name"><xsl:value-of select="$name" /></xsl:attribute>
        <xsl:attribute name="type"><xsl:value-of select="'xsd:string'" /></xsl:attribute>
      </xsl:element>
    </xsl:when>

    <xsl:when test="$safearray='yes'">
      <xsl:element name="{$elname}"> <!-- <part> or <element> -->
        <xsl:attribute name="name"><xsl:value-of select="$name" /></xsl:attribute>
        <xsl:attribute name="minOccurs"><xsl:value-of select="'0'" /></xsl:attribute>
        <xsl:attribute name="maxOccurs"><xsl:value-of select="'unbounded'" /></xsl:attribute>
        <xsl:attribute name="{$attrname}">
          <xsl:call-template name="emitConvertedType">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="methodname" select="$methodname" />
            <xsl:with-param name="type" select="$type" />
          </xsl:call-template>
        </xsl:attribute>
      </xsl:element>
    </xsl:when>
    <xsl:otherwise>
      <xsl:element name="{$elname}"> <!-- <part> or <element> -->
        <xsl:attribute name="name"><xsl:value-of select="$name" /></xsl:attribute>
        <xsl:attribute name="{$attrname}">
          <xsl:call-template name="emitConvertedType">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="methodname" select="$methodname" />
            <xsl:with-param name="type" select="$type" />
          </xsl:call-template>
        </xsl:attribute>
      </xsl:element>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
    function emitRequestArgs
    -->
<xsl:template name="emitRequestArgs">
  <xsl:param name="_ifname" />          <!-- interface name -->
  <xsl:param name="_wsmap" />           <!-- interface's wsmap attribute -->
  <xsl:param name="_methodname" />
  <xsl:param name="_params" />
  <xsl:param name="_valuetype" />       <!-- optional, for attribute setter messages -->
  <xsl:param name="_valuesafearray" />  <!-- optional, 'yes' if attribute of setter has safearray=yes -->
  <xsl:param name="_elname" />          <!-- "part" or "xsd:element" -->
  <xsl:param name="_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->

  <!-- first parameter will be object on which method is called, depending on wsmap attribute -->
  <xsl:choose>
    <xsl:when test="($_wsmap='managed') or ($_wsmap='explicit')">
      <xsl:call-template name="convertTypeAndEmitPartOrElement">
        <xsl:with-param name="ifname" select="$_ifname" />
        <xsl:with-param name="methodname" select="$_methodname" />
        <xsl:with-param name="name" select="$G_nameObjectRef" />
        <xsl:with-param name="type" select="$_wsmap" />
        <xsl:with-param name="safearray" select="'no'" />
        <xsl:with-param name="elname" select="$_elname" /> <!-- "part" or "element" -->
        <xsl:with-param name="attrname" select="$_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
      </xsl:call-template>
    </xsl:when>
  </xsl:choose>
  <!-- now for the real parameters, if any -->
  <xsl:for-each select="$_params">
    <!-- emit only parts for "in" parameters -->
    <xsl:if test="@dir='in'">
      <xsl:call-template name="convertTypeAndEmitPartOrElement">
        <xsl:with-param name="ifname" select="$_ifname" />
        <xsl:with-param name="methodname" select="$_methodname" />
        <xsl:with-param name="name" select="@name" />
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="elname" select="$_elname" /> <!-- "part" or "element" -->
        <xsl:with-param name="attrname" select="$_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
      </xsl:call-template>
    </xsl:if>
  </xsl:for-each>
  <xsl:if test="$_valuetype">
    <!-- <part>
      <xsl:attribute name="name">value</xsl:attribute>
      <xsl:attribute name="type"><xsl:value-of select='string($_valuetype)' /></xsl:attribute>
    </part> -->
    <xsl:call-template name="convertTypeAndEmitPartOrElement">
      <xsl:with-param name="ifname" select="$_ifname" />
      <xsl:with-param name="methodname" select="$_methodname" />
      <xsl:with-param name="name" select="@name" />
      <xsl:with-param name="type" select="@type" />
      <xsl:with-param name="safearray" select="@safearray" />
      <xsl:with-param name="elname" select="$_elname" /> <!-- "part" or "element" -->
      <xsl:with-param name="attrname" select="$_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!--
    function emitResultArgs
    -->
<xsl:template name="emitResultArgs">
  <xsl:param name="_ifname" />
  <xsl:param name="_methodname" />
  <xsl:param name="_params" />          <!-- set of parameter elements -->
  <xsl:param name="_resulttype" />      <!-- for attribute getter methods only -->
  <xsl:param name="_resultsafearray" /> <!-- for attribute getter methods only -->
  <xsl:param name="_elname" />          <!-- "part" or "xsd:element" -->
  <xsl:param name="_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->

  <xsl:choose>
    <xsl:when test="$_resulttype">
      <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', $_ifname, '::', $_methodname, ': ', 'resultmsg for attr of type ', $_resulttype)" /></xsl:call-template>
      <xsl:call-template name="convertTypeAndEmitPartOrElement">
        <xsl:with-param name="ifname" select="$_ifname" />
        <xsl:with-param name="methodname" select="$_methodname" />
        <xsl:with-param name="name" select="$G_result" />
        <xsl:with-param name="type" select="$_resulttype" />
        <xsl:with-param name="safearray" select="$_resultsafearray" />
        <xsl:with-param name="elname" select="$_elname" /> <!-- "part" or "element" -->
        <xsl:with-param name="attrname" select="$_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', 'resultmsg for method: ', $_ifname, '::', $_methodname)" /></xsl:call-template>
      <xsl:for-each select="$_params">
        <!-- emit only parts for "out" parameters -->
        <xsl:if test="@dir='out'">
          <xsl:call-template name="convertTypeAndEmitPartOrElement">
            <xsl:with-param name="ifname" select="$_ifname" />
            <xsl:with-param name="methodname" select="$_methodname" />
            <xsl:with-param name="name"><xsl:value-of select="@name" /></xsl:with-param>
            <xsl:with-param name="type"><xsl:value-of select="@type" /></xsl:with-param>
            <xsl:with-param name="safearray" select="@safearray" />
            <xsl:with-param name="elname" select="$_elname" /> <!-- "part" or "element" -->
            <xsl:with-param name="attrname" select="$_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
          </xsl:call-template>
        </xsl:if>
        <xsl:if test="@dir='return'">
          <xsl:call-template name="convertTypeAndEmitPartOrElement">
            <xsl:with-param name="ifname" select="$_ifname" />
            <xsl:with-param name="methodname" select="$_methodname" />
            <xsl:with-param name="name"><xsl:value-of select="$G_result" /></xsl:with-param>
            <xsl:with-param name="type"><xsl:value-of select="@type" /></xsl:with-param>
            <xsl:with-param name="safearray" select="@safearray" />
            <xsl:with-param name="elname" select="$_elname" /> <!-- "part" or "element" -->
            <xsl:with-param name="attrname" select="$_attrname" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
          </xsl:call-template>
        </xsl:if>
      </xsl:for-each>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
    function emitRequestElements:
    for "in" parameters
    -->
<xsl:template name="emitRequestElements">
  <xsl:param name="_ifname" />          <!-- interface name -->
  <xsl:param name="_wsmap" />           <!-- interface's wsmap attribute -->
  <xsl:param name="_methodname" />
  <xsl:param name="_params" />
  <xsl:param name="_valuetype" />       <!-- optional, for attribute setter messages -->
  <xsl:param name="_valuesafearray" />  <!-- optional, 'yes' if attribute of setter has safearray=yes -->

  <xsd:element>
    <xsl:attribute name="name"><xsl:value-of select="concat($_ifname, $G_classSeparator, $_methodname, $G_requestMessageElementSuffix)" /></xsl:attribute>
    <xsd:complexType>
      <xsd:sequence>
        <xsl:call-template name="emitRequestArgs">
          <xsl:with-param name="_ifname"  select="$_ifname" />          <!-- interface name -->
          <xsl:with-param name="_wsmap"  select="$_wsmap" />           <!-- interface's wsmap attribute -->
          <xsl:with-param name="_methodname"  select="$_methodname" />
          <xsl:with-param name="_params"  select="$_params" />
          <xsl:with-param name="_valuetype"  select="$_valuetype" />       <!-- optional, for attribute setter messages -->
          <xsl:with-param name="_valuesafearray"  select="$_valuesafearray" />  <!-- optional, for attribute setter messages -->
          <xsl:with-param name="_elname" select="'xsd:element'" />          <!-- "part" or "xsd:element" -->
          <xsl:with-param name="_attrname" select="'type'" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
        </xsl:call-template>
      </xsd:sequence>
    </xsd:complexType>
  </xsd:element>
</xsl:template>

<!--
    function emitResultElements:
    for "out" and "return" parameters
    -->
<xsl:template name="emitResultElements">
  <xsl:param name="_ifname" />
  <xsl:param name="_methodname" />
  <xsl:param name="_params" />          <!-- set of parameter elements -->
  <xsl:param name="_resulttype" />      <!-- optional, for attribute getter methods only -->
  <xsl:param name="_resultsafearray" /> <!-- optional, 'yes' if attribute of getter has safearray=yes -->

  <xsd:element>
    <xsl:attribute name="name"><xsl:value-of select="concat($_ifname, $G_classSeparator, $_methodname, $G_responseMessageElementSuffix)" /></xsl:attribute>
    <xsd:complexType>
      <xsd:sequence>
        <xsl:call-template name="emitResultArgs">
          <xsl:with-param name="_ifname" select="$_ifname" />
          <xsl:with-param name="_methodname" select="$_methodname" />
          <xsl:with-param name="_params" select="$_params" />          <!-- set of parameter elements -->
          <xsl:with-param name="_resulttype" select="$_resulttype" />      <!-- for attribute getter methods only -->
          <xsl:with-param name="_resultsafearray" select="$_resultsafearray" />      <!-- for attribute getter methods only -->
          <xsl:with-param name="_elname" select="'xsd:element'" />          <!-- "part" or "xsd:element" -->
          <xsl:with-param name="_attrname" select="'type'" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
        </xsl:call-template>
      </xsd:sequence>
    </xsd:complexType>
  </xsd:element>
</xsl:template>

<!--
    function emitGetAttributeElements
    -->
<xsl:template name="emitGetAttributeElements">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />
  <xsl:param name="attrname" />
  <xsl:param name="attrtype" />
  <xsl:param name="attrsafearray" />

  <xsl:variable name="attrGetter"><xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
  <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', $ifname, '::', $attrGetter)" /></xsl:call-template>
  <xsl:call-template name="emitRequestElements">
    <xsl:with-param name="_ifname" select="$ifname" />
    <xsl:with-param name="_wsmap" select="$wsmap" />
    <xsl:with-param name="_methodname" select="$attrGetter" />
    <xsl:with-param name="_params" select="/.." /> <!-- empty set -->
  </xsl:call-template>
  <xsl:call-template name="emitResultElements">
    <xsl:with-param name="_ifname" select="$ifname" />
    <xsl:with-param name="_methodname" select="$attrGetter" />
    <xsl:with-param name="_params" select="/.." /> <!-- empty set -->
    <xsl:with-param name="_resulttype" select='$attrtype' />
    <xsl:with-param name="_resultsafearray" select='$attrsafearray' />
  </xsl:call-template>
</xsl:template>

<!--
  function: emitRequestMessage
    for "in" parameters
-->
<xsl:template name="emitRequestMessage">
  <xsl:param name="_ifname" />          <!-- interface name -->
  <xsl:param name="_wsmap" />           <!-- interface's wsmap attribute -->
  <xsl:param name="_methodname" />
  <xsl:param name="_params" />
  <xsl:param name="_valuetype" />       <!-- optional, for attribute setter messages -->

  <wsdl:message>
    <xsl:attribute name="name"><xsl:value-of select="concat($_ifname, $G_classSeparator, $_methodname, $G_methodRequest)" /></xsl:attribute>

    <xsl:call-template name="convertTypeAndEmitPartOrElement">
      <xsl:with-param name="ifname" select="$_ifname" />
      <xsl:with-param name="methodname" select="$_methodname" />
      <xsl:with-param name="name" select="'parameters'" />
      <xsl:with-param name="type" select="$G_typeIsGlobalRequestElementMarker" />
      <xsl:with-param name="safearray" select="'no'" />
      <xsl:with-param name="elname" select="'wsdl:part'" /> <!-- "part" or "element" -->
      <xsl:with-param name="attrname" select="'element'" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
    </xsl:call-template>
  </wsdl:message>
</xsl:template>

<!--
  function: emitResultMessage
    for "out" and "return" parameters
-->
<xsl:template name="emitResultMessage">
  <xsl:param name="_ifname" />
  <xsl:param name="_methodname" />
  <xsl:param name="_params" />          <!-- set of parameter elements -->
  <xsl:param name="_resulttype" />      <!-- for attribute getter methods only -->

  <wsdl:message>
    <xsl:attribute name="name"><xsl:copy-of select="$_ifname" /><xsl:value-of select="$G_classSeparator" /><xsl:value-of select="$_methodname" /><xsl:copy-of select="$G_methodResponse" /></xsl:attribute>

    <!-- <xsl:variable name="cOutParams" select="count($_params[@dir='out']) + count($_params[@dir='return'])" /> -->
    <xsl:call-template name="convertTypeAndEmitPartOrElement">
      <xsl:with-param name="ifname" select="$_ifname" />
      <xsl:with-param name="methodname" select="$_methodname" />
      <xsl:with-param name="name" select="'parameters'" />
      <xsl:with-param name="type" select="$G_typeIsGlobalResponseElementMarker" />
      <xsl:with-param name="safearray" select="'no'" />
      <xsl:with-param name="elname" select="'wsdl:part'" /> <!-- "part" or "element" -->
      <xsl:with-param name="attrname" select="'element'" />   <!-- attrib of part of element: <part type=...> or <part element=...> or <element type=...> -->
    </xsl:call-template>
  </wsdl:message>
</xsl:template>

<!--
  function emitGetAttributeMessages:
-->
<xsl:template name="emitGetAttributeMessages">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />
  <xsl:param name="attrname" />
  <xsl:param name="attrtype" />

  <xsl:variable name="attrGetter"><xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
  <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', $ifname, '::', $attrGetter)" /></xsl:call-template>
  <xsl:call-template name="emitRequestMessage">
    <xsl:with-param name="_ifname" select="$ifname" />
    <xsl:with-param name="_wsmap" select="$wsmap" />
    <xsl:with-param name="_methodname" select="$attrGetter" />
    <xsl:with-param name="_params" select="/.." /> <!-- empty set -->
  </xsl:call-template>
  <xsl:call-template name="emitResultMessage">
    <xsl:with-param name="_ifname" select="$ifname" />
    <xsl:with-param name="_methodname" select="$attrGetter" />
    <xsl:with-param name="_params" select="/.." /> <!-- empty set -->
    <xsl:with-param name="_resulttype" select='$attrtype' />
  </xsl:call-template>
</xsl:template>

<!--
    function emitSetAttributeMessages
    -->
<xsl:template name="emitSetAttributeMessages">
  <xsl:param name="ifname" select="$ifname" />
  <xsl:param name="wsmap" select="$wsmap" />
  <xsl:param name="attrname" select="$attrname" />
  <xsl:param name="attrtype" select="$attrtype" />

  <xsl:variable name="attrSetter"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
  <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', $ifname, '::', $attrSetter)" /></xsl:call-template>
  <xsl:call-template name="emitRequestMessage">
    <xsl:with-param name="_ifname" select="$ifname" />
    <xsl:with-param name="_wsmap" select="$wsmap" />
    <xsl:with-param name="_methodname" select="$attrSetter" />
    <xsl:with-param name="_params" select="/.." /> <!-- empty set -->
    <xsl:with-param name="_valuetype" select="$attrtype" />
    <xsl:with-param name="elname" select="'wsdl:part'" /> <!-- "part" or "element" -->
  </xsl:call-template>
  <xsl:call-template name="emitResultMessage">
    <xsl:with-param name="_ifname" select="$ifname" />
    <xsl:with-param name="_methodname" select="$attrSetter" />
    <xsl:with-param name="_params" select="/.." /> <!-- empty set -->
    <xsl:with-param name="elname" select="'wsdl:part'" /> <!-- "part" or "element" -->
  </xsl:call-template>
</xsl:template>

<!--
    function emitInOutOperation:
    referencing the messages that must have been emitted previously
-->
<xsl:template name="emitInOutOperation">
  <xsl:param name="_ifname" />            <!-- interface name -->
  <xsl:param name="_methodname" />        <!-- method name -->
  <xsl:param name="_params" />
  <xsl:param name="_resulttype" />      <!-- for attribute getter methods only -->
  <xsl:param name="_fSoap" />

  <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('....emitInOutOperation ', $_ifname, '::', $_methodname)" /></xsl:call-template>

  <wsdl:operation>
    <xsl:attribute name="name">
      <xsl:value-of select="concat($_ifname, '_', $_methodname)" />
    </xsl:attribute>
    <xsl:if test="$_fSoap">
      <soap:operation>
        <!-- VMware has an empty attribute like this as well -->
        <xsl:attribute name="soapAction"><xsl:value-of select="''" /></xsl:attribute>
        <xsl:attribute name="style"><xsl:value-of select="$G_basefmt" /></xsl:attribute>
      </soap:operation>
    </xsl:if>
    <wsdl:input>
      <xsl:choose>
        <xsl:when test="$_fSoap">
          <soap:body>
            <xsl:attribute name="use"><xsl:value-of select="$G_parmfmt" /></xsl:attribute>
            <!-- avoid jax-ws warning: <xsl:attribute name="namespace"><xsl:value-of select="concat($G_targetNamespace, $G_targetNamespaceSeparator)" /></xsl:attribute>-->
          </soap:body>
        </xsl:when>
        <xsl:otherwise>
          <xsl:attribute name="message">vbox:<xsl:copy-of select="$_ifname" /><xsl:value-of select="$G_classSeparator" /><xsl:value-of select="$_methodname" /><xsl:copy-of select="$G_methodRequest" /></xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>
    </wsdl:input>
    <xsl:choose>
      <xsl:when test="$_resulttype">
        <wsdl:output>
          <xsl:choose>
            <xsl:when test="$_fSoap">
              <soap:body>
                <xsl:attribute name="use"><xsl:value-of select="$G_parmfmt" /></xsl:attribute>
                <!-- avoid jax-ws warning: <xsl:attribute name="namespace"><xsl:value-of select="concat($G_targetNamespace, $G_targetNamespaceSeparator)" /></xsl:attribute> -->
              </soap:body>
            </xsl:when>
            <xsl:otherwise>
              <xsl:attribute name="message">vbox:<xsl:copy-of select="$_ifname" /><xsl:value-of select="$G_classSeparator" /><xsl:value-of select="$_methodname" /><xsl:copy-of select="$G_methodResponse" /></xsl:attribute>
            </xsl:otherwise>
          </xsl:choose>
        </wsdl:output>
      </xsl:when>
      <xsl:otherwise>
        <!-- <xsl:if test="count($_params[@dir='out'] | $_params[@dir='return']) > 0"> -->
          <wsdl:output>
            <xsl:choose>
              <xsl:when test="$_fSoap">
                <soap:body>
                  <xsl:attribute name="use"><xsl:value-of select="$G_parmfmt" /></xsl:attribute>
                  <!-- avoid jax-ws warning: <xsl:attribute name="namespace"><xsl:value-of select="concat($G_targetNamespace, $G_targetNamespaceSeparator)" /></xsl:attribute> -->
                </soap:body>
              </xsl:when>
              <xsl:otherwise>
                <xsl:attribute name="message">vbox:<xsl:copy-of select="$_ifname" /><xsl:value-of select="$G_classSeparator" /><xsl:value-of select="$_methodname" /><xsl:copy-of select="$G_methodResponse" /></xsl:attribute>
              </xsl:otherwise>
            </xsl:choose>
          </wsdl:output>
        <!-- </xsl:if> -->
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="not($_fSoap)">
        <wsdl:fault name="InvalidObjectFault" message="vbox:InvalidObjectFaultMsg" />
        <wsdl:fault name="RuntimeFault" message="vbox:RuntimeFaultMsg" />
      </xsl:when>
      <xsl:otherwise>
        <wsdl:fault name="InvalidObjectFault">
          <soap:fault name="InvalidObjectFault">
            <xsl:attribute name="use"><xsl:value-of select="$G_parmfmt" /></xsl:attribute>
          </soap:fault>
        </wsdl:fault>
        <wsdl:fault name="RuntimeFault">
          <soap:fault name="RuntimeFault">
            <xsl:attribute name="use"><xsl:value-of select="$G_parmfmt" /></xsl:attribute>
          </soap:fault>
        </wsdl:fault>
      </xsl:otherwise>
    </xsl:choose>
  </wsdl:operation>
</xsl:template>

<!--
    function verifyInterface
-->
<xsl:template name="verifyInterface">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />

  <xsl:choose>
    <xsl:when test="$wsmap='global'" />
    <xsl:when test="$wsmap='managed'" />
    <xsl:when test="$wsmap='explicit'" />
    <xsl:when test="$wsmap='struct'" />
    <xsl:when test="$wsmap='suppress'" />
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat(local-name(), ' template: Interface &quot;', $ifname, '&quot; has invalid wsmap attribute &quot;', $wsmap, '&quot; in XIDL.')" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <!-- now make sure we have each interface only once -->
  <xsl:if test="(count(//library/interface[@name=$ifname]) > 1)">
    <xsl:call-template name="fatalError">
      <xsl:with-param name="msg" select="concat(local-name(), ' template: There is more than one interface with a name=&quot;', $ifname, '&quot; attribute.')" />
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!--
    function emitMessagesForInterface
-->
<xsl:template name="emitMessagesForInterface">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />

    <!-- 1) outside the portType, here come the in/out methods for all the "operations" we declare below;
         a) for attributes (get/set methods)
         b) for "real" methods
         -->
    <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('************* messages for interface &quot;', $ifname, '&quot;')" /></xsl:call-template>
    <!-- a) attributes first -->
    <xsl:for-each select="attribute">
      <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
      <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
      <xsl:variable name="attrreadonly"><xsl:value-of select="@readonly" /></xsl:variable>
      <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('messages for ', $ifname, '::', $attrname, ': attribute of type &quot;', $attrtype, '&quot;, readonly: ', $attrreadonly)" /></xsl:call-template>
      <!-- skip this attribute if it has parameters of a type that has wsmap="suppress" -->
      <xsl:choose>
        <xsl:when test="( $attrtype=($G_setSuppressedInterfaces/@name) )">
          <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrname, ' for it is of a suppressed type')" /></xsl:comment>
        </xsl:when>
        <xsl:when test="@wsmap = 'suppress'">
          <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrname, ' for it is suppressed')" /></xsl:comment>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="@readonly='yes'">
              <xsl:comment> readonly attribute <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$attrname" /> </xsl:comment>
            </xsl:when>
            <xsl:otherwise>
              <xsl:comment> read/write attribute <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$attrname" /> </xsl:comment>
            </xsl:otherwise>
          </xsl:choose>
          <!-- aa) get method: emit request and result -->
          <xsl:call-template name="emitGetAttributeMessages">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="wsmap" select="$wsmap" />
            <xsl:with-param name="attrname" select="$attrname" />
            <xsl:with-param name="attrtype" select="$attrtype" />
          </xsl:call-template>
          <!-- bb) emit a set method if the attribute is read/write -->
          <xsl:if test="not($attrreadonly='yes')">
            <xsl:call-template name="emitSetAttributeMessages">
              <xsl:with-param name="ifname" select="$ifname" />
              <xsl:with-param name="wsmap" select="$wsmap" />
              <xsl:with-param name="attrname" select="$attrname" />
              <xsl:with-param name="attrtype" select="$attrtype" />
            </xsl:call-template>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each> <!-- select="attribute" -->
    <!-- b) "real" methods after the attributes -->
    <xsl:for-each select="method">
      <xsl:variable name="methodname"><xsl:value-of select="@name" /></xsl:variable>
      <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('messages for ', $ifname, '::', $methodname, ': method')" /></xsl:call-template>
      <xsl:comment> method <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$methodname" /> </xsl:comment>
      <!-- skip this method if it has parameters of a type that has wsmap="suppress" -->
      <xsl:choose>
        <xsl:when test="   (param[@type=($G_setSuppressedInterfaces/@name)])
                        or (param[@mod='ptr'])" >
          <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it has parameters with suppressed types')" /></xsl:comment>
        </xsl:when>
        <xsl:when test="@wsmap = 'suppress'">
          <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it is suppressed')" /></xsl:comment>
        </xsl:when>
        <xsl:otherwise>
          <!-- always emit a request message -->
          <xsl:call-template name="emitRequestMessage">
            <xsl:with-param name="_ifname" select="$ifname" />
            <xsl:with-param name="_wsmap" select="$wsmap" />
            <xsl:with-param name="_methodname" select="$methodname" />
            <xsl:with-param name="_params" select="param" />
            <xsl:with-param name="elname" select="'wsdl:part'" /> <!-- "part" or "element" -->
          </xsl:call-template>
          <!-- emit a second "result" message only if the method has "out" arguments or a return value -->
          <!-- <xsl:if test="(count(param[@dir='out'] | param[@dir='return']) > 0)"> -->
            <xsl:call-template name="emitResultMessage">
              <xsl:with-param name="_ifname" select="$ifname" />
              <xsl:with-param name="_wsmap" select="$wsmap" />
              <xsl:with-param name="_methodname" select="@name" />
              <xsl:with-param name="_params" select="param" />
              <xsl:with-param name="elname" select="'wsdl:part'" /> <!-- "part" or "element" -->
            </xsl:call-template>
          <!-- </xsl:if> -->
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
</xsl:template>

<!--
    function emitOperationsForInterface
    -->
<xsl:template name="emitOperationsInPortTypeForInterface">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />

  <!-- a) again, first for the attributes whose messages we produced above -->
  <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('************* portType for interface &quot;', $ifname, '&quot;')" /></xsl:call-template>
  <xsl:for-each select="attribute">
    <xsl:variable name="attrname" select="@name" />
    <xsl:variable name="attrtype" select="@type" />
    <xsl:variable name="attrreadonly" select="@readonly" />
    <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('operations for ', $ifname, '::', $attrname, ': attribute of type &quot;', $attrtype, '&quot;, readonly: ', $attrreadonly)" /></xsl:call-template>
    <xsl:choose>
      <!-- skip this attribute if it has parameters of a type that has wsmap="suppress" -->
      <xsl:when test="( $attrtype=($G_setSuppressedInterfaces/@name) )">
        <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrname, ' for it is of a suppressed type')" /></xsl:comment>
      </xsl:when>
      <xsl:when test="@wsmap = 'suppress'">
        <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrname, ' for it is suppressed')" /></xsl:comment>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="attrGetter"><xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
        <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', $G_attributeGetPrefix, $attrname)" /></xsl:call-template>
        <xsl:call-template name="emitInOutOperation">
          <xsl:with-param name="_ifname" select="$ifname" />
          <xsl:with-param name="_methodname" select="$attrGetter" />
          <xsl:with-param name="_params" select="/.." />
          <xsl:with-param name="_resulttype" select='$attrtype' />
        </xsl:call-template>
        <xsl:if test="not($attrreadonly='yes')">
          <xsl:variable name="attrSetter"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
          <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', $attrSetter)" /></xsl:call-template>
          <xsl:call-template name="emitInOutOperation">
            <xsl:with-param name="_ifname" select="$ifname" />
            <xsl:with-param name="_methodname" select="$attrSetter" />
            <xsl:with-param name="_params" select="/.." />
            <xsl:with-param name="_resulttype" select='$attrtype' />
          </xsl:call-template>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
  <!-- b) then for the "real" methods whose messages we produced above -->
  <xsl:for-each select="method">
    <xsl:variable name="methodname"><xsl:value-of select="@name" /></xsl:variable>
    <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('operations for ', $ifname, '::', $methodname, ': method')" /></xsl:call-template>
    <!-- skip this method if it has parameters of a type that has wsmap="suppress" -->
    <xsl:choose>
      <xsl:when test="   (param[@type=($G_setSuppressedInterfaces/@name)])
                      or (param[@mod='ptr'])" >
        <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it has parameters with suppressed types')" /></xsl:comment>
      </xsl:when>
      <xsl:when test="@wsmap = 'suppress'">
        <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it is suppressed')" /></xsl:comment>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="emitInOutOperation">
          <xsl:with-param name="_ifname" select="$ifname" />
          <xsl:with-param name="_methodname" select="$methodname" />
          <xsl:with-param name="_params" select="param" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<!--
    function emitOperationsInBindingForInterface
    -->
<xsl:template name="emitOperationsInBindingForInterface">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />

  <!-- a) again, first for the attributes whose messages we produced above -->
  <xsl:for-each select="attribute">
    <xsl:variable name="attrname" select="@name" />
    <xsl:variable name="attrtype" select="@type" />
    <xsl:variable name="attrreadonly" select="@readonly" />
    <!-- skip this attribute if it has parameters of a type that has wsmap="suppress" -->
    <xsl:choose>
      <xsl:when test="( $attrtype=($G_setSuppressedInterfaces/@name) )">
        <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrname, ' for it is of a suppressed type')" /></xsl:comment>
      </xsl:when>
      <xsl:when test="@wsmap = 'suppress'">
        <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrname, ' for it is suppressed')" /></xsl:comment>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="attrGetter"><xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
        <xsl:call-template name="emitInOutOperation">
          <xsl:with-param name="_ifname" select="$ifname" />
          <xsl:with-param name="_methodname" select="$attrGetter" />
          <xsl:with-param name="_params" select="/.." />
          <xsl:with-param name="_resulttype" select='$attrtype' />
          <xsl:with-param name="_fSoap" select="1" />
        </xsl:call-template>
        <xsl:if test="not($attrreadonly='yes')">
          <xsl:variable name="attrSetter"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
          <xsl:call-template name="emitInOutOperation">
            <xsl:with-param name="_ifname" select="$ifname" />
            <xsl:with-param name="_methodname" select="$attrSetter" />
            <xsl:with-param name="_params" select="/.." />
            <xsl:with-param name="_resulttype" select='$attrtype' />
            <xsl:with-param name="_fSoap" select="1" />
          </xsl:call-template>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
  <!-- b) then for the "real" methods whose messages we produced above -->
  <xsl:for-each select="method">
    <xsl:variable name="methodname"><xsl:value-of select="@name" /></xsl:variable>
    <!-- skip this method if it has parameters of a type that has wsmap="suppress" -->
    <xsl:choose>
      <xsl:when test="   (param[@type=($G_setSuppressedInterfaces/@name)])
                      or (param[@mod='ptr'])" >
        <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it has parameters with suppressed types')" /></xsl:comment>
      </xsl:when>
      <xsl:when test="@wsmap = 'suppress'">
        <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it is suppressed')" /></xsl:comment>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="emitInOutOperation">
          <xsl:with-param name="_ifname" select="$ifname" />
          <xsl:with-param name="_methodname" select="$methodname" />
          <xsl:with-param name="_params" select="param" />
          <xsl:with-param name="_fSoap" select="1" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<!--**********************************************************************
 *
 *  matches
 *
 **********************************************************************-->

<!--
    template for "idl" match; this emits the header of the target file
    and recurses into the libraries with interfaces (which are matched below)
    -->
<xsl:template match="/idl">
  <xsl:comment>
  DO NOT EDIT! This is a generated file.
  Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
  Generator: src/VBox/Main/webservice/websrv-wsdl.xsl
</xsl:comment>

  <xsl:apply-templates />

</xsl:template>

<!--
    template for "if" match: ignore all ifs except those for wsdl
    -->
<xsl:template match="if">
  <xsl:if test="@target='wsdl'">
    <xsl:apply-templates/>
  </xsl:if>
</xsl:template>

<!--
    template for "cpp": ignore
    -->
<xsl:template match="cpp">
<!--  ignore this -->
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  class
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="module/class">
<!--  swallow -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  enum
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="enum">
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  desc
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="desc">
<!--  swallow -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  note
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="note">
  <xsl:apply-templates />
</xsl:template>

<!--
    "library" match: we use this to emit most of the WSDL <types> section.
    With WSDL "document" style, this requires us to go through all interfaces
    and emit complexTypes for all method arguments and return values.
-->
<xsl:template match="library">
  <wsdl:definitions
        name="VirtualBox"
        xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/">
    <xsl:attribute name="targetNamespace"><xsl:value-of select="$G_targetNamespace" /></xsl:attribute>
    <!-- at top of WSDL file, dump a <types> section with user-defined types -->
      <xsl:comment>
  ******************************************************
  *
  * WSDL type definitions in XML Schema
  *
  ******************************************************
</xsl:comment>
    <wsdl:types>
      <xsd:schema>
        <xsl:attribute name="targetNamespace"><xsl:value-of select='$G_targetNamespace' /></xsl:attribute>

        <!-- type-define all enums -->
        <xsl:comment>
      ******************************************************
      * enumerations
      ******************************************************
</xsl:comment>
        <xsl:for-each select="//enum">
          <xsl:comment> enum: <xsl:value-of select="@name" /> -
            <xsl:for-each select="const">
              <xsl:value-of select="@name" />: <xsl:value-of select="@value" /> -
            </xsl:for-each>
</xsl:comment>
          <xsd:simpleType>
            <xsl:attribute name="name"><xsl:value-of select="@name" /></xsl:attribute>
            <xsd:restriction base="xsd:string">
            <!-- XML Schema does not seem to have a C-like mapping between identifiers and numbers;
                 instead, it treats enumerations like strings that can have only specific values. -->
              <xsl:for-each select="const">
                <xsd:enumeration>
                  <xsl:attribute name="value"><xsl:value-of select="@name" /></xsl:attribute>
                </xsd:enumeration>
              </xsl:for-each>
            </xsd:restriction>
          </xsd:simpleType>
        </xsl:for-each>

        <!-- type-define all interfaces that have wsmap=struct as structs (complexTypes) -->
        <xsl:comment>
      ******************************************************
      * structs
      ******************************************************
</xsl:comment>
        <xsl:for-each select="//interface[@wsmap='struct']">
          <xsl:comment> interface <xsl:value-of select="@name" /> as struct: </xsl:comment>
          <xsd:complexType>
            <xsl:attribute name="name"><xsl:value-of select="@name" /></xsl:attribute>
            <xsd:sequence>
              <xsl:for-each select="attribute">
                <xsd:element>
                  <xsl:attribute name="name"><xsl:value-of select="@name" /></xsl:attribute>
                  <xsl:attribute name="type">
                    <xsl:call-template name="emitConvertedType">
                      <xsl:with-param name="type" select="@type" />
                    </xsl:call-template>
                  </xsl:attribute>
                </xsd:element>
              </xsl:for-each>
            </xsd:sequence>
          </xsd:complexType>
        </xsl:for-each>

        <!-- for WSDL 'document' style, we need to emit elements since we can't
             refer to types in message parts as with RPC style -->
        <xsl:if test="$G_basefmt='document'">
          <xsl:comment>
      ******************************************************
      * elements for message arguments (parts); generated for WSDL 'document' style
      ******************************************************
</xsl:comment>

          <xsl:for-each select="//interface">
            <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
            <xsl:variable name="wsmap"><xsl:value-of select="@wsmap" /></xsl:variable>

            <xsl:if test='not( ($wsmap="suppress") or ($wsmap="struct") )'>
              <xsl:comment>Interface <xsl:copy-of select="$ifname" /></xsl:comment>
              <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('************* types: elements for interface &quot;', $ifname, '&quot;')" /></xsl:call-template>
              <!-- a) attributes first -->
              <xsl:for-each select="attribute">
                <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
                <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
                <xsl:variable name="attrsafearray"><xsl:value-of select="@safearray" /></xsl:variable>
                <xsl:variable name="attrreadonly"><xsl:value-of select="@readonly" /></xsl:variable>
                <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('elements for ', $ifname, '::', $attrname, ': attribute of type &quot;', $attrtype, '&quot;, readonly: ', $attrreadonly)" /></xsl:call-template>
                <!-- skip this attribute if it has parameters of a type that has wsmap="suppress" -->
                <xsl:choose>
                  <xsl:when test="( $attrtype=($G_setSuppressedInterfaces/@name) )">
                    <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrtype, ' for it is of a suppressed type')" /></xsl:comment>
                  </xsl:when>
                  <xsl:when test="@wsmap = 'suppress'">
                    <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrname, ' for it is suppressed')" /></xsl:comment>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:choose>
                      <xsl:when test="@readonly='yes'">
                        <xsl:comment> readonly attribute <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$attrname" /> </xsl:comment>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:comment> read/write attribute <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$attrname" /> </xsl:comment>
                      </xsl:otherwise>
                    </xsl:choose>
                    <!-- aa) get method: emit request and result -->
                    <xsl:call-template name="emitGetAttributeElements">
                      <xsl:with-param name="ifname" select="$ifname" />
                      <xsl:with-param name="wsmap" select="$wsmap" />
                      <xsl:with-param name="attrname" select="$attrname" />
                      <xsl:with-param name="attrtype" select="$attrtype" />
                      <xsl:with-param name="attrsafearray" select="$attrsafearray" />
                    </xsl:call-template>
                    <!-- bb) emit a set method if the attribute is read/write -->
                    <xsl:if test="not($attrreadonly='yes')">
                      <xsl:variable name="attrSetter"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
                      <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('..', $ifname, '::', $attrSetter)" /></xsl:call-template>
                      <xsl:call-template name="emitRequestElements">
                        <xsl:with-param name="_ifname" select="$ifname" />
                        <xsl:with-param name="_wsmap" select="$wsmap" />
                        <xsl:with-param name="_methodname" select="$attrSetter" />
                        <xsl:with-param name="_params" select="/.." />
                        <xsl:with-param name="_valuetype" select="$attrtype" />
                        <xsl:with-param name="_valuesafearray" select="$attrsafearray" />
                      </xsl:call-template>
                      <xsl:call-template name="emitResultElements">
                        <xsl:with-param name="_ifname" select="$ifname" />
                        <xsl:with-param name="_methodname" select="$attrSetter" />
                        <xsl:with-param name="_params" select="/.." />
                      </xsl:call-template>
                    </xsl:if>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:for-each> <!-- select="attribute" -->
              <!-- b) "real" methods after the attributes -->
              <xsl:for-each select="method">
                <xsl:variable name="methodname"><xsl:value-of select="@name" /></xsl:variable>
                <xsl:call-template name="debugMsg"><xsl:with-param name="msg" select="concat('messages for ', $ifname, '::', $methodname, ': method')" /></xsl:call-template>
                <xsl:comment> method <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$methodname" /> </xsl:comment>
                <!-- skip this method if it has parameters of a type that has wsmap="suppress" -->
                <xsl:choose>
                  <xsl:when test="   (param[@type=($G_setSuppressedInterfaces/@name)])
                                  or (param[@mod='ptr'])" >
                    <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it has parameters with suppressed types')" /></xsl:comment>
                  </xsl:when>
                  <xsl:when test="@wsmap = 'suppress'">
                    <xsl:comment><xsl:value-of select="concat('skipping method ', $methodname, ' for it is suppressed')" /></xsl:comment>
                  </xsl:when>
                  <xsl:otherwise>
                    <!-- always emit a request message -->
                    <xsl:call-template name="emitRequestElements">
                      <xsl:with-param name="_ifname" select="$ifname" />
                      <xsl:with-param name="_wsmap" select="$wsmap" />
                      <xsl:with-param name="_methodname" select="$methodname" />
                      <xsl:with-param name="_params" select="param" />
                    </xsl:call-template>
                    <!-- emit a second "result" message only if the method has "out" arguments or a return value -->
                    <!-- <xsl:if test="(count(param[@dir='out'] | param[@dir='return']) > 0)"> -->
                      <xsl:call-template name="emitResultElements">
                        <xsl:with-param name="_ifname" select="$ifname" />
                        <xsl:with-param name="_wsmap" select="$wsmap" />
                        <xsl:with-param name="_methodname" select="$methodname" />
                        <xsl:with-param name="_params" select="param" />
                      </xsl:call-template>
                    <!-- </xsl:if> -->
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:for-each>
            </xsl:if> <!-- <xsl:if test='not( ($wsmap="suppress") or ($wsmap="struct") )'> -->
          </xsl:for-each>

        </xsl:if> <!-- <xsl:if test="$G_basefmt='document'"> -->

        <xsl:comment>
      ******************************************************
      * faults
      ******************************************************
</xsl:comment>

        <xsd:element name="InvalidObjectFault">
          <xsd:complexType>
            <xsd:sequence>
              <xsd:element name="badObjectID">
                <xsl:attribute name="type">
                  <xsl:value-of select="$G_typeObjectRef" />
                </xsl:attribute>
              </xsd:element>
            </xsd:sequence>
          </xsd:complexType>
        </xsd:element>

        <xsd:element name="RuntimeFault">
          <xsd:complexType>
            <xsd:sequence>
              <xsd:element name="resultCode" type="xsd:int" />
              <xsd:element name="returnval">
                <xsl:attribute name="type">
                  <xsl:value-of select="$G_typeObjectRef" />
                </xsl:attribute>
              </xsd:element>
            </xsd:sequence>
          </xsd:complexType>
        </xsd:element>

      <!-- done! -->
      </xsd:schema>


    </wsdl:types>

    <wsdl:message name="InvalidObjectFaultMsg">
      <wsdl:part name="fault" element="vbox:InvalidObjectFault" />
    </wsdl:message>
    <wsdl:message name="RuntimeFaultMsg">
      <wsdl:part name="fault" element="vbox:RuntimeFault" />
    </wsdl:message>

    <xsl:comment>
  ******************************************************
  *
  * messages for all interfaces
  *
  ******************************************************
</xsl:comment>

    <xsl:for-each select="//interface">
      <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
      <xsl:variable name="wsmap"><xsl:value-of select="@wsmap" /></xsl:variable>

      <xsl:call-template name="verifyInterface">
        <xsl:with-param name="ifname" select="$ifname" />
        <xsl:with-param name="wsmap" select="$wsmap" />
      </xsl:call-template>

      <xsl:comment>
        *************************************
        messages for interface <xsl:copy-of select="$ifname" />
        *************************************
      </xsl:comment>

      <xsl:if test='not( ($wsmap="suppress") or ($wsmap="struct") )'>
        <xsl:call-template name="emitMessagesForInterface">
          <xsl:with-param name="ifname" select="$ifname" />
          <xsl:with-param name="wsmap" select="$wsmap" />
        </xsl:call-template>
      </xsl:if>
    </xsl:for-each>

    <xsl:comment>
      ******************************************************
      *
      * one portType for all interfaces
      *
      ******************************************************
    </xsl:comment>

    <wsdl:portType>
      <xsl:attribute name="name"><xsl:copy-of select="'vbox'" /><xsl:value-of select="$G_portTypeSuffix" /></xsl:attribute>

      <xsl:for-each select="//interface">
        <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
        <xsl:variable name="wsmap"><xsl:value-of select="@wsmap" /></xsl:variable>

        <xsl:comment>
          *************************************
          operations in portType for interface <xsl:copy-of select="$ifname" />
          *************************************
        </xsl:comment>

        <xsl:if test='not( ($wsmap="suppress") or ($wsmap="struct") )'>
          <xsl:call-template name="emitOperationsInPortTypeForInterface">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="wsmap" select="$wsmap" />
          </xsl:call-template>
        </xsl:if>
      </xsl:for-each>
    </wsdl:portType>

    <xsl:comment>
      ******************************************************
      *
      * one binding for all interfaces
      *
      ******************************************************
    </xsl:comment>

    <wsdl:binding>
      <xsl:attribute name="name"><xsl:value-of select="concat('vbox', $G_bindingSuffix)" /></xsl:attribute>
      <xsl:attribute name="type"><xsl:value-of select="concat('vbox:vbox', $G_portTypeSuffix)" /></xsl:attribute>

      <soap:binding>
        <xsl:attribute name="style"><xsl:value-of select="$G_basefmt" /></xsl:attribute>
        <xsl:attribute name="transport">http://schemas.xmlsoap.org/soap/http</xsl:attribute>
      </soap:binding>

      <xsl:for-each select="//interface">
        <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
        <xsl:variable name="wsmap"><xsl:value-of select="@wsmap" /></xsl:variable>

        <xsl:comment>
          *************************************
          operations in portType for interface <xsl:copy-of select="$ifname" />
          *************************************
        </xsl:comment>

        <xsl:if test='not( ($wsmap="suppress") or ($wsmap="struct") )'>
          <xsl:call-template name="emitOperationsInBindingForInterface">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="wsmap" select="$wsmap" />
          </xsl:call-template>
        </xsl:if>
      </xsl:for-each>
    </wsdl:binding>

  </wsdl:definitions>
</xsl:template>


</xsl:stylesheet>
