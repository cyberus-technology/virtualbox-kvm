<?xml version="1.0"?>

<!--
 *  A template to generate a generic IDL file from the generic interface
 *  definition expressed in XML. The generated file is intended solely to
 *  generate the documentation using Doxygen.
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
  exclude-result-prefixes="str"
>
<!-- The exclude-result-prefixes attribute is needed since otherwise the <tt>
  tag added for the ID in interface and enum descriptions would get the
  namespace, confusing doxygen completely. -->

<xsl:import href="string.xsl"/>

<!-- Don't indent the output, as it's not exactly html but IDL.
     (doxygen 1.9.6 gets confused by <dl> indent.) -->
<xsl:output method="html" indent="no"/>

<xsl:strip-space elements="*"/>


<!--
//  Doxygen transformation rules
/////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  all text elements that are not explicitly matched are normalized
 *  (all whitespace chars are converted to single spaces)
-->
<!--xsl:template match="desc//text()">
    <xsl:value-of select="concat(' ',normalize-space(.),' ')"/>
</xsl:template-->

<!--
  Replace /* and */ sequences in the text so they won't confuse doxygen with
  comment nesting (see IPerformanceCollector).  Doxygen doesn't have any escape
  sequence for '/' nor for '*', and xsltproc is in html mode so we cannot easily
  output dummy elements.  So, we replace the '*' with '@SLASH-ASTERISK@' and
  '@ASTERISK-SLASH@' and run sed afterwards to change them to sequences with
  a dummy 'b' element in-between the characters (&#42; does not work).

  TODO: Find better fix for this.

  ~~Also, strip leading whitespace from the first child of a 'desc' element so
  that doxygen 1.9.6 doesn't confuse the text for a tt or pre block (older
  versions (1.8.13) didn't used to do this).~~ - fixed by MARKDOWN_SUPPORT=NO.
  -->
<xsl:template match="desc//text()" name="default-text-processing">
  <xsl:param name="text" select="."/>

  <!-- xsl:variable name="stripped">
    <xsl:choose>
      <xsl:when test="parent::desc and position() = 1">
        <xsl:call-template name="strip-left">
          <xsl:with-param name="text" select="$text"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable -->

  <xsl:variable name="subst1">
    <xsl:call-template name="str:subst">
      <!-- xsl:with-param name="text" select="$stripped" / -->
      <xsl:with-param name="text" select="$text" />
      <xsl:with-param name="replace" select="'/*'" />
      <xsl:with-param name="with" select="'/@SLASH-ASTERISK@'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="subst2">
    <xsl:call-template name="str:subst">
      <xsl:with-param name="text" select="$subst1" />
      <xsl:with-param name="replace" select="'*/'" />
      <xsl:with-param name="with" select="'@ASTERISK-SLASH@/'" />
    </xsl:call-template>
  </xsl:variable>

  <!-- xsl:value-of select="concat('-dbg-',position(),'-gbd-')"/ -->
  <xsl:value-of select="$subst2"/>
</xsl:template>

<!-- Strips leading spaces from $text.  Helper for default-text-processing.  -->
<xsl:template name="strip-left">
  <xsl:param name="text"/>
  <xsl:choose>
    <xsl:when test="string-length($text) > 0 and (substring($text, 1, 1) = ' ' or substring($text, 1, 1) = '&#x0A;' or substring($text, 1, 1) = '&#x0D;')">
      <xsl:call-template name="strip-left">
        <xsl:with-param name="text" select="substring($text, 2)"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:otherwise>
      <xsl:value-of select="$text"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  all elements that are not explicitly matched are considered to be html tags
 *  and copied w/o modifications
-->
<xsl:template match="desc//*">
  <xsl:copy>
    <xsl:apply-templates/>
  </xsl:copy>
</xsl:template>

<!--
 * special treatment of <tt>, making sure that Doxygen will not interpret its
 * contents (assuming it is a leaf tag)
-->
<xsl:template match="desc//tt/text()">
  <xsl:variable name="subst1">
    <xsl:call-template name="str:subst">
      <xsl:with-param name="text" select="." />
      <xsl:with-param name="replace" select="'::'" />
      <xsl:with-param name="with" select="'\::'" />
    </xsl:call-template>
  </xsl:variable>
  <xsl:call-template name="default-text-processing">
    <xsl:with-param name="text" select="$subst1"/>
  </xsl:call-template>
</xsl:template>

<!--
 * same like desc//* but place <ol> at start of line otherwise Doxygen will not
 * find it
-->
<xsl:template match="desc//ol">
  <xsl:text>&#x0A;</xsl:text>
  <xsl:copy>
    <xsl:apply-templates/>
  </xsl:copy>
</xsl:template>

<!--
 * same like desc//* but place <ul> at start of line otherwise Doxygen will not
 * find it
-->
<xsl:template match="desc//ul">
  <xsl:text>&#x0A;</xsl:text>
  <xsl:copy>
    <xsl:apply-templates/>
  </xsl:copy>
</xsl:template>

<!--
 * same like desc//* but place <pre> at start of line otherwise Doxygen will not
 * find it
-->
<xsl:template match="desc//pre">
  <xsl:text>&#x0A;</xsl:text>
  <xsl:copy>
    <xsl:apply-templates/>
  </xsl:copy>
</xsl:template>

<!--
 *  paragraph
-->
<xsl:template match="desc//p">
  <xsl:text>&#x0A;</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>&#x0A;</xsl:text>
</xsl:template>

<!--
 *  link
-->
<xsl:template match="desc//link">
  <xsl:text>@link </xsl:text>
  <!--
   *  sometimes Doxygen is stupid and cannot resolve global enums properly,
   *  thinking they are members of the current class. Fix it by adding ::
   *  in front of any @to value that doesn't start with #.
  -->
  <xsl:choose>
    <xsl:when test="not(starts-with(@to, '#')) and not(contains(@to, '::'))">
      <xsl:text>::</xsl:text>
    </xsl:when>
  </xsl:choose>
  <!--
   *  Doxygen doesn't understand autolinks like Class::func() if Class
   *  doesn't actually contain a func with no arguments. Fix it.
  -->
  <xsl:choose>
    <xsl:when test="substring(@to, string-length(@to)-1)='()'">
      <xsl:value-of select="substring-before(@to, '()')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="@to"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:text> </xsl:text>
  <xsl:choose>
    <xsl:when test="normalize-space(text())">
      <xsl:value-of select="normalize-space(text())"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="starts-with(@to, '#')">
          <xsl:value-of select="substring-after(@to, '#')"/>
        </xsl:when>
        <xsl:when test="starts-with(@to, '::')">
          <xsl:value-of select="substring-after(@to, '::')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="@to"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:text>@endlink</xsl:text>
  <!--
   *  insert a dummy empty B element to distinctly separate @endlink
   *  from the following text
   -->
  <xsl:element name="b"/>
</xsl:template>

<!--
 *  note
-->
<xsl:template match="desc/note">
  <xsl:if test="not(@internal='yes')">
    <xsl:text>&#x0A;@note </xsl:text>
    <xsl:apply-templates/>
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>

<!--
 *  see
-->
<xsl:template match="desc/see">
  <xsl:text>&#x0A;@see </xsl:text>
  <xsl:apply-templates/>
  <xsl:text>&#x0A;</xsl:text>
</xsl:template>


<!--
 *  common comment prologue (handles group IDs)
-->
<xsl:template match="desc" mode="begin">
  <xsl:param name="id" select="@group | preceding::descGroup[1]/@id"/>
  <xsl:text>/**&#x0A;</xsl:text>
  <xsl:if test="$id">
    <xsl:value-of select="concat(' @ingroup ',$id,'&#x0A;')"/>
  </xsl:if>
</xsl:template>

<!--
 *  common brief comment prologue (handles group IDs)
-->
<xsl:template match="desc" mode="begin_brief">
  <xsl:param name="id" select="@group | preceding::descGroup[1]/@id"/>
  <xsl:text>/**&#x0A;</xsl:text>
  <xsl:if test="$id">
    <xsl:value-of select="concat(' @ingroup ',$id,'&#x0A;')"/>
  </xsl:if>
  <xsl:text> @brief </xsl:text>
</xsl:template>

<!--
 *  common middle part of the comment block
-->
<xsl:template match="desc" mode="middle">
  <xsl:apply-templates select="text() | *[not(self::note or self::see)]"/>
  <xsl:apply-templates select="note"/>
  <xsl:apply-templates select="see"/>
</xsl:template>

<!--
 *  result part of the comment block
-->
<xsl:template match="desc" mode="results">
  <xsl:if test="result">
    <xsl:text>
@par Expected result codes:
    </xsl:text>
      <table>
    <xsl:for-each select="result">
      <tr>
        <xsl:choose>
          <xsl:when test="ancestor::library/result[@name=current()/@name]">
            <td><xsl:value-of select=
                  "concat('@link ::',@name,' ',@name,' @endlink')"/></td>
          </xsl:when>
          <xsl:otherwise>
            <td><xsl:value-of select="@name"/></td>
          </xsl:otherwise>
        </xsl:choose>
        <td>
          <xsl:apply-templates select="text() | *[not(self::note or self::see or
                                                  self::result)]"/>
        </td>
      </tr>
    </xsl:for-each>
      </table>
  </xsl:if>
</xsl:template>


<!--
 *  comment for interfaces
-->
<xsl:template match="interface/desc">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="." mode="middle"/>
@par Interface ID:
<tt>{<xsl:call-template name="str:to-upper">
    <xsl:with-param name="text" select="../@uuid"/>
  </xsl:call-template>}</tt>
  <xsl:text>&#x0A;*/&#x0A;</xsl:text>
</xsl:template>

<!--
 *  comment for attributes
-->
<xsl:template match="attribute/desc">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="text() | *[not(self::note or self::see or self::result)]"/>
  <xsl:apply-templates select="." mode="results"/>
  <xsl:apply-templates select="note"/>
  <xsl:if test="../@mod='ptr'">
    <xsl:text>

@warning This attribute is non-scriptable. In particular, this also means that an
attempt to get or set it from a process other than the process that has created and
owns the object will most likely fail or crash your application.
</xsl:text>
  </xsl:if>
  <xsl:apply-templates select="see"/>
  <xsl:text>&#x0A;*/&#x0A;</xsl:text>
</xsl:template>

<!--
 *  comment for methods
-->
<xsl:template match="method/desc">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="text() | *[not(self::note or self::see or self::result)]"/>
  <xsl:for-each select="../param">
    <xsl:apply-templates select="desc"/>
  </xsl:for-each>
  <xsl:apply-templates select="." mode="results"/>
  <xsl:apply-templates select="note"/>
  <xsl:apply-templates select="../param/desc/note"/>
  <xsl:if test="../param/@mod='ptr'">
    <xsl:text>

@warning This method is non-scriptable. In particular, this also means that an
attempt to call it from a process other than the process that has created and
owns the object will most likely fail or crash your application.
</xsl:text>
  </xsl:if>
  <xsl:apply-templates select="see"/>
  <xsl:text>&#x0A;*/&#x0A;</xsl:text>
</xsl:template>

<!--
 *  comment for method parameters
-->
<xsl:template match="method/param/desc">
  <xsl:text>&#x0A;@param </xsl:text>
  <xsl:value-of select="../@name"/>
  <xsl:text> </xsl:text>
  <xsl:apply-templates select="text() | *[not(self::note or self::see)]"/>
  <xsl:text>&#x0A;</xsl:text>
</xsl:template>

<!--
 *  comment for enums
-->
<xsl:template match="enum/desc">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="." mode="middle"/>
@par Interface ID:
<tt>{<xsl:call-template name="str:to-upper">
    <xsl:with-param name="text" select="../@uuid"/>
  </xsl:call-template>}</tt>
  <xsl:text>&#x0A;*/&#x0A;</xsl:text>
</xsl:template>

<!--
 *  comment for enum values
-->
<xsl:template match="enum/const/desc">
  <xsl:apply-templates select="." mode="begin_brief"/>
  <xsl:apply-templates select="." mode="middle"/>
  <xsl:text>&#x0A;*/&#x0A;</xsl:text>
</xsl:template>

<!--
 *  comment for result codes
-->
<xsl:template match="result/desc">
  <xsl:apply-templates select="." mode="begin_brief"/>
  <xsl:apply-templates select="." mode="middle"/>
  <xsl:text>&#x0A;*/&#x0A;</xsl:text>
</xsl:template>

<!--
 *  ignore descGroups by default (processed in /idl)
-->
<xsl:template match="descGroup"/>

<!--
//  templates
/////////////////////////////////////////////////////////////////////////////
-->


<!--
 *  header
-->
<xsl:template match="/idl">
/*
 *  DO NOT EDIT! This is a generated file.
 *
 *  Doxygen IDL definition for VirtualBox Main API (COM interfaces)
 *  generated from XIDL (XML interface definition).
 *
 *  Source    : src/VBox/Main/idl/VirtualBox.xidl
 *  Generator : src/VBox/Main/idl/doxygen.xsl
 *
 *  This IDL is generated using some generic OMG IDL-like syntax SOLELY
 *  for the purpose of generating the documentation using Doxygen and
 *  is not syntactically valid.
 *
 *  DO NOT USE THIS HEADER IN ANY OTHER WAY!
 */

  <!-- general description -->
  <xsl:text>/** @mainpage &#x0A;</xsl:text>
  <xsl:apply-templates select="desc" mode="middle"/>
  <xsl:text>&#x0A;*/&#x0A;</xsl:text>

  <!-- group (module) definitions -->
  <xsl:for-each select="//descGroup">
    <xsl:if test="@id and (@title or desc)">
      <xsl:value-of select="concat('/** @defgroup ', @id, ' ', @title, '&#x0A;')"/>
      <xsl:apply-templates select="desc" mode="middle"/>
      <xsl:text>&#x0A;*/&#x0A;</xsl:text>
    </xsl:if>
  </xsl:for-each>

  <!-- everything else -->
  <xsl:apply-templates select="*[not(self::desc)]"/>

</xsl:template>


<!--
 *  accept all <if>s
-->
<xsl:template match="if">
  <xsl:apply-templates/>
</xsl:template>


<!--
 *  cpp_quote (ignore)
-->
<xsl:template match="cpp">
</xsl:template>


<!--
 *  #ifdef statement (@if attribute)
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
 *  libraries
-->
<xsl:template match="application">
  <!-- result codes -->
  <xsl:for-each select="result">
    <xsl:apply-templates select="."/>
  </xsl:for-each>
  <!-- all enums go first -->
  <xsl:apply-templates select="enum | if/enum"/>
  <!-- everything else but result codes and enums -->
  <xsl:apply-templates select="*[not(self::result or self::enum) and
                                 not(self::if[result] or self::if[enum])]"/>
