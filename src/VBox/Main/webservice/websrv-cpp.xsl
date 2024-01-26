<?xml version="1.0"?>

<!--
    websrv-cpp.xsl:
        XSLT stylesheet that generates methodmaps.cpp from
        VirtualBox.xidl. This generated C++ code contains
        all the service implementations that one would
        normally have to implement manually to create a
        web service; our generated code automatically maps
        all SOAP calls into COM/XPCOM method calls.
        See webservice/Makefile.kmk for an overview of all the things
        generated for the webservice.
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

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl">

  <xsl:output method="text"/>

  <xsl:strip-space elements="*"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_xsltFilename" select="'websrv-cpp.xsl'" />

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<!-- collect all interfaces with "wsmap='suppress'" in a global variable for
     quick lookup -->
<xsl:variable name="G_setSuppressedInterfaces"
              select="//interface[@wsmap='suppress']" />

<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  root match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="/idl">
  <xsl:text><![CDATA[
/* DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
 * Generator: src/VBox/Main/webservice/websrv-cpp.xsl
 */

// shared webservice header
#include "vboxweb.h"

// vbox headers
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/VBoxAuth.h>

#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/string.h>

// gSOAP headers (must come after vbox includes because it checks for conflicting defs)
#include "soapH.h"

// standard headers
#include <map>
#include <iprt/sanitized/sstream>

// shared strings for debug output
const char *g_pcszCallingComMethod = "   calling COM method %s\n";
const char *g_pcszDoneCallingComMethod = "   done calling COM method\n";
const char *g_pcszConvertComOutputBack = "   convert COM output \"%s\" back to caller format\n";
const char *g_pcszDoneConvertingComOutputBack = "   done converting COM output \"%s\" back to caller format\n";
const char *g_pcszEntering = "-- entering %s\n";
const char *g_pcszLeaving = "-- leaving %s, rc: %#lx (%d)\n";

// generated string constants for all interface names
const char *g_pcszIUnknown = "IUnknown";
]]></xsl:text>

  <xsl:for-each select="//interface">
    <xsl:variable name="ifname" select="@name" />
    <xsl:value-of select="concat('const char *g_pcsz', $ifname, ' = &quot;', $ifname, '&quot;;')" />
    <xsl:call-template name="emitNewline" />
  </xsl:for-each>
  <xsl:apply-templates />
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  if
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
 *  ignore all |if|s except those for WSDL target
-->
<xsl:template match="if">
    <xsl:if test="@target='wsdl'">
        <xsl:apply-templates/>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  cpp
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="cpp">
<!--  ignore this -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  library
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="library">
  <xsl:text>
/****************************************************************************
 *
 * types: enum converter helper functions
 *
 ****************************************************************************/
  </xsl:text>
  <!--
    enum converter functions at top of file
    -->
  <xsl:for-each select="//enum">
    <xsl:variable name="enumname" select="@name" />
    <!-- generate enum converter for COM-to-SOAP -->
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('vbox__', $enumname, ' ', $G_funcPrefixOutputEnumConverter, $enumname, '(', $enumname, '_T e)')" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>{</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('    vbox__', $enumname, ' v;')" />
    <xsl:call-template name="emitNewline" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>    switch(e)</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:text>    {</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:for-each select="const[not(@wsmap='suppress')]">
      <xsl:variable name="enumconst" select="@name" />
      <xsl:value-of select="concat('        case ', $enumname, '_', $enumconst, ':')" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="concat('    v = vbox__', $enumname, '__')" />
      <!-- escape all "_" in $enumconst -->
      <xsl:call-template name="escapeUnderscores">
        <xsl:with-param name="string" select="$enumconst" />
      </xsl:call-template>
      <xsl:value-of select="';'" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:text>break;</xsl:text>
      <xsl:call-template name="emitNewline" />
    </xsl:for-each>
    <!-- Add a default case so gcc gives us a rest, esp. on darwin. -->
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:text>default:</xsl:text>
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:text>    AssertMsgFailed(("e=%d\n", (int)e));</xsl:text>
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:value-of select="concat('    v = (vbox__', $enumname, ')0x7fffdead;')" />
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:text>break; </xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:text>    }</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>    return v;</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:text>}</xsl:text>
    <xsl:call-template name="emitNewline" />
    <!-- generate enum converter for SOAP-to-COM -->
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat($enumname, '_T ', $G_funcPrefixInputEnumConverter, $enumname, '(vbox__', $enumname, ' v)')" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>{</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('    ', $enumname, '_T e;')" />
    <xsl:call-template name="emitNewline" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>    switch(v)</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:text>    {</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:for-each select="const[not(@wsmap='suppress')]">
      <xsl:variable name="enumconst" select="@name" />
      <xsl:value-of select="concat('        case vbox__', $enumname, '__')" />
      <!-- escape all "_" in $enumconst -->
      <xsl:call-template name="escapeUnderscores">
        <xsl:with-param name="string" select="$enumconst" />
      </xsl:call-template>
      <xsl:value-of select="':'" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="concat('    e = ',  $enumname, '_', $enumconst, ';')" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:text>break;</xsl:text>
      <xsl:call-template name="emitNewline" />
    </xsl:for-each>
    <!-- Insert a default case so gcc gives us a rest, esp. on darwin. -->
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:text>default:</xsl:text>
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:text>    AssertMsgFailed(("v=%d\n", (int)v));</xsl:text>
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:value-of select="concat('    e = (', $enumname, '_T)0x7fffbeef;')" />
    <xsl:call-template name="emitNewlineIndent8" />
    <xsl:text>break; </xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:text>    }</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>    return e;</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:text>}</xsl:text>
    <xsl:call-template name="emitNewline" />
  </xsl:for-each>

  <xsl:text>
