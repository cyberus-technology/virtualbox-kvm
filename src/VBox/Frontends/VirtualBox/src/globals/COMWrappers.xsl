<?xml version="1.0"?>

<!--
 *  A template to generate wrapper classes for [XP]COM interfaces
 *  (defined in XIDL) to use them in the main Qt-based GUI
 *  in platform-independent script-like manner.
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

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:include href="../../../../Main/idl/typemap-shared.inc.xsl" />


<!--
 * Keys for more efficiently looking up of types.
-->
<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>


<!--
 *  File start bracket
-->
<xsl:template name="startFile">
  <xsl:param name="file" />
  <xsl:value-of select="concat('&#10;// ##### BEGINFILE &quot;', $file, '&quot;&#10;')" />
</xsl:template>

<!--
 *  File end bracket
-->
<xsl:template name="endFile">
  <xsl:param name="file" />
  <xsl:call-template name="xsltprocNewlineOutputHack"/>
  <xsl:value-of select="concat('// ##### ENDFILE &quot;', $file, '&quot;&#10;&#10;')" />
</xsl:template>


<!--
 *  Shut down all implicit templates
-->
<xsl:template match="*"/>
<xsl:template match="*|/" mode="declare"/>
<xsl:template match="*|/" mode="include"/>
<xsl:template match="*|/" mode="define"/>
<xsl:template match="*|/" mode="end"/>
<xsl:template match="*|/" mode="begin"/>


<!--
 *  Main entry point (idl):
-->
<xsl:template match="idl">
  <!-- Apply underlying template (library): -->
  <xsl:apply-templates/>
</xsl:template>


<!--
 *  Encloses |if| element's contents (unconditionally expanded by
 *  <apply-templates mode="include"/>) with #ifdef / #endif.
 *
 *  @note this can produce an empty #if/#endif block if |if|'s children
 *  expand to nothing (such as |cpp|). I see no need to handle this situation
 *  specially.
-->
<xsl:template match="if" mode="include">
  <xsl:if test="(@target='xpidl') or (@target='midl')">
    <xsl:apply-templates select="." mode="begin"/>
    <xsl:apply-templates mode="include"/>
    <xsl:apply-templates select="." mode="end"/>
  </xsl:if>
</xsl:template>

<!--
 *  Encloses |if| element's contents (unconditionally expanded by
 *  <apply-templates mode="define"/>) with #ifdef / #endif.
 *
 *  @note this can produce an empty #if/#endif block if |if|'s children
 *  expand to nothing (such as |cpp|). I see no need to handle this situation
 *  specially.
-->
<xsl:template match="if" mode="define">
  <xsl:if test="(@target='xpidl') or (@target='midl')">
    <xsl:apply-templates select="." mode="begin"/>
    <xsl:apply-templates mode="define"/>
    <xsl:apply-templates select="." mode="end"/>
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>

<!--
 *  Encloses |if| element's contents (unconditionally expanded by
 *  <apply-templates mode="declare"/>) with #ifdef / #endif.
 *
 *  @note this can produce an empty #if/#endif block if |if|'s children
 *  expand to nothing (such as |cpp|). I see no need to handle this situation
 *  specially.
-->
<xsl:template match="if" mode="declare">
  <xsl:if test="(@target='xpidl') or (@target='midl')">
    <xsl:apply-templates select="." mode="begin"/>
    <xsl:apply-templates mode="declare"/>
    <xsl:apply-templates select="." mode="end"/>
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>

<!--
 *  |<if target="...">| element): begin and end.
-->
<xsl:template match="if" mode="begin">
  <xsl:if test="@target='xpidl'">
    <xsl:text>#if !defined(Q_WS_WIN)&#x0A;</xsl:text>
  </xsl:if>
  <xsl:if test="@target='midl'">
    <xsl:text>#if defined(Q_WS_WIN)&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>
<xsl:template match="if" mode="end">
  <xsl:if test="(@target='xpidl') or (@target='midl')">
    <xsl:text>#endif&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>


<!--
 *  cpp_quote
-->
<xsl:template match="cpp"/>


<!--
 *  #ifdef statement (@if attribute): begin and end
-->
<xsl:template match="@if" mode="begin">
  <xsl:text>#if </xsl:text>
  <xsl:value-of select="."/>
  <xsl:text>&#x0A;</xsl:text>
</xsl:template>
<xsl:template match="@if" mode="end">
  <xsl:text>#endif&#x0A;</xsl:text>
</xsl:template>


<!--
 *  Library
-->
<xsl:template match="library">
    <!-- Declare enums: -->
    <xsl:call-template name="declareEnums"/>

    <!-- Declare interfaces: -->
    <xsl:apply-templates select="application/if | application/interface[not(@internal='yes')]" mode="declare"/>

    <!-- Define interfaces: -->
    <xsl:call-template name="defineInterfaces"/>
</xsl:template>


<!--
 *  Declare enums:
-->
<xsl:template name="declareEnums">
    <!-- Starting COMEnums.h file: -->
    <xsl:call-template name="startFile">
        <xsl:with-param name="file" select="'COMEnums.h'" />
    </xsl:call-template>

    <!-- Write down file header: -->
    <xsl:text>/*&#x0A;</xsl:text>
    <xsl:text> * DO NOT EDIT! This is a generated file.&#x0A;</xsl:text>
    <xsl:text> *&#x0A;</xsl:text>
    <xsl:text> * Qt-based wrappers for VirtualBox Main API (COM) enums.&#x0A;</xsl:text>
    <xsl:text> * Generated from XIDL (XML interface definition).&#x0A;</xsl:text>
    <xsl:text> *&#x0A;</xsl:text>
    <xsl:text> * Source    : src/VBox/Main/idl/VirtualBox.xidl&#x0A;</xsl:text>
    <xsl:text> * Generator : src/VBox/Frontends/VirtualBox/src/globals/COMWrappers.xsl&#x0A;</xsl:text>
    <xsl:text> */&#x0A;&#x0A;</xsl:text>
    <xsl:text>#ifndef ___COMEnums_h___&#x0A;</xsl:text>
    <xsl:text>#define ___COMEnums_h___&#x0A;&#x0A;</xsl:text>
    <xsl:text>/* GUI includes: */&#x0A;</xsl:text>
    <xsl:text>#include "QMetaType"&#x0A;&#x0A;</xsl:text>

    <!-- Enumerate all enums: -->
    <xsl:for-each select="application/enum">
        <xsl:text>/* </xsl:text>
        <xsl:value-of select="concat('K',@name)"/>
        <xsl:text> enum: */&#x0A;</xsl:text>
        <xsl:text>enum </xsl:text>
        <xsl:value-of select="concat('K',@name)"/>
        <xsl:text>&#x0A;{&#x0A;</xsl:text>
        <xsl:for-each select="const">
            <xsl:text>    </xsl:text>
            <xsl:value-of select="concat('K',../@name,'_',@name)"/>
            <xsl:text> = </xsl:text>
            <xsl:value-of select="@value"/>
            <xsl:text>,&#x0A;</xsl:text>
        </xsl:for-each>
        <xsl:text>    </xsl:text>
        <xsl:value-of select="concat('K',@name)"/>
        <xsl:text>_Max&#x0A;</xsl:text>
        <xsl:text>};&#x0A;&#x0A;</xsl:text>
    </xsl:for-each>

    <!-- Declare enums to QMetaObject: -->
    <xsl:text>/* Let QMetaType know about generated enums: */&#x0A;</xsl:text>
    <xsl:for-each select="application/enum">
        <xsl:text>Q_DECLARE_METATYPE(</xsl:text>
        <xsl:value-of select="concat('K',@name)"/>
        <xsl:text>)&#x0A;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#x0A;</xsl:text>

    <!-- Write down file footer: -->
    <xsl:text>#endif /* __COMEnums_h__ */&#x0A;&#x0A;</xsl:text>

    <!-- Finishing COMEnums.h file: -->
    <xsl:call-template name="endFile">
        <xsl:with-param name="file" select="'COMEnums.h'" />
    </xsl:call-template>
