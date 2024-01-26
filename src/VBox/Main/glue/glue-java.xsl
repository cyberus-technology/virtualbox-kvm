<xsl:stylesheet version = '1.0'
    xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
    xmlns:vbox="http://www.virtualbox.org/"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

<!--
    glue-java.xsl:
        XSLT stylesheet that generates Java glue code for XPCOM, MSCOM and JAX-WS from
        VirtualBox.xidl.
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

<xsl:variable name="G_xsltFilename" select="'glue-java.xsl'" />
<xsl:variable name="G_virtualBoxPackage" select="concat('org.virtualbox', $G_vboxApiSuffix)" />
<xsl:variable name="G_virtualBoxPackageCom" select="concat('org.virtualbox', $G_vboxApiSuffix, '.', $G_vboxGlueStyle)" />
<xsl:variable name="G_virtualBoxWsdl" select="concat('&quot;vboxwebService', $G_vboxApiSuffix, '.wsdl&quot;')" />
<!-- collect all interfaces with "wsmap='suppress'" in a global variable for quick lookup -->
<xsl:variable name="G_setSuppressedInterfaces"
              select="//interface[@wsmap='suppress']" />

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<xsl:strip-space elements="*"/>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>


<!-- - - - - - - - - - - - - - - - - - - - - - -
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template name="fileheader">
  <xsl:param name="name" />
  <xsl:text>/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
 *
 * This file is part of a free software library; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License version 2.1 as published by the Free Software
 * Foundation and shipped in the \"COPYING.LIB\" file with this library.
 * The library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY of any kind.
 *
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if
 * any license choice other than GPL or LGPL is available it will
 * apply instead, Oracle elects to use only the Lesser General Public
 * License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the
 * language indicating that LGPLv2 or any later version may be used,
 * or where a choice of which version of the LGPL is applied is
 * otherwise unspecified.

 * http://www.virtualbox.org.  This library is free software; you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, in version 2.1
 * as it comes in the "COPYING.LIB" file of the VirtualBox SDK distribution.
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/*
</xsl:text>
  <xsl:value-of select="concat(' * ', $name)"/>
<xsl:text>
 *
 * DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
 * Generator: src/VBox/Main/glue/glue-java.xsl
 */

</xsl:text>
</xsl:template>

<xsl:template name="startFile">
  <xsl:param name="file" />
  <xsl:param name="package" />

  <xsl:choose>
    <xsl:when test="$filelistonly=''">
      <xsl:value-of select="concat('&#10;// ##### BEGINFILE &quot;', $G_vboxDirPrefix, $file, '&quot;&#10;&#10;')" />
      <xsl:call-template name="fileheader">
        <xsl:with-param name="name" select="$file" />
      </xsl:call-template>

      <xsl:value-of select="concat('package ', $package, ';&#10;&#10;')" />
      <xsl:value-of select="concat('import ', $G_virtualBoxPackageCom, '.*;&#10;')" />

      <xsl:choose>
        <xsl:when test="$G_vboxGlueStyle='xpcom'">
          <xsl:text>import org.mozilla.interfaces.*;&#10;</xsl:text>
        </xsl:when>

        <xsl:when test="$G_vboxGlueStyle='mscom'">
          <xsl:text>import com.jacob.com.*;&#10;</xsl:text>
          <xsl:text>import com.jacob.activeX.ActiveXComponent;&#10;</xsl:text>
        </xsl:when>

        <xsl:when test="$G_vboxGlueStyle='jaxws'">
          <xsl:text>import javax.xml.ws.*;&#10;</xsl:text>
        </xsl:when>

        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="'no header rule (startFile)'" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat(' \', $G_sNewLine, '&#9;', $G_vboxDirPrefix, $file)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="endFile">
  <xsl:param name="file" />
  <xsl:if test="$filelistonly=''">
    <xsl:value-of select="concat('&#10;// ##### ENDFILE &quot;', $file, '&quot;&#10;')" />
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
  </xsl:if>
</xsl:template>

<!-- strip-and-normalize-desc
 Removes leading and trailing white space on each line in the given text.
 -->
<xsl:template name="strip-and-normalize-desc">
  <xsl:param name="text"/>

  <!-- Strip the whole string first so we won't leave trailing new line chars behind. -->
  <xsl:variable name="sStrippedText">
    <xsl:call-template name="strip-string">
      <xsl:with-param name="text" select="$text"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <!-- If there are multiple lines, strip them one by one on a recursive fasion. -->
    <xsl:when test="contains($sStrippedText, '&#10;')">
      <xsl:call-template name="strip-string-right">
        <xsl:with-param name="text" select="substring-before($sStrippedText, '&#10;')"/>
      </xsl:call-template>
      <xsl:value-of select="'&#10;'"/>
      <xsl:call-template name="strip-and-normalize-desc-recursive">
        <xsl:with-param name="text" select="substring-after($sStrippedText, '&#10;')"/>
      </xsl:call-template>
    </xsl:when>

    <!-- Single line, we're done. -->
    <xsl:otherwise>
      <xsl:value-of select="$sStrippedText"/>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<!-- Internal worker for strip-and-normalize-desc, don't use. -->
<xsl:template name="strip-and-normalize-desc-recursive">
  <xsl:param name="text"/>

  <xsl:choose>
    <!-- If there are multiple lines, strip them one by one on a recursive fasion. -->
    <xsl:when test="contains($text, '&#10;')">
      <xsl:call-template name="strip-string">
        <xsl:with-param name="text" select="substring-before($text, '&#10;')"/>
      </xsl:call-template>
      <xsl:value-of select="'&#10;'"/>
      <xsl:call-template name="strip-and-normalize-desc-recursive">
        <xsl:with-param name="text" select="substring-after($text, '&#10;')"/>
      </xsl:call-template>
    </xsl:when>

    <!-- Single line: Left strip it. -->
    <xsl:otherwise>
      <xsl:call-template name="strip-string-left">
        <xsl:with-param name="text" select="$text"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<!-- descriptions -->

<xsl:template match="*/text()">
  <!-- TODO: strip out @c/@a for now. long term solution is changing that to a
       tag in the xidl file, and translate it when generating doxygen etc. -->
  <xsl:variable name="rep1">
    <xsl:call-template name="string-replace">
      <xsl:with-param name="haystack" select="."/>
      <xsl:with-param name="needle" select="'@c'"/>
      <xsl:with-param name="replacement" select="''"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="rep2">
    <xsl:call-template name="string-replace">
      <xsl:with-param name="haystack" select="$rep1"/>
      <xsl:with-param name="needle" select="'@a'"/>
      <xsl:with-param name="replacement" select="''"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="rep3">
    <xsl:call-template name="string-replace">
      <xsl:with-param name="haystack" select="$rep2"/>
      <xsl:with-param name="needle" select="'@todo'"/>
      <xsl:with-param name="replacement" select="'TODO'"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- &amp;, &lt; and &gt; must remain as they are or javadoc 8 throws a fit. -->
  <xsl:variable name="rep4">
    <xsl:call-template name="string-replace">
      <xsl:with-param name="haystack" select="$rep3"/>
      <xsl:with-param name="needle" select="'&amp;'"/>
      <xsl:with-param name="replacement" select="'&amp;amp;'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="rep5">
    <xsl:call-template name="string-replace">
      <xsl:with-param name="haystack" select="$rep4"/>
      <xsl:with-param name="needle" select="'&lt;'"/>
      <xsl:with-param name="replacement" select="'&amp;lt;'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="rep6">
    <xsl:call-template name="string-replace">
      <xsl:with-param name="haystack" select="$rep5"/>
      <xsl:with-param name="needle" select="'&gt;'"/>
      <xsl:with-param name="replacement" select="'&amp;gt;'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="rep7">
    <xsl:call-template name="strip-and-normalize-desc">
      <xsl:with-param name="text" select="$rep6"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:value-of select="$rep7"/>
</xsl:template>

<!--
 * all sub-elements that are not explicitly matched are considered to be
 * html tags and copied w/o modifications
-->
<xsl:template match="desc//*">
  <xsl:variable name="tagname" select="local-name()"/>
  <xsl:choose>
    <xsl:when test="$tagname = 'tt'">
      <xsl:text>&lt;code&gt;</xsl:text>
    </xsl:when>
    <xsl:when test="$tagname = 'h3'">
      <xsl:text>&lt;h2&gt;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat('&lt;', $tagname, '&gt;')"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:apply-templates/>
  <xsl:choose>
    <xsl:when test="$tagname = 'tt'">
      <xsl:text>&lt;/code&gt;</xsl:text>
    </xsl:when>
    <xsl:when test="$tagname = 'h3'">
      <xsl:text>&lt;/h2&gt;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat('&lt;/', $tagname, '&gt;')"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="emit_refsig">
  <xsl:param name="context"/>
  <xsl:param name="identifier"/>

  <xsl:choose>
    <xsl:when test="key('G_keyEnumsByName', $context)/const[@name=$identifier]">
      <xsl:value-of select="$identifier"/>
    </xsl:when>
    <xsl:when test="key('G_keyInterfacesByName', $context)/method[@name=$identifier]">
      <xsl:value-of select="$identifier"/>
      <xsl:text>(</xsl:text>
      <xsl:for-each select="key('G_keyInterfacesByName', $context)/method[@name=$identifier]/param">
        <xsl:if test="@dir!='return'">
          <xsl:if test="position() > 1">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:choose>
            <xsl:when test="@dir='out'">
              <xsl:text>Holder</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="typeIdl2Glue">
                <xsl:with-param name="type" select="@type"/>
                <xsl:with-param name="safearray" select="@safearray"/>
                <xsl:with-param name="skiplisttype" select="'yes'"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:when>
    <xsl:when test="key('G_keyInterfacesByName', $context)/attribute[@name=$identifier]">
      <xsl:call-template name="makeGetterName">
        <xsl:with-param name="attrname" select="$identifier" />
      </xsl:call-template>
      <xsl:text>()</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('unknown reference destination in @see/@link: context=', $context, ' identifier=', $identifier)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
 * link
-->
<xsl:template match="desc//link">
  <xsl:text>{@link </xsl:text>
  <xsl:apply-templates select="." mode="middle"/>
  <xsl:text>}</xsl:text>
</xsl:template>

<xsl:template match="link" mode="middle">
  <xsl:variable name="linktext">
    <xsl:call-template name="string-replace-first">
      <xsl:with-param name="haystack" select="@to"/>
      <xsl:with-param name="needle" select="'_'"/>
      <xsl:with-param name="replacement" select="'#'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="substring($linktext, 1, 1)='#'">
      <xsl:variable name="context">
        <xsl:choose>
          <xsl:when test="local-name(../..)='interface' or local-name(../..)='enum'">
            <xsl:value-of select="../../@name"/>
          </xsl:when>
          <xsl:when test="local-name(../../..)='interface' or local-name(../../..)='enum'">
            <xsl:value-of select="../../../@name"/>
          </xsl:when>
          <xsl:when test="local-name(../../../..)='interface' or local-name(../../../..)='enum'">
            <xsl:value-of select="../../../../@name"/>
          </xsl:when>
          <xsl:when test="local-name(../../../../..)='interface' or local-name(../../../../..)='enum'">
            <xsl:value-of select="../../../../../@name"/>
          </xsl:when>
          <xsl:when test="local-name(../../../../../..)='interface' or local-name(../../../../../..)='enum'">
            <xsl:value-of select="../../../../../../@name"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="fatalError">
              <xsl:with-param name="msg" select="concat('cannot determine context for identifier ', $linktext)" />
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="linkname">
        <xsl:value-of select="substring($linktext, 2)"/>
      </xsl:variable>
      <xsl:text>#</xsl:text>
      <xsl:call-template name="emit_refsig">
        <xsl:with-param name="context" select="$context"/>
        <xsl:with-param name="identifier" select="$linkname"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="contains($linktext, '::')">
      <xsl:variable name="context">
        <xsl:value-of select="substring-before($linktext, '::')"/>
      </xsl:variable>
      <xsl:variable name="linkname">
        <xsl:value-of select="substring-after($linktext, '::')"/>
      </xsl:variable>
      <xsl:value-of select="concat($G_virtualBoxPackage, '.', $context, '#')"/>
      <xsl:call-template name="emit_refsig">
        <xsl:with-param name="context" select="$context"/>
        <xsl:with-param name="identifier" select="$linkname"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat($G_virtualBoxPackage, '.', $linktext)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!--
 * note
-->
<xsl:template match="desc/note">
  <xsl:if test="not(@internal='yes')">
    <xsl:text>&#10;NOTE: </xsl:text>
    <xsl:apply-templates/>
    <xsl:text>&#10;</xsl:text>
  </xsl:if>
</xsl:template>

<!--
 * see
-->
<xsl:template match="desc/see">
  <!-- TODO: quirk in our xidl file: only one <see> tag with <link> nested
       into it, translate this to multiple @see lines and strip the rest.
       Should be replaced in the xidl by multiple <see> without nested tag -->
  <xsl:text>&#10;</xsl:text>
  <xsl:apply-templates match="link"/>
</xsl:template>

<xsl:template match="desc/see/text()"/>

<xsl:template match="desc/see/link">
  <xsl:text>@see </xsl:text>
  <xsl:apply-templates select="." mode="middle"/>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<!--
 * common comment prologue (handles group IDs)
-->
<xsl:template match="desc" mode="begin">
  <!-- TODO,XXX: This is a hot spot. The whole $id crap isn't working though,
                 so it's been disabled to save precious time. -->
<!--
  <xsl:param name="id" select="@group | preceding::descGroup[1]/@id"/>
  <xsl:text>&#10;/**&#10;</xsl:text>
  <xsl:if test="$id">
    <xsl:value-of select="concat(' @ingroup ', $id, '&#10;')"/>
  </xsl:if>
-->
  <xsl:value-of select="concat($G_sNewLine, '/**', $G_sNewLine)"/>
</xsl:template>

<!--
 * common middle part of the comment block
-->
<xsl:template match="desc" mode="middle">
  <xsl:apply-templates select="text() | *[not(self::note or self::see)]"/>
  <xsl:apply-templates select="note"/>
  <xsl:apply-templates select="see"/>
</xsl:template>

<!--
 * result part of the comment block
-->
<xsl:template match="desc" mode="results">
  <xsl:if test="result">
    <xsl:text>&#10;&lt;p&gt;&lt;/p&gt;&lt;dl&gt;&lt;dt&gt;&lt;b&gt;Expected result codes:&lt;/b&gt;&lt;/dt&gt;&#10;</xsl:text>
    <xsl:for-each select="result">
      <xsl:text>&lt;dd&gt;&lt;code&gt;</xsl:text>
      <xsl:choose>
        <xsl:when test="ancestor::library/result[@name=current()/@name]">
          <xsl:value-of select="concat('@link ::', @name, ' ', @name)"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="@name"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>&lt;/code&gt; - </xsl:text>
      <xsl:apply-templates select="text() | *[not(self::note or self::see or
                                                  self::result)]"/>
      <xsl:text>&lt;/dd&gt;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>&lt;/dl&gt;&#10;</xsl:text>
  </xsl:if>
</xsl:template>

<!--
 * comment for interfaces
-->
<xsl:template match="desc" mode="interface">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="." mode="middle"/>
  <xsl:text>&#10;&#10;Interface ID: &lt;code&gt;{</xsl:text>
  <xsl:call-template name="string-to-upper">
    <xsl:with-param name="str" select="../@uuid"/>
  </xsl:call-template>
  <xsl:text>}&lt;/code&gt;&#10;*/&#10;</xsl:text>
</xsl:template>

<!--
 * comment for attribute getters
-->
<xsl:template match="desc" mode="attribute_get">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="text() | *[not(self::note or self::see or self::result)]"/>
  <xsl:apply-templates select="." mode="results"/>
  <xsl:apply-templates select="note"/>
  <xsl:text>&#10;@return </xsl:text>
  <xsl:call-template name="typeIdl2Glue">
    <xsl:with-param name="type" select="../@type"/>
    <xsl:with-param name="safearray" select="../@safearray"/>
    <xsl:with-param name="doubleescape">yes</xsl:with-param>
  </xsl:call-template>
  <xsl:text>&#10;</xsl:text>
  <xsl:apply-templates select="see"/>
  <xsl:text>*/&#10;</xsl:text>
</xsl:template>

<!--
 * comment for attribute setters
-->
<xsl:template match="desc" mode="attribute_set">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="text() | *[not(self::note or self::see or self::result)]"/>
  <xsl:apply-templates select="." mode="results"/>
  <xsl:apply-templates select="note"/>
  <xsl:text>&#10;@param value </xsl:text>
  <xsl:call-template name="typeIdl2Glue">
    <xsl:with-param name="type" select="../@type"/>
    <xsl:with-param name="safearray" select="../@safearray"/>
    <xsl:with-param name="doubleescape">yes</xsl:with-param>
  </xsl:call-template>
  <xsl:text>&#10;</xsl:text>
  <xsl:apply-templates select="see"/>
  <xsl:text>&#10;*/&#10;</xsl:text>
</xsl:template>

<!--
 * comment for methods
