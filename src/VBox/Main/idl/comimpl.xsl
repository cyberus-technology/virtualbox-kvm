<xsl:stylesheet version = '1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:vbox="http://www.virtualbox.org/"
     xmlns:exsl="http://exslt.org/common"
     extension-element-prefixes="exsl">

<!--
    comimpl.xsl:
        XSLT stylesheet that generates COM C++ classes implementing
        interfaces described in VirtualBox.xidl.
        For now we generate implementation for events, as they are
        rather trivial container classes for their read-only attributes.
        Further extension to other interfaces is possible and anticipated.
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

<xsl:include href="typemap-shared.inc.xsl" />

<!-- $G_kind contains what kind of COM class implementation we generate -->
<xsl:variable name="G_xsltFilename" select="'autogen.xsl'" />
<xsl:variable name="G_generateBstrVariants" select="'no'" />


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template name="fileheader">
  <xsl:param name="name" />
  <xsl:text>/** @file </xsl:text>
  <xsl:value-of select="$name"/>
  <xsl:text>
 * DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
 * Generator:      src/VBox/Main/idl/comimpl.xsl
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

<xsl:template name="genComEntry">
  <xsl:param name="name" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:value-of select="concat('        COM_INTERFACE_ENTRY(', $name, ')&#10;')" />
  <xsl:choose>
    <xsl:when test="$extends='$unknown'">
      <!-- Reached base -->
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genComEntry">
        <xsl:with-param name="name" select="$extends" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $extends)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="typeIdl2Back">
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="param" />
  <xsl:param name="dir" />
  <xsl:param name="mod" />
  <xsl:param name="utf8str" select="'no'" />

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemtype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="$type" />
          <xsl:with-param name="safearray" select="''" />
          <xsl:with-param name="dir" select="'in'" />
          <xsl:with-param name="utf8str" select="$utf8str" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="$param and ($dir='in')">
          <xsl:value-of select="concat('ComSafeArrayIn(',$elemtype,',', $param,')')"/>
        </xsl:when>
        <xsl:when test="$param and ($dir='out')">
          <xsl:value-of select="concat('ComSafeArrayOut(',$elemtype,', ', $param, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('com::SafeArray&lt;',$elemtype,'&gt;')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$mod='ptr'">
          <xsl:value-of select="'BYTE *'" />
        </xsl:when>
        <xsl:when test="(($type='wstring') or ($type='uuid'))">
          <xsl:choose>
            <xsl:when test="$param and ($dir='in') and ($utf8str!='yes')">
              <xsl:value-of select="'CBSTR'"/>
            </xsl:when>
            <xsl:when test="$param and ($dir='in') and ($utf8str='yes')">
              <xsl:value-of select="'const Utf8Str &amp;'"/>
            </xsl:when>
            <xsl:when test="$param and ($dir='out')">
              <xsl:value-of select="'BSTR'"/>
            </xsl:when>
            <xsl:when test="$param and ($dir='out') and ($utf8str='yes')">
              <xsl:value-of select="'Utf8Str &amp;'"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="'Utf8Str'"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
          <xsl:value-of select="concat($type,'_T')"/>
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <xsl:choose>
            <xsl:when test="$param">
              <xsl:value-of select="concat($type,' *')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat('ComPtr&lt;',$type,'&gt;')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$type='boolean'">
          <xsl:value-of select="'BOOL'" />
        </xsl:when>
         <xsl:when test="$type='octet'">
          <xsl:value-of select="'BYTE'" />
        </xsl:when>
        <xsl:when test="$type='unsigned short'">
          <xsl:value-of select="'USHORT'" />
        </xsl:when>
        <xsl:when test="$type='short'">
          <xsl:value-of select="'SHORT'" />
        </xsl:when>
        <xsl:when test="$type='unsigned long'">
          <xsl:value-of select="'ULONG'" />
        </xsl:when>
        <xsl:when test="$type='long'">
          <xsl:value-of select="'LONG'" />
        </xsl:when>
        <xsl:when test="$type='unsigned long long'">
          <xsl:value-of select="'ULONG64'" />
        </xsl:when>
        <xsl:when test="$type='long long'">
          <xsl:value-of select="'LONG64'" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('Unhandled type: ', $type)" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="$dir='out'">
        <xsl:value-of select="' *'"/>
      </xsl:if>
      <xsl:if test="$param and not($param='_')">
        <xsl:value-of select="concat(' ', $param)"/>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<!-- Checks if interface $name has any string attributes, producing '1' for each string attrib.
     No output if no string attributes -->
<xsl:template name="hasStringAttributes">
  <xsl:param name="name" />

  <!-- Recurse into parent interfaces: -->
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="hasStringAttributes">
        <xsl:with-param name="name" select="$extends" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <!-- Find immediate string and uuid attributes and output '1' for each one: -->
  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute[(@type = 'wstring' or @type = 'uuid') and (@name != 'midlDoesNotLikeEmptyInterfaces')]">
    <xsl:text>1</xsl:text>
  </xsl:for-each>
</xsl:template>


<xsl:template name="genSetParam">
  <xsl:param name="member"/>
  <xsl:param name="param"/>
  <xsl:param name="type"/>
  <xsl:param name="safearray"/>
  <xsl:param name="internal"/>

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemtype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="$type" />
          <xsl:with-param name="safearray" select="''" />
          <xsl:with-param name="dir" select="'in'" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:text>        SafeArray&lt;</xsl:text><xsl:value-of select="$elemtype"/>
      <xsl:text>&gt; aArr(ComSafeArrayInArg(</xsl:text><xsl:value-of select="$param"/><xsl:text>));&#10;</xsl:text>
      <xsl:text>        return </xsl:text><xsl:value-of select="$member"/><xsl:text>.initFrom(aArr);&#10;</xsl:text>
    </xsl:when>
    <xsl:when test="($type='wstring') or ($type='uuid')">
      <xsl:text>        return </xsl:text><xsl:value-of select="$member"/><xsl:text>.assignEx(</xsl:text>
      <xsl:value-of select="$param"/><xsl:text>);&#10;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat('         ', $member, ' = ', $param, ';&#10;')"/>
      <xsl:if test="$internal!='yes'">
        <xsl:text>        return S_OK;&#10;</xsl:text>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genRetParam">
  <xsl:param name="member"/>
  <xsl:param name="param"/>
  <xsl:param name="type"/>
  <xsl:param name="safearray"/>
  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemtype">
        <xsl:call-template name="typeIdl2Back">
             <xsl:with-param name="type" select="$type" />
             <xsl:with-param name="safearray" select="''" />
             <xsl:with-param name="dir" select="'in'" />
        </xsl:call-template>
      </xsl:variable>
<!-- @todo String arrays probably needs work, I doubt we're generating sensible code for them at the moment. -->
      <xsl:text>        SafeArray&lt;</xsl:text><xsl:value-of select="$elemtype"/><xsl:text>&gt; result;&#10;</xsl:text>
      <xsl:text>        HRESULT hrc = </xsl:text><xsl:value-of select="$member"/><xsl:text>.cloneTo(result);&#10;</xsl:text>
      <xsl:text>        if (SUCCEEDED(hrc))&#10;</xsl:text>
      <xsl:text>            result.detachTo(ComSafeArrayOutArg(</xsl:text><xsl:value-of select="$param"/><xsl:text>));&#10;</xsl:text>
      <xsl:text>        return hrc;&#10;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="($type='wstring') or ($type = 'uuid')">
          <xsl:text>        return </xsl:text><xsl:value-of select="$member"/><xsl:text>.cloneToEx(</xsl:text>
          <xsl:value-of select="$param"/><xsl:text>);&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <xsl:text>        return </xsl:text><xsl:value-of select="$member"/><xsl:text>.queryInterfaceTo(</xsl:text>
          <xsl:value-of select="$param"/><xsl:text>);&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>        *</xsl:text><xsl:value-of select="$param"/><xsl:text> = </xsl:text>
          <xsl:value-of select="$member"/><xsl:text>;&#10;</xsl:text>
          <xsl:text>        return S_OK;&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genFormalParams">
  <xsl:param name="name" />
  <xsl:param name="utf8str" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genFormalParams">
        <xsl:with-param name="name" select="$extends" />
        <xsl:with-param name="utf8str" select="$utf8str" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute[@name != 'midlDoesNotLikeEmptyInterfaces']">
    <xsl:variable name="aName" select="concat('a_',@name)"/>
    <xsl:variable name="aTypeName">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$aName" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
        <xsl:with-param name="utf8str" select="$utf8str" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat(', ',$aTypeName)"/>
  </xsl:for-each>
</xsl:template>

<xsl:template name="genCallParams">
  <xsl:param name="name" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genCallParams">
        <xsl:with-param name="name" select="$extends" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute[@name != 'midlDoesNotLikeEmptyInterfaces']">
    <xsl:variable name="aName" select="concat('a_',@name)"/>
    <xsl:choose>
      <xsl:when test="@safearray='yes'">
        <xsl:value-of select="concat(', ComSafeArrayInArg(',$aName,')')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat(', ',$aName)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<xsl:template name="genAttrInitCode">
  <xsl:param name="name" />
  <xsl:param name="obj" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genAttrInitCode">
        <xsl:with-param name="name" select="$extends" />
        <xsl:with-param name="obj" select="$obj" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute[@name != 'midlDoesNotLikeEmptyInterfaces']">
    <xsl:variable name="aName" select="concat('a_',@name)"/>
    <xsl:variable name="aTypeName">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$aName" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="aType">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="'_'" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="@safearray='yes'">
        <xsl:variable name="elemtype">
          <xsl:call-template name="typeIdl2Back">
            <xsl:with-param name="type" select="@type" />
            <xsl:with-param name="safearray" select="''" />
            <xsl:with-param name="dir" select="'in'" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:text>        if (SUCCEEDED(hrc))&#10;</xsl:text>
        <xsl:value-of select="concat('            hrc = ',$obj, '->set_', @name, '(ComSafeArrayInArg(a_', @name, '));&#10;')"/>
      </xsl:when>
      <xsl:when test="(@type='wstring') or (@type = 'uuid')">
        <xsl:text>        if (SUCCEEDED(hrc))&#10;</xsl:text>
        <xsl:value-of select="concat('            hrc = ',$obj, '->set_', @name, '(',$aName, ');&#10;')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('        ',$obj, '->set_', @name, '(',$aName, ');&#10;')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<xsl:template name="genImplList">
  <xsl:param name="impl" />
  <xsl:param name="name" />
  <xsl:param name="depth" />
  <xsl:param name="parents" />

  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$name='IEvent'">
      <xsl:value-of select="       '#ifdef VBOX_WITH_XPCOM&#10;'" />
      <xsl:value-of select="concat('NS_DECL_CLASSINFO(', $impl, ')&#10;')" />
      <xsl:value-of select="concat('NS_IMPL_THREADSAFE_ISUPPORTS',$depth,'_CI(', $impl, $parents, ', IEvent)&#10;')" />
      <xsl:value-of select="       '#endif&#10;&#10;'"/>
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genImplList">
        <xsl:with-param name="impl" select="$impl" />
        <xsl:with-param name="name" select="$extends" />
        <xsl:with-param name="depth" select="$depth+1" />
        <xsl:with-param name="parents" select="concat($parents, ', ', $name)" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genAttrCode">
  <xsl:param name="name" />
  <xsl:param name="depth" />
  <xsl:param name="parents" />
  <xsl:param name="fGenBstr" />

  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute">
    <xsl:variable name="mName">
      <xsl:value-of select="concat('m_', @name)" />
    </xsl:variable>
    <xsl:variable name="mType">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="pName">
      <xsl:value-of select="concat('a_', @name)" />
    </xsl:variable>
    <xsl:variable name="pTypeNameOut">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$pName" />
        <xsl:with-param name="dir" select="'out'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="pTypeNameIn">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$pName" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="capsName">
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:value-of select="       '&#10;'" />
    <xsl:value-of select="concat('    // attribute ', @name,'&#10;')" />
    <xsl:value-of select="       'private:&#10;'" />
    <xsl:value-of select="concat('    ', $mType, '    ', $mName,';&#10;')" />
    <xsl:value-of select="       'public:&#10;'" />
    <xsl:value-of select="concat('    STDMETHOD(COMGETTER(', $capsName,'))(',$pTypeNameOut,') RT_OVERRIDE&#10;    {&#10;')" />
    <xsl:call-template name="genRetParam">
      <xsl:with-param name="type" select="@type" />
      <xsl:with-param name="member" select="$mName" />
      <xsl:with-param name="param" select="$pName" />
      <xsl:with-param name="safearray" select="@safearray" />
    </xsl:call-template>
    <xsl:value-of select="       '    }&#10;'" />

    <xsl:if test="not(@readonly='yes')">
      <xsl:value-of select="concat('    STDMETHOD(COMSETTER(', $capsName,'))(',$pTypeNameIn,') RT_OVERRIDE&#10;    {&#10;')" />
      <xsl:call-template name="genSetParam">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="member" select="$mName" />
        <xsl:with-param name="param" select="$pName" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="internal" select="'no'" />
      </xsl:call-template>
      <xsl:value-of select="       '    }&#10;'" />
    </xsl:if>

    <xsl:text>    // purely internal setter&#10;</xsl:text>
    <xsl:if test="(@type='wstring') or (@type = 'uuid')">
      <xsl:text>    inline HRESULT set_</xsl:text><xsl:value-of select="@name"/><xsl:text>(const Utf8Str &amp;a_rString)&#10;</xsl:text>
      <xsl:text>    {&#10;</xsl:text>
      <xsl:text>        return </xsl:text><xsl:value-of select="$mName"/><xsl:text>.assignEx(a_rString);&#10;</xsl:text>
      <xsl:text>    }&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="not((@type='wstring') or (@type = 'uuid')) or $fGenBstr">
      <xsl:text>    inline </xsl:text>
      <xsl:choose>
        <xsl:when test="(@safearray='yes') or (@type='wstring') or (@type = 'uuid')">
          <xsl:text>HRESULT</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>void</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:value-of select="concat(' set_', @name,'(',$pTypeNameIn, ')&#10;    {&#10;')" />
      <xsl:call-template name="genSetParam">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="member" select="$mName" />
        <xsl:with-param name="param" select="$pName" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="internal" select="'yes'" />
      </xsl:call-template>
      <xsl:value-of select="       '    }&#10;'" />
    </xsl:if>

  </xsl:for-each>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
      <xsl:value-of select="   '    // skipping IEvent attributes &#10;'" />
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
      <xsl:value-of select="   '    // skipping IReusableEvent attributes &#10;'" />
    </xsl:when>
     <xsl:when test="$extends='IVetoEvent'">
      <xsl:value-of select="   '    // skipping IVetoEvent attributes &#10;'" />
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genAttrCode">
        <xsl:with-param name="name" select="$extends" />
        <xsl:with-param name="depth" select="$depth+1" />
        <xsl:with-param name="parents" select="concat($parents, ', ', @name)" />
        <xsl:with-param name="fGenBstr" select="$fGenBstr" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $extends)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genReinitFunction">
  <xsl:param name="name"/>
  <xsl:param name="evname"/>
  <xsl:param name="ifname"/>
  <xsl:param name="implName"/>
  <xsl:param name="utf8str"/>

  <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Reinit', $evname, '(IEvent *aEvent')"/>
  <xsl:call-template name="genFormalParams">
    <xsl:with-param name="name" select="$ifname" />
    <xsl:with-param name="utf8str" select="'no'" />
  </xsl:call-template>
  <xsl:text>)&#10;</xsl:text>
  <xsl:text>{&#10;</xsl:text>
  <xsl:text>    </xsl:text><xsl:value-of select="$implName"/><xsl:text> *pEvtImpl = dynamic_cast&lt;</xsl:text>
  <xsl:value-of select="$implName"/><xsl:text> *&gt;(aEvent);&#10;</xsl:text>
  <xsl:text>    if (pEvtImpl)&#10;</xsl:text>
  <xsl:text>    {&#10;</xsl:text>
  <xsl:text>        pEvtImpl->Reuse();&#10;</xsl:text>
  <xsl:text>        HRESULT hrc = S_OK;&#10;</xsl:text>
  <xsl:call-template name="genAttrInitCode">
    <xsl:with-param name="name" select="$name" />
    <xsl:with-param name="obj" select="'pEvtImpl'" />
  </xsl:call-template>
  <xsl:text>        return hrc;&#10;</xsl:text>
  <xsl:text>    }&#10;</xsl:text>
  <xsl:text>    return E_INVALIDARG;&#10;</xsl:text>
  <xsl:text>}&#10;</xsl:text>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template name="genCreateFunction">
  <xsl:param name="name"/>
  <xsl:param name="evname"/>
  <xsl:param name="ifname"/>
  <xsl:param name="implName"/>
  <xsl:param name="waitable"/>
  <xsl:param name="evid"/>
  <xsl:param name="utf8str"/>

  <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Create', $evname, '(IEvent **aEvent, IEventSource *aSource')"/>
  <xsl:call-template name="genFormalParams">
    <xsl:with-param name="name" select="$ifname" />
    <xsl:with-param name="utf8str" select="$utf8str" />
  </xsl:call-template>
  <xsl:text>)&#10;</xsl:text>
  <xsl:text>{&#10;</xsl:text>
  <xsl:text>    ComObjPtr&lt;</xsl:text><xsl:value-of select="$implName"/><xsl:text>&gt; EvtObj;&#10;</xsl:text>
  <xsl:text>    HRESULT hrc = EvtObj.createObject();&#10;</xsl:text>
  <xsl:text>    if (SUCCEEDED(hrc))&#10;</xsl:text>
  <xsl:text>    {&#10;</xsl:text>
  <xsl:text>        hrc = EvtObj-&gt;init(aSource, VBoxEventType_</xsl:text><xsl:value-of select="$evid"/>
  <xsl:text>, </xsl:text><xsl:value-of select="$waitable" /><xsl:text> /*waitable*/);&#10;</xsl:text>
  <xsl:call-template name="genAttrInitCode">
    <xsl:with-param name="name" select="$name" />
    <xsl:with-param name="obj" select="'EvtObj'" />
  </xsl:call-template>
  <xsl:text>        if (SUCCEEDED(hrc))&#10;</xsl:text>
  <xsl:text>        {&#10;</xsl:text>
  <xsl:text>            hrc = EvtObj.queryInterfaceTo(aEvent);&#10;</xsl:text>
  <xsl:text>            if (SUCCEEDED(hrc))&#10;</xsl:text>
  <xsl:text>                return hrc;&#10;</xsl:text>
  <xsl:text>        }&#10;</xsl:text>
  <xsl:text>    }&#10;</xsl:text>
  <xsl:text>    *aEvent = NULL;&#10;</xsl:text>
  <xsl:text>    return hrc;&#10;</xsl:text>
  <xsl:text>}&#10;</xsl:text>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template name="genFireFunction">
  <xsl:param name="evname"/>
  <xsl:param name="ifname"/>
  <xsl:param name="utf8str"/>

  <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Fire', $evname, '(IEventSource *aSource')"/>
  <xsl:call-template name="genFormalParams">
    <xsl:with-param name="name" select="$ifname" />
    <xsl:with-param name="utf8str" select="$utf8str" />
  </xsl:call-template>
  <xsl:text>)&#10;</xsl:text>
  <xsl:text>{&#10;</xsl:text>
  <xsl:text>    AssertReturn(aSource, E_INVALIDARG);&#10;</xsl:text>
  <xsl:text>    ComPtr&lt;IEvent&gt; ptrEvent;&#10;</xsl:text>
  <xsl:text>    HRESULT hrc = </xsl:text>
  <xsl:value-of select="concat('Create', $evname, '(ptrEvent.asOutParam(), aSource')"/>
  <xsl:call-template name="genCallParams">
    <xsl:with-param name="name" select="$ifname" />
  </xsl:call-template>
  <xsl:text>);&#10;</xsl:text>
  <xsl:text>    if (SUCCEEDED(hrc))&#10;</xsl:text>
  <xsl:text>    {&#10;</xsl:text>
  <xsl:text>        BOOL fDeliveredIgnored = FALSE;&#10;</xsl:text>
  <xsl:text>        hrc = aSource-&gt;FireEvent(ptrEvent, /* do not wait for delivery */ 0, &amp;fDeliveredIgnored);&#10;</xsl:text>
  <xsl:text>        AssertComRC(hrc);&#10;</xsl:text>
  <xsl:text>    }&#10;</xsl:text>
  <xsl:text>    return hrc;&#10;</xsl:text>
  <xsl:text>}&#10;</xsl:text>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template name="genEventImpl">
  <xsl:param name="implName" />
  <xsl:param name="isVeto" />
  <xsl:param name="isReusable" />

  <xsl:value-of select="concat('class ATL_NO_VTABLE ',$implName,
                        '&#10;    : public VirtualBoxBase&#10;    , VBOX_SCRIPTABLE_IMPL(',
                        @name, ')&#10;{&#10;')" />
  <xsl:value-of select="'public:&#10;'" />
  <xsl:value-of select="concat('    VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(', $implName, ', ', @name, ')&#10;')" />
  <xsl:value-of select="concat('    DECLARE_NOT_AGGREGATABLE(', $implName, ')&#10;')" />
  <xsl:value-of select="       '    DECLARE_PROTECT_FINAL_CONSTRUCT()&#10;'" />
  <xsl:value-of select="concat('    BEGIN_COM_MAP(', $implName, ')&#10;')" />
  <xsl:value-of select="       '        COM_INTERFACE_ENTRY(ISupportErrorInfo)&#10;'" />
  <xsl:value-of select="concat('        COM_INTERFACE_ENTRY(', @name, ')&#10;')" />
  <xsl:value-of select="concat('        COM_INTERFACE_ENTRY2(IDispatch, ', @name, ')&#10;')" />
  <xsl:value-of select="concat('        VBOX_TWEAK_INTERFACE_ENTRY(', @name, ')&#10;')" />

  <xsl:call-template name="genComEntry">
    <xsl:with-param name="name" select="@name" />
  </xsl:call-template>
  <xsl:value-of select="       '    END_COM_MAP()&#10;'" />
  <xsl:value-of select="concat('    ',$implName,'() { Log12((&quot;',$implName,' %p\n&quot;, this)); }&#10;')" />
  <xsl:value-of select="concat('    virtual ~',$implName,'() { Log12((&quot;~',$implName,' %p\n&quot;, this)); uninit(); }&#10;')" />
  <xsl:text><![CDATA[
    HRESULT FinalConstruct()
    {
        BaseFinalConstruct();
        return mEvent.createObject();
    }
    void FinalRelease()
    {
        uninit();
        BaseFinalRelease();
    }
    STDMETHOD(COMGETTER(Type))(VBoxEventType_T *aType) RT_OVERRIDE
    {
        return mEvent->COMGETTER(Type)(aType);
    }
    STDMETHOD(COMGETTER(Source))(IEventSource * *aSource) RT_OVERRIDE
    {
        return mEvent->COMGETTER(Source)(aSource);
    }
    STDMETHOD(COMGETTER(Waitable))(BOOL *aWaitable) RT_OVERRIDE
    {
        return mEvent->COMGETTER(Waitable)(aWaitable);
    }
    STDMETHOD(SetProcessed)() RT_OVERRIDE
    {
       return mEvent->SetProcessed();
    }
    STDMETHOD(WaitProcessed)(LONG aTimeout, BOOL *aResult) RT_OVERRIDE
    {
        return mEvent->WaitProcessed(aTimeout, aResult);
    }
    void uninit()
    {
        if (!mEvent.isNull())
        {
           mEvent->uninit();
           mEvent.setNull();
        }
    }
]]></xsl:text>
  <xsl:choose>
    <xsl:when test="$isVeto='yes'">