</xsl:template>


<!--
 *  Define interfaces:
-->
<xsl:template name="defineInterfaces">
    <!-- Starting COMWrappers.cpp file: -->
    <xsl:call-template name="startFile">
        <xsl:with-param name="file" select="'COMWrappers.cpp'" />
    </xsl:call-template>

    <!-- Write down file header: -->
    <xsl:text>/*&#x0A;</xsl:text>
    <xsl:text> * DO NOT EDIT! This is a generated file.&#x0A;</xsl:text>
    <xsl:text> *&#x0A;</xsl:text>
    <xsl:text> * Qt-based wrappers definitions for VirtualBox Main API (COM) interfaces.&#x0A;</xsl:text>
    <xsl:text> * Generated from XIDL (XML interface definition).&#x0A;</xsl:text>
    <xsl:text> *&#x0A;</xsl:text>
    <xsl:text> * Source    : src/VBox/Main/idl/VirtualBox.xidl&#x0A;</xsl:text>
    <xsl:text> * Generator : src/VBox/Frontends/VirtualBox/src/globals/COMWrappers.xsl&#x0A;</xsl:text>
    <xsl:text> */&#x0A;&#x0A;</xsl:text>

    <xsl:text>#include "VBox/com/VirtualBox.h"&#x0A;&#x0A;</xsl:text>

    <xsl:text>/* COM includes: */&#x0A;</xsl:text>
    <xsl:text>#include "COMEnums.h"&#x0A;</xsl:text>

    <!-- Enumerate all interface definitions: -->
    <xsl:apply-templates select="application/if | application/interface[not(@internal='yes')]" mode="include"/>
    <xsl:text>&#x0A;</xsl:text>
    <xsl:apply-templates select="application/if | application/interface[not(@internal='yes')]" mode="define"/>

    <!-- Finishing COMWrappers.cpp file: -->
    <xsl:call-template name="endFile">
        <xsl:with-param name="file" select="'COMWrappers.cpp'" />
    </xsl:call-template>
</xsl:template>


<!--
 *  Declare interface:
