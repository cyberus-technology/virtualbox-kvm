<xsl:stylesheet version = '1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:vbox="http://www.virtualbox.org/"
     xmlns:exsl="http://exslt.org/common"
     extension-element-prefixes="exsl">

<!--
    genjifaces.xsl:
        XSLT stylesheet that generates Java XPCOM bridge interface code from VirtualBox.xidl.
-->
<!--
    Copyright (C) 2010-2023 Oracle and/or its affiliates.

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

<xsl:output
  method="text"
  version="1.0"
  encoding="utf-8"
  indent="no"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_xsltFilename" select="'genjifaces.xsl'" />


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
- - - - - - - - - - - - - - - - - - - - - - -->
<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>

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

<xsl:template name="uppercase">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="translate($str, 'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')" />
</xsl:template>

<xsl:template name="capitalize">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="
        concat(translate(substring($str,1,1),'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ'),
               substring($str,2))"/>
</xsl:template>

<xsl:template name="makeGetterName">
  <xsl:param name="attrname" />
  <xsl:variable name="capsname">
    <xsl:call-template name="capitalize">
      <xsl:with-param name="str" select="$attrname" />
  </xsl:call-template>
  </xsl:variable>
  <xsl:value-of select="concat('get', $capsname)" />
</xsl:template>

<xsl:template name="makeSetterName">
  <xsl:param name="attrname" />
  <xsl:variable name="capsname">
    <xsl:call-template name="capitalize">
      <xsl:with-param name="str" select="$attrname" />
  </xsl:call-template>
  </xsl:variable>
  <xsl:value-of select="concat('set', $capsname)" />
</xsl:template>

<xsl:template name="fileheader">
  <xsl:param name="name" />
  <xsl:text>/** @file
</xsl:text>
  <xsl:value-of select="concat(' * ',$name)"/>
<xsl:text>
 *
 * DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
 * Generator: src/VBox/src/libs/xpcom18a4/java/tools/genjifaces.xsl
 */