/****************************************************************************
 *
 * types: struct converter helper functions
 *
 ****************************************************************************/
  </xsl:text>

  <xsl:for-each select="//interface[@wsmap='struct']">
    <xsl:variable name="structname" select="@name" />

    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('// ', $structname, ' converter: called from method mappers to convert data from')" />
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('// COM interface ', $structname, ', which has wsmap=&quot;struct&quot;, to SOAP structures')" />
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('vbox__', $structname, '* ', $G_funcPrefixOutputStructConverter, $structname, '(')" />
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="'    struct soap *soap,'" />
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="'    const WSDLT_ID &amp;idThis,'" />
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="'    HRESULT &amp;rc,'" />
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('    ComPtr&lt;', $structname, '&gt; &amp;in)')" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>{</xsl:text>
    <xsl:call-template name="emitNewline" />

    <xsl:value-of select="concat('    vbox__', $structname, ' *resp = NULL;')" />
    <xsl:call-template name="emitNewline" />

    <xsl:call-template name="emitPrologue"><xsl:with-param name="fSkipHRESULT" select="'1'"/></xsl:call-template>

    <xsl:value-of select="concat('        resp = soap_new_vbox__', $structname, '(soap, -1);')" />
    <xsl:call-template name="emitNewline" />
    <xsl:text>        if (!in)&#10;</xsl:text>
    <xsl:text>        {&#10;</xsl:text>
    <xsl:text>            // @todo ambiguous. Problem is the MOR for the object converted to struct&#10;</xsl:text>
    <xsl:text>            RaiseSoapInvalidObjectFault(soap, "");&#10;</xsl:text>
    <xsl:text>            break;&#10;</xsl:text>
    <xsl:text>        }&#10;</xsl:text>
    <xsl:call-template name="emitNewline" />

    <xsl:for-each select="key('G_keyInterfacesByName', $structname)/attribute">
      <xsl:if test="not(@wsmap = 'suppress')">
        <xsl:value-of select="concat('        // -- ', $structname, '.', @name)" />
        <xsl:call-template name="emitNewline" />
        <!-- recurse! -->
        <xsl:call-template name="emitGetAttributeComCall">
          <xsl:with-param name="ifname" select="$structname" />
          <xsl:with-param name="object" select="'in'" />
          <xsl:with-param name="attrname" select="@name" />
          <xsl:with-param name="attrtype" select="@type" />
          <xsl:with-param name="callerprefix" select="concat('out', '.')" />
        </xsl:call-template>
        <xsl:call-template name="emitNewline" />
      </xsl:if>
    </xsl:for-each>

    <xsl:call-template name="emitEpilogue"><xsl:with-param name="fSkipHRESULT" select="'1'"/></xsl:call-template>

  </xsl:for-each>

  <xsl:apply-templates />
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  class
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="module/class">
<!--  TODO swallow for now -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  enum
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="enum">
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  const
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
<xsl:template match="const">
  <xsl:apply-templates />
</xsl:template>
-->

<!-- - - - - - - - - - - - - - - - - - - - - - -
  desc
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="desc">
<!--  TODO swallow for now -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  note
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="note">
<!--  TODO -->
  <xsl:apply-templates />
</xsl:template>

<!--
   emitBeginOfFunctionHeader:
-->

<xsl:template name="emitBeginOfFunctionHeader">
  <xsl:param name="ifname" />
  <xsl:param name="method" />

  <xsl:call-template name="emitNewline" />
  <xsl:value-of select="concat('int __vbox__', $ifname, '_USCORE', $method, '(')" />
  <xsl:call-template name="emitNewline" />
  <xsl:text>    struct soap *soap</xsl:text>
</xsl:template>

<!--
    emitCppTypeForIDLType:
    emits the C++ type that corresponds to the given WSDL type in $type.
    -->
<xsl:template name="emitCppTypeForIDLType">
  <xsl:param name="method" />
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="varprefix" />      <!-- only with nested get-attribute calls -->
  <xsl:param name="inptr" />          <!-- whether to add INPTR to BSTR (Dmitry template magic) -->

  <!-- look up C++ glue type from IDL type from table array in typemap-shared.inc.xsl -->
  <xsl:variable name="gluetypefield" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@gluename" />

  <xsl:choose>
    <xsl:when test="$type='wstring' or $type='uuid'">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:choose>
            <xsl:when test="$inptr='yes'">
              <xsl:value-of select="'com::SafeArray&lt;IN_BSTR&gt;'" />   <!-- input string arrays must use IN_BSTR (see com/array.h) -->
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="'com::SafeArray&lt;BSTR&gt;'" />   <!-- output string arrays use raw BSTR -->
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="'com::Bstr'" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- if above lookup in table succeeded, use that type -->
    <xsl:when test="string-length($gluetypefield)">
      <xsl:call-template name="emitTypeOrArray">
        <xsl:with-param name="type" select="$gluetypefield"/>
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
      <xsl:call-template name="emitTypeOrArray">
        <xsl:with-param name="type" select="concat($type, '_T ')"/>
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$type='$unknown'">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="'com::SafeIfaceArray&lt;IUnknown&gt;'" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="'ComPtr&lt;IUnknown&gt;'" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
      <xsl:variable name="thatif" select="key('G_keyInterfacesByName', $type)" />
      <xsl:variable name="thatifname" select="$thatif/@name" />
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('com::SafeIfaceArray&lt;', $thatifname, '&gt;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('ComPtr&lt;', $thatifname, '&gt;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('emitCppTypeForIDLType: Type &quot;', $type, '&quot; in method &quot;', $method, '&quot; is not supported.')" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
  emitDocumentStyleArgStructs:
    with WSDL "document" style only, emits those lengthy structs for
    the input and output argument in the function header.
-->
<xsl:template name="emitDocumentStyleArgStructs">
  <xsl:param name="ifname" />
  <xsl:param name="methodname" />
  <xsl:param name="fOutputs" />    <!-- if 1, emit output struct as well -->

  <xsl:text>,</xsl:text>
  <xsl:call-template name="emitNewline" />
  <xsl:value-of select="concat('    _vbox__', $ifname, '_USCORE', $methodname, $G_requestMessageElementSuffix, ' *', $G_requestElementVarName)" />
  <xsl:if test="$fOutputs">
    <xsl:text>,</xsl:text>
    <xsl:call-template name="emitNewline" />
    <xsl:value-of select="concat('    _vbox__', $ifname, '_USCORE', $methodname, $G_responseMessageElementSuffix, ' *', $G_responseElementVarName)" />
    <!-- <xsl:value-of select="concat('    struct ', $ifname, '__', $methodname, 'Response &amp;', $G_result)" /> -->
  </xsl:if>

</xsl:template>

<!--
    emitPrologue:
    emits the closing ")" for the parameter list and the beginning
    of the function body.
    -->
<xsl:template name="emitPrologue">
  <xsl:text>    WEBDEBUG((g_pcszEntering, __FUNCTION__));

    do {</xsl:text>
  <xsl:call-template name="emitNewline" />
</xsl:template>

<!--
    emitEpilogue
    -->
<xsl:template name="emitEpilogue">
  <xsl:param name="fSkipHRESULT" />

  <xsl:text>    } while (0);</xsl:text>
  <xsl:call-template name="emitNewline" />
  <xsl:call-template name="emitNewline" />
  <xsl:text>    WEBDEBUG((g_pcszLeaving, __FUNCTION__, rc, rc));</xsl:text>
  <xsl:call-template name="emitNewline" />
  <xsl:if test="not($fSkipHRESULT)">
    <xsl:text>
    if (FAILED(rc))
        return SOAP_FAULT;
    return SOAP_OK;</xsl:text>
  </xsl:if>
  <xsl:if test="$fSkipHRESULT">
    <xsl:text>    return resp;</xsl:text>
  </xsl:if>
  <xsl:call-template name="emitNewline" />
  <xsl:text>}</xsl:text>
  <xsl:call-template name="emitNewline" />
</xsl:template>

<!--
  emitObjForMethod:
    after the function prologue, emit a "pObj" object that
    specifies the object upon which the method should be invoked.
