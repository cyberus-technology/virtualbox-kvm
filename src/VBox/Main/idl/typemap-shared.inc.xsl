<!--
    typemap-shared.inc.xsl:
    this gets included from other XSLT stylesheets including those
    for the webservice, so we can share some definitions that must
    be the same for all of them (like method prefixes/suffices).
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
  targetNamespace="http://schemas.xmlsoap.org/wsdl/"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"
  xmlns:vbox="http://www.virtualbox.org/">

<xsl:variable name="G_xsltIncludeFilename" select="'typemap-shared.inc.xsl'" />

<xsl:variable name="G_lowerCase" select="'abcdefghijklmnopqrstuvwxyz'" />
<xsl:variable name="G_upperCase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />
<xsl:variable name="G_sNewLine">
  <xsl:text>
</xsl:text>
</xsl:variable>

<!-- List of white space characters that the strip functions will remove -->
<xsl:variable name="G_sWhiteSpace" select="' &#10;&#13;&#09;'"/>

<!-- target namespace; this must match the xmlns:vbox in stylesheet opening tags! -->
<xsl:variable name="G_targetNamespace"
              select='"http://www.virtualbox.org/"' />
<xsl:variable name="G_targetNamespaceSeparator"
              select='""' />

<!-- ENCODING SCHEME

    See: http://www-128.ibm.com/developerworks/webservices/library/ws-whichwsdl/

    Essentially "document" style means that each SOAP message is a complete and
    self-explanatory document that does not rely on outside information for
    validation.

    By contrast, the (older) "RPC" style allows for much shorter SOAP messages
    that do not contain validation info like all types that are used, but then
    again, caller and receiver must have agreed on a valid format in some other way.
    With RPC, WSDL typically looks like this:

            <message name="myMethodRequest">
                <part name="x" type="xsd:int"/>
                <part name="y" type="xsd:float"/>
            </message>

    This is why today "document" style is preferred. However, with document style,
    one _cannot_ use "type" in <part> elements. Instead, one must use "element"
    attributes that refer to <element> items in the type section. Like this:

        <types>
            <schema>
                <element name="xElement" type="xsd:int"/>
                <element name="yElement" type="xsd:float"/>
            </schema>
        </types>

        <message name="myMethodRequest">
            <part name="x" element="xElement"/>
            <part name="y" element="yElement"/>
        </message>

    The "encoded" and "literal" sub-styles then only determine whether the
    individual types in the soap messages carry additional information in
    attributes. "Encoded" was only used with RPC styles, really, and even that
    is not widely supported any more.

-->
<!-- These are the settings: all the other XSLTs react on this and are supposed
     to be able to generate both valid RPC and document-style code. The only
     allowed values are 'rpc' or 'document'. -->
<xsl:variable name="G_basefmt"
              select='"document"' />
<xsl:variable name="G_parmfmt"
              select='"literal"' />
<!-- <xsl:variable name="G_basefmt"
              select='"rpc"' />
<xsl:variable name="G_parmfmt"
              select='"encoded"' /> -->

<!-- with document style, this is how we name the request and return element structures -->
<xsl:variable name="G_requestElementVarName"
              select='"req"' />
<xsl:variable name="G_responseElementVarName"
              select='"resp"' />
<!-- this is how we name the result parameter in messages -->
<xsl:variable name="G_result"
              select='"returnval"' />

<!-- we represent interface attributes by creating "get" and "set" methods; these
     are the prefixes we use for that -->
<xsl:variable name="G_attributeGetPrefix"
              select='"get"' />
<xsl:variable name="G_attributeSetPrefix"
              select='"set"' />
<!-- separator between class name and method/attribute name; would be "::" in C++
     but i'm unsure whether WSDL appreciates that (WSDL only) -->
<xsl:variable name="G_classSeparator"
              select='"_"' />
<!-- for each interface method, we need to create both a "request" and a "response"
     message; these are the suffixes we append to the method names for that -->
<xsl:variable name="G_methodRequest"
              select='"RequestMsg"' />
<xsl:variable name="G_methodResponse"
              select='"ResultMsg"' />