</xsl:template>


<!--
 *  result codes
-->
<xsl:template match="application//result">
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:apply-templates select="desc"/>
  <xsl:value-of select="concat('const HRESULT ',@name,' = ',@value,';')"/>
  <xsl:text>&#x0A;</xsl:text>
  <xsl:apply-templates select="@if" mode="end"/>
</xsl:template>


<!--
 *  interfaces
-->
<xsl:template match="interface">
  <xsl:apply-templates select="desc"/>
  <xsl:text>interface </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> : </xsl:text>
  <xsl:value-of select="@extends"/>
  <xsl:text>&#x0A;{&#x0A;</xsl:text>
  <!-- attributes (properties) -->
  <xsl:apply-templates select="attribute"/>
  <!-- methods -->
  <xsl:apply-templates select="method"/>
  <!-- 'if' enclosed elements, unsorted -->
  <xsl:apply-templates select="if"/>
  <!-- -->
  <xsl:text>}; /* interface </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> */&#x0A;&#x0A;</xsl:text>
</xsl:template>


<!--
 *  attributes
-->
<xsl:template match="interface//attribute">
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:apply-templates select="desc"/>
  <xsl:text>    </xsl:text>
  <xsl:if test="@readonly='yes'">
    <xsl:text>readonly </xsl:text>
  </xsl:if>
  <xsl:text>attribute </xsl:text>
  <xsl:apply-templates select="@type"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>;&#x0A;</xsl:text>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:text>&#x0A;</xsl:text>