-->
<xsl:template name="emitObjForMethod">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />
  <xsl:param name="structprefix" />   <!-- with WSDL document style: req element prefix, like "vbox__IVirtualBox_USCOREcreateMachineRequestElement->" -->

  <xsl:choose>
    <xsl:when test="$wsmap='global'">
      <xsl:choose>
        <xsl:when test="$ifname='IVirtualBox'">
          <xsl:text>        // invoke method on global IVirtualBox instance</xsl:text>
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:text>ComPtr&lt;IVirtualBox&gt; pObj = G_pVirtualBox;</xsl:text>
          <xsl:call-template name="emitNewline" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('emitObjForMethod: Unknown interface &quot;', $ifname, '&quot; with wsmap=global in XIDL.')" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="($wsmap='managed')">
      <xsl:text>        // look up managed object reference for method call&#10;</xsl:text>
      <xsl:value-of select="concat('        ComPtr&lt;', $ifname, '&gt; pObj;&#10;')" />
      <xsl:value-of select="concat('        if (!', $G_requestElementVarName, ')&#10;')" />
      <xsl:text>        {&#10;</xsl:text>
      <xsl:text>            RaiseSoapInvalidObjectFault(soap, "");&#10;</xsl:text>
      <xsl:text>            break;&#10;</xsl:text>
      <xsl:text>        }&#10;</xsl:text>
      <xsl:value-of select="concat('        const WSDLT_ID &amp;idThis = ', $structprefix, $G_nameObjectRefEncoded, ';&#10;')" />
      <xsl:value-of select="'        if ((rc = findComPtrFromId(soap, idThis, pObj, false)) != S_OK)&#10;'" />
      <xsl:text>            break;&#10;</xsl:text>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!--
  emitInputArgConverter:
    another type converter (from wsdl type to COM types),
    that generates temporary variables on the stack with
    the WSDL input parameters converted to the COM types,
    so we can then pass them to the actual COM method call.
-->
<xsl:template name="emitInputArgConverter">
  <xsl:param name="ifname" />
  <xsl:param name="object" />       <!-- normally "pObj" -->
  <xsl:param name="method" />
  <xsl:param name="methodname" />
  <xsl:param name="structprefix" />   <!-- with WSDL document style: req element prefix, like "vbox__IVirtualBox_USCOREcreateMachineRequestElement->" -->
  <xsl:param name="name" />
  <xsl:param name="type" />
  <xsl:param name="safearray" />

  <xsl:value-of select="concat('        // convert input arg ', $name, '(safearray: ', $safearray, ')')" />
  <xsl:call-template name="emitNewlineIndent8" />

  <xsl:choose>
     <xsl:when test="$safearray='yes' and $type='octet'">
       <xsl:value-of select="concat('com::SafeArray&lt;BYTE&gt; comcall_',$name, ';')" />
       <xsl:call-template name="emitNewlineIndent8" />
       <xsl:value-of select="concat('Base64DecodeByteArray(soap, ',$structprefix,$name,', ComSafeArrayAsOutParam(comcall_',$name, '), idThis, &quot;', $ifname, '::', $methodname, '&quot;, ', $object, ', COM_IIDOF(', $ifname, '));')" />
    </xsl:when>

    <xsl:when test="$safearray='yes'">
      <xsl:value-of select="concat('size_t c', $name, ' = ', $structprefix, $name, '.size();')" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:call-template name="emitCppTypeForIDLType">
        <xsl:with-param name="method" select="$method"/>
        <xsl:with-param name="type" select="$type"/>
        <xsl:with-param name="safearray" select="$safearray"/>
        <xsl:with-param name="inptr" select="'yes'"/>
      </xsl:call-template>
      <xsl:value-of select="concat(' comcall_', $name, '(c', $name, ');')" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="concat('for (size_t i = 0; i &lt; c', $name, '; ++i)')" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="'{'" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:choose>
        <xsl:when test="$type='$unknown'">
          <xsl:value-of select="'    ComPtr&lt;IUnknown&gt; tmpObject;'" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    if ((rc = findComPtrFromId(soap, ', $structprefix, $name, '[i], tmpObject, true)) != S_OK)')" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:text>        break;</xsl:text>
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    IUnknown *tmpObject2(tmpObject); tmpObject2->AddRef(); comcall_', $name, '[i] = tmpObject;')" />
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <xsl:value-of select="concat('    ComPtr&lt;', $type, '&gt; tmpObject;')" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    if ((rc = findComPtrFromId(soap, ', $structprefix, $name, '[i], tmpObject, true)) != S_OK)')" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:text>        break;</xsl:text>
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    ', $type, ' *tmpObject2(tmpObject); tmpObject2->AddRef(); comcall_', $name, '[i] = tmpObject;')" />
        </xsl:when>
        <xsl:when test="$type='wstring'">
          <xsl:value-of select="concat('    com::Bstr tmpObject(', $structprefix, $name, '[i].c_str());')" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="'    BSTR tmpObjectB;'" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="'    tmpObject.detachTo(&amp;tmpObjectB);'" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    comcall_', $name, '[i] = tmpObjectB;')" />
        </xsl:when>
        <xsl:when test="$type='long'">
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    comcall_', $name, '[i] = ', $structprefix, $name, '[i];')" />
        </xsl:when>
        <xsl:when test="$type='long long'">
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    comcall_', $name, '[i] = ', $structprefix, $name, '[i];')" />
        </xsl:when>
        <xsl:when test="$type='boolean'">
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    comcall_', $name, '[i] = ', $structprefix, $name, '[i];')" />
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('    comcall_', $name, '[i] = ', $G_funcPrefixInputEnumConverter, $type, '(', $structprefix, $name, '[i]);')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('emitInputArgConverter Type &quot;', $type, '&quot; in arg &quot;', $name, '&quot; of method &quot;', $method, '&quot; is not yet supported in safearrays.')" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="'}'" />
      <xsl:call-template name="emitNewline" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="emitCppTypeForIDLType">
        <xsl:with-param name="method" select="$method"/>
        <xsl:with-param name="type" select="$type"/>
        <xsl:with-param name="safearray" select="$safearray"/>
        <xsl:with-param name="inptr" select="'yes'"/>
      </xsl:call-template>
      <xsl:choose>
        <xsl:when test="$type='wstring' or $type='uuid'">
          <xsl:value-of select="concat(' comcall_', $name, '(', $structprefix, $name, '.c_str())')" />
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
          <xsl:value-of select="concat(' comcall_', $name, ' = ', $G_funcPrefixInputEnumConverter, $type, '(', $structprefix, $name, ')')" />
        </xsl:when>
        <xsl:when test="$type='$unknown'">
          <xsl:value-of select="concat(' comcall_', $name, ';')" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:value-of select="concat('if ((rc = findComPtrFromId(soap, ', $structprefix, $name, ', comcall_', $name,', true)) != S_OK)')" />
          <xsl:call-template name="emitNewlineIndent8" />
          <xsl:text>    break</xsl:text>
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <!-- the type is one of our own interfaces: then it must have a wsmap attr -->
          <xsl:variable name="thatif" select="key('G_keyInterfacesByName', $type)" />
          <xsl:variable name="wsmap" select="$thatif/@wsmap" />
          <xsl:variable name="thatifname" select="$thatif/@name" />
          <xsl:choose>
            <xsl:when test="not($wsmap)">
              <xsl:call-template name="fatalError">
                <xsl:with-param name="msg" select="concat('emitInputArgConverter: Type &quot;', $type, '&quot; in arg &quot;', $name, '&quot; of method &quot;', $method, '&quot; lacks wsmap attribute in XIDL.')" />
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="($wsmap='managed')">
              <xsl:value-of select="concat(' comcall_', $name, ';')" />
              <xsl:call-template name="emitNewlineIndent8" />
              <xsl:value-of select="concat('if ((rc = findComPtrFromId(soap, ', $structprefix, $name, ', comcall_', $name,', true)) != S_OK)')" />
              <xsl:call-template name="emitNewlineIndent8" />
              <xsl:text>    break</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="fatalError">
                <xsl:with-param name="msg" select="concat('emitInputArgConverter: Type &quot;', $type, '&quot; in arg &quot;', $name, '&quot; of method &quot;', $method, '&quot; has unsupported wsmap attribute value &quot;', $wsmap, '&quot; in XIDL.')" />
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat(' comcall_', $name, ' = ', $structprefix, $name)" />
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>;
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<!--
    emitTypeOrArray