<xsl:text><![CDATA[
    HRESULT init(IEventSource *aSource, VBoxEventType_T aType, BOOL aWaitable = TRUE)
    {
        NOREF(aWaitable);
        return mEvent->init(aSource, aType);
    }
    STDMETHOD(AddVeto)(IN_BSTR aVeto) RT_OVERRIDE
    {
        return mEvent->AddVeto(aVeto);
    }
    STDMETHOD(IsVetoed)(BOOL *aResult) RT_OVERRIDE
    {
       return mEvent->IsVetoed(aResult);
    }
    STDMETHOD(GetVetos)(ComSafeArrayOut(BSTR, aVetos)) RT_OVERRIDE
    {
       return mEvent->GetVetos(ComSafeArrayOutArg(aVetos));
    }
    STDMETHOD(AddApproval)(IN_BSTR aReason) RT_OVERRIDE
    {
        return mEvent->AddApproval(aReason);
    }
    STDMETHOD(IsApproved)(BOOL *aResult) RT_OVERRIDE
    {
       return mEvent->IsApproved(aResult);
    }
    STDMETHOD(GetApprovals)(ComSafeArrayOut(BSTR, aReasons)) RT_OVERRIDE
    {
       return mEvent->GetApprovals(ComSafeArrayOutArg(aReasons));
    }
private:
    ComObjPtr<VBoxVetoEvent>      mEvent;
]]></xsl:text>
    </xsl:when>
    <xsl:when test="$isReusable='yes'">
      <xsl:text>