/*
 * Copyright (C) 2010-2022 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see &lt;https://www.gnu.org/licenses&gt;.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
</xsl:text>
</xsl:template>

<xsl:template name="startFile">
  <xsl:param name="file" />

  <xsl:value-of select="concat('&#10;// ##### BEGINFILE &quot;', $file, '&quot;&#10;&#10;')" />
  <xsl:call-template name="fileheader">
    <xsl:with-param name="name" select="$file" />
  </xsl:call-template>

  <xsl:value-of select="       'package org.mozilla.interfaces;&#10;&#10;'" />
</xsl:template>

<xsl:template name="endFile">
 <xsl:param name="file" />
 <xsl:value-of select="concat('&#10;// ##### ENDFILE &quot;', $file, '&quot;&#10;')" />
 <xsl:call-template name="xsltprocNewlineOutputHack"/>
</xsl:template>


<xsl:template name="emitHandwritten">

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsISupports.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsISupports
{
  public static final String NS_ISUPPORTS_IID =
    "{00000000-0000-0000-c000-000000000046}";

  public nsISupports queryInterface(String arg1);
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsISupports.java'" />
 </xsl:call-template>

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsIComponentManager.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsIComponentManager extends nsISupports
{
  public static final String NS_ICOMPONENTMANAGER_IID =
    "{a88e5a60-205a-4bb1-94e1-2628daf51eae}";

  public nsISupports getClassObject(String arg1, String arg2);

  public nsISupports getClassObjectByContractID(String arg1, String arg2);

  public nsISupports createInstance(String arg1, nsISupports arg2, String arg3);

  public nsISupports createInstanceByContractID(String arg1, nsISupports arg2, String arg3);
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsIComponentManager.java'" />
 </xsl:call-template>

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsIServiceManager.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsIServiceManager extends nsISupports
{
  public static final String NS_ISERVICEMANAGER_IID =
    "{8bb35ed9-e332-462d-9155-4a002ab5c958}";

  public nsISupports getService(String arg1, String arg2);

  public nsISupports getServiceByContractID(String arg1, String arg2);

  public boolean isServiceInstantiated(String arg1, String arg2);

  public boolean isServiceInstantiatedByContractID(String arg1, String arg2);
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsIServiceManager.java'" />
 </xsl:call-template>

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsIExceptionManager.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsIExceptionManager extends nsISupports
{
  public static final String NS_IEXCEPTIONMANAGER_IID =
    "{efc9d00b-231c-4feb-852c-ac017266a415}";

  public nsIException getCurrentException();
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsISupports.java'" />
 </xsl:call-template>

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsIExceptionService.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsIExceptionService extends nsIExceptionManager
{
  public static final String NS_IEXCEPTIONSERVICE_IID =
    "{35a88f54-f267-4414-92a7-191f6454ab52}";

  public nsIExceptionManager getCurrentExceptionManager();
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsISupports.java'" />
 </xsl:call-template>

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsIException.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsIException extends nsISupports
{
  public static final String NS_IEXCEPTION_IID =
    "{f3a8d3b4-c424-4edc-8bf6-8974c983ba78}";

   // No methods - placeholder
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsISupports.java'" />
 </xsl:call-template>

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsIComponentRegistrar.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsIComponentRegistrar extends nsISupports
{
   public static final String NS_ICOMPONENTREGISTRAR_IID =
    "{2417cbfe-65ad-48a6-b4b6-eb84db174392}";

   // No methods - placeholder
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsIComponentRegistrar.java'" />
 </xsl:call-template>


 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsIFile.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsIFile extends nsISupports
{
  public static final String NS_IFILE_IID =
    "{c8c0a080-0868-11d3-915f-d9d889d48e3c}";

  // No methods - placeholder
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsIFile.java'" />
 </xsl:call-template>

 <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'nsILocalFile.java'" />
 </xsl:call-template>

 <xsl:text><![CDATA[
public interface nsILocalFile extends nsIFile
{
  public static final String NS_ILOCALFILE_IID =
    "{aa610f20-a889-11d3-8c81-000064657374}";

  // No methods - placeholder
}
]]></xsl:text>

 <xsl:call-template name="endFile">
   <xsl:with-param name="file" select="'nsILocalFile.java'" />
 </xsl:call-template>

</xsl:template>

<xsl:template name="genEnum">
  <xsl:param name="enumname" />
  <xsl:param name="filename" />

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="$filename" />
  </xsl:call-template>

  <xsl:value-of select="concat('public interface ', $enumname, ' {&#10;&#10;')" />

  <xsl:variable name="uppername">
    <xsl:call-template name="uppercase">
      <xsl:with-param name="str" select="$enumname" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:value-of select="concat('  public static final String ', $uppername, '_IID = &#10;',
                               '     &quot;{',@uuid, '}&quot;;&#10;&#10;')" />

  <xsl:for-each select="const">
    <xsl:variable name="enumconst" select="@name" />
    <xsl:value-of select="concat('  public static final long ', @name, ' = ', @value, 'L;&#10;&#10;')" />
  </xsl:for-each>

  <xsl:value-of select="'}&#10;&#10;'" />

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="$filename" />
  </xsl:call-template>

</xsl:template>

<xsl:template name="typeIdl2Back">
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="forceelem" />

  <xsl:variable name="needarray" select="($safearray='yes') and not($forceelem='yes')" />

  <xsl:choose>
    <xsl:when test="$type='unsigned long long'">
      <!-- stupid, rewrite the bridge -->
      <xsl:value-of select="'double'" />
    </xsl:when>

    <xsl:when test="$type='long long'">
      <xsl:value-of select="'long'" />
    </xsl:when>

    <xsl:when test="$type='unsigned long'">
      <xsl:value-of select="'long'" />
    </xsl:when>

    <xsl:when test="$type='long'">
      <xsl:value-of select="'int'" />
    </xsl:when>

    <xsl:when test="$type='unsigned short'">
      <xsl:value-of select="'int'" />
    </xsl:when>

    <xsl:when test="$type='short'">
      <xsl:value-of select="'short'" />
    </xsl:when>

    <xsl:when test="$type='octet'">
      <xsl:value-of select="'byte'" />
    </xsl:when>

    <xsl:when test="$type='boolean'">
      <xsl:value-of select="'boolean'" />
    </xsl:when>

    <xsl:when test="$type='$unknown'">
      <xsl:value-of select="'nsISupports'"/>
    </xsl:when>

    <xsl:when test="$type='wstring'">
      <xsl:value-of select="'String'" />
    </xsl:when>

    <xsl:when test="$type='uuid'">
      <xsl:value-of select="'String'" />
    </xsl:when>

    <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
      <xsl:value-of select="$type" />
    </xsl:when>

    <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
      <xsl:value-of select="'long'" />
    </xsl:when>

  </xsl:choose>

  <xsl:if test="$needarray">
    <xsl:value-of select="'[]'" />
  </xsl:if>

</xsl:template>

<xsl:template name="genIface">
  <xsl:param name="ifname" />
  <xsl:param name="filename" />

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="$filename" />
  </xsl:call-template>

  <xsl:variable name="extendsidl" select="key('G_keyInterfacesByName', $ifname)/@extends" />

  <xsl:variable name="extends">
    <xsl:choose>
      <xsl:when test="($extendsidl = '$unknown') or ($extendsidl = '$dispatched') or ($extendsidl = '$errorinfo')">
        <xsl:value-of select="'nsISupports'" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$extendsidl" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:value-of select="concat('public interface ', $ifname, ' extends ', $extends, ' {&#10;&#10;')" />

  <xsl:variable name="uppername">
    <xsl:call-template name="uppercase">
      <xsl:with-param name="str" select="$ifname" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:value-of select="concat('  public static final String ', $uppername, '_IID =&#10;',
                               '    &quot;{',@uuid, '}&quot;;&#10;&#10;')" />

  <xsl:for-each select="attribute">
    <xsl:variable name="attrname" select="@name" />
    <xsl:variable name="attrtype" select="@type" />

    <xsl:variable name="gettername">
      <xsl:call-template name="makeGetterName">
        <xsl:with-param name="attrname" select="$attrname" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="backtype">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="$attrtype" />
        <xsl:with-param name="safearray" select="@safearray" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="callparam">
      <xsl:if test="@safearray='yes'">
        <xsl:value-of select="concat('  long[] ',  @name, 'Size')" />
      </xsl:if>
    </xsl:variable>

    <xsl:value-of select="concat('  public ', $backtype, ' ', $gettername, '(',$callparam,');&#10;&#10;')" />

    <xsl:if test="not(@readonly='yes')">
      <xsl:variable name="settername">
        <xsl:call-template name="makeSetterName">
          <xsl:with-param name="attrname" select="$attrname" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat('  public void ',  $settername, '(', $backtype, ' arg1);&#10;&#10;')" />
    </xsl:if>

  </xsl:for-each>

  <xsl:for-each select="method">
    <xsl:variable name="methodname" select="@name" />
    <xsl:variable name="returnidltype" select="param[@dir='return']/@type" />
    <xsl:variable name="returnidlsafearray" select="param[@dir='return']/@safearray" />

    <xsl:variable name="returntype">
      <xsl:choose>
        <xsl:when test="$returnidltype">
          <xsl:call-template name="typeIdl2Back">
            <xsl:with-param name="type" select="$returnidltype" />
            <xsl:with-param name="safearray" select="$returnidlsafearray" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>void</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:value-of select="concat('  public ', $returntype, ' ', $methodname, '(')" />
    <xsl:for-each select="param">
      <xsl:variable name="paramtype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="@type" />
          <xsl:with-param name="safearray" select="@safearray" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="(@safearray='yes') and (@dir='return')">
          <xsl:value-of select="concat('long[] ', @name)" />
        </xsl:when>
        <xsl:when test="(@safearray='yes') and (@dir='out')">
          <xsl:value-of select="concat('long[] ', @name, 'Size, ', $paramtype, '[] ', @name)" />
        </xsl:when>
        <xsl:when test="(@safearray='yes') and (@dir='in') and (@type='octet')">
          <xsl:value-of select="concat($paramtype, ' ', @name)" />
        </xsl:when>
        <xsl:when test="(@safearray='yes') and (@dir='in')">
          <xsl:value-of select="concat('long ', @name, 'Size, ', $paramtype, ' ', @name)" />
        </xsl:when>
        <xsl:when test="@dir='out'">
          <xsl:value-of select="concat($paramtype, '[] ', @name)" />
        </xsl:when>
         <xsl:when test="@dir='in'">
           <xsl:value-of select="concat($paramtype, ' ', @name)" />
         </xsl:when>
      </xsl:choose>
      <xsl:if test="not(position()=last()) and not(following-sibling::param[1]/@dir='return' and not(following-sibling::param[1]/@safearray='yes'))">
        <xsl:value-of select="', '" />
      </xsl:if>
    </xsl:for-each>
    <xsl:value-of select="       ');&#10;&#10;'" />

  </xsl:for-each>

  <xsl:value-of select="'}&#10;&#10;'" />

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="$filename" />
  </xsl:call-template>

</xsl:template>


<xsl:template match="/">

  <!-- Handwritten files -->
  <xsl:call-template name="emitHandwritten"/>

   <!-- Enums -->
  <xsl:for-each select="//enum">
    <xsl:call-template name="genEnum">
      <xsl:with-param name="enumname" select="@name" />
      <xsl:with-param name="filename" select="concat(@name, '.java')" />
    </xsl:call-template>
  </xsl:for-each>

  <!-- Interfaces -->
  <xsl:for-each select="//interface">
    <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>
    <xsl:variable name="module" select="current()/ancestor::module/@name"/>

    <!-- We don't need WSDL-specific nor MIDL-specific interfaces here -->
    <xsl:if test="not($self_target='wsdl') and not($self_target='midl') and not($module)">
      <xsl:call-template name="genIface">
        <xsl:with-param name="ifname" select="@name" />
        <xsl:with-param name="filename" select="concat(@name, '.java')" />
      </xsl:call-template>
    </xsl:if>

  </xsl:for-each>

</xsl:template>

</xsl:stylesheet>