-->

<xsl:template name="emitTypeOrArray">
  <xsl:param name="type" />
  <xsl:param name="safearray" />

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:value-of select="concat('com::SafeArray&lt;', $type, '&gt;')" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$type" />
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
  emitOutputArgBuffer:
    another type converter (from wsdl type to COM types)
    that generates a buffer variable which receives the
    data from 'out' and 'return' parameters of the COM method call.
-->
<xsl:template name="emitOutputArgBuffer">
  <xsl:param name="method" />
  <xsl:param name="name" />
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="varprefix" />      <!-- only with nested get-attribute calls -->

  <xsl:text>        // com output arg for </xsl:text><xsl:value-of select="concat($name, ' (safearray: ', $safearray, ')')" /><xsl:text>
        </xsl:text>
  <xsl:call-template name="emitCppTypeForIDLType">
    <xsl:with-param name="method" select="$method" />
    <xsl:with-param name="type" select="$type" />
    <xsl:with-param name="safearray" select="$safearray" />
  </xsl:call-template>
  <xsl:value-of select="concat(' comcall_', $varprefix, $name, ';')" />
  <xsl:call-template name="emitNewline" />
</xsl:template>

<!--
    emitInParam:
-->
<xsl:template name="emitInParam">
  <xsl:param name="name" />
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="varprefix" />      <!-- only with nested set-attribute calls -->

  <xsl:variable name="varname" select="concat('comcall_', $varprefix, $name)" />

  <xsl:choose>
    <xsl:when test="@safearray='yes'">
      <xsl:value-of select="concat('ComSafeArrayAsInParam(', $varname, ')')" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$varname" />
      <xsl:if test="@type='wstring' or @type='uuid'">
        <xsl:text>.raw()</xsl:text>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
    emitOutParam:
-->
<xsl:template name="emitOutParam">
  <xsl:param name="name" />
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="varprefix" />      <!-- only with nested get-attribute calls -->

  <xsl:variable name="varname" select="concat('comcall_', $varprefix, $name)" />

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:value-of select="concat('ComSafeArrayAsOutParam(', $varname, ')')" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="   ($type='boolean')
                        or ($type='short')
                        or ($type='unsigned short')
                        or ($type='long')
                        or ($type='unsigned long')
                        or ($type='long long')
                        or ($type='unsigned long long')
                        or ($type='result')
                        or (count(key('G_keyEnumsByName', $type)) > 0)">
          <xsl:text>&amp;</xsl:text><xsl:value-of select="$varname" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$varname" /><xsl:text>.asOutParam()</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
  emitComCall:
    emits the actual method call with the arguments.
-->
<xsl:template name="emitComCall">
  <xsl:param name="ifname" />
  <xsl:param name="object" />       <!-- normally "pObj" -->
  <xsl:param name="methodname" />
  <xsl:param name="attrname" />     <!-- with attributes only -->
  <xsl:param name="attrtype" />     <!-- with attributes only -->
  <xsl:param name="attrsafearray" /> <!-- with attributes only -->
  <xsl:param name="attrdir" />      <!-- with attributes only: "in" or "return" -->
  <xsl:param name="varprefix" />      <!-- only with nested get-attribute calls -->

  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:value-of select="concat('WEBDEBUG((g_pcszCallingComMethod, &quot;', $methodname, '&quot;));')" />
  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:value-of select="concat('rc = ', $object, '-&gt;', $methodname, '(')" />
  <xsl:if test="$attrtype">
    <xsl:choose>
      <xsl:when test="$attrdir='in'">
        <xsl:call-template name="emitInParam">
          <xsl:with-param name="name" select="$attrname" />
          <xsl:with-param name="type" select="$attrtype" />
          <xsl:with-param name="safearray" select="$attrsafearray" />
          <xsl:with-param name="varprefix" select="$varprefix" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$attrdir='return'">
        <xsl:call-template name="emitOutParam">
          <xsl:with-param name="name" select="$attrname" />
          <xsl:with-param name="type" select="$attrtype" />
          <xsl:with-param name="safearray" select="$attrsafearray" />
          <xsl:with-param name="varprefix" select="$varprefix" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
  </xsl:if>
  <xsl:for-each select="param">
    <xsl:if test="position()=1">
      <xsl:call-template name="emitNewline" />
    </xsl:if>
    <xsl:if test="position() > 1">
      <xsl:text>,</xsl:text>
      <xsl:call-template name="emitNewline" />
    </xsl:if>
    <xsl:text>                                   </xsl:text>
    <xsl:choose>
      <xsl:when test="@dir='in'">
        <xsl:call-template name="emitInParam">
          <xsl:with-param name="name" select="@name" />
          <xsl:with-param name="type" select="@type" />
          <xsl:with-param name="safearray" select="@safearray" />
          <xsl:with-param name="varprefix" select="$varprefix" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@dir='out'">
        <xsl:call-template name="emitOutParam">
          <xsl:with-param name="name" select="@name" />
          <xsl:with-param name="type" select="@type" />
          <xsl:with-param name="safearray" select="@safearray" />
          <xsl:with-param name="varprefix" select="$varprefix" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="@dir='return'">
        <xsl:call-template name="emitOutParam">
          <xsl:with-param name="name" select="$G_result" />
          <xsl:with-param name="type" select="@type" />
          <xsl:with-param name="safearray" select="@safearray" />
          <xsl:with-param name="varprefix" select="$varprefix" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
  </xsl:for-each>
  <xsl:text>);</xsl:text>
  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:text>if (FAILED(rc))</xsl:text>
  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:text>{</xsl:text>
  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:value-of select="concat('    RaiseSoapRuntimeFault(soap, idThis, &quot;', $ifname, '::', $methodname,'&quot;, rc, ', $object, ', COM_IIDOF(', $ifname, '));')" />
  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:text>    break;</xsl:text>
  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:text>}</xsl:text>
  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:text>WEBDEBUG((g_pcszDoneCallingComMethod));</xsl:text>
  <xsl:call-template name="emitNewline" />