<!-- suffix for element declarations that describe request message parameters (WSDL only) -->
<xsl:variable name="G_requestMessageElementSuffix"
              select='""' />
<!-- suffix for element declarations that describe request message parameters (WSDL only) -->
<xsl:variable name="G_responseMessageElementSuffix"
              select='"Response"' />
<!-- suffix for portType names (WSDL only) -->
<xsl:variable name="G_portTypeSuffix"
              select='"PortType"' />
<!-- suffix for binding names (WSDL only) -->
<xsl:variable name="G_bindingSuffix"
              select='"Binding"' />
<!-- schema type to use for object references; while it is theoretically
     possible to use a self-defined type (e.g. some vboxObjRef type that's
     really an int), gSOAP gets a bit nasty and creates complicated structs
     for function parameters when these types are used as output parameters.
     So we just use "int" even though it's not as lucid.
     One setting is for the WSDL emitter, one for the C++ emitter -->
<!--
<xsl:variable name="G_typeObjectRef"
              select='"xsd:unsignedLong"' />
<xsl:variable name="G_typeObjectRef_gsoapH"
              select='"ULONG64"' />
<xsl:variable name="G_typeObjectRef_CPP"
              select='"WSDLT_ID"' />
-->
<xsl:variable name="G_typeObjectRef"
              select='"xsd:string"' />
<xsl:variable name="G_typeObjectRef_gsoapH"
              select='"std::string"' />
<xsl:variable name="G_typeObjectRef_CPP"
              select='"std::string"' />
<!-- and what to call first the object parameter -->
<xsl:variable name="G_nameObjectRef"
              select='"_this"' />
<!-- gSOAP encodes underscores with USCORE so this is used in our C++ code -->
<xsl:variable name="G_nameObjectRefEncoded"
              select='"_USCOREthis"' />

<!-- type to represent enums within C++ COM callers -->
<xsl:variable name="G_funcPrefixInputEnumConverter"
              select='"EnumSoap2Com_"' />
<xsl:variable name="G_funcPrefixOutputEnumConverter"
              select='"EnumCom2Soap_"' />

<!-- type to represent structs within C++ COM callers -->
<xsl:variable name="G_funcPrefixOutputStructConverter"
              select='"StructCom2Soap_"' />

<xsl:variable name="G_aSharedTypes">
  <type idlname="octet"              xmlname="unsignedByte"  cname="unsigned char"    gluename="BYTE"    gluefmt="%RU8"      javaname="byte"       dtracename="uint8_t"     />
  <type idlname="boolean"            xmlname="boolean"       cname="bool"             gluename="BOOL"    gluefmt="%RTbool"   javaname="Boolean"    dtracename="int8_t"      />
  <type idlname="short"              xmlname="short"         cname="short"            gluename="SHORT"   gluefmt="%RI16"     javaname="Short"      dtracename="int16_t"     />
  <type idlname="unsigned short"     xmlname="unsignedShort" cname="unsigned short"   gluename="USHORT"  gluefmt="%RU16"     javaname="Integer"    dtracename="uint16_t"    />
  <type idlname="long"               xmlname="int"           cname="int"              gluename="LONG"    gluefmt="%RI32"     javaname="Integer"    dtracename="int32_t"     />
  <type idlname="unsigned long"      xmlname="unsignedInt"   cname="unsigned int"     gluename="ULONG"   gluefmt="%RU32"     javaname="Long"       dtracename="uint32_t"    />
  <type idlname="long long"          xmlname="long"          cname="LONG64"           gluename="LONG64"  gluefmt="%RI64"     javaname="Long"       dtracename="int64_t"     />
  <type idlname="unsigned long long" xmlname="unsignedLong"  cname="ULONG64"          gluename="ULONG64" gluefmt="%RU64"     javaname="BigInteger" dtracename="uint64_t"    />
  <type idlname="double"             xmlname="double"        cname="double"           gluename="DOUBLE"  gluefmt="%#RX64"    javaname="Double"     dtracename="double"      />
  <type idlname="float"              xmlname="float"         cname="float"            gluename="FLOAT"   gluefmt="%#RX32"    javaname="Float"      dtracename="float"       />
  <type idlname="wstring"            xmlname="string"        cname="std::string"      gluename="BSTR"    gluefmt="%ls"       javaname="String"     dtracename="const char *"/>
  <type idlname="uuid"               xmlname="string"        cname="std::string"      gluename="BSTR"    gluefmt="%ls"       javaname="String"     dtracename="const char *"/>
  <type idlname="result"             xmlname="unsignedInt"   cname="unsigned int"     gluename="HRESULT" gluefmt="%Rhrc"     javaname="Long"       dtracename="int32_t"     />