</xsl:template>

<!--
 *  methods
-->
<xsl:template match="interface//method">
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:apply-templates select="desc"/>
  <xsl:text>    void </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:if test="param">
    <xsl:text> (&#x0A;</xsl:text>
    <xsl:for-each select="param [position() != last()]">
      <xsl:text>        </xsl:text>
      <xsl:apply-templates select="."/>
      <xsl:text>,&#x0A;</xsl:text>
    </xsl:for-each>
    <xsl:text>        </xsl:text>
    <xsl:apply-templates select="param [last()]"/>
    <xsl:text>&#x0A;    );&#x0A;</xsl:text>
  </xsl:if>
  <xsl:if test="not(param)">
    <xsl:text>();&#x0A;</xsl:text>
  </xsl:if>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:text>&#x0A;</xsl:text>
</xsl:template>


<!--
 *  co-classes
-->
<xsl:template match="module/class">
  <!-- class and contract id: later -->
  <!-- CLSID_xxx declarations for XPCOM, for compatibility with Win32: later -->
</xsl:template>


<!--
 *  enums
-->
<xsl:template match="enum">
  <xsl:apply-templates select="desc"/>
  <xsl:text>enum </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>&#x0A;{&#x0A;</xsl:text>
  <xsl:for-each select="const">
    <xsl:apply-templates select="desc"/>
    <xsl:text>    </xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>_</xsl:text>
    <xsl:value-of select="@name"/> = <xsl:value-of select="@value"/>
    <xsl:text>,&#x0A;</xsl:text>
  </xsl:for-each>
  <xsl:text>};&#x0A;&#x0A;</xsl:text>