<![CDATA[
    HRESULT init(IEventSource *aSource, VBoxEventType_T aType, BOOL aWaitable = FALSE)
    {
        mGeneration = 1;
        return mEvent->init(aSource, aType, aWaitable);
    }
    STDMETHOD(COMGETTER(Generation))(ULONG *aGeneration) RT_OVERRIDE
    {
        *aGeneration = mGeneration;
        return S_OK;
    }
    STDMETHOD(Reuse)() RT_OVERRIDE
    {
        ASMAtomicIncU32((volatile uint32_t *)&mGeneration);
        return S_OK;
    }
private:
    volatile ULONG              mGeneration;
    ComObjPtr<VBoxEvent>        mEvent;
]]></xsl:text>
    </xsl:when>
    <xsl:otherwise>
<xsl:text><![CDATA[
    HRESULT init(IEventSource *aSource, VBoxEventType_T aType, BOOL aWaitable)
    {
        return mEvent->init(aSource, aType, aWaitable);
    }
private:
    ComObjPtr<VBoxEvent>      mEvent;
]]></xsl:text>
    </xsl:otherwise>
  </xsl:choose>

  <!-- Before we generate attribute code, we check and make sure there are attributes here. -->
  <xsl:if test="count(attribute) = 0 and @name != 'INATNetworkAlterEvent'">
    <xsl:call-template name="fatalError">
      <xsl:with-param name="msg">error: <xsl:value-of select="@name"/> has no attributes</xsl:with-param>
    </xsl:call-template>
  </xsl:if>

  <xsl:call-template name="genAttrCode">
    <xsl:with-param name="name" select="@name" />
    <xsl:with-param name="fGenBstr" select="($G_generateBstrVariants = 'yes') or (contains(@autogenflags, 'BSTR'))" />
  </xsl:call-template>
  <xsl:value-of select="'};&#10;'" />


  <xsl:call-template name="genImplList">
    <xsl:with-param name="impl" select="$implName" />
    <xsl:with-param name="name" select="@name" />
    <xsl:with-param name="depth" select="'1'" />
    <xsl:with-param name="parents" select="''" />
  </xsl:call-template>

  <!-- Associate public functions. -->
  <xsl:variable name="evname">
    <xsl:value-of select="substring(@name, 2)" />
  </xsl:variable>
  <xsl:variable name="evid">
    <xsl:value-of select="concat('On', substring(@name, 2, string-length(@name)-6))" />
  </xsl:variable>
  <xsl:variable name="ifname">
    <xsl:value-of select="@name" />
  </xsl:variable>
  <xsl:variable name="waitable">
    <xsl:choose>
      <xsl:when test="@waitable='yes'">
        <xsl:value-of select="'TRUE'"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="'FALSE'"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="hasStringAttribs">
    <xsl:call-template name="hasStringAttributes">
      <xsl:with-param name="name" select="@name"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- Generate ReinitXxxxEvent functions if reusable. -->
  <xsl:if test="$isReusable='yes'">
    <xsl:call-template name="genReinitFunction">
      <xsl:with-param name="name" select="@name"/>
      <xsl:with-param name="evname" select="$evname"/>
      <xsl:with-param name="ifname" select="$ifname"/>
      <xsl:with-param name="implName" select="$implName"/>
      <xsl:with-param name="utf8str" select="'yes'"/>
    </xsl:call-template>

    <xsl:if test="($hasStringAttribs != '') and (($G_generateBstrVariants = 'yes') or (contains(@autogenflags, 'BSTR')))">
      <xsl:call-template name="genReinitFunction">
        <xsl:with-param name="name" select="@name"/>
        <xsl:with-param name="evname" select="$evname"/>
        <xsl:with-param name="ifname" select="$ifname"/>
        <xsl:with-param name="implName" select="$implName"/>
        <xsl:with-param name="utf8str" select="'no'"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:if>

  <!-- Generate the CreateXxxxEvent function. -->
  <xsl:call-template name="genCreateFunction">
    <xsl:with-param name="name" select="@name"/>
    <xsl:with-param name="evname" select="$evname"/>
    <xsl:with-param name="ifname" select="$ifname"/>
    <xsl:with-param name="implName" select="$implName"/>
    <xsl:with-param name="waitable" select="$waitable"/>
    <xsl:with-param name="evid" select="$evid"/>
    <xsl:with-param name="utf8str" select="'yes'"/>
  </xsl:call-template>

  <xsl:if test="($hasStringAttribs != '') and (($G_generateBstrVariants = 'yes') or (contains(@autogenflags, 'BSTR')))">
    <xsl:call-template name="genCreateFunction">
      <xsl:with-param name="name" select="@name"/>
      <xsl:with-param name="evname" select="$evname"/>
      <xsl:with-param name="ifname" select="$ifname"/>
      <xsl:with-param name="implName" select="$implName"/>
      <xsl:with-param name="waitable" select="$waitable"/>
      <xsl:with-param name="evid" select="$evid"/>
      <xsl:with-param name="utf8str" select="'no'"/>
    </xsl:call-template>
  </xsl:if>

  <!-- Generate the FireXxxxEvent function. -->
  <xsl:call-template name="genFireFunction">
    <xsl:with-param name="evname" select="$evname"/>
    <xsl:with-param name="ifname" select="$ifname"/>
    <xsl:with-param name="utf8str" select="'yes'"/>
  </xsl:call-template>

  <xsl:if test="($hasStringAttribs != '') and (($G_generateBstrVariants = 'yes') or (contains(@autogenflags, 'BSTR')))">
    <xsl:call-template name="genFireFunction">
      <xsl:with-param name="evname" select="$evname"/>
      <xsl:with-param name="ifname" select="$ifname"/>
      <xsl:with-param name="utf8str" select="'no'"/>
    </xsl:call-template>
  </xsl:if>