</xsl:variable>

<!--
    warning:
  -->

<xsl:template name="warning">
  <xsl:param name="msg" />

  <xsl:message terminate="no">
    <xsl:value-of select="concat('[', $G_xsltFilename, '] Warning in ', $msg)" />
  </xsl:message>
</xsl:template>

<!--
    fatalError:
  -->

<xsl:template name="fatalError">
  <xsl:param name="msg" />

  <xsl:message terminate="yes">
    <xsl:value-of select="concat('[', $G_xsltFilename, '] Error in ', $msg)" />
  </xsl:message>
</xsl:template>

<!--
    debugMsg
    -->

<xsl:template name="debugMsg">
  <xsl:param name="msg" />

  <xsl:if test="$G_argDebug">
    <xsl:message terminate="no">
      <xsl:value-of select="concat('[', $G_xsltFilename, '] ', $msg)" />
    </xsl:message>
  </xsl:if>
</xsl:template>

<!--
    uncapitalize
    -->

<xsl:template name="uncapitalize">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="
        concat(
            translate(substring($str,1,1),$G_upperCase,$G_lowerCase),
            substring($str,2)
        )
  "/>
</xsl:template>
<!--
    uncapitalize in the way JAX-WS understands, see #2910
    -->

<xsl:template name="uncapitalize2">
  <xsl:param name="str" select="."/>
  <xsl:variable name="strlen">
    <xsl:value-of select="string-length($str)"/>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="$strlen>1">
     <xsl:choose>
       <xsl:when test="contains($G_upperCase,substring($str,1,1))
                       and
                       contains($G_upperCase,substring($str,2,1))">
         <xsl:variable name="cdr">
           <xsl:call-template name="uncapitalize2">
             <xsl:with-param name="str" select="substring($str,2)"/>
           </xsl:call-template>
         </xsl:variable>
         <xsl:value-of select="
           concat(
            translate(substring($str,1,1),
                      $G_upperCase,
                      $G_lowerCase),
            $cdr
           )
           "/>
         </xsl:when>
         <xsl:otherwise>
           <!--<xsl:value-of select="concat(substring($str,1,1),$cdr)"/>-->
           <xsl:value-of select="$str"/>
         </xsl:otherwise>
     </xsl:choose>
    </xsl:when>
    <xsl:when test="$strlen=1">
      <xsl:value-of select="
                            translate($str,
                            $G_upperCase,
                            $G_lowerCase)
                            "/>
    </xsl:when>
    <xsl:otherwise>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!--
    capitalize
    -->

<xsl:template name="capitalize">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="
        concat(
            translate(substring($str,1,1),$G_lowerCase,$G_upperCase),
            substring($str,2)
        )
  "/>
</xsl:template>

<!--
    makeGetterName:
    -->
<xsl:template name="makeGetterName">
  <xsl:param name="attrname" />
  <xsl:variable name="capsname"><xsl:call-template name="capitalize"><xsl:with-param name="str" select="$attrname" /></xsl:call-template></xsl:variable>
  <xsl:value-of select="concat($G_attributeGetPrefix, $capsname)" />
</xsl:template>

<!--
    makeSetterName:
    -->
<xsl:template name="makeSetterName">
  <xsl:param name="attrname" />
  <xsl:variable name="capsname"><xsl:call-template name="capitalize"><xsl:with-param name="str" select="$attrname" /></xsl:call-template></xsl:variable>
  <xsl:value-of select="concat($G_attributeSetPrefix, $capsname)" />