</xsl:template>


<!--
 *  method parameters
-->
<xsl:template match="method/param">
  <xsl:choose>
    <xsl:when test="@dir='in'">in </xsl:when>
    <xsl:when test="@dir='out'">out </xsl:when>
    <xsl:when test="@dir='return'">[retval] out </xsl:when>
    <xsl:otherwise>in</xsl:otherwise>
  </xsl:choose>
  <xsl:apply-templates select="@type"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="@name"/>
</xsl:template>


<!--
 *  attribute/parameter type conversion
-->
<xsl:template match="attribute/@type | param/@type">
  <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>

  <xsl:choose>
    <!-- modifiers (ignored for 'enumeration' attributes)-->
    <xsl:when test="name(current())='type' and ../@mod">
      <xsl:choose>
        <xsl:when test="../@mod='ptr'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='boolean'">booleanPtr</xsl:when>
            <xsl:when test=".='octet'">octetPtr</xsl:when>
            <xsl:when test=".='short'">shortPtr</xsl:when>
            <xsl:when test=".='unsigned short'">ushortPtr</xsl:when>
            <xsl:when test=".='long'">longPtr</xsl:when>
            <xsl:when test=".='long long'">llongPtr</xsl:when>
            <xsl:when test=".='unsigned long'">ulongPtr</xsl:when>
            <xsl:when test=".='unsigned long long'">ullongPtr</xsl:when>
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
            <xsl:when test=".='uuid'">wstringUUID</xsl:when>
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
        <xsl:when test=".='result'">result</xsl:when>
        <xsl:when test=".='boolean'">boolean</xsl:when>
        <xsl:when test=".='octet'">octet</xsl:when>
        <xsl:when test=".='short'">short</xsl:when>
        <xsl:when test=".='unsigned short'">unsigned short</xsl:when>
        <xsl:when test=".='long'">long</xsl:when>
        <xsl:when test=".='long long'">long long</xsl:when>
        <xsl:when test=".='unsigned long'">unsigned long</xsl:when>
        <xsl:when test=".='unsigned long long'">unsigned long long</xsl:when>
        <xsl:when test=".='char'">char</xsl:when>
        <xsl:when test=".='wchar'">wchar</xsl:when>
        <xsl:when test=".='string'">string</xsl:when>
        <xsl:when test=".='wstring'">wstring</xsl:when>
        <!-- UUID type -->
        <xsl:when test=".='uuid'">uuid</xsl:when>
        <!-- system interface types -->
        <xsl:when test=".='$unknown'">$unknown</xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <!-- enum types -->
            <xsl:when test="
                 (ancestor::library/application/enum[@name=current()])
              or (ancestor::library/if/application/enum[@name=current()])
              or (ancestor::library/application/if[@target=$self_target]/enum[@name=current()])
              or (ancestor::library/if/application/if[@target=$self_target]/enum[@name=current()])
            ">
              <xsl:value-of select="."/>
            </xsl:when>
            <!-- custom interface types -->
            <xsl:when test="
              (   (ancestor::library/application/interface[@name=current()])
               or (ancestor::library/if/application/interface[@name=current()])
               or (ancestor::library/application/if[@target=$self_target]/interface[@name=current()])
               or (ancestor::library/if/application/if[@target=$self_target]/interface[@name=current()])
              )
            ">
              <xsl:value-of select="."/>
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
    </xsl:otherwise>
  </xsl:choose>
  <xsl:if test="../@safearray='yes'">
    <xsl:text>[]</xsl:text>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>