</xsl:template>


<!--
 Produces VBoxEvents.cpp
 -->
<xsl:template name="genCommonEventCode">
  <xsl:call-template name="fileheader">
    <xsl:with-param name="name" select="'VBoxEvents.cpp'" />
  </xsl:call-template>

<xsl:text><![CDATA[
#define LOG_GROUP LOG_GROUP_MAIN_EVENT
#include <VBox/com/array.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include "VBoxEvents.h"

]]></xsl:text>

  <!-- Interfaces -->
  <xsl:for-each select="//interface[@autogen=$G_kind]">
    <xsl:value-of select="concat('// ', @name,  ' implementation code')" />
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:variable name="implName">
      <xsl:value-of select="substring(@name, 2)" />
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$G_kind='VBoxEvent'">
        <xsl:variable name="isVeto">
          <xsl:if test="@extends='IVetoEvent'">
            <xsl:value-of select="'yes'" />
          </xsl:if>
        </xsl:variable>
        <xsl:variable name="isReusable">
          <xsl:if test="@extends='IReusableEvent'">
            <xsl:value-of select="'yes'" />
          </xsl:if>
        </xsl:variable>
        <xsl:call-template name="genEventImpl">
          <xsl:with-param name="implName" select="$implName" />
          <xsl:with-param name="isVeto" select="$isVeto" />
          <xsl:with-param name="isReusable" select="$isReusable" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
  </xsl:for-each>