</xsl:template>

<!--
    emitOutputArgBackConverter2: implementation details of emitOutputArgBackConverter.
     -->

<xsl:template name="emitOutputArgBackConverter2">
  <xsl:param name="name" />
  <xsl:param name="varname" />
  <xsl:param name="type" />
  <xsl:param name="callerprefix" />

  <xsl:choose>
    <xsl:when test="$type='wstring' or $type='uuid'">
      <xsl:value-of select="concat('ConvertComString(', $varname, ')')" />
    </xsl:when>
    <xsl:when test="$type='boolean'">
      <!-- the "!!" avoids a microsoft compiler warning -->
      <xsl:value-of select="concat('!!', $varname)" />
    </xsl:when>
    <xsl:when test="   ($type='octet')
                    or ($type='short')
                    or ($type='unsigned short')
                    or ($type='long')
                    or ($type='unsigned long')
                    or ($type='long long')
                    or ($type='unsigned long long')
                    or ($type='result')">
      <xsl:value-of select="$varname" />
    </xsl:when>
    <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
      <xsl:value-of select="concat($G_funcPrefixOutputEnumConverter, $type, '(', $varname, ')')" />
    </xsl:when>
    <xsl:when test="$type='$unknown'">
      <xsl:value-of select="concat('createOrFindRefFromComPtr(idThis, g_pcszIUnknown, ', $varname, ')')" />
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
      <!-- the type is one of our own interfaces: then it must have a wsmap attr -->
      <xsl:variable name="thatif" select="key('G_keyInterfacesByName', $type)" />
      <xsl:variable name="wsmap" select="$thatif/@wsmap" />
      <xsl:variable name="thatifname" select="$thatif/@name" />
      <xsl:choose>
        <xsl:when test=" ($wsmap='managed') or ($wsmap='global')">
          <xsl:value-of select="concat('createOrFindRefFromComPtr(idThis, g_pcsz', $thatifname, ', ', $varname, ')')" />
        </xsl:when>
        <xsl:when test="$wsmap='struct'">
          <!-- prevent infinite recursion -->
          <!-- <xsl:call-template name="fatalError"><xsl:with-param name="msg" select="concat('emitOutputArgBackConverter2: attempted infinite recursion for type &quot;', $type, '&quot; in arg &quot;', $name, '&quot; of method &quot;', $ifname, '::', $method)" /></xsl:call-template> -->
          <xsl:if test="not($callerprefix)">
            <xsl:value-of select="concat('/* convert COM interface to struct */ ', $G_funcPrefixOutputStructConverter, $type, '(soap, idThis, rc, ', $varname, ')')" />
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('emitOutputArgBackConverter2: Type &quot;', $type, '&quot; in arg &quot;', $name, '&quot; of method &quot;', $thatifname, '::', $method, '&quot; has invalid wsmap attribute value &quot;', $wsmap, '&quot; in XIDL.')" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('emitOutputArgBackConverter2: Type &quot;', $type, '&quot; in arg &quot;', $name, '&quot; of method &quot;', $method, '&quot; is not supported.')" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<!--
  emitOutputArgBackConverter:
    another type converter (from COM type back to WSDL)
    which converts the output argument from the COM
    method call back to the WSDL type passed in by the
    caller.
-->
<xsl:template name="emitOutputArgBackConverter">
  <xsl:param name="ifname" />
  <xsl:param name="method" />
  <xsl:param name="name" />
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="varprefix" />       <!-- only when called recursively from emitGetAttributeComCall -->
  <xsl:param name="callerprefix" />    <!-- only for out params or when called recursively from emitGetAttributeComCall -->

  <xsl:variable name="topname" select="$name" />
  <xsl:variable name="varname" select="concat('comcall_', $varprefix, $name)" />

  <xsl:call-template name="emitNewlineIndent8" />
  <xsl:value-of select="concat('WEBDEBUG((g_pcszConvertComOutputBack, &quot;', $name, '&quot;));')" />
  <xsl:call-template name="emitNewlineIndent8" />

  <xsl:variable name="receiverVariable">
    <xsl:choose>
      <xsl:when test="(not($varprefix))">
        <xsl:choose>
          <xsl:when test="$callerprefix"> <!-- callerprefix set but varprefix not: then this is an out parameter :-) -->
            <xsl:value-of select="concat($G_responseElementVarName, '-&gt;', $name)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($G_responseElementVarName, '-&gt;', $G_result)" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($callerprefix, $G_result, '-&gt;', $name)" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$safearray='yes' and $type='octet'">
      <xsl:value-of select="concat($receiverVariable, ' = Base64EncodeByteArray(ComSafeArrayAsInParam(', $varname,'));')" />
      <xsl:call-template name="emitNewlineIndent8" />
    </xsl:when>

    <xsl:when test="$safearray='yes'">
      <xsl:value-of select="concat('for (size_t i = 0; i &lt; ', $varname, '.size(); ++i)')" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="'{'" />
      <xsl:call-template name="emitNewlineIndent8" />
      <!-- look up C++ glue type from IDL type from table array in typemap-shared.inc.xsl -->
      <xsl:variable name="gluetypefield" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@gluename" />
      <xsl:choose>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <xsl:value-of select="concat('    ComPtr&lt;', $type, '&gt; tmpObject(', $varname, '[i]);')" />
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
          <xsl:value-of select="concat('    ', $type, '_T tmpObject(', $varname, '[i]);')" />
        </xsl:when>
        <xsl:when test="$type='$unknown'">
          <xsl:value-of select="concat('    ComPtr&lt;IUnknown&gt; tmpObject(', $varname, '[i]);')" />
        </xsl:when>
        <xsl:when test="$type='wstring' or $type='uuid'">
          <xsl:value-of select="concat('    com::Bstr tmpObject(', $varname, '[i]);')" />
        </xsl:when>
        <xsl:when test="$gluetypefield">
          <xsl:value-of select="concat('    ', $gluetypefield, ' tmpObject(', $varname, '[i]);')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('emitOutputArgBackConverter (1): Type &quot;', $type, '&quot; in arg &quot;', $name, '&quot; of method &quot;', $method, '&quot; is not yet supported in safearrays.')" />
          </xsl:call-template>

        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="concat('    ', $receiverVariable, '.push_back(')" />
      <xsl:call-template name="emitOutputArgBackConverter2">
        <xsl:with-param name="name" select="$name"/>
        <xsl:with-param name="varname" select="'tmpObject'"/>
        <xsl:with-param name="type" select="$type"/>
        <xsl:with-param name="callerprefix" select="$callerprefix"/>
      </xsl:call-template>
      <xsl:value-of select="');'" />
      <xsl:call-template name="emitNewlineIndent8" />
      <xsl:value-of select="'}'" />
      <xsl:call-template name="emitNewline" />
    </xsl:when>
    <xsl:otherwise>
      <!-- emit variable name: "resp->retval = " -->
      <xsl:value-of select="$receiverVariable" />

      <xsl:value-of select="' = '" />
      <xsl:call-template name="emitOutputArgBackConverter2">
        <xsl:with-param name="name" select="$name"/>
        <xsl:with-param name="varname" select="$varname"/>
        <xsl:with-param name="type" select="$type"/>
        <xsl:with-param name="callerprefix" select="$callerprefix"/>
      </xsl:call-template>
      <xsl:value-of select="';'" />
      <xsl:call-template name="emitNewline" />

    </xsl:otherwise>
  </xsl:choose>

  <xsl:value-of select="concat('        WEBDEBUG((g_pcszDoneConvertingComOutputBack, &quot;', $name, '&quot;));')" />
  <xsl:call-template name="emitNewline" />