-->
<xsl:template match="desc" mode="method">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="text() | *[not(self::note or self::see or self::result)]"/>
  <xsl:apply-templates select="." mode="results"/>
  <xsl:for-each select="../param">
    <xsl:apply-templates select="desc"/>
  </xsl:for-each>
  <xsl:apply-templates select="note"/>
  <xsl:apply-templates select="../param/desc/note"/>
  <xsl:apply-templates select="see"/>
  <xsl:text>&#10;*/&#10;</xsl:text>
</xsl:template>

<!--
 * comment for method parameters
-->
<xsl:template match="method/param/desc">
  <xsl:if test="text() | *[not(self::note or self::see)]">
    <xsl:choose>
      <xsl:when test="../@dir='return'">
        <xsl:text>&#10;@return </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#10;@param </xsl:text>
        <xsl:value-of select="../@name"/>
        <xsl:text> </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="text() | *[not(self::note or self::see)]"/>
    <xsl:text>&#10;</xsl:text>
  </xsl:if>
</xsl:template>

<!--
 * comment for enums
-->
<xsl:template match="desc" mode="enum">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="." mode="middle"/>
  <xsl:text>&#10;Interface ID: &lt;code&gt;{</xsl:text>
  <xsl:call-template name="string-to-upper">
    <xsl:with-param name="str" select="../@uuid"/>
  </xsl:call-template>
  <xsl:text>}&lt;/code&gt;&#10;*/&#10;</xsl:text>
</xsl:template>

<!--
 * comment for enum values
-->
<xsl:template match="desc" mode="enum_const">
  <xsl:apply-templates select="." mode="begin"/>
  <xsl:apply-templates select="." mode="middle"/>
  <xsl:text>&#10;*/&#10;</xsl:text>
</xsl:template>

<!--
 * ignore descGroups by default (processed in /idl)
-->
<xsl:template match="descGroup"/>



<!-- actual code generation -->

<xsl:template name="genEnum">
  <xsl:param name="enumname" />
  <xsl:param name="filename" />

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="$filename" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:apply-templates select="desc" mode="enum"/>
    <xsl:value-of select="concat('public enum ', $enumname, '&#10;')" />
    <xsl:text>{&#10;</xsl:text>
    <xsl:for-each select="const">
      <xsl:apply-templates select="desc" mode="enum_const"/>
      <xsl:variable name="enumconst" select="@name" />
      <xsl:value-of select="concat('    ', $enumconst, '(', @value, ')')" />
      <xsl:choose>
        <xsl:when test="not(position()=last())">
          <xsl:text>,&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>;&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>

    <xsl:text>&#10;</xsl:text>
    <xsl:text>    private final int value;&#10;&#10;</xsl:text>

    <xsl:value-of select="concat('    ', $enumname, '(int v)&#10;')" />
    <xsl:text>    {&#10;</xsl:text>
    <xsl:text>        value = v;&#10;</xsl:text>
    <xsl:text>    }&#10;&#10;</xsl:text>

    <xsl:text>    public int value()&#10;</xsl:text>
    <xsl:text>    {&#10;</xsl:text>
    <xsl:text>        return value;&#10;</xsl:text>
    <xsl:text>    }&#10;&#10;</xsl:text>

    <xsl:value-of select="concat('    public static ', $enumname, ' fromValue(long v)&#10;')" />
    <xsl:text>    {&#10;</xsl:text>
    <xsl:value-of select="concat('        for (', $enumname, ' c: ', $enumname, '.values())&#10;')" />
    <xsl:text>        {&#10;</xsl:text>
    <xsl:text>            if (c.value == (int)v)&#10;</xsl:text>
    <xsl:text>            {&#10;</xsl:text>
    <xsl:text>                return c;&#10;</xsl:text>
    <xsl:text>            }&#10;</xsl:text>
    <xsl:text>        }&#10;</xsl:text>
    <xsl:text>        throw new IllegalArgumentException(Long.toString(v));&#10;</xsl:text>
    <xsl:text>    }&#10;&#10;</xsl:text>

    <xsl:value-of select="concat('    public static ', $enumname, ' fromValue(String v)&#10;')" />
    <xsl:text>    {&#10;</xsl:text>
    <xsl:value-of select="concat('        return valueOf(', $enumname, '.class, v);&#10;')" />
    <xsl:text>    }&#10;</xsl:text>
    <xsl:text>}&#10;&#10;</xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="$filename" />
  </xsl:call-template>

</xsl:template>

<xsl:template name="startExcWrapper">
  <xsl:param name="preventObjRelease" />

  <xsl:if test="$G_vboxGlueStyle='jaxws' and $preventObjRelease">
    <xsl:text>        this.getObjMgr().preventObjRelease();&#10;</xsl:text>
  </xsl:if>
  <xsl:text>        try&#10;</xsl:text>
  <xsl:text>        {&#10;</xsl:text>
</xsl:template>

<xsl:template name="endExcWrapper">
  <xsl:param name="allowObjRelease" />

  <xsl:choose>
    <xsl:when test="$G_vboxGlueStyle='xpcom'">
      <xsl:text>        }&#10;</xsl:text>
      <xsl:text>        catch (org.mozilla.xpcom.XPCOMException e)&#10;</xsl:text>
      <xsl:text>        {&#10;</xsl:text>
      <xsl:text>            throw new VBoxException(e.getMessage(), e);&#10;</xsl:text>
      <xsl:text>        }&#10;</xsl:text>
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='mscom'">
      <xsl:text>        }&#10;</xsl:text>
      <xsl:text>        catch (com.jacob.com.ComException e)&#10;</xsl:text>
      <xsl:text>        {&#10;</xsl:text>
      <xsl:text>            throw new VBoxException(e.getMessage(), e);&#10;</xsl:text>
      <xsl:text>        }&#10;</xsl:text>
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='jaxws'">
      <xsl:text>        }&#10;</xsl:text>
      <xsl:text>        catch (InvalidObjectFaultMsg e)&#10;</xsl:text>
      <xsl:text>        {&#10;</xsl:text>
      <xsl:text>            throw new VBoxException(e.getMessage(), e, this.getObjMgr(), this.port);&#10;</xsl:text>
      <xsl:text>        }&#10;</xsl:text>
      <xsl:text>        catch (RuntimeFaultMsg e)&#10;</xsl:text>
      <xsl:text>        {&#10;</xsl:text>
      <xsl:text>            throw new VBoxException(e.getMessage(), e, this.getObjMgr(), this.port);&#10;</xsl:text>
      <xsl:text>        }&#10;</xsl:text>
      <xsl:if test="$allowObjRelease">
        <xsl:text>        finally&#10;</xsl:text>
        <xsl:text>        {&#10;</xsl:text>
        <xsl:text>            getObjMgr().allowObjRelease();&#10;</xsl:text>
        <xsl:text>        }&#10;</xsl:text>
      </xsl:if>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'no header rule (startFile)'" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="wrappedName">
  <xsl:param name="ifname" />

  <xsl:choose>
    <xsl:when test="$G_vboxGlueStyle='xpcom'">
      <xsl:value-of select="concat('org.mozilla.interfaces.', $ifname)" />
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='mscom'">
      <xsl:text>com.jacob.com.Dispatch</xsl:text>
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='jaxws'">
      <xsl:text>String</xsl:text>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'no wrapper naming rule defined (wrappedName)'" />
      </xsl:call-template>
    </xsl:otherwise>

  </xsl:choose>
</xsl:template>

<xsl:template name="fullClassName">
  <xsl:param name="name" />
  <xsl:param name="origname" />
  <xsl:param name="collPrefix" />

  <xsl:choose>
    <xsl:when test="(count(key('G_keyEnumsByName', $name)) > 0) or (count(key('G_keyEnumsByName', $origname)) > 0)">
      <xsl:value-of select="concat($G_virtualBoxPackage, concat('.', $name))" />
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $name)) > 0">
      <xsl:value-of select="concat($G_virtualBoxPackage, concat('.', $name))" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('fullClassName: Type &quot;', $name, '&quot; is not supported.')" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="typeIdl2Glue">
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="forceelem" />
  <xsl:param name="skiplisttype" />
  <xsl:param name="doubleescape" />

  <xsl:variable name="needarray" select="($safearray='yes') and not($forceelem='yes')" />
  <xsl:variable name="needlist" select="($needarray) and not($type='octet')" />

  <xsl:if test="($needlist)">
    <xsl:text>List</xsl:text>
    <xsl:if test="not($skiplisttype='yes')">
      <xsl:choose>
        <xsl:when test="$doubleescape='yes'">
          <xsl:text>&amp;lt;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>&lt;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:if>

  <xsl:if test="not($needlist) or not($skiplisttype='yes')">
    <!-- look up Java type from IDL type from table array in typemap-shared.inc.xsl -->
    <xsl:variable name="javatypefield" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@javaname" />

    <xsl:choose>
      <xsl:when test="string-length($javatypefield)">
        <xsl:value-of select="$javatypefield" />
      </xsl:when>
      <!-- not a standard type: then it better be one of the types defined in the XIDL -->
      <xsl:when test="$type='$unknown'">IUnknown</xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="fullClassName">
          <xsl:with-param name="name" select="$type" />
          <xsl:with-param name="collPrefix" select="''"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

  <xsl:choose>
    <xsl:when test="($needlist)">
      <xsl:if test="not($skiplisttype='yes')">
        <xsl:choose>
          <xsl:when test="$doubleescape='yes'">
            <xsl:text>&amp;gt;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>&gt;</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:when>
    <xsl:when test="($needarray)">
      <xsl:text>[]</xsl:text>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!--
    typeIdl2Back: converts $type into a type as used by the backend.
    -->
<xsl:template name="typeIdl2Back">
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="forceelem" />

  <xsl:choose>
    <xsl:when test="($G_vboxGlueStyle='xpcom')">
      <xsl:variable name="needarray" select="($safearray='yes') and not($forceelem='yes')" />

      <xsl:choose>
        <xsl:when test="$type='long long'">
          <xsl:text>long</xsl:text>
        </xsl:when>

        <xsl:when test="$type='unsigned long'">
          <xsl:text>long</xsl:text>
        </xsl:when>

        <xsl:when test="$type='long'">
          <xsl:text>int</xsl:text>
        </xsl:when>

        <xsl:when test="$type='unsigned short'">
          <xsl:text>int</xsl:text>
        </xsl:when>

        <xsl:when test="$type='short'">
          <xsl:text>short</xsl:text>
        </xsl:when>

        <xsl:when test="$type='octet'">
          <xsl:text>byte</xsl:text>
        </xsl:when>

        <xsl:when test="$type='boolean'">
          <xsl:text>boolean</xsl:text>
        </xsl:when>

        <xsl:when test="$type='$unknown'">
          <xsl:text>nsISupports</xsl:text>
        </xsl:when>

        <xsl:when test="$type='wstring'">
          <xsl:text>String</xsl:text>
        </xsl:when>

        <xsl:when test="$type='uuid'">
          <xsl:text>String</xsl:text>
        </xsl:when>

        <xsl:when test="key('G_keyInterfacesByName', $type)/@wsmap='struct'">
          <xsl:call-template name="wrappedName">
            <xsl:with-param name="ifname" select="$type" />
          </xsl:call-template>
        </xsl:when>

        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <xsl:call-template name="wrappedName">
            <xsl:with-param name="ifname" select="$type" />
          </xsl:call-template>
        </xsl:when>

        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
          <xsl:text>long</xsl:text>
        </xsl:when>

        <xsl:otherwise>
          <xsl:call-template name="fullClassName">
            <xsl:with-param name="name" select="$type" />
          </xsl:call-template>
        </xsl:otherwise>

      </xsl:choose>
      <xsl:if test="$needarray">
        <xsl:text>[]</xsl:text>
      </xsl:if>
    </xsl:when>

    <xsl:when test="($G_vboxGlueStyle='mscom')">
      <xsl:text>Variant</xsl:text>
    </xsl:when>

    <xsl:when test="($G_vboxGlueStyle='jaxws')">
      <xsl:variable name="needarray" select="($safearray='yes' and not($type='octet')) and not($forceelem='yes')" />

      <xsl:if test="$needarray">
        <xsl:text>List&lt;</xsl:text>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="$type='$unknown'">
          <xsl:text>String</xsl:text>
        </xsl:when>

        <xsl:when test="key('G_keyInterfacesByName', $type)/@wsmap='managed'">
          <xsl:text>String</xsl:text>
        </xsl:when>

        <xsl:when test="key('G_keyInterfacesByName', $type)/@wsmap='struct'">
          <xsl:value-of select="concat($G_virtualBoxPackageCom, '.', $type)" />
        </xsl:when>

        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
          <xsl:value-of select="concat($G_virtualBoxPackageCom, '.', $type)" />
        </xsl:when>

        <!-- we encode byte arrays as Base64 strings. -->
        <xsl:when test="$type='octet'">
          <xsl:text>/*base64*/String</xsl:text>
        </xsl:when>

        <xsl:when test="$type='long long'">
          <xsl:text>Long</xsl:text>
        </xsl:when>

        <xsl:when test="$type='unsigned long'">
          <xsl:text>Long</xsl:text>
        </xsl:when>

        <xsl:when test="$type='long'">
          <xsl:text>Integer</xsl:text>
        </xsl:when>

        <xsl:when test="$type='unsigned short'">
          <xsl:text>Integer</xsl:text>
        </xsl:when>

        <xsl:when test="$type='short'">
          <xsl:text>Short</xsl:text>
        </xsl:when>

        <xsl:when test="$type='boolean'">
          <xsl:text>Boolean</xsl:text>
        </xsl:when>

        <xsl:when test="$type='wstring'">
          <xsl:text>String</xsl:text>
        </xsl:when>

        <xsl:when test="$type='uuid'">
          <xsl:text>String</xsl:text>
        </xsl:when>

        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('Unhandled type ', $type, ' (typeIdl2Back)')" />
          </xsl:call-template>
        </xsl:otherwise>

      </xsl:choose>

      <xsl:if test="$needarray">
        <xsl:text>&gt;</xsl:text>
      </xsl:if>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Write typeIdl2Back for this style (typeIdl2Back)'" />
      </xsl:call-template>
    </xsl:otherwise>

  </xsl:choose>
</xsl:template>