</xsl:template>


<!--
 Produces VBoxEvents.h
 -->
<xsl:template name="genCommonEventHeader">
  <xsl:call-template name="fileheader">
    <xsl:with-param name="name" select="'VBoxEvents.h'" />
  </xsl:call-template>

  <xsl:text><![CDATA[
#include "EventImpl.h"

]]></xsl:text>

  <!-- Simple methods for firing off events. -->
 <xsl:text>/** @name Fire off events&#10;</xsl:text>
 <xsl:text> * @{ */&#10;</xsl:text>
  <xsl:for-each select="//interface[@autogen='VBoxEvent']">
    <xsl:value-of select="concat('/** Fire an ', @name,  ' event. */&#10;')" />
    <xsl:variable name="evname">
      <xsl:value-of select="substring(@name, 2)" />
    </xsl:variable>
    <xsl:variable name="ifname">
      <xsl:value-of select="@name" />
    </xsl:variable>
    <xsl:variable name="hasStringAttribs">
      <xsl:call-template name="hasStringAttributes">
        <xsl:with-param name="name" select="@name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Fire', $evname, '(IEventSource *aSource')"/>
    <xsl:call-template name="genFormalParams">
      <xsl:with-param name="name" select="$ifname" />
      <xsl:with-param name="utf8str" select="'yes'" />
    </xsl:call-template>
    <xsl:text>);&#10;</xsl:text>

    <xsl:if test="($hasStringAttribs != '') and (($G_generateBstrVariants = 'yes') or (contains(@autogenflags, 'BSTR')))">
      <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Fire', $evname, '(IEventSource *aSource')"/>
      <xsl:call-template name="genFormalParams">
        <xsl:with-param name="name" select="$ifname" />
        <xsl:with-param name="utf8str" select="'no'" />
      </xsl:call-template>
      <xsl:text>);&#10;</xsl:text>
    </xsl:if>
  </xsl:for-each>
  <xsl:text>/** @} */&#10;&#10;</xsl:text>

  <!-- Event instantiation methods. -->
  <xsl:text>/** @name Instantiate events&#10;</xsl:text>
  <xsl:text> * @{ */&#10;</xsl:text>
  <xsl:for-each select="//interface[@autogen='VBoxEvent']">
    <xsl:value-of select="concat('/** Create an ', @name,  ' event. */&#10;')" />
    <xsl:variable name="evname">
      <xsl:value-of select="substring(@name, 2)" />
    </xsl:variable>
    <xsl:variable name="ifname">
      <xsl:value-of select="@name" />
    </xsl:variable>
    <xsl:variable name="hasStringAttribs">
      <xsl:call-template name="hasStringAttributes">
        <xsl:with-param name="name" select="@name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Create', $evname, '(IEvent **aEvent, IEventSource *aSource')"/>
    <xsl:call-template name="genFormalParams">
      <xsl:with-param name="name" select="$ifname" />
      <xsl:with-param name="utf8str" select="'yes'" />
    </xsl:call-template>
    <xsl:text>);&#10;</xsl:text>

    <xsl:if test="($hasStringAttribs != '') and (($G_generateBstrVariants = 'yes') or (contains(@autogenflags, 'BSTR')))">
      <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Create', $evname, '(IEvent **aEvent, IEventSource *aSource')"/>
      <xsl:call-template name="genFormalParams">
        <xsl:with-param name="name" select="$ifname" />
        <xsl:with-param name="utf8str" select="'no'" />
      </xsl:call-template>
      <xsl:text>);&#10;</xsl:text>
    </xsl:if>
  </xsl:for-each>
  <xsl:text>/** @} */&#10;</xsl:text>
  <xsl:text>&#10;</xsl:text>

  <!-- Reinitialization methods for reusable events. -->
  <xsl:text>/** @name Re-init reusable events&#10;</xsl:text>
  <xsl:text> * @{ */&#10;</xsl:text>
  <xsl:for-each select="//interface[@autogen='VBoxEvent']">
    <xsl:if test="@extends='IReusableEvent'">
      <xsl:value-of select="concat('/** Re-init an ', @name,  ' event. */&#10;')" />
      <xsl:variable name="evname">
        <xsl:value-of select="substring(@name, 2)" />
      </xsl:variable>
      <xsl:variable name="ifname">
        <xsl:value-of select="@name" />
      </xsl:variable>
      <xsl:variable name="hasStringAttribs">
        <xsl:call-template name="hasStringAttributes">
          <xsl:with-param name="name" select="@name"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Reinit', $evname, '(IEvent *aEvent')"/>
      <xsl:call-template name="genFormalParams">
        <xsl:with-param name="name" select="$ifname" />
        <xsl:with-param name="utf8str" select="'yes'" />
      </xsl:call-template>
      <xsl:text>);&#10;</xsl:text>

      <xsl:if test="($hasStringAttribs != '') and (($G_generateBstrVariants = 'yes') or (contains(@autogenflags, 'BSTR')))">
        <xsl:value-of select="concat('DECLHIDDEN(HRESULT) Reinit', $evname, '(IEvent *aEvent')"/>
        <xsl:call-template name="genFormalParams">
          <xsl:with-param name="name" select="$ifname" />
          <xsl:with-param name="utf8str" select="'no'" />
        </xsl:call-template>
        <xsl:text>);&#10;</xsl:text>
      </xsl:if>
    </xsl:if>
  </xsl:for-each>
  <xsl:text>/** @} */&#10;</xsl:text>

</xsl:template>

<xsl:template match="/">
  <!-- Global code -->
   <xsl:choose>
      <xsl:when test="$G_kind='VBoxEvent'">
        <xsl:call-template name="genCommonEventCode">
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$G_kind='VBoxEventHeader'">
        <xsl:call-template name="genCommonEventHeader">
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="fatalError">
          <xsl:with-param name="msg" select="concat('Request unsupported: ', $G_kind)" />
        </xsl:call-template>
      </xsl:otherwise>
   </xsl:choose>
</xsl:template>

</xsl:stylesheet>