-->
<xsl:template match="interface" mode="declare">
    <!-- Starting file: -->
    <xsl:call-template name="startFile">
        <xsl:with-param name="file" select="concat('C', substring(@name,2), '.h')" />
    </xsl:call-template>

    <!-- Write down file header: -->
    <xsl:text>/*&#x0A;</xsl:text>
    <xsl:text> * DO NOT EDIT! This is a generated file.&#x0A;</xsl:text>
    <xsl:text> *&#x0A;</xsl:text>
    <xsl:text> * Qt-based wrapper declaration for VirtualBox Main API (COM) interface.&#x0A;</xsl:text>
    <xsl:text> * Generated from XIDL (XML interface definition).&#x0A;</xsl:text>
    <xsl:text> *&#x0A;</xsl:text>
    <xsl:text> * Source    : src/VBox/Main/idl/VirtualBox.xidl&#x0A;</xsl:text>
    <xsl:text> * Generator : src/VBox/Frontends/VirtualBox/src/globals/COMWrappers.xsl&#x0A;</xsl:text>
    <xsl:text> */&#x0A;&#x0A;</xsl:text>
    <xsl:text>#ifndef __C</xsl:text>
    <xsl:value-of select="substring(@name,2)"/>
    <xsl:text>_h__&#x0A;</xsl:text>
    <xsl:text>#define __C</xsl:text>
    <xsl:value-of select="substring(@name,2)"/>
    <xsl:text>_h__&#x0A;&#x0A;</xsl:text>
    <xsl:if test="@name='IVirtualBox' or @name='IMachine'">
        <xsl:text>/* Qt includes: */&#x0A;</xsl:text>
        <xsl:text>#include &lt;QList&gt;&#x0A;</xsl:text>
        <xsl:text>#include &lt;QRect&gt;&#x0A;</xsl:text>
        <xsl:text>#include &lt;QStringList&gt;&#x0A;&#x0A;</xsl:text>
    </xsl:if>
    <xsl:text>/* GUI includes: */&#x0A;</xsl:text>
    <xsl:text>#include "COMDefs.h"&#x0A;</xsl:text>
    <xsl:text>#include "UILibraryDefs.h"&#x0A;&#x0A;</xsl:text>
    <xsl:text>/* VirtualBox interface declarations: */&#x0A;</xsl:text>
    <xsl:text>#ifndef VBOX_WITH_LESS_VIRTUALBOX_INCLUDING&#x0A;</xsl:text>
    <xsl:text># include &lt;VBox/com/VirtualBox.h&gt;&#x0A;</xsl:text>
    <xsl:text>#else&#x0A;</xsl:text>
    <xsl:text>COM_STRUCT_OR_CLASS(</xsl:text><xsl:value-of select="@name"/><xsl:text>);&#x0A;</xsl:text>
    <xsl:text>#endif&#x0A;</xsl:text>

    <!-- Forward declarations: -->
    <xsl:text>/* Forward declarations: */&#x0A;</xsl:text>
    <xsl:for-each select="//interface[not(@internal='yes')]">
        <xsl:text>class C</xsl:text>
        <xsl:value-of select="substring(@name,2)"/>
        <xsl:text>;&#x0A;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#x0A;</xsl:text>

    <!-- Interface forward declaration: -->
    <xsl:text>/* Interface forward declaration: */&#x0A;</xsl:text>
    <xsl:text>COM_STRUCT_OR_CLASS(I</xsl:text>
    <xsl:value-of select="substring(@name,2)"/>
    <xsl:text>);&#x0A;&#x0A;</xsl:text>

    <!-- Interface wrapper declaration: -->
    <xsl:text>/* Interface wrapper declaration: */&#x0A;</xsl:text>
    <xsl:text>class SHARED_LIBRARY_STUFF C</xsl:text>
    <xsl:value-of select="substring(@name,2)"/>
    <xsl:text> : public CInterface&lt;</xsl:text>
    <xsl:value-of select="@name"/>

    <!-- Use the correct base if supportsErrorInfo: -->
    <xsl:call-template name="tryComposeFetchErrorInfo">
        <xsl:with-param name="mode" select="'getBaseClassName'"/>
    </xsl:call-template>
    <xsl:text>&gt;&#x0A;{&#x0A;public:&#x0A;&#x0A;</xsl:text>

    <!-- Generate the Base typedef: -->
    <xsl:text>    typedef CInterface&lt;</xsl:text>
    <xsl:value-of select="@name"/>

    <!-- Use the correct base if supportsErrorInfo: -->
    <xsl:call-template name="tryComposeFetchErrorInfo">
        <xsl:with-param name="mode" select="'getBaseClassName'"/>
    </xsl:call-template>
    <xsl:text>&gt; Base;&#x0A;&#x0A;</xsl:text>

    <!-- Generate member declarations: -->
    <xsl:if test="name()='interface'">
        <xsl:call-template name="declareMembers"/>
    </xsl:if>

    <!-- Interface declaration: -->
    <xsl:text>};&#x0A;&#x0A;</xsl:text>

    <!-- Declare metatype: -->
    <xsl:text>/* Let QMetaType know about generated interface: */&#x0A;</xsl:text>
    <xsl:text>Q_DECLARE_METATYPE(</xsl:text>
    <xsl:value-of select="concat('C',substring(@name,2))"/>
    <xsl:text>);&#x0A;&#x0A;</xsl:text>

    <!-- Declare safe-array -->
    <xsl:if test="
        (name()='interface')
        and
        ((//attribute[@safearray='yes' and not(@internal='yes') and @type=current()/@name])
         or
         (//param[@safearray='yes' and not(../@internal='yes') and @type=current()/@name]))
        ">
            <xsl:text>/* Declare safe-array: */&#x0A;</xsl:text>
            <xsl:text>typedef QVector&lt;C</xsl:text>
            <xsl:value-of select="substring(@name,2)"/>
            <xsl:text>&gt; C</xsl:text>
            <xsl:value-of select="substring(@name,2)"/>
            <xsl:text>Vector;&#x0A;&#x0A;</xsl:text>
    </xsl:if>

    <!-- Write down file footer: -->
    <xsl:text>#endif /* __C</xsl:text>
    <xsl:value-of select="substring(@name,2)"/>
    <xsl:text>_h__ */&#x0A;&#x0A;</xsl:text>

    <!-- Finishing file: -->
    <xsl:call-template name="endFile">
        <xsl:with-param name="file" select="concat('C', substring(@name,2), '.h')" />
    </xsl:call-template>
</xsl:template>

<xsl:template name="declareAttributes">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//attribute[not(@internal='yes')]" mode="declare"/>
  <xsl:if test="$iface//attribute[not(@internal='yes')]">
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within application/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="declareAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within application -->
      <xsl:otherwise>
        <xsl:call-template name="declareAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="declareMethods">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//method[not(@internal='yes')]" mode="declare"/>
  <xsl:if test="$iface//method[not(@internal='yes')]">
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within application/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="declareMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within application -->
      <xsl:otherwise>
        <xsl:call-template name="declareMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="declareExtraDataHelpers">

<xsl:text>    void SetExtraDataBool(const QString &amp;strKey, bool fValue);
    bool GetExtraDataBool(const QString &amp;strKey, bool fDef = true);
    void SetExtraDataInt(const QString &amp;strKey, int value);
    int GetExtraDataInt(const QString &amp;strKey, int def = 0);
    void SetExtraDataRect(const QString &amp;strKey, const QRect &amp;value);
    QRect GetExtraDataRect(const QString &amp;strKey, const QRect &amp;def = QRect());
    void SetExtraDataStringList(const QString &amp;strKey, const QStringList &amp;value);
    QStringList GetExtraDataStringList(const QString &amp;strKey, QStringList def = QStringList());
    void SetExtraDataIntList(const QString &amp;strKey, const QList&lt;int&gt; &amp;value);
    QList&lt;int&gt; GetExtraDataIntList(const QString &amp;strKey, QList&lt;int&gt; def = QList&lt;int&gt;());

</xsl:text>

</xsl:template>

<xsl:template name="declareMembers">

  <xsl:text>    /* Constructors and assignments taking CUnknown and raw iface pointer: */&#x0A;&#x0A;</xsl:text>
  <!-- default constructor -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>();&#x0A;</xsl:text>
  <!-- default destructor -->
  <xsl:text>    ~C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>();&#x0A;&#x0A;</xsl:text>
  <!-- constructor taking CWhatever -->
  <xsl:text>    template&lt;class OI, class OB&gt; explicit C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text>(const CInterface&lt;OI, OB&gt; &amp; that)
    {
        attach(that.raw());
        if (SUCCEEDED(mRC))
        {
            mRC = that.lastRC();
            setErrorInfo(that.errorInfo());
        }
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- specialization for ourselves (copy constructor) -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>(const C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> &amp; that);&#x0A;&#x0A;</xsl:text>
  <!-- constructor taking a raw iface pointer -->
  <xsl:text>    template&lt;class OI&gt; explicit C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>(OI * aIface) { attach(aIface); }&#x0A;&#x0A;</xsl:text>
  <!-- specialization for ourselves -->
  <xsl:text>    explicit C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>(</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> * aIface);&#x0A;&#x0A;</xsl:text>
  <!-- assignment taking CWhatever -->
  <xsl:text>    template&lt;class OI, class OB&gt; C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> &amp; operator=(const CInterface&lt;OI, OB&gt; &amp; that)
    {
        attach(that.raw());
        if (SUCCEEDED(mRC))
        {
            mRC = that.lastRC();
            setErrorInfo(that.errorInfo());
        }
        return *this;
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- specialization for ourselves -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> &amp; operator=(const C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> &amp; that);&#x0A;</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- assignment taking a raw iface pointer -->
  <xsl:text>    template&lt;class OI&gt; C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> &amp; operator=(OI * aIface)
    {
        attach(aIface);
        return *this;
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- specialization for ourselves -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> &amp; operator=(</xsl:text>
  <xsl:value-of select="@name"/>
<xsl:text> * aIface);&#x0A;</xsl:text>
  <xsl:text>&#x0A;</xsl:text>

  <xsl:text>#ifdef VBOX_WITH_LESS_VIRTUALBOX_INCLUDING&#x0A;</xsl:text>
  <xsl:text>const IID &amp;getIID() const RT_OVERRIDE;&#x0A;</xsl:text>
  <xsl:text>#endif&#x0A;&#x0A;</xsl:text>

  <xsl:text>    /* Attributes (properties): */&#x0A;</xsl:text>
  <xsl:call-template name="declareAttributes">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>

  <xsl:text>    /* Methods: */&#x0A;</xsl:text>
  <xsl:call-template name="declareMethods">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>

  <xsl:if test="@name='IVirtualBox' or @name='IMachine'">
    <xsl:text>    /* ExtraData helpers: */&#x0A;</xsl:text>
    <xsl:call-template name="declareExtraDataHelpers">
      <xsl:with-param name="iface" select="."/>
    </xsl:call-template>
  </xsl:if>

  <xsl:text>    /* Friend wrappers: */&#x0A;</xsl:text>
  <xsl:text>    friend class CUnknown;&#x0A;</xsl:text>
  <xsl:variable name="name" select="@name"/>
  <xsl:variable name="parent" select=".."/>
  <!-- for definitions inside <if> -->
  <xsl:if test="name(..)='if'">
    <xsl:for-each select="
      preceding-sibling::*[self::interface] |
      following-sibling::*[self::interface] |
      ../preceding-sibling::*[self::interface] |
      ../following-sibling::*[self::interface] |
      ../preceding-sibling::if[@target=$parent/@target]/*[self::interface] |
      ../following-sibling::if[@target=$parent/@target]/*[self::interface]
    ">
      <xsl:if test="
        ((name()='interface')
         and
         ((name(..)!='if' and (if[@target=$parent/@target]/method/param[@type=$name]
                               or
                               if[@target=$parent/@target]/attribute[@type=$name]))
          or
          (.//method/param[@type=$name] or attribute[@type=$name])))
      ">
        <xsl:text>    friend class C</xsl:text>
        <xsl:value-of select="substring(@name,2)"/>
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:if>
  <!-- for definitions outside <if> (i.e. inside <application>) -->
  <xsl:if test="name(..)!='if'">
    <xsl:for-each select="
      preceding-sibling::*[self::interface] |
      following-sibling::*[self::interface] |
      preceding-sibling::if/*[self::interface] |
      following-sibling::if/*[self::interface]
    ">
      <xsl:if test="
        name()='interface' and (.//method/param[@type=$name] or attribute[@type=$name])
      ">
        <xsl:text>    friend class C</xsl:text>
        <xsl:value-of select="substring(@name,2)"/>
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:if>

</xsl:template>

<!-- attribute declarations -->
<xsl:template match="interface//attribute" mode="declare">
  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod">
    <xsl:with-param name="return" select="."/>
  </xsl:call-template>
  <xsl:if test="not(@readonly='yes')">
    <xsl:call-template name="composeMethod">
      <xsl:with-param name="return" select="''"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
</xsl:template>

<!-- method declarations -->
<xsl:template match="interface//method" mode="declare">
  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod"/>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
</xsl:template>


<!--
 *  interface includes
-->
<xsl:template match="interface" mode="include">

  <xsl:text>#include "C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>.h"&#x0A;</xsl:text>
</xsl:template>


<!--
 *  interface definitions
-->
<xsl:template match="interface" mode="define">

  <xsl:text>// </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> wrapper&#x0A;</xsl:text>
  <xsl:call-template name="xsltprocNewlineOutputHack"/>

  <xsl:if test="name()='interface'">
    <xsl:call-template name="defineMembers"/>
  </xsl:if>

</xsl:template>

<xsl:template name="defineConstructors">

  <!-- default constructor -->
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>() {}&#x0A;&#x0A;</xsl:text>

  <!-- default destructor -->
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::~C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>() {}&#x0A;&#x0A;</xsl:text>

  <!-- copy constructor -->
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>(const C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> &amp;that) : Base(that) {}&#x0A;&#x0A;</xsl:text>

  <!-- copy constructor taking interface pointer -->
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>(</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> *pIface) : Base(pIface) {}&#x0A;&#x0A;</xsl:text>

  <!-- operator= -->
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>&amp; </xsl:text>
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::operator=(const C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> &amp;that)
{
    Base::operator=(that);
    return *this;
}
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>

  <!-- operator= taking interface pointer -->
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>&amp; </xsl:text>
  <xsl:text>C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::operator=(</xsl:text>
  <xsl:value-of select="@name"/>
<xsl:text> *pIface)
{
    Base::operator=(pIface);
    return *this;
}
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>

</xsl:template>

<xsl:template name="defineIIDGetter">
  <xsl:text>#ifdef VBOX_WITH_LESS_VIRTUALBOX_INCLUDING&#x0A;</xsl:text>
  <xsl:text>const IID &amp;C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::getIID() const&#x0A;</xsl:text>
  <xsl:text>{&#x0A;</xsl:text>
  <xsl:text>    return COM_IIDOF(</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>);&#x0A;</xsl:text>
  <xsl:text>}&#x0A;</xsl:text>
  <xsl:text>#endif&#x0A;&#x0A;</xsl:text>

</xsl:template>

<xsl:template name="defineAttributes">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//attribute[not(@internal='yes')]" mode="define">
    <xsl:with-param name="namespace" select="."/>
  </xsl:apply-templates>

  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within application/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="defineAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within application -->
      <xsl:otherwise>
        <xsl:call-template name="defineAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="defineMethods">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//method[not(@internal='yes')]" mode="define">
    <xsl:with-param name="namespace" select="."/>
  </xsl:apply-templates>

  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within application/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="defineMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within application -->
      <xsl:otherwise>
        <xsl:call-template name="defineMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="defineExtraDataHelpers">

  <xsl:param name="iface"/>

  <xsl:text>void C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::SetExtraDataBool(const QString &amp;strKey, bool fValue)</xsl:text>
<xsl:text>
{
    SetExtraData(strKey, fValue == true ? "true" : "false");
}

</xsl:text>

  <xsl:text>bool C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::GetExtraDataBool(const QString &amp;strKey, bool fDef /* = true */)</xsl:text>
<xsl:text>
{
    bool fResult = fDef;
    QString value = GetExtraData(strKey);
    if (   value == "true"
        || value == "on"
        || value == "yes")
        fResult = true;
    else if (   value == "false"
             || value == "off"
             || value == "no")
             fResult = false;
    return fResult;
}

</xsl:text>

  <xsl:text>void C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::SetExtraDataInt(const QString &amp;strKey, int value)</xsl:text>
<xsl:text>
{
    SetExtraData(strKey, QString::number(value));
}

</xsl:text>

  <xsl:text>int C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::GetExtraDataInt(const QString &amp;strKey, int def /* = 0 */)</xsl:text>
<xsl:text>
{
    QString value = GetExtraData(strKey);
    bool fOk;
    int result = value.toInt(&amp;fOk);
    if (fOk)
        return result;
    return def;
}

</xsl:text>

  <xsl:text>void C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::SetExtraDataRect(const QString &amp;strKey, const QRect &amp;value)</xsl:text>
<xsl:text>
{
    SetExtraData(strKey, QString("%1,%2,%3,%4")
                         .arg(value.x())
                         .arg(value.y())
                         .arg(value.width())
                         .arg(value.height()));
}

</xsl:text>

  <xsl:text>QRect C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::GetExtraDataRect(const QString &amp;strKey, const QRect &amp;def /* = QRect() */)</xsl:text>
<xsl:text>
{
    QRect result = def;
    QList&lt;int&gt; intList = GetExtraDataIntList(strKey);
    if (intList.size() == 4)
    {
        result.setRect(intList.at(0),
                       intList.at(1),
                       intList.at(2),
                       intList.at(3));
    }
    return result;
}

</xsl:text>

  <xsl:text>void C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::SetExtraDataStringList(const QString &amp;strKey, const QStringList &amp;value)</xsl:text>
<xsl:text>
{
    SetExtraData(strKey, value.join(","));
}

</xsl:text>

  <xsl:text>QStringList C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::GetExtraDataStringList(const QString &amp;strKey, QStringList def /* = QStringList() */)</xsl:text>
<xsl:text>
{
    QString strValue = GetExtraData(strKey);
    if (strValue.isEmpty())
        return def;
    else
        return strValue.split(",");
}

</xsl:text>

  <xsl:text>void C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::SetExtraDataIntList(const QString &amp;strKey, const QList&lt;int&gt; &amp;value)</xsl:text>
<xsl:text>
{
    QStringList strList;
    for (int i=0; i &lt; value.size(); ++i)
        strList &lt;&lt; QString::number(value.at(i));
    SetExtraDataStringList(strKey, strList);
}

</xsl:text>

  <xsl:text>QList&lt;int&gt; C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text>::GetExtraDataIntList(const QString &amp;strKey, QList&lt;int&gt; def /* = QList&lt;int&gt;() */)</xsl:text>
<xsl:text>
{
    QStringList strList = GetExtraDataStringList(strKey);
    if (strList.size() > 0)
    {
        QList&lt;int&gt; intList;
        bool fOk;
        for (int i=0; i &lt; strList.size(); ++i)
        {
            intList &lt;&lt; strList.at(i).toInt(&amp;fOk);
            if (!fOk)
                return def;
        }
        return intList;
    }
    return def;
}

</xsl:text>

</xsl:template>

<xsl:template name="defineMembers">
  <xsl:call-template name="defineConstructors">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>
  <xsl:call-template name="defineIIDGetter">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>
  <xsl:call-template name="defineAttributes">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>
  <xsl:call-template name="defineMethods">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>
  <xsl:if test="@name='IVirtualBox' or @name='IMachine'">
    <xsl:text>/* ExtraData helpers: */&#x0A;</xsl:text>
    <xsl:call-template name="defineExtraDataHelpers">
      <xsl:with-param name="iface" select="."/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!-- attribute definitions -->
<xsl:template match="interface//attribute" mode="define">

  <xsl:param name="namespace" select="ancestor::interface[1]"/>

  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod">
    <xsl:with-param name="return" select="."/>
    <xsl:with-param name="define" select="'yes'"/>
    <xsl:with-param name="namespace" select="$namespace"/>
  </xsl:call-template>
  <xsl:if test="not(@readonly='yes')">
    <xsl:call-template name="composeMethod">
      <xsl:with-param name="return" select="''"/>
      <xsl:with-param name="define" select="'yes'"/>
      <xsl:with-param name="namespace" select="$namespace"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
  <xsl:text>&#x0A;</xsl:text>

</xsl:template>

<!-- method definitions -->
<xsl:template match="interface//method" mode="define">

  <xsl:param name="namespace" select="ancestor::interface[1]"/>

  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod">
    <xsl:with-param name="define" select="'yes'"/>
    <xsl:with-param name="namespace" select="$namespace"/>
  </xsl:call-template>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
  <xsl:text>&#x0A;</xsl:text>

</xsl:template>


<!--
 *  co-classes
-->
<xsl:template match="module/class"/>


<!--
 *  enums
-->
<xsl:template match="enum"/>


<!--
 *  base template to produce interface methods
 *
 *  @param return
 *      - in <attribute> context, must be '.' for getters and
 *        '' for setters
 *      - in <method> context, must not be specified (the default value
 *        will apply)
 *  @param define
 *      'yes' to produce inlined definition outside the class
 *      declaration, or
 *      empty string to produce method declaration only (w/o body)
 *  @param namespace
 *      actual interface node for which this method is being defined
 *      (necessary to properly set a class name for inherited methods).
 *      If not specified, will default to the parent interface
 *      node of the method being defined.
-->
<xsl:template name="composeMethod">
  <xsl:param name="return" select="param[@dir='return']"/>
  <xsl:param name="define" select="''"/>
  <xsl:param name="namespace" select="ancestor::interface[1]"/>
  <xsl:choose>
    <!-- no return value -->
    <xsl:when test="not($return)">
      <xsl:choose>
        <xsl:when test="$define">
          <xsl:text></xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>    </xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>void </xsl:text>
      <xsl:if test="$define">
        <xsl:text>C</xsl:text>
        <xsl:value-of select="substring($namespace/@name,2)"/>
        <xsl:text>::</xsl:text>
      </xsl:if>
      <xsl:call-template name="composeMethodDecl">
        <xsl:with-param name="isSetter" select="'yes'"/>
      </xsl:call-template>
      <xsl:if test="$define">
        <xsl:text>&#x0A;{&#x0A;</xsl:text>
        <!-- iface assertion -->
        <xsl:text>    AssertReturnVoid(ptr());&#x0A;</xsl:text>
        <!-- method call -->
        <xsl:call-template name="composeMethodCall">
          <xsl:with-param name="isSetter" select="'yes'"/>
        </xsl:call-template>
        <xsl:text>}&#x0A;</xsl:text>
      </xsl:if>
      <xsl:if test="not($define)">
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:when>
    <!-- has a return value -->
    <xsl:when test="count($return) = 1">
      <xsl:choose>
        <xsl:when test="$define">
          <xsl:text></xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>    </xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="$return/@type"/>
      <xsl:text> </xsl:text>
      <xsl:if test="$define">
        <xsl:text>C</xsl:text>
        <xsl:value-of select="substring($namespace/@name,2)"/>
        <xsl:text>::</xsl:text>
      </xsl:if>
      <xsl:call-template name="composeMethodDecl"/>
      <xsl:if test="$define">
        <xsl:text>&#x0A;{&#x0A;    </xsl:text>
        <xsl:apply-templates select="$return/@type"/>
        <xsl:text> a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="$return/@name"/>
        </xsl:call-template>
        <xsl:apply-templates select="$return/@type" mode="initializer"/>
        <xsl:text>;&#x0A;</xsl:text>
        <!-- iface assertion -->
        <xsl:text>    AssertReturn(ptr(), a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="$return/@name"/>
        </xsl:call-template>
        <xsl:text>);&#x0A;</xsl:text>
        <!-- method call -->
        <xsl:call-template name="composeMethodCall"/>
        <!-- return statement -->
        <xsl:text>    return a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="$return/@name"/>
        </xsl:call-template>
        <xsl:text>;&#x0A;}&#x0A;</xsl:text>
      </xsl:if>
      <xsl:if test="not($define)">
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:when>
    <!-- otherwise error -->
    <xsl:otherwise>
      <xsl:message terminate="yes">
        <xsl:text>More than one return value in method: </xsl:text>
        <xsl:value-of select="$namespace/@name"/>
        <xsl:text>::</xsl:text>
        <xsl:value-of select="@name"/>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeMethodDecl">
  <xsl:param name="isSetter" select="''"/>
  <xsl:choose>
    <!-- attribute method call -->
    <xsl:when test="name()='attribute'">
      <xsl:choose>
        <xsl:when test="$isSetter">
          <!-- name -->
          <xsl:text>Set</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>(</xsl:text>
          <!-- parameter -->
          <xsl:apply-templates select="@type" mode="param"/>
          <xsl:text> a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <!-- name -->
          <xsl:text>Get</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>(</xsl:text>
          <!-- const method -->
          <xsl:text>) const</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- regular method call -->
    <xsl:when test="name()='method'">
      <!-- name -->
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
      <xsl:text>(</xsl:text>
      <!-- parameters -->
      <xsl:for-each select="param[@dir!='return']">
        <xsl:apply-templates select="@type" mode="param"/>
        <xsl:text> a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="@name"/>
        </xsl:call-template>
        <xsl:if test="position() != last()">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
      <!-- const method -->
      <xsl:if test="@const='yes'"> const</xsl:if>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeMethodCall">
  <xsl:param name="isSetter" select="''"/>
  <!-- apply 'pre-call' hooks -->
  <xsl:choose>
    <xsl:when test="name()='attribute'">
      <xsl:call-template name="hooks">
        <xsl:with-param name="when" select="'pre-call'"/>
        <xsl:with-param name="isSetter" select="$isSetter"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name()='method'">
      <xsl:for-each select="param">
        <xsl:call-template name="hooks">
          <xsl:with-param name="when" select="'pre-call'"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>
  <!-- start the call -->
  <xsl:text>    mRC = ptr()-></xsl:text>
  <xsl:choose>
    <!-- attribute method call -->
    <xsl:when test="name()='attribute'">
      <!-- method name -->
      <xsl:choose>
        <xsl:when test="$isSetter">
          <xsl:text>COMSETTER(</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>COMGETTER(</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
      <xsl:text>)(</xsl:text>
      <!-- parameter -->
      <xsl:call-template name="composeMethodCallParam">
        <xsl:with-param name="isIn" select="$isSetter"/>
        <xsl:with-param name="isOut" select="not($isSetter)"/>
      </xsl:call-template>
    </xsl:when>
    <!-- regular method call -->
    <xsl:when test="name()='method'">
      <!-- method name -->
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
      <xsl:text>(</xsl:text>
      <!-- parameters -->
      <xsl:for-each select="param">
        <xsl:call-template name="composeMethodCallParam"/>
        <xsl:if test="position() != last()">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>
  <xsl:text>);&#x0A;</xsl:text>

  <xsl:text>#ifdef RT_OS_WINDOWS&#x0A;</xsl:text>
  <xsl:text>    Assert(mRC != RPC_E_WRONG_THREAD);&#x0A;</xsl:text>
  <xsl:text>    Assert(mRC != CO_E_NOTINITIALIZED);&#x0A;</xsl:text>
  <xsl:text>    Assert(mRC != RPC_E_CANTCALLOUT_ININPUTSYNCCALL);&#x0A;</xsl:text>
  <xsl:text>#endif&#x0A;</xsl:text>

  <!-- apply 'post-call' hooks -->
  <xsl:choose>
    <xsl:when test="name()='attribute'">
      <xsl:call-template name="hooks">
        <xsl:with-param name="when" select="'post-call'"/>
        <xsl:with-param name="isSetter" select="$isSetter"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name()='method'">
      <xsl:for-each select="param">
        <xsl:call-template name="hooks">
          <xsl:with-param name="when" select="'post-call'"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>
  <!-- -->
  <xsl:call-template name="tryComposeFetchErrorInfo"/>
</xsl:template>

<!--
 *  Composes a 'fetch error info' call or returns the name of the
 *  appropriate base class name that provides error info functionality
 *  (depending on the mode parameter). Does nothing if the current
 *  interface does not support error info.
 *
 *  @param mode
 *      - 'getBaseClassName': expands to the base class name
 *      - any other value: composes a 'fetch error info' method call
-->
<xsl:template name="tryComposeFetchErrorInfo">
  <xsl:param name="mode" select="''"/>

  <xsl:variable name="ifaceSupportsErrorInfo" select="
    ancestor-or-self::interface[1]/@supportsErrorInfo
  "/>
  <xsl:variable name="applicationSupportsErrorInfo" select="ancestor::application/@supportsErrorInfo"/>

  <xsl:choose>
    <xsl:when test="$ifaceSupportsErrorInfo">
      <xsl:call-template name="composeFetchErrorInfo">
        <xsl:with-param name="supports" select="string($ifaceSupportsErrorInfo)"/>
        <xsl:with-param name="mode" select="$mode"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$applicationSupportsErrorInfo">
      <xsl:call-template name="composeFetchErrorInfo">
        <xsl:with-param name="supports" select="string($applicationSupportsErrorInfo)"/>
        <xsl:with-param name="mode" select="$mode"/>
      </xsl:call-template>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeFetchErrorInfo">
  <xsl:param name="supports" select="''"/>
  <xsl:param name="mode" select="''"/>

  <xsl:choose>
    <xsl:when test="$mode='getBaseClassName'">
      <xsl:if test="$supports='strict' or $supports='yes'">
        <xsl:text>, COMBaseWithEI</xsl:text>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="$supports='strict' or $supports='yes'">
        <xsl:text>    if (RT_UNLIKELY(mRC != S_OK))&#x0A;    {&#x0A;</xsl:text>
        <xsl:text>        fetchErrorInfo(ptr(), &amp;COM_IIDOF(Base::Iface));&#x0A;</xsl:text>
        <xsl:if test="$supports='strict'">
          <xsl:text>        AssertMsg(errInfo.isFullAvailable(), </xsl:text>
          <xsl:text>("for RC=0x%08X\n", mRC));&#x0A;</xsl:text>
        </xsl:if>
        <xsl:text>    }&#x0A;</xsl:text>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeMethodCallParam">
  <xsl:param name="isIn" select="@dir='in'"/>
  <xsl:param name="isOut" select="@dir='out' or @dir='return'"/>

  <xsl:choose>
    <!-- safearrays -->
    <xsl:when test="@safearray='yes'">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>ComSafeArrayAsInParam(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>ComSafeArrayAsOutParam(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>)</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- string types -->
    <xsl:when test="@type = 'wstring'">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>BSTRIn(a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>BSTROut(a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- uuid is represented as string in the com -->
    <xsl:when test="@type = 'uuid'">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>GuidAsBStrIn(a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>GuidAsBStrOut(a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- enum types -->
    <xsl:when test="count(key('G_keyEnumsByName', current()/@type)) > 0">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>(</xsl:text>
          <xsl:value-of select="@type"/>
          <xsl:text>_T) a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>ENUMOut&lt;K</xsl:text>
          <xsl:value-of select="@type"/>
          <xsl:text>, </xsl:text>
          <xsl:value-of select="@type"/>
          <xsl:text>_T&gt;(a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- interface types -->
    <xsl:when test="@type='$unknown' or (count(key('G_keyInterfacesByName', current()/@type)) > 0)">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>.ptr()</xsl:text>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:value-of select="concat('&amp;', @name, 'Ptr')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- currently unsupported types -->
    <xsl:when test="@type = 'string'">
      <xsl:message terminate="yes">
        <xsl:text>Parameter type </xsl:text>
        <xsl:value-of select="@type"/>
        <xsl:text>is not currently supported</xsl:text>
      </xsl:message>
    </xsl:when>
    <!-- assuming scalar types -->
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>&amp;a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type conversion (returns plain Qt type name)
-->
<xsl:template match="attribute/@type | param/@type">
  <xsl:choose>
    <!-- modifiers -->
    <xsl:when test="name(current())='type' and ../@mod">
      <xsl:if test="../@safearray='yes' and ../@mod='ptr'">
        <xsl:message terminate="yes">
          <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
          <xsl:text>either 'safearray' or 'mod' attribute is allowed, but not both!</xsl:text>
        </xsl:message>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="../@mod='ptr'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='boolean'">BOOL *</xsl:when>
            <xsl:when test=".='octet'">BYTE *</xsl:when>
            <xsl:when test=".='short'">SHORT *</xsl:when>
            <xsl:when test=".='unsigned short'">USHORT *</xsl:when>
            <xsl:when test=".='long'">LONG *</xsl:when>
            <xsl:when test=".='long long'">LONG64 *</xsl:when>
            <xsl:when test=".='unsigned long'">ULONG *</xsl:when>
            <xsl:when test=".='unsigned long long'">ULONG64 *</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="../@mod='string'">
          <xsl:if test="../@safearray='yes'">
            <xsl:text>QVector&lt;</xsl:text>
          </xsl:if>
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='uuid'">QUuid</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:if test="../@safearray='yes'">
            <xsl:text>&gt;</xsl:text>
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
            <xsl:value-of select="concat('value &quot;',../@mod,'&quot; ')"/>
            <xsl:text>of attribute 'mod' is invalid!</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- no modifiers -->
    <xsl:otherwise>
      <xsl:if test="../@safearray='yes'">
        <xsl:text>QVector&lt;</xsl:text>
      </xsl:if>
      <xsl:choose>
        <!-- standard types -->
        <xsl:when test=".='result'">HRESULT</xsl:when>
        <xsl:when test=".='boolean'">BOOL</xsl:when>
        <xsl:when test=".='octet'">BYTE</xsl:when>
        <xsl:when test=".='short'">SHORT</xsl:when>
        <xsl:when test=".='unsigned short'">USHORT</xsl:when>
        <xsl:when test=".='long'">LONG</xsl:when>
        <xsl:when test=".='long long'">LONG64</xsl:when>
        <xsl:when test=".='unsigned long'">ULONG</xsl:when>
        <xsl:when test=".='unsigned long long'">ULONG64</xsl:when>
        <xsl:when test=".='char'">CHAR</xsl:when>
        <xsl:when test=".='string'">CHAR *</xsl:when>
        <xsl:when test=".='wchar'">OLECHAR</xsl:when>
        <xsl:when test=".='wstring'">QString</xsl:when>
        <!-- UUID type -->
        <xsl:when test=".='uuid'">QUuid</xsl:when>
        <!-- system interface types -->
        <xsl:when test=".='$unknown'">CUnknown</xsl:when>
        <!-- enum types -->
        <xsl:when test="count(key('G_keyEnumsByName', current())) > 0">
          <xsl:value-of select="concat('K',string(.))"/>
        </xsl:when>
        <!-- custom interface types -->
        <xsl:when test="count(key('G_keyInterfacesByName', current())) > 0">
          <xsl:value-of select="concat('C',substring(.,2))"/>
        </xsl:when>
        <!-- other types -->
        <xsl:otherwise>
          <xsl:message terminate="yes"><xsl:text>Unknown parameter type: </xsl:text><xsl:value-of select="."/></xsl:message>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="../@safearray='yes'">
        <xsl:text>&gt;</xsl:text>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  generates a null initializer for all scalar types (such as bool or long)
 *  and enum types in the form of ' = <null_initializer>', or nothing for other
 *  types.
-->
<xsl:template match="attribute/@type | param/@type" mode="initializer">
  <xsl:choose>
    <!-- safearrays don't need initializers -->
    <xsl:when test="../@safearray='yes'">
    </xsl:when>
    <!-- modifiers -->
    <xsl:when test="name(current())='type' and ../@mod">
      <xsl:choose>
        <xsl:when test="../@mod='ptr'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='boolean'"> = NULL</xsl:when>
            <xsl:when test=".='octet'"> = NULL</xsl:when>
            <xsl:when test=".='short'"> = NULL</xsl:when>
            <xsl:when test=".='unsigned short'"> = NULL</xsl:when>
            <xsl:when test=".='long'"> = NULL</xsl:when>
            <xsl:when test=".='long long'"> = NULL</xsl:when>
            <xsl:when test=".='unsigned long'"> = NULL</xsl:when>
            <xsl:when test=".='unsigned long long'"> = NULL</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="../@mod='string'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='uuid'"></xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
            <xsl:value-of select="concat('value &quot;',../@mod,'&quot; ')"/>
            <xsl:text>of attribute 'mod' is invalid!</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- no modifiers -->
    <xsl:otherwise>
      <xsl:choose>
        <!-- standard types that need a zero initializer -->
        <xsl:when test=".='result'"> = S_OK</xsl:when>
        <xsl:when test=".='boolean'"> = FALSE</xsl:when>
        <xsl:when test=".='octet'"> = 0</xsl:when>
        <xsl:when test=".='short'"> = 0</xsl:when>
        <xsl:when test=".='unsigned short'"> = 0</xsl:when>
        <xsl:when test=".='long'"> = 0</xsl:when>
        <xsl:when test=".='long long'"> = 0</xsl:when>
        <xsl:when test=".='unsigned long'"> = 0</xsl:when>
        <xsl:when test=".='unsigned long long'"> = 0</xsl:when>
        <xsl:when test=".='char'"> = 0</xsl:when>
        <xsl:when test=".='string'"> = NULL</xsl:when>
        <xsl:when test=".='wchar'"> = 0</xsl:when>
        <!-- enum types initialized with 0 -->
        <xsl:when test="count(key('G_keyEnumsByName', current())) > 0">
          <xsl:value-of select="concat(' = (K',string(.),') 0')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type conversion (for method declaration)
-->
<xsl:template match="attribute/@type | param/@type" mode="param">
  <xsl:choose>
    <!-- class types -->
    <xsl:when test="
         . = 'string'
      or . = 'wstring'
      or . = '$unknown'
      or ../@safearray = 'yes'
      or (count(key('G_keyEnumsByName',      current())) > 0)
      or (count(key('G_keyInterfacesByName', current())) > 0)
    ">
      <xsl:choose>
        <!-- <attribute> context -->
        <xsl:when test="name(..)='attribute'">
          <xsl:text>const </xsl:text>
          <xsl:apply-templates select="."/>
          <xsl:text> &amp;</xsl:text>
        </xsl:when>
        <!-- <param> context -->
        <xsl:when test="name(..)='param'">
          <xsl:choose>
            <xsl:when test="../@dir='in'">
              <xsl:text>const </xsl:text>
              <xsl:apply-templates select="."/>
              <xsl:text> &amp;</xsl:text>
            </xsl:when>
            <xsl:when test="../@dir='out'">
              <xsl:apply-templates select="."/>
              <xsl:text> &amp;</xsl:text>
            </xsl:when>
            <xsl:when test="../@dir='return'">
              <xsl:apply-templates select="."/>
            </xsl:when>
          </xsl:choose>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- assume scalar types -->
    <xsl:otherwise>
      <xsl:choose>
        <!-- <attribute> context -->
        <xsl:when test="name(..)='attribute'">
          <xsl:apply-templates select="."/>
        </xsl:when>
        <!-- <param> context -->
        <xsl:when test="name(..)='param'">
          <xsl:apply-templates select="."/>
          <xsl:if test="../@dir='out'">
            <xsl:text> &amp;</xsl:text>
          </xsl:if>
        </xsl:when>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type conversion (returns plain COM type name)
 *  (basically, copied from midl.xsl)
-->
<xsl:template match="attribute/@type | param/@type" mode="com">
  <xsl:choose>
    <!-- modifiers -->
    <xsl:when test="name(current())='type' and ../@mod">
      <xsl:choose>
        <xsl:when test="../@mod='ptr'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='boolean'">BOOL *</xsl:when>
            <xsl:when test=".='octet'">BYTE *</xsl:when>
            <xsl:when test=".='short'">SHORT *</xsl:when>
            <xsl:when test=".='unsigned short'">USHORT *</xsl:when>
            <xsl:when test=".='long'">LONG *</xsl:when>
            <xsl:when test=".='long long'">LONG64 *</xsl:when>
            <xsl:when test=".='unsigned long'">ULONG *</xsl:when>
            <xsl:when test=".='unsigned long long'">ULONG64 *</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="../@mod='string'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='uuid'">BSTR</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
            <xsl:value-of select="concat('value &quot;',../@mod,'&quot; ')"/>
            <xsl:text>of attribute 'mod' is invalid!</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- no modifiers -->
    <xsl:otherwise>
      <xsl:choose>
        <!-- standard types -->
        <xsl:when test=".='result'">HRESULT</xsl:when>
        <xsl:when test=".='boolean'">BOOL</xsl:when>
        <xsl:when test=".='octet'">BYTE</xsl:when>
        <xsl:when test=".='short'">SHORT</xsl:when>
        <xsl:when test=".='unsigned short'">USHORT</xsl:when>
        <xsl:when test=".='long'">LONG</xsl:when>
        <xsl:when test=".='long long'">LONG64</xsl:when>
        <xsl:when test=".='unsigned long'">ULONG</xsl:when>
        <xsl:when test=".='unsigned long long'">ULONG64</xsl:when>
        <xsl:when test=".='char'">CHAR</xsl:when>
        <xsl:when test=".='string'">CHAR *</xsl:when>
        <xsl:when test=".='wchar'">OLECHAR</xsl:when>
        <xsl:when test=".='wstring'">BSTR</xsl:when>
        <!-- UUID type -->
        <xsl:when test=".='uuid'">GUID</xsl:when>
        <!-- system interface types -->
        <xsl:when test=".='$unknown'">IUnknown *</xsl:when>
        <!-- enum types -->
        <xsl:when test="count(key('G_keyEnumsByName', current())) > 0">
          <xsl:value-of select="."/>
        </xsl:when>
        <!-- custom interface types -->
        <xsl:when test="count(key('G_keyInterfacesByName', current())) > 0">
          <xsl:value-of select="."/><xsl:text> *</xsl:text>
        </xsl:when>
        <!-- other types -->
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:text>Unknown parameter type: </xsl:text>
            <xsl:value-of select="."/>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type additional hooks.
 *
 *  Called in the context of <attribute> or <param> elements.
 *
 *  @param when     When the hook is being called:
 *                  'pre-call'  - right before the method call
 *                  'post-call' - right after the method call
 *  @param isSetter Non-empty if called in the cotext of the attribute setter
 *                  call.
-->
<xsl:template name="hooks">
  <xsl:param name="when" select="''"/>
  <xsl:param name="isSetter" select="''"/>

  <xsl:variable name="is_iface" select="count(key('G_keyInterfacesByName', current()/@type)) > 0"/>
  <xsl:variable name="is_out" select="(
      (name()='attribute' and not($isSetter)) or
      (name()='param' and (@dir='out' or @dir='return'))
   )"/>

  <xsl:choose>
    <xsl:when test="$when='pre-call'">
      <xsl:variable name="is_enum" select="count(key('G_keyEnumsByName', current()/@type)) > 0"/>
      <xsl:choose>
        <xsl:when test="@safearray='yes'">
          <!-- declare a SafeArray variable -->
          <xsl:choose>
            <!-- interface types need special treatment here -->
            <xsl:when test="@type='$unknown'">
              <xsl:text>    com::SafeIfaceArray &lt;IUnknown&gt; </xsl:text>
            </xsl:when>
            <xsl:when test="$is_iface">
              <xsl:text>    com::SafeIfaceArray &lt;</xsl:text>
              <xsl:value-of select="@type"/>
              <xsl:text>&gt; </xsl:text>
            </xsl:when>
            <!-- enums need the _T prefix -->
            <xsl:when test="$is_enum">
              <xsl:text>    com::SafeArray &lt;</xsl:text>
              <xsl:value-of select="@type"/>
              <xsl:text>_T&gt; </xsl:text>
            </xsl:when>
            <!-- GUID is special too -->
            <xsl:when test="@type='uuid' and @mod!='string'">
              <xsl:text>    com::SafeGUIDArray </xsl:text>
            </xsl:when>
            <!-- everything else is not -->
            <xsl:otherwise>
              <xsl:text>    com::SafeArray &lt;</xsl:text>
              <xsl:apply-templates select="@type" mode="com"/>
              <xsl:text>&gt; </xsl:text>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:value-of select="@name"/>
          <xsl:text>;&#x0A;</xsl:text>
          <xsl:if test="(name()='attribute' and $isSetter) or
                        (name()='param' and @dir='in')">
            <!-- convert QVector to SafeArray -->
            <xsl:choose>
              <!-- interface types need special treatment here -->
              <xsl:when test="@type='$unknown' or $is_iface">
                <xsl:text>    ToSafeIfaceArray(</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:text>    ToSafeArray(</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
              <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>);&#x0A;</xsl:text>
          </xsl:if>
        </xsl:when>
        <xsl:when test="$is_out and ($is_iface or (@type='$unknown'))">
          <xsl:text>    </xsl:text>
          <xsl:choose>
            <xsl:when test="@type='$unknown'">
              <xsl:text>IUnknown</xsl:text>
              </xsl:when>
             <xsl:otherwise>
                <xsl:value-of select="@type"/>
             </xsl:otherwise>
           </xsl:choose>
           <xsl:value-of select="concat('* ',@name,'Ptr = NULL;&#10;')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$when='post-call'">
      <xsl:choose>
        <xsl:when test="@safearray='yes'">
          <xsl:if test="$is_out">
            <!-- convert SafeArray to QVector -->
            <xsl:choose>
              <!-- interface types need special treatment here -->
              <xsl:when test="@type='$unknown' or $is_iface">
                <xsl:text>    FromSafeIfaceArray(</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:text>    FromSafeArray(</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:value-of select="@name"/>
            <xsl:text>, </xsl:text>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
              <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:text>);&#x0A;</xsl:text>
          </xsl:if>
        </xsl:when>
        <xsl:when test="$is_out and ($is_iface or (@type='$unknown'))">
            <xsl:text>    a</xsl:text>
            <xsl:call-template name="capitalize">
              <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
           <xsl:value-of select="concat('.setPtr(',@name,'Ptr);&#10;')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
        <xsl:text>Invalid when value: </xsl:text>
        <xsl:value-of select="$when"/>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>


</xsl:stylesheet>