<xsl:template name="cookOutParamXpcom">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>
  <xsl:variable name="isstruct"
                select="key('G_keyInterfacesByName', $idltype)/@wsmap='struct'" />

  <xsl:variable name="gluetype">
    <xsl:call-template name="typeIdl2Glue">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="elemgluetype">
    <xsl:if test="$safearray='yes'">
      <xsl:call-template name="typeIdl2Glue">
        <xsl:with-param name="type" select="$idltype" />
        <xsl:with-param name="safearray" select="'no'" />
        <xsl:with-param name="forceelem" select="'yes'" />
      </xsl:call-template>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$idltype = '$unknown' or (count(key('G_keyInterfacesByName', $idltype)) > 0)">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:variable name="elembacktype">
            <xsl:call-template name="typeIdl2Back">
              <xsl:with-param name="type" select="$idltype" />
              <xsl:with-param name="safearray" select="$safearray" />
              <xsl:with-param name="forceelem" select="'yes'" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat('Helper.wrap2(', $elemgluetype, '.class, ', $elembacktype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('(', $value, ' != null) ? new ', $gluetype, '(', $value, ') : null')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="count(key('G_keyEnumsByName', $idltype)) > 0">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:variable name="elembacktype">
            <xsl:call-template name="typeIdl2Back">
              <xsl:with-param name="type" select="$idltype" />
              <xsl:with-param name="safearray" select="$safearray" />
              <xsl:with-param name="forceelem" select="'yes'" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat('Helper.wrapEnum(', $elemgluetype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat($gluetype, '.fromValue(', $value, ')')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="($safearray='yes') and ($idltype='octet')">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.wrap(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$value"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="cookOutParamMscom">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>

  <xsl:variable name="gluetype">
    <xsl:call-template name="typeIdl2Glue">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemgluetype">
        <xsl:call-template name="typeIdl2Glue">
          <xsl:with-param name="type" select="$idltype" />
          <xsl:with-param name="safearray" select="'no'" />
          <xsl:with-param name="forceelem" select="'yes'" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="($idltype='octet')">
          <xsl:value-of select="concat('Helper.wrapBytes(', $value, '.toSafeArray())')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('Helper.wrap(', $elemgluetype, '.class, ', $value, '.toSafeArray())')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="$idltype = '$unknown' or (count(key('G_keyInterfacesByName', $idltype)) > 0)">
      <xsl:value-of select="concat('Helper.wrapDispatch(', $gluetype, '.class, ', $value, '.getDispatch())')"/>
    </xsl:when>

    <xsl:when test="count(key('G_keyEnumsByName', $idltype)) > 0">
      <xsl:value-of select="concat($gluetype, '.fromValue(', $value, '.getInt())')"/>
    </xsl:when>

    <xsl:when test="$idltype='wstring'">
      <xsl:value-of select="concat($value, '.getString()')"/>
    </xsl:when>

    <xsl:when test="$idltype='uuid'">
      <xsl:value-of select="concat($value, '.getString()')"/>
    </xsl:when>

    <xsl:when test="$idltype='boolean'">
      <xsl:value-of select="concat($value, '.toBoolean()')"/>
    </xsl:when>

    <xsl:when test="$idltype='unsigned short'">
      <xsl:value-of select="concat('(int)', $value, '.getShort()')"/>
    </xsl:when>

    <xsl:when test="$idltype='short'">
      <xsl:value-of select="concat($value, '.getShort()')"/>
    </xsl:when>

    <xsl:when test="$idltype='long'">
      <xsl:value-of select="concat($value, '.getInt()')"/>
    </xsl:when>


    <xsl:when test="$idltype='unsigned long'">
      <xsl:value-of select="concat('(long)', $value, '.getInt()')"/>
    </xsl:when>

    <xsl:when test="$idltype='long'">
      <xsl:value-of select="concat($value, '.getInt()')"/>
    </xsl:when>

    <xsl:when test="$idltype='long long'">
      <xsl:value-of select="concat($value, '.getLong()')"/>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('Unhandled type' , $idltype, ' (cookOutParamMscom)')" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<xsl:template name="cookOutParamJaxws">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>

  <xsl:variable name="isstruct"
                select="key('G_keyInterfacesByName', $idltype)/@wsmap='struct'" />

  <xsl:variable name="gluetype">
    <xsl:call-template name="typeIdl2Glue">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemgluetype">
        <xsl:call-template name="typeIdl2Glue">
          <xsl:with-param name="type" select="$idltype" />
          <xsl:with-param name="safearray" select="''" />
          <xsl:with-param name="forceelem" select="'yes'" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="elembacktype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="$idltype" />
          <xsl:with-param name="safearray" select="''" />
          <xsl:with-param name="forceelem" select="'yes'" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="$isstruct">
          <xsl:value-of select="concat('Helper.wrap2(', $elemgluetype, '.class, ', $elembacktype, '.class, objMgr, port, ', $value, ')')"/>
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $idltype)) > 0">
          <xsl:value-of select="concat('Helper.convertEnums(', $elembacktype, '.class, ', $elemgluetype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:when test="$idltype = '$unknown' or (count(key('G_keyInterfacesByName', $idltype)) > 0)">
          <xsl:value-of select="concat('Helper.wrap(', $elemgluetype, '.class, getObjMgr(), port, ', $value, ')')"/>
        </xsl:when>
        <xsl:when test="$idltype='octet'">
          <xsl:value-of select="concat('Helper.decodeBase64(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$value" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="count(key('G_keyEnumsByName', $idltype)) > 0">
          <xsl:value-of select="concat($gluetype, '.fromValue(', $value, '.value())')"/>
        </xsl:when>
        <xsl:when test="$idltype='boolean'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='long long'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='unsigned long long'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='long'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='unsigned long'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='short'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='unsigned short'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='wstring'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$idltype='uuid'">
          <xsl:value-of select="$value"/>
        </xsl:when>
        <xsl:when test="$isstruct">
          <xsl:value-of select="concat('(', $value, ' != null) ? new ', $gluetype, '(', $value, ', getObjMgr(), port) : null')" />
        </xsl:when>
        <xsl:when test="$idltype = '$unknown' or (count(key('G_keyInterfacesByName', $idltype)) > 0)">
          <!-- if the MOR string is empty, that means NULL, so return NULL instead of an object then -->
          <xsl:value-of select="concat('(', $value, '.length() > 0) ? new ', $gluetype, '(', $value, ', getObjMgr(), port) : null')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('Unhandled type ', $idltype, ' (cookOutParamJaxws)')" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<xsl:template name="cookOutParam">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>
  <xsl:choose>
    <xsl:when test="($G_vboxGlueStyle='xpcom')">
      <xsl:call-template name="cookOutParamXpcom">
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="idltype" select="$idltype" />
        <xsl:with-param name="safearray" select="$safearray" />
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="($G_vboxGlueStyle='mscom')">
      <xsl:call-template name="cookOutParamMscom">
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="idltype" select="$idltype" />
        <xsl:with-param name="safearray" select="$safearray" />
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="($G_vboxGlueStyle='jaxws')">
      <xsl:call-template name="cookOutParamJaxws">
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="idltype" select="$idltype" />
        <xsl:with-param name="safearray" select="$safearray" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Unhandled style(cookOutParam)'" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="cookInParamXpcom">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>
  <xsl:variable name="isstruct"
                select="key('G_keyInterfacesByName', $idltype)/@wsmap='struct'" />
  <xsl:variable name="gluetype">
    <xsl:call-template name="typeIdl2Glue">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="backtype">
    <xsl:call-template name="typeIdl2Back">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="elemgluetype">
    <xsl:if test="$safearray='yes'">
      <xsl:call-template name="typeIdl2Glue">
        <xsl:with-param name="type" select="$idltype" />
        <xsl:with-param name="safearray" select="'no'" />
        <xsl:with-param name="forceelem" select="'yes'" />
      </xsl:call-template>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="count(key('G_keyInterfacesByName', $idltype)) > 0">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:variable name="elembacktype">
            <xsl:call-template name="typeIdl2Back">
              <xsl:with-param name="type" select="$idltype" />
              <xsl:with-param name="safearray" select="$safearray" />
              <xsl:with-param name="forceelem" select="'yes'" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat('Helper.unwrap2(', $elemgluetype, '.class, ', $elembacktype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('(', $value, ' != null) ? ', $value, '.getTypedWrapped() : null')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="$idltype='$unknown'">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrap2(', $elemgluetype, '.class, nsISupports.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('(', $value, ' != null) ? (nsISupports)', $value, '.getWrapped() : null')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="count(key('G_keyEnumsByName', $idltype)) > 0">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrapEnum(', $elemgluetype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat($value, '.value()')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="($idltype='octet') and ($safearray='yes')">
      <xsl:value-of select="$value"/>
    </xsl:when>

    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:choose>
            <xsl:when test="$idltype='boolean'">
                <xsl:value-of select="concat('Helper.unwrapBoolean(', $value, ')')"/>
            </xsl:when>
            <xsl:when test="($idltype='long') or ($idltype='unsigned long') or ($idltype='integer')">
                <xsl:value-of select="concat('Helper.unwrapInteger(', $value, ')')"/>
            </xsl:when>
            <xsl:when test="($idltype='short') or ($idltype='unsigned short')">
                <xsl:value-of select="concat('Helper.unwrapUShort(', $value, ')')"/>
            </xsl:when>
            <xsl:when test="($idltype='unsigned long long') or ($idltype='long long')">
                <xsl:value-of select="concat('Helper.unwrapULong(', $value, ')')"/>
            </xsl:when>
            <xsl:when test="($idltype='wstring') or ($idltype='uuid')">
                <xsl:value-of select="concat('Helper.unwrapStr(', $value, ')')"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$value"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$value"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="cookInParamMscom">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>

  <xsl:variable name="gluetype">
    <xsl:call-template name="typeIdl2Glue">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="backtype">
    <xsl:call-template name="typeIdl2Back">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="elemgluetype">
    <xsl:if test="$safearray='yes'">
      <xsl:call-template name="typeIdl2Glue">
        <xsl:with-param name="type" select="$idltype" />
        <xsl:with-param name="safearray" select="'no'" />
        <xsl:with-param name="forceelem" select="'yes'" />
      </xsl:call-template>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="count(key('G_keyInterfacesByName', $idltype)) > 0">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:variable name="elembacktype">
            <xsl:call-template name="typeIdl2Back">
              <xsl:with-param name="type" select="$idltype" />
              <xsl:with-param name="safearray" select="$safearray" />
              <xsl:with-param name="forceelem" select="'yes'" />
            </xsl:call-template>
          </xsl:variable>
          <!-- Sometimes javac needs a boost of self-confidence regarding
               varargs calls, and this (Object) cast makes sure that it calls
               the varargs method - as if there is any other. -->
          <xsl:value-of select="concat('(Object)Helper.unwrap2(', $elemgluetype, '.class, ', $elembacktype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('(', $value, ' != null) ? ', $value, '.getTypedWrapped() : null')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="$idltype='$unknown'">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrap2(', $elemgluetype, '.class, Dispatch.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('(', $value, ' != null) ? (Dispatch)', $value, '.getWrapped() : null')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="count(key('G_keyEnumsByName', $idltype)) > 0">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrapEnum(', $elemgluetype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat($value, '.value()')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="$idltype='boolean'">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrapBool(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('new Variant(', $value, ')')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="($idltype='short') or ($idltype='unsigned short')">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrapShort(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('new Variant(', $value, ')')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>


    <xsl:when test="($idltype='long') or ($idltype='unsigned long')">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrapInt(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('new Variant(', $value, ')')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="($idltype='wstring') or ($idltype='uuid')">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrapString(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('new Variant(', $value, ')')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="($idltype='unsigned long long') or ($idltype='long long')">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrapLong(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('new Variant(', $value, '.longValue())')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="($idltype='octet') and ($safearray='yes')">
      <xsl:value-of select="$value"/>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('Unhandled type: ', $idltype)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<xsl:template name="cookInParamJaxws">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>
  <xsl:variable name="isstruct"
                select="key('G_keyInterfacesByName', $idltype)/@wsmap='struct'" />

  <xsl:variable name="gluetype">
    <xsl:call-template name="typeIdl2Glue">
      <xsl:with-param name="type" select="$idltype" />
      <xsl:with-param name="safearray" select="$safearray" />
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="elemgluetype">
    <xsl:if test="$safearray='yes'">
      <xsl:call-template name="typeIdl2Glue">
        <xsl:with-param name="type" select="$idltype" />
        <xsl:with-param name="safearray" select="'no'" />
        <xsl:with-param name="forceelem" select="'yes'" />
      </xsl:call-template>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$idltype = '$unknown' or (count(key('G_keyInterfacesByName', $idltype)) > 0)">
      <xsl:choose>
        <xsl:when test="@safearray='yes'">
          <xsl:value-of select="concat('Helper.unwrap(', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('((', $value, ' == null) ? null :', $value, '.getWrapped())')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="count(key('G_keyEnumsByName', $idltype)) > 0">
      <xsl:choose>
        <xsl:when test="$safearray='yes'">
          <xsl:variable name="elembacktype">
            <xsl:call-template name="typeIdl2Back">
              <xsl:with-param name="type" select="$idltype" />
              <xsl:with-param name="safearray" select="'no'" />
              <xsl:with-param name="forceelem" select="'yes'" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat('Helper.convertEnums(', $elemgluetype, '.class, ', $elembacktype, '.class, ', $value, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="backtype">
            <xsl:call-template name="typeIdl2Back">
              <xsl:with-param name="type" select="$idltype" />
              <xsl:with-param name="safearray" select="'no'" />
              <xsl:with-param name="forceelem" select="'yes'" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat($backtype, '.fromValue(', $value, '.name())')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="($idltype='octet') and ($safearray='yes')">
      <xsl:value-of select="concat('Helper.encodeBase64(', $value, ')')"/>
    </xsl:when>

    <xsl:otherwise>
      <xsl:value-of select="$value"/>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<xsl:template name="cookInParam">
  <xsl:param name="value"/>
  <xsl:param name="idltype"/>
  <xsl:param name="safearray"/>
  <xsl:choose>
    <xsl:when test="($G_vboxGlueStyle='xpcom')">
      <xsl:call-template name="cookInParamXpcom">
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="idltype" select="$idltype" />
        <xsl:with-param name="safearray" select="$safearray" />
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="($G_vboxGlueStyle='mscom')">
      <xsl:call-template name="cookInParamMscom">
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="idltype" select="$idltype" />
        <xsl:with-param name="safearray" select="$safearray" />
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="($G_vboxGlueStyle='jaxws')">
      <xsl:call-template name="cookInParamJaxws">
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="idltype" select="$idltype" />
        <xsl:with-param name="safearray" select="$safearray" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Unhandled style (cookInParam)'" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Invoke backend method, including parameter conversion -->
<xsl:template name="genBackMethodCall">
  <xsl:param name="ifname"/>
  <xsl:param name="methodname"/>
  <xsl:param name="retval"/>

  <xsl:choose>
    <xsl:when test="($G_vboxGlueStyle='xpcom')">
      <xsl:text>            </xsl:text>
      <xsl:if test="param[@dir='return']">
        <xsl:value-of select="concat($retval, ' = ')" />
      </xsl:if>
      <xsl:value-of select="concat('getTypedWrapped().', $methodname, '(')"/>
      <xsl:for-each select="param">
        <xsl:choose>
          <xsl:when test="@dir='return'">
            <xsl:if test="@safearray='yes'">
              <xsl:text>null</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="@dir='out'">
            <xsl:if test="@safearray='yes'">
              <xsl:text>null, </xsl:text>
            </xsl:if>
            <xsl:value-of select="concat('tmp_', @name)" />
          </xsl:when>
          <xsl:when test="@dir='in'">
            <xsl:if test="(@safearray='yes') and not(@type = 'octet')">
              <xsl:value-of select="concat(@name, ' != null ? ', @name, '.size() : 0, ')" />
            </xsl:if>
            <xsl:variable name="unwrapped">
              <xsl:call-template name="cookInParam">
                <xsl:with-param name="value" select="@name" />
                <xsl:with-param name="idltype" select="@type" />
                <xsl:with-param name="safearray" select="@safearray" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="$unwrapped"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="fatalError">
              <xsl:with-param name="msg" select="concat('Unsupported param dir: ', @dir, '&quot;.')" />
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:if test="not(position()=last()) and not(following-sibling::param[1]/@dir='return' and not(following-sibling::param[1]/@safearray='yes'))">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>);&#10;</xsl:text>
    </xsl:when>

    <xsl:when test="($G_vboxGlueStyle='mscom')">
      <xsl:text>            </xsl:text>
      <xsl:if test="param[@dir='return']">
        <xsl:value-of select="concat($retval, ' = ')" />
      </xsl:if>
      <xsl:value-of select="concat('Helper.invoke(getTypedWrapped(), &quot;', $methodname, '&quot; ')"/>
      <xsl:for-each select="param[not(@dir='return')]">
        <xsl:text>, </xsl:text>
        <xsl:choose>
          <xsl:when test="@dir='out'">
            <xsl:value-of select="concat('tmp_', @name)" />
          </xsl:when>
          <xsl:when test="@dir='in'">
            <xsl:variable name="unwrapped">
              <xsl:call-template name="cookInParam">
                <xsl:with-param name="value" select="@name" />
                <xsl:with-param name="idltype" select="@type" />
                <xsl:with-param name="safearray" select="@safearray" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="$unwrapped"/>
          </xsl:when>
        </xsl:choose>
      </xsl:for-each>
      <xsl:text>);&#10;</xsl:text>
    </xsl:when>

    <xsl:when test="($G_vboxGlueStyle='jaxws')">
      <xsl:variable name="jaxwsmethod">
        <xsl:call-template name="makeJaxwsMethod">
          <xsl:with-param name="ifname" select="$ifname" />
          <xsl:with-param name="methodname" select="$methodname" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="portArg">
        <xsl:if test="not(key('G_keyInterfacesByName', $ifname)/@wsmap='global')">
          <xsl:text>obj</xsl:text>
        </xsl:if>
      </xsl:variable>
      <xsl:variable name="paramsinout" select="param[@dir='in' or @dir='out']" />

      <xsl:text>        </xsl:text>
      <xsl:if test="param[@dir='return'] and not(param[@dir='out'])">
        <xsl:value-of select="concat($retval, ' = ')" />
      </xsl:if>
      <xsl:value-of select="concat('port.', $jaxwsmethod, '(', $portArg)" />
      <xsl:if test="$paramsinout and not($portArg='')">
        <xsl:text>, </xsl:text>
      </xsl:if>

      <!-- jax-ws has an oddity: if both out params and a return value exist,
           then the return value is moved to the function's argument list... -->
      <xsl:choose>
        <xsl:when test="param[@dir='out'] and param[@dir='return']">
          <xsl:for-each select="param">
            <xsl:choose>
              <xsl:when test="@dir='return'">
                <xsl:value-of select="$retval"/>
              </xsl:when>
              <xsl:when test="@dir='out'">
                <xsl:value-of select="concat('tmp_', @name)" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:call-template name="cookInParam">
                  <xsl:with-param name="value" select="@name" />
                  <xsl:with-param name="idltype" select="@type" />
                  <xsl:with-param name="safearray" select="@safearray" />
                </xsl:call-template>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:if test="not(position()=last())">
              <xsl:text>, </xsl:text>
            </xsl:if>
          </xsl:for-each>
        </xsl:when>
        <xsl:otherwise>
          <xsl:for-each select="$paramsinout">
            <xsl:choose>
              <xsl:when test="@dir='return'">
                <xsl:value-of select="$retval"/>
              </xsl:when>
              <xsl:when test="@dir='out'">
                <xsl:value-of select="concat('tmp_', @name)" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:call-template name="cookInParam">
                  <xsl:with-param name="value" select="@name" />
                  <xsl:with-param name="idltype" select="@type" />
                  <xsl:with-param name="safearray" select="@safearray" />
                </xsl:call-template>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:if test="not(position()=last())">
              <xsl:text>, </xsl:text>
            </xsl:if>
          </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>);&#10;</xsl:text>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Style unknown (genBackMethodCall)'" />
      </xsl:call-template>
    </xsl:otherwise>

  </xsl:choose>
</xsl:template>

<xsl:template name="genGetterCall">
  <xsl:param name="ifname"/>
  <xsl:param name="gettername"/>
  <xsl:param name="backtype"/>
  <xsl:param name="retval"/>

  <xsl:choose>
    <xsl:when test="$G_vboxGlueStyle='xpcom'">
      <xsl:value-of select="concat('            ', $backtype, ' ', $retval, ' = getTypedWrapped().', $gettername, '(')" />
      <xsl:if test="@safearray='yes'">
        <xsl:text>null</xsl:text>
      </xsl:if>
      <xsl:text>);&#10;</xsl:text>
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='mscom'">
      <xsl:value-of select="concat('            ', $backtype, ' ', $retval, ' = Dispatch.get(getTypedWrapped(), &quot;', @name, '&quot;);&#10;')" />
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='jaxws'">
      <xsl:variable name="jaxwsGetter">
        <xsl:call-template name="makeJaxwsMethod">
          <xsl:with-param name="ifname" select="$ifname" />
          <xsl:with-param name="methodname" select="$gettername" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat('            ', $backtype, ' ', $retval, ' = port.', $jaxwsGetter, '(obj);&#10;')" />
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Style unknown (genGetterCall)'" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genSetterCall">
  <xsl:param name="ifname"/>
  <xsl:param name="settername"/>
  <xsl:param name="value"/>

  <xsl:choose>
    <xsl:when test="$G_vboxGlueStyle='xpcom'">
      <xsl:value-of select="concat('            getTypedWrapped().', $settername, '(', $value, ');&#10;')" />
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='mscom'">
      <xsl:value-of select="concat('            Dispatch.put(getTypedWrapped(), &quot;', @name, '&quot;, ', $value, ');&#10;')" />
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='jaxws'">
      <xsl:variable name="jaxwsSetter">
        <xsl:call-template name="makeJaxwsMethod">
          <xsl:with-param name="ifname" select="$ifname" />
          <xsl:with-param name="methodname" select="$settername" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat('        port.', $jaxwsSetter, '(obj, ', $value, ');&#10;')" />
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Style unknown (genSetterCall)'" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genStructWrapperJaxws">
  <xsl:param name="ifname"/>

  <xsl:value-of select="concat('    private ', $G_virtualBoxPackageCom, '.', $ifname, ' real;&#10;')"/>
  <xsl:text>    private VboxPortType port;&#10;</xsl:text>
  <xsl:text>    private ObjectRefManager objMgr;&#10;</xsl:text>

  <!-- For structs which contain references to other objects we have to create the stub object during construction time
       or it is possible that the reference was released by the garbage collector because the reference is not
       accounted for. -->
  <!-- Create the private members for containing objects here. -->
  <xsl:for-each select="attribute">
    <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
    <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
    <xsl:variable name="attrsafearray"><xsl:value-of select="@safearray" /></xsl:variable>

    <xsl:if test="(not(@wsmap = 'suppress')) and ($attrtype = '$unknown' or (count(key('G_keyInterfacesByName', $attrtype)) > 0))">
      <xsl:variable name="gluegettertype">
        <xsl:call-template name="typeIdl2Glue">
          <xsl:with-param name="type" select="$attrtype" />
          <xsl:with-param name="safearray" select="@safearray" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat('    private ', $gluegettertype, ' ', $attrname, ';&#10;')" />
    </xsl:if>
  </xsl:for-each>

  <xsl:value-of select="concat('&#10;    public ', $ifname, '(', $G_virtualBoxPackageCom, '.', $ifname, ' real, ObjectRefManager objMgr, VboxPortType port)&#10;')" />
  <xsl:text>    {&#10;</xsl:text>
  <xsl:text>        this.real = real;&#10;</xsl:text>
  <xsl:text>        this.port = port;&#10;</xsl:text>
  <xsl:text>        this.objMgr = objMgr;&#10;</xsl:text>
  <!-- Construct stub objects for every attribute containing a reference to a webserver side object -->
  <xsl:for-each select="attribute">
    <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
    <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
    <xsl:variable name="attrsafearray"><xsl:value-of select="@safearray" /></xsl:variable>

    <xsl:if test="(not(@wsmap = 'suppress')) and ($attrtype = '$unknown' or (count(key('G_keyInterfacesByName', $attrtype)) > 0))">
      <xsl:variable name="backgettername">
        <xsl:choose>
          <!-- Stupid, but backend boolean getters called isFoo(), not getFoo() -->
          <xsl:when test="$attrtype = 'boolean'">
            <xsl:variable name="capsname">
              <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="$attrname" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="concat('is', $capsname)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="makeGetterName">
              <xsl:with-param name="attrname" select="$attrname" />
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="wrapped">
      <xsl:call-template name="cookOutParam">
        <xsl:with-param name="value" select="concat('real.', $backgettername, '()')" />
        <xsl:with-param name="idltype" select="$attrtype" />
        <xsl:with-param name="safearray" select="@safearray" />
      </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat('        ', $attrname, ' = ', $wrapped, ';&#10;')" />
    </xsl:if>
  </xsl:for-each>
  <xsl:text>    }&#10;&#10;</xsl:text>

  <xsl:text><![CDATA[    private ObjectRefManager getObjMgr()
    {
        return this.objMgr;
    }
]]></xsl:text>

  <xsl:for-each select="attribute">
    <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
    <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
    <xsl:variable name="attrsafearray"><xsl:value-of select="@safearray" /></xsl:variable>

    <xsl:if test="not(@wsmap = 'suppress')">

      <xsl:if test="not(@readonly = 'yes')">
        <xsl:call-template name="fatalError">
          <xsl:with-param name="msg" select="concat('Non read-only struct (genStructWrapperJaxws) in interface ', $ifname, ', attribute ', $attrname)" />
        </xsl:call-template>
      </xsl:if>

      <!-- Emit getter -->
      <xsl:variable name="backgettername">
        <xsl:choose>
          <!-- Stupid, but backend boolean getters called isFoo(), not getFoo() -->
          <xsl:when test="$attrtype = 'boolean'">
            <xsl:variable name="capsname">
              <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="$attrname" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="concat('is', $capsname)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="makeGetterName">
              <xsl:with-param name="attrname" select="$attrname" />
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:variable name="gluegettername">
        <xsl:call-template name="makeGetterName">
          <xsl:with-param name="attrname" select="$attrname" />
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="gluegettertype">
        <xsl:call-template name="typeIdl2Glue">
          <xsl:with-param name="type" select="$attrtype" />
          <xsl:with-param name="safearray" select="@safearray" />
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="backgettertype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="$attrtype" />
          <xsl:with-param name="safearray" select="@safearray" />
        </xsl:call-template>
      </xsl:variable>

      <!-- For attributes containing a reference to another object just return the already cerated stub -->
      <xsl:apply-templates select="desc" mode="attribute_get"/>
      <xsl:value-of select="concat('    public ', $gluegettertype, ' ', $gluegettername, '()&#10;')" />
      <xsl:text>    {&#10;</xsl:text>
      <xsl:choose>
        <xsl:when test="$attrtype = '$unknown' or (count(key('G_keyInterfacesByName', $attrtype)) > 0)">
          <xsl:value-of select="concat('        return ', $attrname, ';&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('        ', $backgettertype, ' retVal = real.', $backgettername, '();&#10;')" />
          <xsl:variable name="wrapped">
            <xsl:call-template name="cookOutParam">
              <xsl:with-param name="value" select="'retVal'" />
              <xsl:with-param name="idltype" select="$attrtype" />
              <xsl:with-param name="safearray" select="@safearray" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat('        return ', $wrapped, ';&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>    }&#10;</xsl:text>
    </xsl:if>

  </xsl:for-each>

</xsl:template>

<!-- Interface method wrapper -->
<xsl:template name="genMethod">
  <xsl:param name="ifname"/>
  <xsl:param name="methodname"/>

  <xsl:choose>
    <xsl:when test="(param[@mod='ptr']) or (($G_vboxGlueStyle='jaxws') and (param[@type=($G_setSuppressedInterfaces/@name)]))" >
      <xsl:value-of select="concat('    // Skipping method ', $methodname, ' for it has parameters with suppressed types&#10;')" />
    </xsl:when>
    <xsl:when test="($G_vboxGlueStyle='jaxws') and (@wsmap = 'suppress')" >
      <xsl:value-of select="concat('    // Skipping method ', $methodname, ' for it is suppressed&#10;')" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="hasReturnParms" select="param[@dir='return']" />
      <xsl:variable name="hasOutParms" select="count(param[@dir='out']) > 0" />
      <xsl:variable name="returnidltype" select="param[@dir='return']/@type" />
      <xsl:variable name="returnidlsafearray" select="param[@dir='return']/@safearray" />
      <xsl:if test="$hasOutParms and not($hasReturnParms) and (string-length(@wsmap) = 0) and (count(param[@dir='out']) = 1)">
        <xsl:call-template name="fatalError">
          <xsl:with-param name="msg" select="concat('genMethod: ', $ifname, $hasOutParms, not($hasReturnParms), 'a', string-length(@wsmap) = 0, 'b', @wsmap, (count(param[@dir='out']) = 1), '::', $methodname, ' has exactly one out parameter and no return parameter, this causes trouble with JAX-WS and the out parameter needs to be converted to return')" />
        </xsl:call-template>
      </xsl:if>
      <xsl:variable name="returngluetype">
        <xsl:choose>
          <xsl:when test="$returnidltype">
            <xsl:call-template name="typeIdl2Glue">
              <xsl:with-param name="type" select="$returnidltype" />
              <xsl:with-param name="safearray" select="$returnidlsafearray" />
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>void</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="retValValue">
        <xsl:choose>
          <xsl:when test="(param[@dir='out']) and ($G_vboxGlueStyle='jaxws')">
            <xsl:text>retVal.value</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>retVal</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:apply-templates select="desc" mode="method"/>
      <xsl:value-of select="concat('    public ', $returngluetype, ' ', $methodname, '(')" />
      <xsl:variable name="paramsinout" select="param[@dir='in' or @dir='out']" />
      <xsl:for-each select="exsl:node-set($paramsinout)">
        <xsl:variable name="paramgluetype">
          <xsl:call-template name="typeIdl2Glue">
            <xsl:with-param name="type" select="@type" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="@dir='out'">
            <xsl:value-of select="concat('Holder&lt;', $paramgluetype, '&gt; ', @name)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($paramgluetype, ' ', @name)" />
          </xsl:otherwise>
        </xsl:choose>
        <xsl:if test="not(position()=last())">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>)&#10;</xsl:text>
      <xsl:text>    {&#10;</xsl:text>

      <xsl:call-template name="startExcWrapper">
          <xsl:with-param name="preventObjRelease" select="$hasReturnParms and ($returnidltype = '$unknown' or (count(key('G_keyInterfacesByName', $returnidltype)) > 0))" />
      </xsl:call-template>

      <!-- declare temp out params -->
      <xsl:for-each select="param[@dir='out']">
        <xsl:variable name="backouttype">
          <xsl:call-template name="typeIdl2Back">
            <xsl:with-param name="type" select="@type" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$G_vboxGlueStyle='xpcom'">
            <xsl:value-of select="concat('        ', $backouttype, '[] tmp_', @name, ' = (', $backouttype, '[])java.lang.reflect.Array.newInstance(', $backouttype, '.class, 1);&#10;')"/>
          </xsl:when>
          <xsl:when test="$G_vboxGlueStyle='mscom'">
            <xsl:value-of select="concat('        Variant tmp_', @name, ' = new Variant();&#10;')"/>
          </xsl:when>
          <xsl:when test="$G_vboxGlueStyle='jaxws'">
            <xsl:value-of select="concat('        javax.xml.ws.Holder&lt;', $backouttype, '&gt; tmp_', @name, ' = new javax.xml.ws.Holder&lt;', $backouttype, '&gt;();&#10;')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="fatalError">
              <xsl:with-param name="msg" select="'Handle out param (genMethod)'" />
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>

      <!-- declare return param, if any -->
      <xsl:if test="$hasReturnParms">
        <xsl:variable name="backrettype">
          <xsl:call-template name="typeIdl2Back">
            <xsl:with-param name="type" select="$returnidltype" />
            <xsl:with-param name="safearray" select="$returnidlsafearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="(param[@dir='out']) and ($G_vboxGlueStyle='jaxws')">
            <xsl:value-of select="concat('        javax.xml.ws.Holder&lt;', $backrettype, '&gt;',
                                  ' retVal = new javax.xml.ws.Holder&lt;', $backrettype,
                                  '&gt;();&#10;')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('            ', $backrettype, ' retVal;&#10;')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <!-- Method call -->
      <xsl:call-template name="genBackMethodCall">
        <xsl:with-param name="ifname" select="$ifname" />
        <xsl:with-param name="methodname" select="$methodname" />
        <xsl:with-param name="retval" select="'retVal'" />
      </xsl:call-template>

      <!-- return out params -->
      <xsl:for-each select="param[@dir='out']">
        <xsl:variable name="varval">
          <xsl:choose>
            <xsl:when test="$G_vboxGlueStyle='xpcom'">
              <xsl:value-of select="concat('tmp_', @name, '[0]')" />
            </xsl:when>
            <xsl:when test="$G_vboxGlueStyle='mscom'">
              <xsl:value-of select="concat('tmp_', @name)" />
            </xsl:when>
            <xsl:when test="$G_vboxGlueStyle='jaxws'">
              <xsl:value-of select="concat('tmp_', @name, '.value')" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="fatalError">
                <xsl:with-param name="msg" select="'Style unknown (genMethod, outparam)'" />
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="wrapped">
          <xsl:call-template name="cookOutParam">
            <xsl:with-param name="value" select="$varval" />
            <xsl:with-param name="idltype" select="@type" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('        ', @name, '.value = ', $wrapped, ';&#10;')"/>
      </xsl:for-each>

      <xsl:if test="$hasReturnParms">
        <!-- actual 'return' statement -->
        <xsl:variable name="wrapped">
          <xsl:call-template name="cookOutParam">
            <xsl:with-param name="value" select="$retValValue" />
            <xsl:with-param name="idltype" select="$returnidltype" />
            <xsl:with-param name="safearray" select="$returnidlsafearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('           return ', $wrapped, ';&#10;')" />
      </xsl:if>
      <xsl:call-template name="endExcWrapper">
        <xsl:with-param name="allowObjRelease" select="$hasReturnParms and ($returnidltype = '$unknown' or (count(key('G_keyInterfacesByName', $returnidltype)) > 0))" />
      </xsl:call-template>

      <xsl:text>    }&#10;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<!-- Callback interface method -->
<xsl:template name="genCbMethodDecl">
  <xsl:param name="ifname"/>
  <xsl:param name="methodname"/>

  <xsl:choose>
    <xsl:when test="(param[@mod='ptr'])" >
      <xsl:value-of select="concat('    // Skipping method ', $methodname, ' for it has parameters with suppressed types&#10;')" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="returnidltype" select="param[@dir='return']/@type" />
      <xsl:variable name="returnidlsafearray" select="param[@dir='return']/@safearray" />
      <xsl:variable name="returngluetype">
        <xsl:choose>
          <xsl:when test="$returnidltype">
            <xsl:call-template name="typeIdl2Glue">
              <xsl:with-param name="type" select="$returnidltype" />
              <xsl:with-param name="safearray" select="$returnidlsafearray" />
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>void</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:value-of select="concat('    public ', $returngluetype, ' ', $methodname, '(')" />
      <xsl:variable name="paramsinout" select="param[@dir='in' or @dir='out']" />
      <xsl:for-each select="exsl:node-set($paramsinout)">
        <xsl:variable name="paramgluetype">
          <xsl:call-template name="typeIdl2Glue">
            <xsl:with-param name="type" select="@type" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="@dir='out'">
            <xsl:value-of select="concat('Holder&lt;', $paramgluetype, '&gt; ', @name)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($paramgluetype, ' ', @name)" />
          </xsl:otherwise>
        </xsl:choose>
        <xsl:if test="not(position()=last())">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>);&#10;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- queryInterface wrapper -->
<xsl:template name="genQI">
  <xsl:param name="ifname"/>
  <xsl:param name="uuid" />

  <xsl:value-of select="concat('    public static ', $ifname, ' queryInterface(IUnknown obj)&#10;')" />
  <xsl:text>    {&#10;</xsl:text>
  <xsl:choose>
    <xsl:when test="$G_vboxGlueStyle='xpcom'">
      <xsl:variable name="backtype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="$ifname" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:text>        nsISupports nsobj = obj != null ? (nsISupports)obj.getWrapped() : null;&#10;</xsl:text>
      <xsl:text>        if (nsobj == null) return null;&#10;</xsl:text>
      <xsl:value-of select="concat('        ', $backtype, ' qiobj = Helper.queryInterface(nsobj, &quot;{', $uuid, '}&quot;, ', $backtype, '.class);&#10;')" />
      <xsl:value-of select="concat('        return qiobj == null ? null : new ', $ifname, '(qiobj);&#10;')" />
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='mscom'">
      <xsl:value-of select="concat('        return', ' obj == null ? null : new ', $ifname, '((com.jacob.com.Dispatch)obj.getWrapped());&#10;')" />
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='jaxws'">
      <!-- bad, need to check that we really can be casted to this type -->
      <xsl:value-of select="concat('        return obj == null ?  null : new ', $ifname, '(obj.getWrapped(), obj.getObjMgr(), obj.getRemoteWSPort());&#10;')" />
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Style unknown (genQI)'" />
      </xsl:call-template>
    </xsl:otherwise>

  </xsl:choose>
  <xsl:text>    }&#10;</xsl:text>
</xsl:template>


<xsl:template name="genCbMethodImpl">
  <xsl:param name="ifname"/>
  <xsl:param name="methodname"/>

  <xsl:choose>
    <xsl:when test="(param[@mod='ptr'])" >
      <xsl:value-of select="concat('    // Skipping method ', $methodname, ' for it has parameters with suppressed types&#10;')" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="hasReturnParms" select="param[@dir='return']" />
      <xsl:variable name="returnidltype" select="param[@dir='return']/@type" />
      <xsl:variable name="returnidlsafearray" select="param[@dir='return']/@safearray" />
      <xsl:variable name="returnbacktype">
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
      <xsl:variable name="paramsinout" select="param[@dir='in' or @dir='out']" />
      <xsl:choose>
        <xsl:when test="$G_vboxGlueStyle='xpcom'">
          <xsl:value-of select="concat('    public ', $returnbacktype, ' ', $methodname, '(')" />
          <xsl:for-each select="exsl:node-set($paramsinout)">
            <xsl:variable name="parambacktype">
              <xsl:call-template name="typeIdl2Back">
                <xsl:with-param name="type" select="@type" />
                <xsl:with-param name="safearray" select="@safearray" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:choose>
              <xsl:when test="@dir='out'">
                <xsl:value-of select="concat($parambacktype, '[] ', @name)" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:if test="@safearray='yes'">
                  <xsl:value-of select="concat('long len_', @name, ', ')" />
                </xsl:if>
                <xsl:value-of select="concat($parambacktype, ' ', @name)" />
              </xsl:otherwise>
            </xsl:choose>
            <xsl:if test="not(position()=last())">
              <xsl:text>, </xsl:text>
            </xsl:if>
          </xsl:for-each>
          <xsl:text>)&#10;</xsl:text>
          <xsl:text>    {&#10;</xsl:text>
        </xsl:when>

        <xsl:when test="$G_vboxGlueStyle='mscom'">
          <xsl:variable name="capsname">
            <xsl:call-template name="capitalize">
              <xsl:with-param name="str" select="$methodname" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat('    public ', $returnbacktype, ' ', $capsname, '(')" />
          <xsl:text>Variant _args[])&#10;</xsl:text>
          <xsl:text>    {&#10;</xsl:text>
          <xsl:for-each select="exsl:node-set($paramsinout)">
            <xsl:variable name="parambacktype">
              <xsl:call-template name="typeIdl2Back">
                <xsl:with-param name="type" select="@type" />
                <xsl:with-param name="safearray" select="@safearray" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="concat('        ', $parambacktype, ' ', @name, '=_args[', count(preceding-sibling::param), '];&#10;')" />
          </xsl:for-each>
        </xsl:when>

        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="'Style unknown (genSetterCall)'" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>

      <!-- declare temp out params -->
      <xsl:for-each select="param[@dir='out']">
        <xsl:variable name="glueouttype">
          <xsl:call-template name="typeIdl2Glue">
            <xsl:with-param name="type" select="@type" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('        Holder&lt;', $glueouttype, '&gt;     tmp_', @name, ' = new Holder&lt;', $glueouttype, '&gt;();&#10;')"/>
      </xsl:for-each>

      <!-- declare return param, if any -->
      <xsl:if test="$hasReturnParms">
        <xsl:variable name="gluerettype">
          <xsl:call-template name="typeIdl2Glue">
            <xsl:with-param name="type" select="$returnidltype" />
            <xsl:with-param name="safearray" select="$returnidlsafearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('        ', $gluerettype, '     retVal = &#10;')"/>
      </xsl:if>

      <!-- Method call -->
      <xsl:value-of select="concat('        sink.', $methodname, '(')"/>
      <xsl:for-each select="param[not(@dir='return')]">
        <xsl:choose>
          <xsl:when test="@dir='out'">
            <xsl:value-of select="concat('tmp_', @name)" />
          </xsl:when>
          <xsl:when test="@dir='in'">
            <xsl:variable name="wrapped">
              <xsl:call-template name="cookOutParam">
                <xsl:with-param name="value" select="@name" />
                <xsl:with-param name="idltype" select="@type" />
                <xsl:with-param name="safearray" select="@safearray" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="$wrapped"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="fatalError">
              <xsl:with-param name="msg" select="concat('Unsupported param dir: ', @dir, '&quot;.')" />
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:if test="not(position()=last())">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>);&#10;</xsl:text>

      <!-- return out params -->
      <xsl:for-each select="param[@dir='out']">

        <xsl:variable name="unwrapped">
          <xsl:call-template name="cookInParam">
            <xsl:with-param name="value" select="concat('tmp_', @name, '.value')" />
            <xsl:with-param name="idltype" select="@type" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$G_vboxGlueStyle='xpcom'">
            <xsl:value-of select="concat('        ', @name, '[0] = ', $unwrapped, ';&#10;')"/>
          </xsl:when>
          <xsl:when test="$G_vboxGlueStyle='mscom'">
            <xsl:value-of select="concat('        _args[', count(preceding-sibling::param), '] = ', $unwrapped, ';&#10;')"/>
          </xsl:when>
        </xsl:choose>
      </xsl:for-each>

      <xsl:if test="$hasReturnParms">
        <!-- actual 'return' statement -->
        <xsl:variable name="unwrapped">
          <xsl:call-template name="cookInParam">
            <xsl:with-param name="value" select="'retVal'" />
            <xsl:with-param name="idltype" select="$returnidltype" />
            <xsl:with-param name="safearray" select="$returnidlsafearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('        return ', $unwrapped, ';&#10;')" />
      </xsl:if>
      <xsl:text>    }&#10;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Interface method -->
<xsl:template name="genIfaceWrapper">
  <xsl:param name="ifname"/>

  <xsl:variable name="wrappedType">
    <xsl:call-template name="wrappedName">
      <xsl:with-param name="ifname" select="$ifname" />
    </xsl:call-template>
  </xsl:variable>

  <!-- Constructor -->
  <xsl:choose>
      <xsl:when test="($G_vboxGlueStyle='jaxws')">
        <xsl:value-of select="concat('    public ', $ifname, '(String wrapped, ObjectRefManager objMgr, VboxPortType port)&#10;')" />
        <xsl:text>    {&#10;</xsl:text>
        <xsl:text>        super(wrapped, objMgr, port);&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
      </xsl:when>

      <xsl:when test="($G_vboxGlueStyle='xpcom') or ($G_vboxGlueStyle='mscom')">
        <xsl:value-of select="concat('    public ', $ifname, '(',  $wrappedType, ' wrapped)&#10;')" />
        <xsl:text>    {&#10;</xsl:text>
        <xsl:text>        super(wrapped);&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>

        <!-- Typed wrapped object accessor -->
        <xsl:value-of select="concat('    public ', $wrappedType, ' getTypedWrapped()&#10;')" />
        <xsl:text>    {&#10;</xsl:text>
        <xsl:value-of select="concat('        return (', $wrappedType, ') getWrapped();&#10;')" />
        <xsl:text>    }&#10;</xsl:text>
      </xsl:when>

      <xsl:otherwise>
        <xsl:call-template name="fatalError">
          <xsl:with-param name="msg" select="'Style unknown (root, ctr)'" />
        </xsl:call-template>
      </xsl:otherwise>
  </xsl:choose>
  <!-- Attributes -->
  <xsl:for-each select="attribute[not(@mod='ptr')]">
    <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
    <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
    <xsl:variable name="attrsafearray"><xsl:value-of select="@safearray" /></xsl:variable>

    <xsl:choose>
      <xsl:when test="($G_vboxGlueStyle='jaxws') and ($attrtype=($G_setSuppressedInterfaces/@name))">
        <xsl:value-of select="concat('  // Skipping attribute ', $attrname, ' of suppressed type ', $attrtype, '&#10;&#10;')" />
      </xsl:when>
      <xsl:when test="($G_vboxGlueStyle='jaxws') and (@wsmap = 'suppress')" >
        <xsl:value-of select="concat('    // Skipping attribute ', $attrname, ' for it is suppressed&#10;')" />
      </xsl:when>

      <xsl:otherwise>
        <!-- emit getter method -->
        <xsl:apply-templates select="desc" mode="attribute_get"/>
        <xsl:variable name="gettername">
          <xsl:call-template name="makeGetterName">
            <xsl:with-param name="attrname" select="$attrname" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="gluetype">
          <xsl:call-template name="typeIdl2Glue">
            <xsl:with-param name="type" select="$attrtype" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="backtype">
          <xsl:call-template name="typeIdl2Back">
            <xsl:with-param name="type" select="$attrtype" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="wrapped">
          <xsl:call-template name="cookOutParam">
            <xsl:with-param name="value" select="'retVal'" />
            <xsl:with-param name="idltype" select="$attrtype" />
            <xsl:with-param name="safearray" select="@safearray" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('    public ', $gluetype, ' ', $gettername, '()&#10;')" />
        <xsl:text>    {&#10;</xsl:text>


        <xsl:call-template name="startExcWrapper">
          <xsl:with-param name="preventObjRelease" select="$attrtype = '$unknown' or (count(key('G_keyInterfacesByName', $attrtype)) > 0)" />
        </xsl:call-template>

        <!-- Actual getter implementation -->
        <xsl:call-template name="genGetterCall">
          <xsl:with-param name="ifname" select="$ifname" />
          <xsl:with-param name="gettername" select="$gettername" />
          <xsl:with-param name="backtype" select="$backtype" />
          <xsl:with-param name="retval" select="'retVal'" />
        </xsl:call-template>

        <xsl:value-of select="concat('            return ', $wrapped, ';&#10;')" />

        <xsl:call-template name="endExcWrapper">
          <xsl:with-param name="allowObjRelease" select="$attrtype = '$unknown' or (count(key('G_keyInterfacesByName', $attrtype)) > 0)" />
        </xsl:call-template>

        <xsl:text>    }&#10;</xsl:text>
        <xsl:if test="not(@readonly = 'yes')">
          <!-- emit setter method -->
          <xsl:apply-templates select="desc" mode="attribute_set"/>
          <xsl:variable name="settername"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname" /></xsl:call-template></xsl:variable>
          <xsl:variable name="unwrapped">
            <xsl:call-template name="cookInParam">
              <xsl:with-param name="ifname" select="$ifname" />
              <xsl:with-param name="value" select="'value'" />
              <xsl:with-param name="idltype" select="$attrtype" />
              <xsl:with-param name="safearray" select="@safearray" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:value-of select="concat('    public void ', $settername, '(', $gluetype, ' value)&#10;')" />
          <xsl:text>    {&#10;</xsl:text>
          <xsl:call-template name="startExcWrapper"/>
          <!-- Actual setter implementation -->
          <xsl:call-template name="genSetterCall">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="settername" select="$settername" />
            <xsl:with-param name="value" select="$unwrapped" />
          </xsl:call-template>
          <xsl:call-template name="endExcWrapper"/>
          <xsl:text>    }&#10;</xsl:text>
        </xsl:if>

      </xsl:otherwise>
    </xsl:choose>

  </xsl:for-each>

  <!-- emit queryInterface() *to* this class -->
  <xsl:call-template name="genQI">
    <xsl:with-param name="ifname" select="$ifname" />
    <xsl:with-param name="uuid" select="@uuid" />
  </xsl:call-template>

  <!-- emit methods -->
  <xsl:for-each select="method">
    <xsl:call-template name="genMethod">
      <xsl:with-param name="ifname" select="$ifname" />
      <xsl:with-param name="methodname" select="@name" />
    </xsl:call-template>
  </xsl:for-each>

</xsl:template>

<xsl:template name="genIface">
  <xsl:param name="ifname" />
  <xsl:param name="filename" />

  <xsl:variable name="wsmap" select="@wsmap" />

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="$filename" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text>import java.util.List;&#10;&#10;</xsl:text>

    <xsl:apply-templates select="desc" mode="interface"/>

    <xsl:choose>
      <xsl:when test="($wsmap='struct') and ($G_vboxGlueStyle='jaxws')">
        <xsl:value-of select="concat('public class ', $ifname, '&#10;')" />
        <xsl:text>{&#10;&#10;</xsl:text>
        <xsl:call-template name="genStructWrapperJaxws">
          <xsl:with-param name="ifname" select="$ifname" />
        </xsl:call-template>
      </xsl:when>

      <xsl:otherwise>
        <xsl:variable name="extends" select="key('G_keyInterfacesByName', $ifname)/@extends" />
        <xsl:choose>
          <xsl:when test="($extends = '$unknown') or ($extends = '$errorinfo')">
            <xsl:value-of select="concat('public class ', $ifname, ' extends IUnknown&#10;')" />
            <xsl:text>{&#10;&#10;</xsl:text>
          </xsl:when>
          <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
            <xsl:value-of select="concat('public class ', $ifname, ' extends ', $extends, '&#10;')" />
            <xsl:text>{&#10;&#10;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="fatalError">
              <xsl:with-param name="msg" select="concat('Interface generation: interface &quot;', $ifname, '&quot; has invalid &quot;extends&quot; value ', $extends, '.')" />
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="genIfaceWrapper">
          <xsl:with-param name="ifname" select="$ifname" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>

    <!-- end of class -->
    <xsl:text>}&#10;</xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="$filename" />
  </xsl:call-template>

</xsl:template>

<xsl:template name="genCb">
  <xsl:param name="ifname" />
  <xsl:param name="filename" />
  <xsl:param name="filenameimpl" />

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="$filename" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:text>import java.util.List;&#10;</xsl:text>

  <xsl:value-of select="concat('public interface ', $ifname, '&#10;')" />
  <xsl:text>{&#10;</xsl:text>

  <!-- emit methods declarations-->
  <xsl:for-each select="method">
    <xsl:call-template name="genCbMethodDecl">
      <xsl:with-param name="ifname" select="$ifname" />
      <xsl:with-param name="methodname" select="@name" />
    </xsl:call-template>
  </xsl:for-each>

  <xsl:text>}&#10;&#10;</xsl:text>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="$filename" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="$filenameimpl" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:text>import java.util.List;&#10;</xsl:text>

  <xsl:variable name="backtype">
    <xsl:call-template name="typeIdl2Back">
      <xsl:with-param name="type" select="$ifname" />
    </xsl:call-template>
  </xsl:variable>

  <!-- emit glue methods body -->
  <xsl:choose>
    <xsl:when test="$G_vboxGlueStyle='xpcom'">
      <xsl:value-of select="concat('class ', $ifname, 'Impl  extends nsISupportsBase implements ', $backtype, '&#10;')" />
      <xsl:text>{&#10;</xsl:text>
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='mscom'">
      <xsl:value-of select="concat('public class ', $ifname, 'Impl&#10;')" />
      <xsl:text>{&#10;</xsl:text>
    </xsl:when>
  </xsl:choose>

  <xsl:value-of select="concat('   ', $ifname, ' sink;&#10;')" />

  <xsl:value-of select="concat('   ', $ifname, 'Impl(', $ifname, ' sink)&#10;')" />
  <xsl:text>    {&#10;</xsl:text>
  <xsl:text>      this.sink = sink;&#10;</xsl:text>
  <xsl:text>    }&#10;</xsl:text>

  <!-- emit methods implementations -->
  <xsl:for-each select="method">
    <xsl:call-template name="genCbMethodImpl">
      <xsl:with-param name="ifname" select="$ifname" />
      <xsl:with-param name="methodname" select="@name" />
    </xsl:call-template>
  </xsl:for-each>

  <xsl:text>}&#10;&#10;</xsl:text>

  <xsl:call-template name="endFile">
      <xsl:with-param name="file" select="$filenameimpl" />
    </xsl:call-template>
</xsl:template>

<xsl:template name="emitHandwritten">

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'Holder.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[
public class Holder<T>
{
    public T value;

    public Holder()
    {
    }
    public Holder(T value)
    {
        this.value = value;
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'Holder.java'" />
  </xsl:call-template>
</xsl:template>

<xsl:template name="emitHandwrittenXpcom">

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'IUnknown.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackageCom" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[
public class IUnknown
{
    private Object obj;
    public IUnknown(Object obj)
    {
        this.obj = obj;
    }

    public Object getWrapped()
    {
        return this.obj;
    }

    public void setWrapped(Object obj)
    {
        this.obj = obj;
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'IUnknown.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'Helper.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackageCom" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[

import java.util.List;
import java.util.ArrayList;
import java.util.Collections;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

public class Helper
{
    public static List<Short> wrap(byte[] values)
    {
        if (values == null)
            return null;

        List<Short> ret = new ArrayList<Short>(values.length);
        for (short v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<Short> wrap(short[] values)
    {
        if (values == null)
            return null;

        List<Short> ret = new ArrayList<Short>(values.length);
        for (short v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<Integer> wrap(int[] values)
    {
        if (values == null)
            return null;

        List<Integer> ret = new ArrayList<Integer>(values.length);
        for (int v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<Long> wrap(long[] values)
    {
        if (values == null)
            return null;

        List<Long> ret = new ArrayList<Long>(values.length);
        for (long v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<Boolean> wrap(boolean[] values)
    {
        if (values == null)
            return null;

        List<Boolean> ret = new ArrayList<Boolean>(values.length);
        for (boolean v: values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<String> wrap(String[] values)
    {
        if (values == null)
            return null;

        List<String> ret = new ArrayList<String>(values.length);
        for (String v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static <T> List<T> wrap(Class<T> wrapperClass, T[] values)
    {
        if (values == null)
            return null;

        List<T> ret = new ArrayList<T>(values.length);
        for (T v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    @SuppressWarnings( "unchecked")
    public static <T> List<T> wrapEnum(Class<T> wrapperClass, long values[])
    {
        try
        {
            if (values == null)
                return null;
            //// This code is questionable, as it invokes a private constructor
            //// (all enums only have default constructors), and we don't really
            //// know what to pass as the name, and the ordinal may or may not
            //// be sensible, especially if the long was abused as a bitset.
            //Constructor<T> c = wrapperClass.getDeclaredConstructor(String.class, int.class, int.class);
            //c.setAccessible(true); // make it callable
            //List<T> ret = new ArrayList<T>(values.length);
            //for (long v : values)
            //{
            //    T convEnum = c.newInstance("unknown", (int)v, (int)v);
            //    ret.add(convEnum);
            //}

            // Alternative implementation: use the fromValue method, which is
            // what the code handling single enums will do. I see no reason to
            // use the above very ugly hack if there are better alternatives,
            // which as a bonus complain about unknown values. This variant is
            // slower, but also orders of magnitude safer.
            java.lang.reflect.Method fromValue = wrapperClass.getMethod("fromValue", long.class);
            List<T> ret = new ArrayList<T>(values.length);
            for (long v : values)
            {
                T convEnum = (T)fromValue.invoke(null, v);
                ret.add(convEnum);
            }
            return ret;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        //catch (InstantiationException e)
        //{
        //    throw new AssertionError(e);
        //}
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }
    public static short[] unwrapUShort(List<Short> values)
    {
        if (values == null)
            return null;

        short[] ret = new short[values.size()];
        int i = 0;
        for (short l : values)
        {
            ret[i++] = l;
        }
        return ret;
    }

    public static int[] unwrapInteger(List<Integer> values)
    {
        if (values == null)
            return null;

        int[] ret = new int[values.size()];
        int i = 0;
        for (int l : values)
        {
            ret[i++] = l;
        }
        return ret;
    }

    public static long[] unwrapULong(List<Long> values)
    {
        if (values == null)
            return null;

        long[] ret = new long[values.size()];
        int i = 0;
        for (long l : values)
        {
            ret[i++] = l;
        }
        return ret;
    }

    public static boolean[] unwrapBoolean(List<Boolean> values)
    {
        if (values == null)
            return null;

        boolean[] ret = new boolean[values.size()];
        int i = 0;
        for (boolean l : values)
        {
            ret[i++] = l;
        }
        return ret;
    }

    public static String[] unwrapStr(List<String> values)
    {
        if (values == null)
            return null;

        String[] ret = new String[values.size()];
        int i = 0;
        for (String l : values)
        {
            ret[i++] = l;
        }
        return ret;
    }

    public static <T extends Enum <T>> long[] unwrapEnum(Class<T> enumClass, List<T> values)
    {
        if (values == null)
            return null;

        long result[] = new long[values.size()];
        try
        {
            java.lang.reflect.Method valueM = enumClass.getMethod("value");
            int i = 0;
            for (T v : values)
            {
                result[i++] = (Integer)valueM.invoke(v);
            }
            return result;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch(SecurityException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalArgumentException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }

    public static <T1, T2> List<T1> wrap2(Class<T1> wrapperClass1, Class<T2> wrapperClass2, T2[] values)
    {
        try
        {
            if (values == null)
                return null;

            Constructor<T1> c = wrapperClass1.getConstructor(wrapperClass2);
            List<T1> ret = new ArrayList<T1>(values.length);
            for (T2 v : values)
            {
                ret.add(c.newInstance(v));
            }
            return ret;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch (InstantiationException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }

    @SuppressWarnings( "unchecked")
    public static <T> T[] unwrap(Class<T> wrapperClass, List<T> values)
    {
        if (values == null)
            return null;
        if (values.size() == 0)
            return null;
        return (T[])values.toArray((T[])Array.newInstance(wrapperClass, values.size()));
    }

    @SuppressWarnings( "unchecked" )
    public static <T> T queryInterface(Object obj, String uuid, Class<T> iface)
    {
        return (T)queryInterface(obj, uuid);
    }

    public static Object queryInterface(Object obj, String uuid)
    {
        try
        {
            /* Kind of ugly, but does the job of casting */
            org.mozilla.xpcom.Mozilla moz = org.mozilla.xpcom.Mozilla.getInstance();
            long xpobj = moz.wrapJavaObject(obj, uuid);
            return moz.wrapXPCOMObject(xpobj, uuid);
        }
        catch (Exception e)
        {
            return null;
        }
    }

    @SuppressWarnings("unchecked")
    public static <T1 extends IUnknown, T2> T2[] unwrap2(Class<T1> wrapperClass1, Class<T2> wrapperClass2, List<T1> values)
    {
        if (values == null)
            return null;

        T2 ret[] = (T2[])Array.newInstance(wrapperClass2, values.size());
        int i = 0;
        for (T1 obj : values)
        {
            ret[i++] = (T2)obj.getWrapped();
        }
        return ret;
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'Helper.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'VBoxException.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text>
import org.mozilla.xpcom.*;

public class VBoxException extends RuntimeException
{
    private int resultCode;
    private IVirtualBoxErrorInfo errorInfo;

    public VBoxException(String message)
    {
        super(message);
        resultCode = -1;
        errorInfo = null;
    }

    public VBoxException(String message, Throwable cause)
    {
        super(message, cause);
        if (cause instanceof org.mozilla.xpcom.XPCOMException)
        {
            resultCode = (int)((org.mozilla.xpcom.XPCOMException)cause).errorcode;
            try
            {
                Mozilla mozilla = Mozilla.getInstance();
                nsIServiceManager sm = mozilla.getServiceManager();
                nsIExceptionService es = (nsIExceptionService)sm.getServiceByContractID("@mozilla.org/exceptionservice;1", nsIExceptionService.NS_IEXCEPTIONSERVICE_IID);
                nsIExceptionManager em = es.getCurrentExceptionManager();
                nsIException ex = em.getCurrentException();
                errorInfo = new IVirtualBoxErrorInfo((org.mozilla.interfaces.IVirtualBoxErrorInfo)ex.queryInterface(org.mozilla.interfaces.IVirtualBoxErrorInfo.IVIRTUALBOXERRORINFO_IID));
            }
            catch (NullPointerException e)
            {
                e.printStackTrace();
                // nothing we can do
                errorInfo = null;
            }
        }
        else
            resultCode = -1;
    }

    public int getResultCode()
    {
        return resultCode;
    }

    public IVirtualBoxErrorInfo getVirtualBoxErrorInfo()
    {
        return errorInfo;
    }
}
</xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'VBoxException.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'VirtualBoxManager.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[

import java.io.File;

import org.mozilla.xpcom.*;
import org.mozilla.interfaces.*;

public class VirtualBoxManager
{
    private Mozilla mozilla;
    private IVirtualBox vbox;
    private nsIComponentManager componentManager;

    private VirtualBoxManager(Mozilla mozilla)
    {
        this.mozilla = mozilla;
        this.componentManager = mozilla.getComponentManager();
        this.vbox = new IVirtualBox((org.mozilla.interfaces.IVirtualBox) this.componentManager
                    .createInstanceByContractID("@virtualbox.org/VirtualBox;1",
                                                null,
                                                org.mozilla.interfaces.IVirtualBox.IVIRTUALBOX_IID));
    }

    public void connect(String url, String username, String passwd)
    {
        throw new VBoxException("Connect doesn't make sense for local bindings");
    }

    public void disconnect()
    {
        throw new VBoxException("Disconnect doesn't make sense for local bindings");
    }

    public static void initPerThread()
    {
    }

    public static void deinitPerThread()
    {
    }

    public String getClientAPIVersion()
    {
        return "]]></xsl:text>
    <xsl:value-of select="substring($G_vboxApiSuffix, 2)" />
    <xsl:text><![CDATA[";
    }

    public IVirtualBox getVBox()
    {
        return this.vbox;
    }

    public ISession getSessionObject()
    {
        return new ISession((org.mozilla.interfaces.ISession) componentManager
                .createInstanceByContractID("@virtualbox.org/Session;1", null,
                                            org.mozilla.interfaces.ISession.ISESSION_IID));
    }

    public ISession openMachineSession(IMachine m) throws Exception
    {
        ISession s = getSessionObject();
        m.lockMachine(s, LockType.Shared);
        return s;
    }

    public void closeMachineSession(ISession s)
    {
          if (s != null)
            s.unlockMachine();
    }

    private static boolean hasInstance = false;
    private static boolean isMozillaInited = false;

    public static synchronized VirtualBoxManager createInstance(String home)
    {
        if (hasInstance)
            throw new VBoxException("only one instance of VirtualBoxManager at a time allowed");
        if (home == null || home.equals(""))
            home = System.getProperty("vbox.home");

        if (home == null)
            throw new VBoxException("vbox.home Java property must be defined to use XPCOM bridge");

        File grePath = new File(home);

        Mozilla mozilla = Mozilla.getInstance();
        if (!isMozillaInited)
        {
            mozilla.initialize(grePath);
            try
            {
                mozilla.initXPCOM(grePath, null);
                isMozillaInited = true;
            }
            catch (Exception e)
            {
                e.printStackTrace();
                return null;
            }
        }

        hasInstance = true;

        return new VirtualBoxManager(mozilla);
    }

    public IEventListener createListener(Object sink)
    {
        return new IEventListener(new EventListenerImpl(sink));
    }

    public void cleanup()
    {
        deinitPerThread();
        // cleanup, we don't do that, as XPCOM bridge doesn't cleanly
        // shuts down, so we prefer to avoid native shutdown
        // mozilla.shutdownXPCOM(null);
        mozilla = null;
        hasInstance = false;
    }

    public void waitForEvents(long tmo)
    {
        mozilla.waitForEvents(tmo);
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'VirtualBoxManager.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'EventListenerImpl.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[
import org.mozilla.interfaces.*;

public class EventListenerImpl extends nsISupportsBase implements org.mozilla.interfaces.IEventListener
{
    private Object obj;
    private java.lang.reflect.Method handleEvent;
    EventListenerImpl(Object obj)
    {
        this.obj = obj;
        try
        {
            this.handleEvent = obj.getClass().getMethod("handleEvent", IEvent.class);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    public void handleEvent(org.mozilla.interfaces.IEvent ev)
    {
        try
        {
            if (obj != null && handleEvent != null)
                handleEvent.invoke(obj, ev != null ? new IEvent(ev) : null);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'EventListenerImpl.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'VBoxObjectBase.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[
abstract class nsISupportsBase implements org.mozilla.interfaces.nsISupports
{
    public org.mozilla.interfaces.nsISupports queryInterface(String iid)
    {
        return org.mozilla.xpcom.Mozilla.queryInterface(this, iid);
    }
}

]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'VBoxObjectBase.java'" />
  </xsl:call-template>
</xsl:template>


<xsl:template name="emitHandwrittenMscom">

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'IUnknown.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackageCom" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[
public class IUnknown
{
    private Object obj;
    public IUnknown(Object obj)
    {
        this.obj = obj;
    }

    public Object getWrapped()
    {
        return this.obj;
    }

    public void setWrapped(Object obj)
    {
        this.obj = obj;
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'IUnknown.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'Helper.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackageCom" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[

import java.util.List;
import java.util.ArrayList;
import java.util.Collections;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import com.jacob.com.*;

public class Helper
{
    public static List<Short> wrap(short[] values)
    {
        if (values == null)
            return null;

        List<Short> ret = new ArrayList<Short>(values.length);
        for (short v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<Integer> wrap(int[] values)
    {
        if (values == null)
            return null;

        List<Integer> ret = new ArrayList<Integer>(values.length);
        for (int v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<Long> wrap(long[] values)
    {
        if (values == null)
            return null;

        List<Long> ret = new ArrayList<Long>(values.length);
        for (long v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static List<String> wrap(String[] values)
    {
        if (values == null)
            return null;

        List<String> ret = new ArrayList<String>(values.length);
        for (String v : values)
        {
            ret.add(v);
        }
        return ret;
    }

    public static <T> T wrapDispatch(Class<T> wrapperClass, Dispatch d)
    {
        try
        {
            if (d == null || d.m_pDispatch == 0)
                return null;
            Constructor<T> c = wrapperClass.getConstructor(Dispatch.class);
            return (T)c.newInstance(d);
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch (InstantiationException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }

    @SuppressWarnings("unchecked")
    public static <T> Object wrapVariant(Class<T> wrapperClass, Variant v)
    {
        if (v == null)
            return null;

        short vt = v.getvt();
        switch (vt)
        {
            case Variant.VariantNull:
                return null;
            case Variant.VariantBoolean:
                return v.getBoolean();
            case Variant.VariantByte:
                return v.getByte();
            case Variant.VariantShort:
                return v.getShort();
            case Variant.VariantInt:
                return v.getInt();
            case Variant.VariantLongInt:
                return v.getLong();
            case Variant.VariantString:
                return v.getString();
            case Variant.VariantDispatch:
                return wrapDispatch(wrapperClass, v.getDispatch());
            default:
                throw new IllegalArgumentException("unhandled variant type " + vt);
        }
    }

    public static byte[] wrapBytes(SafeArray sa)
    {
        if (sa == null)
            return null;

        int saLen = sa.getUBound() - sa.getLBound() + 1;

        byte[] ret = new byte[saLen];
        int j = 0;
        for (int i = sa.getLBound(); i <= sa.getUBound(); i++)
        {
            Variant v = sa.getVariant(i);
            // come up with more effective approach!!!
            ret[j++] = v.getByte();
        }
        return ret;
    }

    @SuppressWarnings("unchecked")
    public static <T> List<T> wrap(Class<T> wrapperClass, SafeArray sa)
    {
        if (sa == null)
            return null;

        int saLen = sa.getUBound() - sa.getLBound() + 1;
        if (saLen == 0)
            return Collections.emptyList();

        List<T> ret = new ArrayList<T>(saLen);
        for (int i = sa.getLBound(); i <= sa.getUBound(); i++)
        {
            Variant v = sa.getVariant(i);
            ret.add((T)wrapVariant(wrapperClass, v));
        }
        return ret;
    }

    public static <T> List<T> wrapEnum(Class<T> wrapperClass, SafeArray sa)
    {
        try
        {
            if (sa == null)
                return null;

            int saLen = sa.getUBound() - sa.getLBound() + 1;
            if (saLen == 0)
                return Collections.emptyList();
            List<T> ret = new ArrayList<T>(saLen);
            Constructor<T> c = wrapperClass.getConstructor(int.class);
            for (int i = sa.getLBound(); i <= sa.getUBound(); i++)
            {
                Variant v = sa.getVariant(i);
                ret.add(c.newInstance(v.getInt()));
            }
            return ret;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch (InstantiationException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }

    public static SafeArray unwrapInt(List<Integer> values)
    {
        if (values == null)
            return null;
        SafeArray ret = new SafeArray(Variant.VariantInt, values.size());
        int i = 0;
        for (int l : values)
        {
            ret.setInt(i++, l);
        }
        return ret;
    }

    public static SafeArray unwrapLong(List<Long> values)
    {
        if (values == null)
            return null;
        SafeArray ret = new SafeArray(Variant.VariantLongInt, values.size());
        int i = 0;
        for (long l : values)
        {
            ret.setLong(i++, l);
        }
        return ret;
    }

    public static SafeArray unwrapBool(List<Boolean> values)
    {
        if (values == null)
            return null;

        SafeArray result = new SafeArray(Variant.VariantBoolean, values.size());
        int i = 0;
        for (boolean l : values)
        {
            result.setBoolean(i++, l);
        }
        return result;
    }


    public static SafeArray unwrapBytes(byte[] values)
    {
        if (values == null)
            return null;

        SafeArray result = new SafeArray(Variant.VariantByte, values.length);
        int i = 0;
        for (byte l : values)
        {
            result.setByte(i++, l);
        }
        return result;
    }


    public static <T extends Enum <T>> SafeArray unwrapEnum(Class<T> enumClass, List<T> values)
    {
        if (values == null)
            return null;

        SafeArray result = new SafeArray(Variant.VariantInt, values.size());
        try
        {
            java.lang.reflect.Method valueM = enumClass.getMethod("value");
            int i = 0;
            for (T v : values)
            {
                result.setInt(i++, (Integer)valueM.invoke(v));
            }
            return result;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch(SecurityException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalArgumentException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }
    public static SafeArray unwrapString(List<String> values)
    {
        if (values == null)
            return null;
        SafeArray result = new SafeArray(Variant.VariantString, values.size());
        int i = 0;
        for (String l : values)
        {
            result.setString(i++, l);
        }
        return result;
    }

    public static <T1, T2> List<T1> wrap2(Class<T1> wrapperClass1, Class<T2> wrapperClass2, T2[] values)
    {
        try
        {
            if (values == null)
                return null;
            if (values.length == 0)
                return Collections.emptyList();

            Constructor<T1> c = wrapperClass1.getConstructor(wrapperClass2);
            List<T1> ret = new ArrayList<T1>(values.length);
            for (T2 v : values)
            {
                ret.add(c.newInstance(v));
            }
            return ret;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch (InstantiationException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }

    @SuppressWarnings("unchecked")
    public static <T> T[] unwrap(Class<T> wrapperClass, List<T> values)
    {
        if (values == null)
            return null;
        return (T[])values.toArray((T[])Array.newInstance(wrapperClass, values.size()));
    }

    @SuppressWarnings("unchecked")
    public static <T1 extends IUnknown, T2> T2[] unwrap2(Class<T1> wrapperClass1, Class<T2> wrapperClass2, List<T1> values)
    {
        if (values == null)
            return null;

        T2 ret[] = (T2[])Array.newInstance(wrapperClass2, values.size());
        int i = 0;
        for (T1 obj : values)
        {
            ret[i++] = (T2)obj.getWrapped();
        }
        return ret;
    }

    /* We have very long invoke lists sometimes */
    public static Variant invoke(Dispatch d, String method, Object ... args)
    {
        return Dispatch.callN(d, method, args);
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'Helper.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'VBoxException.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text>

public class VBoxException extends RuntimeException
{
    private int resultCode;
    private IVirtualBoxErrorInfo errorInfo;

    public VBoxException(String message)
    {
        super(message);
        resultCode = -1;
        errorInfo = null;
    }

    public VBoxException(String message, Throwable cause)
    {
        super(message, cause);
        if (cause instanceof com.jacob.com.ComException)
        {
            resultCode = ((com.jacob.com.ComException)cause).getHResult();
            // JACOB doesn't support calling GetErrorInfo, which
            // means there is no way of getting an IErrorInfo reference,
            // and that means no way of getting to IVirtualBoxErrorInfo.
            errorInfo = null;
        }
        else
            resultCode = -1;
    }

    public int getResultCode()
    {
        return resultCode;
    }

    public IVirtualBoxErrorInfo getVirtualBoxErrorInfo()
    {
        return errorInfo;
    }
}
</xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'VBoxException.java'" />
  </xsl:call-template>


  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'VirtualBoxManager.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[

import com.jacob.activeX.ActiveXComponent;
import com.jacob.com.ComThread;
import com.jacob.com.Dispatch;
import com.jacob.com.Variant;
import com.jacob.com.SafeArray;
import com.jacob.com.DispatchEvents;

public class VirtualBoxManager
{
    private IVirtualBox vbox;

    private VirtualBoxManager()
    {
        initPerThread();
        vbox = new IVirtualBox(new ActiveXComponent("VirtualBox.VirtualBox"));
    }

    public static void initPerThread()
    {
        ComThread.InitMTA();
    }

    public static void deinitPerThread()
    {
        ComThread.Release();
    }

    public void connect(String url, String username, String passwd)
    {
        throw new VBoxException("Connect doesn't make sense for local bindings");
    }

    public void disconnect()
    {
        throw new VBoxException("Disconnect doesn't make sense for local bindings");
    }

    public String getClientAPIVersion()
    {
        return "]]></xsl:text>
    <xsl:value-of select="substring($G_vboxApiSuffix, 2)" />
    <xsl:text><![CDATA[";
    }

    public IVirtualBox getVBox()
    {
        return this.vbox;
    }

    public ISession getSessionObject()
    {
        return new ISession(new ActiveXComponent("VirtualBox.Session"));
    }

    public ISession openMachineSession(IMachine m)
    {
        ISession s = getSessionObject();
        m.lockMachine(s, LockType.Shared);
        return s;
    }

    public void closeMachineSession(ISession s)
    {
          if (s != null)
          s.unlockMachine();
    }

    private static boolean hasInstance = false;

    public static synchronized VirtualBoxManager createInstance(String home)
    {
        if (hasInstance)
            throw new VBoxException("only one instance of VirtualBoxManager at a time allowed");

        hasInstance = true;
        return new VirtualBoxManager();
    }

    public void cleanup()
    {
        deinitPerThread();
        hasInstance = false;
    }

    public void waitForEvents(long tmo)
    {
        try
        {
          Thread.sleep(tmo);
        }
        catch (InterruptedException ie)
        {
        }
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'VirtualBoxManager.java'" />
  </xsl:call-template>
</xsl:template>

<xsl:template name="emitHandwrittenJaxws">

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'IUnknown.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[
public class IUnknown
{
    protected String obj;
    protected ObjectRefManager objMgr;
    protected final VboxPortType port;

    public IUnknown(String obj, ObjectRefManager objMgr, VboxPortType port)
    {
        this.obj    = obj;
        this.objMgr = objMgr;
        this.port   = port;
        objMgr.registerObj(this);
    }

    public final String getWrapped()
    {
        return this.obj;
    }

    public final VboxPortType getRemoteWSPort()
    {
        return this.port;
    }

    public final ObjectRefManager getObjMgr()
    {
        return this.objMgr;
    }

    public synchronized void releaseRemote() throws WebServiceException
    {
        if (obj == null)
            return;

        try
        {
            this.port.iManagedObjectRefRelease(obj);
            this.obj = null;
        }
        catch (InvalidObjectFaultMsg e)
        {
            throw new WebServiceException(e);
        }
        catch (RuntimeFaultMsg e)
        {
            throw new WebServiceException(e);
        }
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'IUnknown.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'Helper.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text><![CDATA[

import java.util.List;
import java.util.ArrayList;
import java.util.Collections;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.math.BigInteger;

public class Helper
{
    public static <T> List<T> wrap(Class<T> wrapperClass, ObjectRefManager objMgr, VboxPortType pt, List<String> values)
    {
        try
        {
            if (values == null)
                return null;

            Constructor<T> c = wrapperClass.getConstructor(String.class, ObjectRefManager.class, VboxPortType.class);
            List<T> ret = new ArrayList<T>(values.size());
            for (String v : values)
            {
                ret.add(c.newInstance(v, objMgr, pt));
            }
            return ret;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch (InstantiationException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }

    public static <T1, T2> List<T1> wrap2(Class<T1> wrapperClass1, Class<T2> wrapperClass2, ObjectRefManager objMgr, VboxPortType pt, List<T2> values)
    {
        try
        {
            if (values == null)
                return null;

            Constructor<T1> c = wrapperClass1.getConstructor(wrapperClass2, ObjectRefManager.class, VboxPortType.class);
            List<T1> ret = new ArrayList<T1>(values.size());
            for (T2 v : values)
            {
                ret.add(c.newInstance(v, objMgr, pt));
            }
            return ret;
        }
        catch (NoSuchMethodException e)
        {
            throw new AssertionError(e);
        }
        catch (InstantiationException e)
        {
            throw new AssertionError(e);
        }
        catch (IllegalAccessException e)
        {
            throw new AssertionError(e);
        }
        catch (InvocationTargetException e)
        {
            throw new AssertionError(e);
        }
    }

    public static <T extends IUnknown> List<String> unwrap(List<T> values)
    {
        if (values == null)
            return null;

        List<String> ret = new ArrayList<String>(values.size());
        for (T obj : values)
        {
          ret.add(obj.getWrapped());
        }
        return ret;
    }

    @SuppressWarnings("unchecked" )
    public static <T1 extends Enum <T1>, T2 extends Enum <T2>> List<T2> convertEnums(Class<T1> fromClass,
                                                                                     Class<T2> toClass,
                                                                                     List<T1> values)
                                                                                     {
        try
        {
            if (values == null)
                return null;
            List<T2> ret = new ArrayList<T2>(values.size());
            for (T1 v : values)
            {
                // Ordinal based enum conversion, as JAX-WS "invents" its own
                // enum names and has string values with the expected content.
                int enumOrdinal = v.ordinal();
                T2 convEnum = toClass.getEnumConstants()[enumOrdinal];
                ret.add(convEnum);
            }
            return ret;
        }
        catch (ArrayIndexOutOfBoundsException e)
        {
            throw new AssertionError(e);
        }
    }

    /* Pretty naive Base64 encoder/decoder. */
    private static final char[] valToChar = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/".toCharArray();
    private static final int[] charToVal = new int[256];

    /* Initialize recoding alphabet. */
    static
    {
        for (int i = 0; i < charToVal.length; i++)
            charToVal[i] = -1;

        for (int i = 0; i < valToChar.length; i++)
            charToVal[valToChar[i]] = i;

        charToVal['='] = 0;
    }

    public static String encodeBase64(byte[] data)
    {
        if (data == null)
            return null;

        if (data.length == 0)
            return "";

        int fullTriplets = data.length / 3;
        int resultLen = ((data.length - 1) / 3 + 1) * 4;
        char[] result = new char[resultLen];
        int dataIndex = 0, stringIndex = 0;

        for (int i = 0; i < fullTriplets; i++)
        {
            int ch1 = data[dataIndex++] & 0xff;
            result[stringIndex++] = valToChar[ch1 >> 2];
            int ch2 = data[dataIndex++] & 0xff;
            result[stringIndex++] = valToChar[((ch1 << 4) & 0x3f) | (ch2 >> 4)];
            int ch3 = data[dataIndex++] & 0xff;
            result[stringIndex++] = valToChar[((ch2 << 2) & 0x3f) | (ch3 >> 6)];
            result[stringIndex++] = valToChar[ch3 & 0x3f];
        }

        switch (data.length - dataIndex)
        {
            case 0:
                // do nothing
                break;
            case 1:
            {
                int ch1 = data[dataIndex++] & 0xff;
                result[stringIndex++] = valToChar[ch1 >> 2];
                result[stringIndex++] = valToChar[(ch1 << 4) & 0x3f];
                result[stringIndex++] = '=';
                result[stringIndex++] = '=';
                break;
            }
            case 2:
            {
                int ch1 = data[dataIndex++] & 0xff;
                result[stringIndex++] = valToChar[ch1 >> 2];
                int ch2 = data[dataIndex++] & 0xff;
                result[stringIndex++] = valToChar[((ch1 << 4) & 0x3f) | (ch2 >> 4)];
                result[stringIndex++] = valToChar[(ch2 << 2) & 0x3f];
                result[stringIndex++] = '=';
                break;
            }
            default:
                throw new VBoxException("bug!");
        }

        return new String(result);
    }

    private static int skipInvalid(String str, int stringIndex)
    {
        while (charToVal[str.charAt(stringIndex)] < 0)
            stringIndex++;

        return stringIndex;
    }

    public static byte[] decodeBase64(String str)
    {
        if (str == null)
            return null;

        int stringLength = str.length();
        if (stringLength == 0)
            return new byte[0];

        int validChars = 0, padChars = 0;
        for (int i = 0; i < str.length(); i++)
        {
            char ch = str.charAt(i);

            if (charToVal[ch] >= 0)
                validChars++;

            if (ch == '=')
                padChars++;
        }

        if ((validChars * 3 % 4) != 0)
            throw new VBoxException("invalid base64 encoded string " + str);

        int resultLength = validChars * 3 / 4 - padChars;
        byte[] result = new byte[resultLength];

        int dataIndex = 0, stringIndex = 0;
        int quadraplets = validChars / 4;

        for (int i = 0; i < quadraplets; i++)
        {
            stringIndex = skipInvalid(str, stringIndex);
            int ch1 = str.charAt(stringIndex++);
            stringIndex = skipInvalid(str, stringIndex);
            int ch2 = str.charAt(stringIndex++);
            stringIndex = skipInvalid(str, stringIndex);
            int ch3 = str.charAt(stringIndex++);
            stringIndex = skipInvalid(str, stringIndex);
            int ch4 = str.charAt(stringIndex++);

            result[dataIndex++] = (byte)(((charToVal[ch1] << 2) | charToVal[ch2] >> 4) & 0xff);
            /* we check this to ensure that we don't override data with '=' padding. */
            if (dataIndex < result.length)
                result[dataIndex++] = (byte)(((charToVal[ch2] << 4) | charToVal[ch3] >> 2) & 0xff);
            if (dataIndex < result.length)
                result[dataIndex++] = (byte)(((charToVal[ch3] << 6) | charToVal[ch4]) & 0xff);
        }

        return result;
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'Helper.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'VBoxException.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text>
public class VBoxException extends RuntimeException
{
    private int resultCode;
    private IVirtualBoxErrorInfo errorInfo;

    public VBoxException(String message)
    {
        super(message);
        resultCode = -1;
        errorInfo = null;
    }

    public VBoxException(String message, Throwable cause)
    {
        super(message, cause);
        resultCode = -1;
        errorInfo = null;
    }

    public VBoxException(String message, Throwable cause, ObjectRefManager objMgr, VboxPortType port)
    {
        super(message, cause);
        if (cause instanceof RuntimeFaultMsg)
        {
            RuntimeFaultMsg m = (RuntimeFaultMsg)cause;
            RuntimeFault f = m.getFaultInfo();
            resultCode = f.getResultCode();
            String retVal = f.getReturnval();
            errorInfo = (retVal.length() > 0) ? new IVirtualBoxErrorInfo(retVal, objMgr, port) : null;
        }
        else
            resultCode = -1;
    }

    public int getResultCode()
    {
        return resultCode;
    }

    public IVirtualBoxErrorInfo getVirtualBoxErrorInfo()
    {
        return errorInfo;
    }
}
</xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'VBoxException.java'" />
  </xsl:call-template>

  <xsl:call-template name="startFile">
    <xsl:with-param name="file" select="'VirtualBoxManager.java'" />
    <xsl:with-param name="package" select="$G_virtualBoxPackage" />
  </xsl:call-template>

  <xsl:if test="$filelistonly=''">
    <xsl:text>import java.net.URL;
import java.math.BigInteger;
import java.util.Iterator;
import java.util.List;
import java.util.LinkedList;
import java.util.Map;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.lang.Integer;
import java.lang.ref.WeakReference;
import java.lang.ref.ReferenceQueue;
import javax.xml.namespace.QName;
import javax.xml.ws.BindingProvider;
import javax.xml.ws.Holder;
import javax.xml.ws.WebServiceException;
import java.io.IOException;
import java.net.UnknownHostException;
import java.net.Socket;
import java.net.InetAddress;
import javax.net.SocketFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.SSLSocket;

class PortPool
{
    private final static String wsdlFile = </xsl:text>
    <xsl:value-of select="$G_virtualBoxWsdl" />
    <xsl:text><![CDATA[;
    private Map<VboxPortType, Integer> known;
    private boolean initStarted;
    private VboxService svc;

    PortPool(boolean usePreinit)
    {
        known = new HashMap<VboxPortType, Integer>();

        if (usePreinit)
        {
            new Thread(new Runnable()
                {
                    public void run()
                    {
                        // need to sync on something else but 'this'
                        synchronized (known)
                        {
                            initStarted = true;
                            known.notify();
                        }

                        preinit();
                    }
                }).start();

            synchronized (known)
            {
                while (!initStarted)
                {
                    try
                    {
                        known.wait();
                    }
                    catch (InterruptedException e)
                    {
                        break;
                    }
                }
            }
        }
    }

    private synchronized void preinit()
    {
        VboxPortType port = getPort();
        releasePort(port);
    }

    synchronized VboxPortType getPort()
    {
        VboxPortType port = null;
        int ttl = 0;

        for (VboxPortType cur: known.keySet())
        {
            int value = known.get(cur);
            if ((value & 0x10000) == 0)
            {
                port = cur;
                ttl = value & 0xffff;
                break;
            }
        }

        if (port == null)
        {
            if (svc == null)
            {
                URL wsdl = PortPool.class.getClassLoader().getResource(wsdlFile);
                if (wsdl == null)
                    throw new LinkageError(wsdlFile + " not found, but it should have been in the jar");
                svc = new VboxService(wsdl,
                                      new QName("http://www.virtualbox.org/Service",
                                                "vboxService"));
            }
            port = svc.getVboxServicePort();
            // reuse this object 0x10 times
            ttl = 0x10;
        }
        // mark as used
        known.put(port, new Integer(0x10000 | ttl));
        return port;
    }

    synchronized void releasePort(VboxPortType port)
    {
        Integer val = known.get(port);
        if (val == null || val == 0)
        {
            // know you not
            return;
        }

        int v = val;
        int ttl = v & 0xffff;
        // decrement TTL, and throw away port if used too much times
        if (--ttl <= 0)
        {
            known.remove(port);
        }
        else
        {
            v = ttl; // set new TTL and clear busy bit
            known.put(port, v);
        }
    }
}


/**
 * This class manages the object references between us and the webservice server.
 * It makes sure that the object on the server side is destroyed when all
 */
class ObjectRefManager
{
    private final static ReferenceQueue<IUnknown> refQ = new ReferenceQueue<IUnknown>();

    private final ConcurrentMap<String, ManagedObj> map = new ConcurrentHashMap<String, ManagedObj>();
    private final ReentrantReadWriteLock lock = new ReentrantReadWriteLock();
    private final ObjRefMgrCleanupThread objRefMgrCleanup;

    public ObjectRefManager()
    {
        this.objRefMgrCleanup = new ObjRefMgrCleanupThread(this, 100);
        this.objRefMgrCleanup.start();
    }

    /**
     * Prevents the object reference manager cleanup thread from releasing any
     * server side objects to avoid a fundamental race in the multi threaded
     * java environment where it is possible that a wrapper got the object ID
     * from the server but couldn't create the local stub protecting the object
     * before the cleanup thread released it.
     */
    public void preventObjRelease()
    {
        lock.readLock().lock();
    }

    /**
     * Allows releasing server side objects from the cleanup thread again.
     */
    public void allowObjRelease()
    {
        lock.readLock().unlock();
    }

    /**
     * Marks the start of a run to release server side objects which don't hold
     * a reference locally anymore.
     */
    public void startObjRelease()
    {
        lock.writeLock().lock();
    }

    /**
     * Marks the end of a cleanup run.
     */
    public void endObjRelease()
    {
        lock.writeLock().unlock();
    }

    /**
     * Registers a new stub object for automatic reference managing.
     */
    public void registerObj(IUnknown obj)
    {
        assert lock.getReadLockCount() > 0;
        ManagedObjRef ref = new ManagedObjRef(obj);

        ManagedObj mgrobj = map.get(obj.getWrapped());
        if (mgrobj != null)
        {
            mgrobj.addObject(ref);
        }
        else
        {
            /* Create new. */
            mgrobj = new ManagedObj(obj.getWrapped(), obj.getRemoteWSPort());
            mgrobj.addObject(ref);
            map.put(obj.getWrapped(), mgrobj);
        }
    }

    /**
     * Removes a garbage collected object reference from our reference manager.
     *
     * Returns the server side object wrapper if there is no stub referencing it
     * anymore otherwise null is returned.
     */
    public ManagedObj unregisterObj(ManagedObjRef objRef)
    {
        ManagedObj obj = this.map.get(objRef.objId);

        assert obj != null;
        obj.removeObject(objRef);
        if (!obj.isReferenced())
            return obj;

        return null;
    }

    public void releaseRemoteObj(ManagedObj obj)
    {
        assert lock.isWriteLockedByCurrentThread();

        if (!obj.isReferenced())
        {
            try
            {
                obj.port.iManagedObjectRefRelease(obj.objId);
            }
            catch (InvalidObjectFaultMsg e)
            {
                throw new WebServiceException(e);
            }
            catch (RuntimeFaultMsg e)
            {
                throw new WebServiceException(e);
            }
            finally
            {
                this.map.remove(obj.objId);
            }
        }
    }

    /**
     * An object which is living on the server side. This can be referenced
     * by multiple stub objects here.
     */
    static class ManagedObj
    {
        private final String                               objId;
        private final VboxPortType                         port;
        private final ConcurrentLinkedQueue<ManagedObjRef> refQ;

        ManagedObj(String objId,  VboxPortType port)
        {
          this.objId = objId;
          this.port  = port;
          this.refQ  = new ConcurrentLinkedQueue<ManagedObjRef>();
        }

        public void addObject(ManagedObjRef obj)
        {
            this.refQ.add(obj);
        }

        public void removeObject(ManagedObjRef obj)
        {
            this.refQ.remove(obj);
        }

        public boolean isReferenced()
        {
            return !this.refQ.isEmpty();
        }
    }

    /**
     * A private class extending WeakReference to get notified about garbage
     * collected stub objects.
     */
    static class ManagedObjRef extends WeakReference<IUnknown>
    {
        final String objId;

        ManagedObjRef(IUnknown obj)
        {
            super(obj, refQ);
            this.objId = obj.getWrapped();
        }
    }

    /**
     * A private class implementing a thread getting notified
     * about garbage collected objects so it can release the object on the
     * server side if it is not used anymore.
     */
    static class ObjRefMgrCleanupThread extends Thread
    {
        ObjectRefManager            objRefMgr;
        int                         cStubsReleased;
        int                         cStubsReleaseThreshold;
        HashMap<String, ManagedObj> mapToRelease = new HashMap<String, ManagedObj>();

        ObjRefMgrCleanupThread(ObjectRefManager objRefMgr)
        {
            init(objRefMgr, 500);
        }

        ObjRefMgrCleanupThread(ObjectRefManager objRefMgr, int cStubsReleaseThreshold)
        {
            init(objRefMgr, cStubsReleaseThreshold);
        }

        private void init(ObjectRefManager objRefMgr, int cStubsReleaseThreshold)
        {
            this.objRefMgr = objRefMgr;
            this.cStubsReleased = 0;
            this.cStubsReleaseThreshold = cStubsReleaseThreshold;
            setName("ObjectRefManager-VBoxWSObjRefGcThrd");
            /*
             * setDaemon() makes sure the jvm exits and is not blocked
             * if the thread is still running so we don't have to care about
             * tearing it down.
             */
            setDaemon(true);
        }

        public void run()
        {
            while (true)
            {
                while (cStubsReleased < cStubsReleaseThreshold)
                {
                    try
                    {
                        /* Accumulate a few objects before we start. */
                        while (cStubsReleased < cStubsReleaseThreshold)
                        {
                            ManagedObjRef ref = (ManagedObjRef)refQ.remove();
                            ManagedObj obj = this.objRefMgr.unregisterObj(ref);
                            /*
                             * If the server side object is not referenced anymore
                             * promote to map for releasing later.
                             */
                            if (obj != null && !mapToRelease.containsKey(ref.objId))
                                mapToRelease.put(ref.objId, obj);

                            cStubsReleased++;
                        }
                    }
                    catch (InterruptedException e)
                    { /* ignore */ }
                    catch (javax.xml.ws.WebServiceException e)
                    { /* ignore */ }
                }

                /*
                 * After we released enough stubs we go over all non referenced
                 * server side objects and release them if they were not
                 * referenced again in between.
                 */
                cStubsReleased = 0;
                if (!mapToRelease.isEmpty())
                {
                    this.objRefMgr.startObjRelease();
                    try
                    {
                        Iterator<ManagedObj> it = mapToRelease.values().iterator();
                        while (it.hasNext())
                        {
                            ManagedObj obj = it.next();
                            this.objRefMgr.releaseRemoteObj(obj);
                        }

                        mapToRelease.clear();
                    }
                    catch (javax.xml.ws.WebServiceException e)
                    { /* ignore */ }
                    finally
                    {
                        this.objRefMgr.endObjRelease();
                    }
                }
            }
        }
    }
}

class VBoxTLSSocketFactory extends SSLSocketFactory
{
    private final SSLSocketFactory sf;

    private void setupSocket(SSLSocket s)
    {
        String[] oldproto = s.getEnabledProtocols();
        List<String> protolist = new ArrayList<String>();
        for (int i = 0; i < oldproto.length; i++)
            if (oldproto[i].toUpperCase().startsWith("TLS"))
                protolist.add(oldproto[i]);
        String[] newproto = protolist.toArray(new String[protolist.size()]);
        s.setEnabledProtocols(newproto);
    }

    public VBoxTLSSocketFactory()
    {
        SSLSocketFactory tmp = null;
        try
        {
            SSLContext sc = SSLContext.getInstance("TLS");
            sc.init(null, null, null);
            tmp = sc.getSocketFactory();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        sf = tmp;
    }

    public static SocketFactory getDefault()
    {
        return new VBoxTLSSocketFactory();
    }

    public Socket createSocket(Socket socket, String host, int port,
                               boolean autoClose) throws IOException, UnknownHostException
    {
        SSLSocket s = (SSLSocket)sf.createSocket(socket, host, port, autoClose);
        setupSocket(s);
        return s;
    }

    public Socket createSocket() throws IOException
    {
        SSLSocket s = (SSLSocket)sf.createSocket();
        setupSocket(s);
        return s;
    }

    public Socket createSocket(InetAddress host, int port) throws IOException
    {
        SSLSocket s = (SSLSocket)sf.createSocket(host, port);
        setupSocket(s);
        return s;
    }

    public Socket createSocket(InetAddress address, int port,
                               InetAddress localAddress, int localPort) throws IOException
    {
        SSLSocket s = (SSLSocket)sf.createSocket(address, port, localAddress, localPort);
        setupSocket(s);
        return s;
    }

    public Socket createSocket(String host, int port) throws IOException, UnknownHostException
    {
        SSLSocket s = (SSLSocket)sf.createSocket(host, port);
        setupSocket(s);
        return s;
    }

    public Socket createSocket(String host, int port,
                               InetAddress localHost, int localPort) throws IOException, UnknownHostException
    {
        SSLSocket s = (SSLSocket)sf.createSocket(host, port, localHost, localPort);
        setupSocket(s);
        return s;
    }

    public String[] getDefaultCipherSuites()
    {
        return sf.getDefaultCipherSuites();
    }

    public String[] getSupportedCipherSuites()
    {
        return sf.getSupportedCipherSuites();
    }
}


public class VirtualBoxManager
{
    private static PortPool pool = new PortPool(true);
    private static final ObjectRefManager objMgr = new ObjectRefManager();
    protected VboxPortType port;

    private IVirtualBox vbox;

    private VirtualBoxManager()
    {
    }

    public static void initPerThread()
    {
    }

    public static void deinitPerThread()
    {
    }

    public void connect(String url, String username, String passwd)
    {
        this.port = pool.getPort();
        try
        {
            ((BindingProvider)port).getRequestContext().
                put(BindingProvider.ENDPOINT_ADDRESS_PROPERTY, url);

            // Unfortunately there is no official way to make JAX-WS use
            // TLS only, which means that a rather tedious approach is
            // unavoidable (implementing a TLS only SSLSocketFactory,
            // because the default one associated with a TLS SSLContext
            // happily uses SSLv2/3 handshakes, which make TLS servers
            // drop the connection), and additionally a not standardized,
            // shotgun approach is needed to make the relevant JAX-WS
            // implementations use this factory.
            VBoxTLSSocketFactory sf = new VBoxTLSSocketFactory();
            ((BindingProvider)port).getRequestContext().
                put("com.sun.xml.internal.ws.transport.https.client.SSLSocketFactory", sf);
            ((BindingProvider)port).getRequestContext().
                put("com.sun.xml.ws.transport.https.client.SSLSocketFactory", sf);

            String handle = port.iWebsessionManagerLogon(username, passwd);
            this.objMgr.preventObjRelease();
            try
            {
                this.vbox = new IVirtualBox(handle, this.objMgr, port);
            }
            finally
            {
                this.objMgr.allowObjRelease();
            }
        }
        catch (Throwable t)
        {
            if (this.port != null && pool != null)
            {
                pool.releasePort(this.port);
                this.port = null;
            }
            // we have to throw smth derived from RuntimeException
            throw new VBoxException(t.getMessage(), t, this.objMgr, this.port);
        }
    }

    public void connect(String url, String username, String passwd,
                        Map<String, Object> requestContext, Map<String, Object> responseContext)
    {
        this.port = pool.getPort();
        try
        {
            ((BindingProvider)port).getRequestContext();
            if (requestContext != null)
                ((BindingProvider)port).getRequestContext().putAll(requestContext);

            if (responseContext != null)
                ((BindingProvider)port).getResponseContext().putAll(responseContext);

            ((BindingProvider)port).getRequestContext().
                put(BindingProvider.ENDPOINT_ADDRESS_PROPERTY, url);
            String handle = port.iWebsessionManagerLogon(username, passwd);
            this.objMgr.preventObjRelease();
            try
            {
                this.vbox = new IVirtualBox(handle, this.objMgr, port);
            }
            finally
            {
                this.objMgr.allowObjRelease();
            }
        }
        catch (Throwable t)
        {
            if (this.port != null && pool != null)
            {
                pool.releasePort(this.port);
                this.port = null;
            }
            // we have to throw smth derived from RuntimeException
            throw new VBoxException(t.getMessage(), t, this.objMgr, this.port);
        }
    }

    public void disconnect()
    {
        if (this.port == null)
            return;

        try
        {
            if (this.vbox != null && port != null)
                port.iWebsessionManagerLogoff(this.vbox.getWrapped());
        }
        catch (InvalidObjectFaultMsg e)
        {
            throw new VBoxException(e.getMessage(), e, this.objMgr, this.port);
        }
        catch (RuntimeFaultMsg e)
        {
            throw new VBoxException(e.getMessage(), e, this.objMgr, this.port);
        }
        finally
        {
            if (this.port != null)
            {
                pool.releasePort(this.port);
                this.port = null;
            }
        }
    }

    public String getClientAPIVersion()
    {
        return "]]></xsl:text>
    <xsl:value-of select="substring($G_vboxApiSuffix, 2)" />
    <xsl:text><![CDATA[";
    }

    public IVirtualBox getVBox()
    {
        return this.vbox;
    }

    public ISession getSessionObject()
    {
        if (this.vbox == null)
            throw new VBoxException("connect first");
        try
        {
            String handle = port.iWebsessionManagerGetSessionObject(this.vbox.getWrapped());
            this.objMgr.preventObjRelease();
            try
            {
                return new ISession(handle, this.objMgr, port);
            }
            finally
            {
                this.objMgr.allowObjRelease();
            }
        }
        catch (InvalidObjectFaultMsg e)
        {
            throw new VBoxException(e.getMessage(), e, this.objMgr, this.port);
        }
        catch (RuntimeFaultMsg e)
        {
            throw new VBoxException(e.getMessage(), e, this.objMgr, this.port);
        }
    }

    public ISession openMachineSession(IMachine m) throws Exception
    {
        ISession s = getSessionObject();
        m.lockMachine(s, LockType.Shared);
        return s;
    }

    public void closeMachineSession(ISession s)
    {
          if (s != null)
            s.unlockMachine();
    }

    public static synchronized VirtualBoxManager createInstance(String home)
    {
        return new VirtualBoxManager();
    }

    public IEventListener createListener(Object sink)
    {
        throw new VBoxException("no active listeners here");
    }

    public void cleanup()
    {
        disconnect();
        deinitPerThread();
    }

    public void waitForEvents(long tmo)
    {
        try
        {
          Thread.sleep(tmo);
        }
        catch (InterruptedException ie)
        {
        }
    }

    protected void finalize() throws Throwable
    {
        try
        {
            cleanup();
        }
        catch(Exception e)
        {
        }
        finally
        {
            super.finalize();
        }
    }
}
]]></xsl:text>
  </xsl:if>

  <xsl:call-template name="endFile">
    <xsl:with-param name="file" select="'VirtualBoxManager.java'" />
  </xsl:call-template>
</xsl:template>


<xsl:template match="/">

  <xsl:if test="not($G_vboxApiSuffix)">
    <xsl:call-template name="fatalError">
      <xsl:with-param name="msg" select="'G_vboxApiSuffix must be given'" />
    </xsl:call-template>
  </xsl:if>

  <xsl:if test="not($filelistonly='')">
    <xsl:value-of select="concat($filelistonly, ' :=')"/>
  </xsl:if>

  <!-- Handwritten files -->
  <xsl:call-template name="emitHandwritten"/>

  <xsl:choose>
    <xsl:when test="$G_vboxGlueStyle='xpcom'">
      <xsl:call-template name="emitHandwrittenXpcom"/>
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='mscom'">
      <xsl:call-template name="emitHandwrittenMscom"/>
    </xsl:when>

    <xsl:when test="$G_vboxGlueStyle='jaxws'">
      <xsl:call-template name="emitHandwrittenJaxws"/>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="'Style unknown (root)'" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

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

    <xsl:choose>
      <xsl:when test="$G_vboxGlueStyle='jaxws'">
        <xsl:if test="not($module) and not(@wsmap='suppress')">
          <xsl:call-template name="genIface">
            <xsl:with-param name="ifname" select="@name" />
            <xsl:with-param name="filename" select="concat(@name, '.java')" />
          </xsl:call-template>
        </xsl:if>
      </xsl:when>

      <xsl:when test="$G_vboxGlueStyle='xpcom'">
        <!-- We don't need WSDL-specific nor MIDL-specific interfaces here -->
        <xsl:if test="not(@internal='yes') and not($self_target='wsdl') and not($module) and not($self_target='midl')">
          <xsl:call-template name="genIface">
            <xsl:with-param name="ifname" select="@name" />
            <xsl:with-param name="filename" select="concat(@name, '.java')" />
          </xsl:call-template>
        </xsl:if>
      </xsl:when>

      <xsl:otherwise>
        <!-- We don't need WSDL-specific interfaces here -->
        <xsl:if test="not(@internal='yes') and not($self_target='wsdl') and not($module)">
          <xsl:call-template name="genIface">
            <xsl:with-param name="ifname" select="@name" />
            <xsl:with-param name="filename" select="concat(@name, '.java')" />
          </xsl:call-template>
        </xsl:if>
      </xsl:otherwise>

    </xsl:choose>
  </xsl:for-each>

  <xsl:if test="not($filelistonly='')">
    <xsl:value-of select="concat($G_sNewLine, $G_sNewLine)"/>
  </xsl:if>

</xsl:template>
</xsl:stylesheet>