</xsl:template>

<!--
    makeJaxwsMethod: compose idevInterfaceMethod out of IDEVInterface::method
    -->
<xsl:template name="makeJaxwsMethod">
  <xsl:param name="ifname" />
  <xsl:param name="methodname" />
  <xsl:variable name="uncapsif"><xsl:call-template name="uncapitalize2"><xsl:with-param name="str" select="$ifname" /></xsl:call-template></xsl:variable>
  <xsl:variable name="capsmethod"><xsl:call-template name="capitalize"><xsl:with-param name="str" select="$methodname" /></xsl:call-template></xsl:variable>
  <xsl:value-of select="concat($uncapsif, $capsmethod)" />
</xsl:template>


<!--
    makeJaxwsMethod2: compose iInterfaceMethod out of IInterface::method
    -->
<xsl:template name="makeJaxwsMethod2">
  <xsl:param name="ifname" />
  <xsl:param name="methodname" />
  <xsl:variable name="uncapsif"><xsl:call-template name="uncapitalize"><xsl:with-param name="str" select="$ifname" /></xsl:call-template></xsl:variable>
  <xsl:variable name="capsmethod"><xsl:call-template name="capitalize"><xsl:with-param name="str" select="$methodname" /></xsl:call-template></xsl:variable>
  <xsl:value-of select="concat($uncapsif, $capsmethod)" />
</xsl:template>

<!--
    emitNewline:
    -->
<xsl:template name="emitNewline">
  <xsl:text>
</xsl:text>
</xsl:template>

<!--
    emitNewlineIndent8:
    -->
<xsl:template name="emitNewlineIndent8">
  <xsl:text>
        </xsl:text>
</xsl:template>

<!--
    escapeUnderscores
    -->
<xsl:template name="escapeUnderscores">
  <xsl:param name="string" />
  <xsl:if test="contains($string, '_')">
    <xsl:value-of select="substring-before($string, '_')" />_USCORE<xsl:call-template name="escapeUnderscores"><xsl:with-param name="string"><xsl:value-of select="substring-after($string, '_')" /></xsl:with-param></xsl:call-template>
  </xsl:if>
  <xsl:if test="not(contains($string, '_'))"><xsl:value-of select="$string" />
  </xsl:if>
</xsl:template>

<!--
     xsltprocNewlineOutputHack - emits a single new line.

     Hack Alert! This template helps xsltproc split up the output text elements
                 and avoid reallocating them into the MB range. Calls to this
                 template is made occationally while generating larger output
                 file.  It's not necessary for small stuff like header.

                 The trick we're playing on xsltproc has to do with CDATA
                 and/or the escape setting of the xsl:text element.  It forces
                 xsltproc to allocate a new output element, thus preventing
                 things from growing out of proportions and slowing us down.

                 This was successfully employed to reduce a 18+ seconds run to
                 around one second (possibly less due to kmk overhead).
 -->
<xsl:template name="xsltprocNewlineOutputHack">
    <xsl:text disable-output-escaping="yes"><![CDATA[
]]></xsl:text>
</xsl:template>


<!--
    string-to-upper - translates the string to uppercase.
    -->
<xsl:template name="string-to-upper">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="translate($str, $G_lowerCase, $G_upperCase)"/>
</xsl:template>

<!--
    string-to-lower - translates the string to lowercase.
    -->
<xsl:template name="string-to-lower">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="translate($str, $G_upperCase, $G_lowerCase)"/>
</xsl:template>

<!--
    string-replace - Replace all occurencees of needle in haystack.
    -->
<xsl:template name="string-replace">
  <xsl:param name="haystack"/>
  <xsl:param name="needle"/>
  <xsl:param name="replacement"/>
  <xsl:param name="onlyfirst" select="false"/>
  <xsl:choose>
    <xsl:when test="contains($haystack, $needle)">
      <xsl:value-of select="substring-before($haystack, $needle)"/>
      <xsl:value-of select="$replacement"/>
      <xsl:call-template name="string-replace">
        <xsl:with-param name="haystack" select="substring-after($haystack, $needle)"/>
        <xsl:with-param name="needle" select="$needle"/>
        <xsl:with-param name="replacement" select="$replacement"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$haystack"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
    string-replace-first - Replace the _first_ occurence of needle in haystack.
    -->