</xsl:template>

<!--
    emitGetAttributeComCall
  -->
<xsl:template name="emitGetAttributeComCall">
  <xsl:param name="ifname" />
  <xsl:param name="object" />       <!-- normally "pObj->" -->
  <xsl:param name="attrname" />
  <xsl:param name="attrtype" />
  <xsl:param name="attrsafearray" />
  <xsl:param name="varprefix" />      <!-- only when called recursively from emitOutputArgBackConverter-->
  <xsl:param name="callerprefix" />   <!-- only when called recursively from emitOutputArgBackConverter-->

  <xsl:variable name="gettername"><xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
  <xsl:call-template name="emitOutputArgBuffer">
    <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
    <xsl:with-param name="method"><xsl:value-of select="$gettername" /></xsl:with-param>
    <xsl:with-param name="name" select="$attrname" />
    <xsl:with-param name="type" select="$attrtype" />
    <xsl:with-param name="safearray" select="$attrsafearray" />
    <xsl:with-param name="varprefix" select="$varprefix" />
  </xsl:call-template>
  <xsl:variable name="upperattrname"><xsl:call-template name="capitalize"><xsl:with-param name="str" select="$attrname" /></xsl:call-template></xsl:variable>
  <!-- actual COM method call -->
  <xsl:call-template name="emitComCall">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="methodname" select="concat('COMGETTER(', $upperattrname, ')')" />
    <xsl:with-param name="object" select="$object" />
    <xsl:with-param name="attrname" select="$attrname" />
    <xsl:with-param name="attrtype" select="$attrtype" />
    <xsl:with-param name="attrsafearray" select="$attrsafearray" />
    <xsl:with-param name="attrdir" select="'return'" />
    <xsl:with-param name="varprefix" select="$varprefix" />
  </xsl:call-template>
  <!-- convert back the output data -->
  <xsl:call-template name="emitOutputArgBackConverter">
    <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
    <xsl:with-param name="method"><xsl:value-of select="$gettername" /></xsl:with-param>
    <xsl:with-param name="name" select="$attrname" />
    <xsl:with-param name="type" select="$attrtype" />
    <xsl:with-param name="safearray" select="$attrsafearray" />
    <xsl:with-param name="varprefix" select="$varprefix" />
    <xsl:with-param name="callerprefix" select="$callerprefix" />
  </xsl:call-template>
</xsl:template>

<!--
    emitSetAttributeComCall
    -->
<xsl:template name="emitSetAttributeComCall">
  <xsl:param name="ifname" />
  <xsl:param name="object" />       <!-- normally "pObj->" -->
  <xsl:param name="attrname" />
  <xsl:param name="attrtype" />
  <xsl:param name="attrsafearray" />
  <xsl:param name="callerprefix" />   <!-- only when called recursively from emitOutputArgBackConverter-->

  <xsl:variable name="settername"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
  <xsl:variable name="upperattrname"><xsl:call-template name="capitalize"><xsl:with-param name="str" select="$attrname" /></xsl:call-template></xsl:variable>

  <xsl:call-template name="emitInputArgConverter">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="method" select="concat($ifname, '::', $settername)" />
    <xsl:with-param name="methodname" select="concat('COMSETTER(', $upperattrname, ')')" />
    <xsl:with-param name="object" select="$object" />
    <xsl:with-param name="name" select="$attrname" />
    <xsl:with-param name="structprefix" select="concat($G_requestElementVarName, '-&gt;')" />
    <xsl:with-param name="type" select="$attrtype" />
    <xsl:with-param name="safearray" select="$attrsafearray" />
  </xsl:call-template>
  <xsl:call-template name="emitComCall">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="methodname" select="concat('COMSETTER(', $upperattrname, ')')" />
    <xsl:with-param name="object" select="$object" />
    <xsl:with-param name="attrname" select="$attrname" />
    <xsl:with-param name="attrtype" select="$attrtype" />
    <xsl:with-param name="attrsafearray" select="$attrsafearray" />
    <xsl:with-param name="attrdir" select="'in'" />
  </xsl:call-template>
</xsl:template>

<!--
    emitGetAttributeMapper
  -->
<xsl:template name="emitGetAttributeMapper">
  <xsl:param name="ifname" />
  <xsl:param name="wsmap" />
  <xsl:param name="attrname" />
  <xsl:param name="attrtype" />
  <xsl:param name="attrreadonly" />
  <xsl:param name="attrsafearray" />

  <xsl:variable name="gettername"><xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>

  <xsl:call-template name="emitBeginOfFunctionHeader">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="method" select="$gettername" />
  </xsl:call-template>

  <xsl:call-template name="emitDocumentStyleArgStructs">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="methodname" select="$gettername" />
    <xsl:with-param name="fOutputs" select="$attrtype" />
  </xsl:call-template>

  <xsl:text>)</xsl:text>
  <xsl:call-template name="emitNewline" />
  <xsl:text>{</xsl:text>
  <xsl:call-template name="emitNewline" />

  <xsl:value-of select="'    HRESULT rc = S_OK;'" />
  <xsl:call-template name="emitNewline" />

  <xsl:call-template name="emitPrologue" />

  <!-- actual COM method call -->
  <!-- <xsl:choose>
    array attributes/parameters are not supported yet...
    <xsl:when test="@array or @safearray='yes'">
      <xsl:call-template name="warning"><xsl:with-param name="msg" select="concat('emitComCall: SKIPPING ATTRIBUTE IMPLEMENTATION for &quot;', $attrname, '&quot; because it has array type. THIS SOAP METHOD WILL NOT DO ANYTHING!')" /></xsl:call-template>
    </xsl:when>
    <xsl:otherwise>  -->
      <xsl:call-template name="emitObjForMethod">
        <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
        <xsl:with-param name="wsmap"><xsl:value-of select="$wsmap" /></xsl:with-param>
        <xsl:with-param name="structprefix" select="concat($G_requestElementVarName, '-&gt;')" />
      </xsl:call-template>

      <xsl:call-template name="emitGetAttributeComCall">
        <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
        <xsl:with-param name="object" select='"pObj"' />
        <xsl:with-param name="attrname"><xsl:value-of select="$attrname" /></xsl:with-param>
        <xsl:with-param name="attrtype"><xsl:value-of select="$attrtype" /></xsl:with-param>
        <xsl:with-param name="attrsafearray"><xsl:value-of select="$attrsafearray" /></xsl:with-param>
      </xsl:call-template>
     <!-- </xsl:otherwise>
  </xsl:choose> -->

  <xsl:call-template name="emitEpilogue" />
</xsl:template>

<!--
    emitSetAttributeMapper:
  -->
<xsl:template name="emitSetAttributeMapper">
  <xsl:param name="ifname" select="$ifname" />
  <xsl:param name="wsmap" select="$wsmap" />
  <xsl:param name="attrname" select="$attrname" />
  <xsl:param name="attrtype" select="$attrtype" />
  <xsl:param name="attrreadonly" select="$attrreadonly" />
  <xsl:param name="attrsafearray" select="$attrsafearray" />

  <xsl:variable name="settername"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>

  <xsl:call-template name="emitBeginOfFunctionHeader">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="method" select="$settername" />
  </xsl:call-template>

  <xsl:call-template name="emitDocumentStyleArgStructs">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="methodname" select="$settername" />
    <xsl:with-param name="fOutputs" select="1" />
  </xsl:call-template>

  <xsl:text>)</xsl:text>
  <xsl:call-template name="emitNewline" />
  <xsl:text>{</xsl:text>
  <xsl:call-template name="emitNewline" />
  <xsl:value-of select="'    HRESULT rc = S_OK;'" />
  <xsl:value-of select="concat(concat(' NOREF(', $G_responseElementVarName),');')" />
  <xsl:call-template name="emitNewline" />
  <xsl:call-template name="emitPrologue" />

  <!-- actual COM method call -->
  <!-- <xsl:choose>
    array attributes/parameters are not supported yet...
    <xsl:when test="@array or @safearray='yes'">
      <xsl:call-template name="warning"><xsl:with-param name="msg" select="concat('emitComCall: SKIPPING ATTRIBUTE IMPLEMENTATION for &quot;', $attrname, '&quot; because it has array type. THIS SOAP METHOD WILL NOT DO ANYTHING!')" /></xsl:call-template>
    </xsl:when>
    <xsl:otherwise> -->
      <xsl:call-template name="emitObjForMethod">
        <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
        <xsl:with-param name="wsmap"><xsl:value-of select="$wsmap" /></xsl:with-param>
        <xsl:with-param name="structprefix" select="concat($G_requestElementVarName, '-&gt;')" />
      </xsl:call-template>
      <xsl:call-template name="emitSetAttributeComCall">
        <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
        <xsl:with-param name="object" select='"pObj"' />
        <xsl:with-param name="attrname"><xsl:value-of select="$attrname" /></xsl:with-param>
        <xsl:with-param name="attrtype"><xsl:value-of select="$attrtype" /></xsl:with-param>
        <xsl:with-param name="attrsafearray"><xsl:value-of select="$attrsafearray" /></xsl:with-param>
      </xsl:call-template>
    <!-- </xsl:otherwise>
  </xsl:choose> -->

  <xsl:call-template name="emitEpilogue" />
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  interface
  - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="interface">
  <!-- remember the interface name in local variables -->
  <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
  <xsl:variable name="wsmap"><xsl:value-of select="@wsmap" /></xsl:variable>
  <xsl:variable name="wscpp"><xsl:value-of select="@wscpp" /></xsl:variable>

  <!-- we can save ourselves verifying the interface here as it's already
       done in the WSDL converter -->

  <xsl:if test='not( ($wsmap="suppress") or ($wsmap="struct") or ($wscpp="hardcoded") )'>
    <xsl:text>
/****************************************************************************
 *
 * interface </xsl:text>
<xsl:copy-of select="$ifname" />
<xsl:text>
 *
 ****************************************************************************/</xsl:text>
    <xsl:call-template name="xsltprocNewlineOutputHack"/>

    <!--
      here come the attributes
    -->
    <xsl:for-each select="attribute">
      <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
      <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
      <xsl:variable name="attrreadonly"><xsl:value-of select="@readonly" /></xsl:variable>
      <xsl:variable name="attrsafearray"><xsl:value-of select="@safearray" /></xsl:variable>
      <xsl:call-template name="emitNewline" />
      <!-- skip this attribute if it has parameters of a type that has wsmap="suppress" -->
      <xsl:choose>
        <xsl:when test="( $attrtype=($G_setSuppressedInterfaces/@name) )">
          <xsl:value-of select="concat('// Skipping attribute ', $attrname, ' for it is of suppressed type ', $attrtype)" />
        </xsl:when>
        <xsl:when test="@wsmap = 'suppress'">
          <xsl:value-of select="concat('// Skipping attribute ', $attrname, ' for it is suppressed')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="@readonly='yes'">
              <xsl:value-of select="concat('// read-only attribute ', $ifname, '::', $attrname, ' of type ', $attrtype)" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat('// read/write attribute ', $ifname, '::', $attrname, ' of type ', $attrtype)" />
            </xsl:otherwise>
          </xsl:choose>
          <xsl:value-of select="concat(' (safearray: ', $attrsafearray, ')')" />
          <!-- emit getter method -->
          <xsl:call-template name="emitGetAttributeMapper">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="wsmap" select="$wsmap" />
            <xsl:with-param name="attrname" select="$attrname" />
            <xsl:with-param name="attrtype" select="$attrtype" />
            <xsl:with-param name="attrreadonly" select="$attrreadonly" />
            <xsl:with-param name="attrsafearray" select="$attrsafearray" />
          </xsl:call-template>
          <!-- for read-write attributes, emit setter method -->
          <xsl:if test="not(@readonly='yes')">
            <xsl:call-template name="emitSetAttributeMapper">
              <xsl:with-param name="ifname" select="$ifname" />
              <xsl:with-param name="wsmap" select="$wsmap" />
              <xsl:with-param name="attrname" select="$attrname" />
              <xsl:with-param name="attrtype" select="$attrtype" />
              <xsl:with-param name="attrreadonly" select="$attrreadonly" />
              <xsl:with-param name="attrsafearray" select="$attrsafearray" />
            </xsl:call-template>
          </xsl:if>
        </xsl:otherwise> <!-- not wsmap=suppress -->
      </xsl:choose>
    </xsl:for-each>

    <!--
      here come the real methods
    -->

    <xsl:for-each select="method">
      <xsl:variable name="methodname"><xsl:value-of select="@name" /></xsl:variable>
      <!-- method header: return value "int", method name, soap arguments -->
      <!-- skip this method if it has parameters of a type that has wsmap="suppress" -->
      <xsl:choose>
        <xsl:when test="   (param[@type=($G_setSuppressedInterfaces/@name)])
                        or (param[@mod='ptr'])" >
          <xsl:comment><xsl:value-of select="concat('Skipping method ', $methodname, ' for it has parameters with suppressed types')" /></xsl:comment>
        </xsl:when>
        <xsl:when test="@wsmap = 'suppress'">
          <xsl:comment><xsl:value-of select="concat('Skipping method ', $methodname, ' for it is suppressed')" /></xsl:comment>
        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="fHasReturnParms" select="param[@dir='return']" />
          <xsl:variable name="fHasOutParms" select="param[@dir='out']" />

          <xsl:call-template name="emitNewline" />
          <xsl:value-of select="concat('/* method ', $ifname, '::', $methodname, '(')" />
          <xsl:for-each select="param">
            <xsl:call-template name="emitNewline" />
            <xsl:value-of select="concat('        [', @dir, '] ', @type, ' ', @name)" />
              <xsl:if test="@safearray='yes'">
                <xsl:text>[]</xsl:text>
              </xsl:if>
            <xsl:if test="not(position()=last())">
              <xsl:text>,</xsl:text>
            </xsl:if>
          </xsl:for-each>
          <xsl:text>)</xsl:text>
          <xsl:call-template name="emitNewline" />
          <xsl:text> */</xsl:text>

          <xsl:call-template name="emitBeginOfFunctionHeader">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="method" select="$methodname" />
          </xsl:call-template>

          <xsl:call-template name="emitDocumentStyleArgStructs">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="methodname" select="$methodname" />
            <xsl:with-param name="fOutputs" select="1" />
          </xsl:call-template>
          <xsl:text>)</xsl:text>
          <xsl:call-template name="emitNewline" />
          <xsl:text>{</xsl:text>
          <xsl:call-template name="emitNewline" />
          <xsl:value-of select="'    HRESULT rc = S_OK;'" />
          <xsl:value-of select="concat(concat(' NOREF(', $G_responseElementVarName),');')" />
          <xsl:call-template name="emitNewline" />
          <xsl:call-template name="emitPrologue" />

          <xsl:choose>
             <xsl:when test="param[@array]">
              <xsl:call-template name="warning"><xsl:with-param name="msg" select="concat('emitComCall: SKIPPING METHOD IMPLEMENTATION for &quot;', $methodname, '&quot; because it has arguments with &quot;array&quot; types. THIS SOAP METHOD WILL NOT DO ANYTHING!')" /></xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <!-- emit the object upon which to invoke the method -->
              <xsl:call-template name="emitObjForMethod">
                <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
                <xsl:with-param name="wsmap"><xsl:value-of select="$wsmap" /></xsl:with-param>
                <xsl:with-param name="structprefix" select="concat($G_requestElementVarName, '-&gt;')" />
              </xsl:call-template>
              <!-- next, emit storage variables to convert the SOAP/C++ arguments to COM types -->
              <xsl:for-each select="param">
                <xsl:variable name="dir" select="@dir" />
                <xsl:choose>
                  <xsl:when test="$dir='in'">
                    <xsl:call-template name="emitInputArgConverter">
                      <xsl:with-param name="ifname" select="$ifname" />
                      <xsl:with-param name="method" select="concat($ifname, '::', $methodname)" />
                      <xsl:with-param name="methodname">
                        <xsl:call-template name="capitalize">
                          <xsl:with-param name="str" select="$methodname" />
                        </xsl:call-template>
                      </xsl:with-param>
                      <xsl:with-param name="object" select='"pObj"' />
                      <xsl:with-param name="structprefix" select="concat($G_requestElementVarName, '-&gt;')" />
                      <xsl:with-param name="name" select="@name" />
                      <xsl:with-param name="type" select="@type" />
                      <xsl:with-param name="safearray" select="@safearray" />
                    </xsl:call-template>
                  </xsl:when>
                  <xsl:when test="$dir='out'">
                    <xsl:call-template name="emitOutputArgBuffer">
                      <xsl:with-param name="method" select="concat($ifname, '::', $methodname)" />
                      <xsl:with-param name="name" select="@name" />
                      <xsl:with-param name="type" select="@type" />
                      <xsl:with-param name="safearray" select="@safearray" />
                    </xsl:call-template>
                  </xsl:when>
                  <xsl:when test="$dir='return'">
                    <xsl:call-template name="emitOutputArgBuffer">
                      <xsl:with-param name="method" select="concat($ifname, '::', $methodname)" />
                      <xsl:with-param name="name" select="$G_result" />
                      <xsl:with-param name="type" select="@type" />
                      <xsl:with-param name="safearray" select="@safearray" />
                    </xsl:call-template>
                  </xsl:when>
                </xsl:choose>
              </xsl:for-each>
              <!-- actual COM method call -->
              <xsl:call-template name="emitComCall">
                <xsl:with-param name="ifname" select="$ifname" />
                <xsl:with-param name="object" select='"pObj"' />
                <xsl:with-param name="methodname">
                  <xsl:call-template name="capitalize">
                    <xsl:with-param name="str" select="$methodname" />
                  </xsl:call-template>
                </xsl:with-param>
              </xsl:call-template>
              <!-- convert back the output data -->
              <xsl:for-each select="param">
                <xsl:variable name="dir" select="@dir" />
                <xsl:if test="$dir='out'">
                  <xsl:call-template name="emitOutputArgBackConverter">
                    <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
                    <xsl:with-param name="method" select="$methodname" />
                    <xsl:with-param name="name"><xsl:value-of select="@name" /></xsl:with-param>
                    <xsl:with-param name="type"><xsl:value-of select="@type" /></xsl:with-param>
                    <xsl:with-param name="safearray"><xsl:value-of select="@safearray" /></xsl:with-param>
                    <xsl:with-param name="callerprefix" select="'outparms.'"/>
                  </xsl:call-template>
                </xsl:if>
                <xsl:if test="$dir='return'">
                  <!-- return values _normally_ should convert to the input arg from the function prototype,
                      except when there are both return and out params; in that case gsoap squeezes them all
                      into the output args structure and the return thing is called "retval" -->
                  <xsl:choose>
                    <xsl:when test="$fHasOutParms">
                      <xsl:call-template name="emitOutputArgBackConverter">
                        <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
                        <xsl:with-param name="method" select="$methodname" />
                        <xsl:with-param name="name"><xsl:value-of select="$G_result" /></xsl:with-param>
                        <xsl:with-param name="type"><xsl:value-of select="@type" /></xsl:with-param>
                        <xsl:with-param name="safearray"><xsl:value-of select="@safearray" /></xsl:with-param>
                        <xsl:with-param name="callerprefix" select="'outparms.'"/>
                      </xsl:call-template>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:call-template name="emitOutputArgBackConverter">
                        <xsl:with-param name="ifname"><xsl:value-of select="$ifname" /></xsl:with-param>
                        <xsl:with-param name="method" select="$methodname" />
                        <xsl:with-param name="name"><xsl:value-of select="$G_result" /></xsl:with-param>
                        <xsl:with-param name="type"><xsl:value-of select="@type" /></xsl:with-param>
                        <xsl:with-param name="safearray"><xsl:value-of select="@safearray" /></xsl:with-param>
                      </xsl:call-template>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:if>
              </xsl:for-each>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:call-template name="emitEpilogue" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:if>

</xsl:template>


</xsl:stylesheet>