<xsl:template name="string-replace-first">
  <xsl:param name="haystack"/>
  <xsl:param name="needle"/>
  <xsl:param name="replacement"/>
  <xsl:choose>
    <xsl:when test="contains($haystack, $needle)">
      <xsl:value-of select="substring-before($haystack, $needle)"/>
      <xsl:value-of select="$replacement"/>
      <xsl:value-of select="substring-after($haystack, $needle)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$haystack"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
    strip-string-right - String trailing white space from a string.
    -->
<xsl:template name="strip-string-right">
  <xsl:param name="text"/>

  <!-- Check for trailing whitespace. -->
  <xsl:choose>
    <xsl:when test="contains($G_sWhiteSpace, substring($text, string-length($text), 1))">
      <xsl:call-template name="strip-string-right">
        <xsl:with-param name="text" select="substring($text, 1, string-length($text) - 1)"/>
      </xsl:call-template>
    </xsl:when>

    <!-- No trailing white space. Return the string. -->
    <xsl:otherwise>
      <xsl:value-of select="$text"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
    strip-string-left - String leading white space from a string.
    -->
<xsl:template name="strip-string-left">
  <xsl:param name="text"/>

  <!-- Check for leading white space.  To optimize for speed, we check a couple
       of longer space sequences first. -->
  <xsl:choose>
    <xsl:when test="starts-with($text, '        ')">  <!-- 8 leading spaces -->
      <xsl:call-template name="strip-string-left">
        <xsl:with-param name="text" select="substring($text, 9)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="starts-with($text, '    ')">      <!-- 4 leading spaces -->
      <xsl:call-template name="strip-string-left">
        <xsl:with-param name="text" select="substring($text, 5)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="starts-with($text, '  ')">        <!-- 2 leading spaces -->
      <xsl:call-template name="strip-string-left">
        <xsl:with-param name="text" select="substring($text, 3)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="contains($G_sWhiteSpace, substring($text, 1, 1))">
      <xsl:if test="string-length($text) > 0">
        <xsl:call-template name="strip-string">
          <xsl:with-param name="text" select="substring($text, 2)"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:when>

    <!-- No leading white space. Return the string. -->
    <xsl:otherwise>
      <xsl:value-of select="$text"/>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<!--
    strip-string - String leading and trailing white space from a string.
    -->
<xsl:template name="strip-string">
  <xsl:param name="text"/>

  <!-- Check for leading white space.  To optimize for speed, we check a couple
       of longer space sequences first. -->
  <xsl:choose>
    <xsl:when test="starts-with($text, '        ')">  <!-- 8 leading spaces -->
      <xsl:call-template name="strip-string">
        <xsl:with-param name="text" select="substring($text, 9)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="starts-with($text, '    ')">      <!-- 4 leading spaces -->
      <xsl:call-template name="strip-string">
        <xsl:with-param name="text" select="substring($text, 5)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="starts-with($text, '  ')">        <!-- 2 leading spaces -->
      <xsl:call-template name="strip-string">
        <xsl:with-param name="text" select="substring($text, 3)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="contains($G_sWhiteSpace, substring($text, 1, 1))">
      <xsl:if test="string-length($text) > 0">
        <xsl:call-template name="strip-string">
          <xsl:with-param name="text" select="substring($text, 2)"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:when>

    <!-- Then check for trailing whitespace. -->
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="contains($G_sWhiteSpace, substring($text, string-length($text), 1))">
          <xsl:call-template name="strip-string-right">
            <xsl:with-param name="text" select="substring($text, 1, string-length($text) - 1)"/>
          </xsl:call-template>
        </xsl:when>

        <!-- No leading or trailing white space. Return the string. -->
        <xsl:otherwise>
          <xsl:value-of select="$text"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

</xsl:stylesheet>
