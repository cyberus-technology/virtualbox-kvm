<?xml version="1.0"?>

<!--
    xidl2docbook.xsl:
        XSLT stylesheet that generates docbook from
        VirtualBox.xidl.
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
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl">

  <xsl:output
             method="xml"
             version="1.0"
             encoding="utf-8"
             indent="yes"/>

  <xsl:strip-space elements="*"/>

 <!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyEnumsByName"        match="//enum[@name]"       use="@name"/>
<xsl:key name="G_keyInterfacesByName"   match="//interface[@name]"  use="@name"/>
<xsl:key name="G_keyResultsByName"      match="//result[@name]"     use="@name"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_xsltFilename" select="'glue-jaxws.xsl'" />

<!-- collect all interfaces with "wsmap='suppress'" in a global variable for
     quick lookup -->
<xsl:variable name="G_setSuppressedInterfaces"
              select="//interface[@wsmap='suppress']" />

<xsl:template name="makeLinkId">
  <xsl:param name="ifname" />
  <xsl:param name="member" />
  <xsl:value-of select="concat($ifname, '__', $member)"/>
</xsl:template>

<xsl:template name="emitType">
  <xsl:param name="type" />
  <xsl:choose>
    <xsl:when test="$type">
      <xsl:choose>
        <xsl:when test="count(key('G_keyInterfacesByName',$type)) > 0">
          <link>
            <xsl:attribute name="linkend">
              <xsl:value-of select="translate($type, ':', '_')" />
            </xsl:attribute>
            <xsl:value-of select="$type" />
          </link>
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName',$type)) > 0">
          <link>
            <xsl:attribute name="linkend">
              <xsl:value-of select="translate($type, ':', '_')" />
            </xsl:attribute>
            <xsl:value-of select="$type" />
          </link>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$type" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="'void'" />
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="isWebserviceOnly">
  <xsl:for-each select="ancestor-or-self::*">
    <xsl:if test="(name()='if') and (@target='wsdl')">
      <xsl:text>yes</xsl:text>
    </xsl:if>
  </xsl:for-each>
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  root match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="/idl">
 <book> <!-- Need a single top-level element for xi:include, we'll skip it using xpointer. -->
  <chapter id="sdkref_classes">
    <title>Classes (interfaces)</title>
    <xsl:for-each select="//interface">
      <xsl:sort select="@name"/>

      <!-- ignore those interfaces within module sections; they don't have uuid -->
      <xsl:if test="@uuid">
        <xsl:variable name="ifname" select="@name" />
        <xsl:variable name="wsmap" select="@wsmap" />
        <xsl:variable name="wscpp" select="@wscpp" />
        <xsl:variable name="wsonly"><xsl:call-template name="isWebserviceOnly" /></xsl:variable>
        <xsl:variable name="extends" select="@extends" />
        <xsl:variable name="reportExtends" select="not($extends='$unknown') and not($extends='$errorinfo')" />

        <sect1>
          <xsl:attribute name="id">
            <xsl:value-of select="$ifname" />
          </xsl:attribute>
          <title><xsl:value-of select="$ifname" />
              <xsl:if test="$reportExtends">
              <xsl:value-of select="concat(' (', @extends, ')')" />
            </xsl:if>
          </title>

          <xsl:choose>
            <xsl:when test="$wsmap='suppress'">
              <para><note><para>
                This interface is not supported in the web service.
              </para></note></para>
            </xsl:when>
            <xsl:when test="$wsmap='struct'">
              <para><note><para>With the web service, this interface is mapped to a structure. Attributes that return this interface will not return an object, but a complete structure
              containing the attributes listed below as structure members.</para></note></para>
            </xsl:when>
            <xsl:when test="$wsonly='yes'">
              <para><note><para>This interface is supported in the web service only, not in COM/XPCOM.</para></note></para>
            </xsl:when>
          </xsl:choose>

          <xsl:if test="$reportExtends">
            <para><note><para>
                This interface extends
                <link>
                  <xsl:attribute name="linkend"><xsl:value-of select="$extends" /></xsl:attribute>
                  <xsl:value-of select="$extends" />
                </link>
                and therefore supports all its methods and attributes as well.
            </para></note></para>
          </xsl:if>

          <xsl:apply-templates select="desc" />

          <xsl:if test="attribute">
            <sect2>
              <title>Attributes</title>
              <xsl:for-each select="attribute">
                <xsl:variable name="attrtype" select="@type" />
                <sect3>
                  <xsl:attribute name="id">
                    <xsl:call-template name="makeLinkId">
                      <xsl:with-param name="ifname" select="$ifname" />
                      <xsl:with-param name="member" select="@name" />
                    </xsl:call-template>
                  </xsl:attribute>
                  <title>
                    <xsl:choose>
                      <xsl:when test="@readonly='yes'">
                        <xsl:value-of select="concat(@name, ' (read-only)')" />
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:value-of select="concat(@name, ' (read/write)')" />
                      </xsl:otherwise>
                    </xsl:choose>
                  </title>
                  <programlisting>
                    <xsl:call-template name="emitType">
                      <xsl:with-param name="type" select="$attrtype" />
                    </xsl:call-template>
                    <xsl:value-of select="concat(' ', $ifname, '::', @name)" />
                    <xsl:if test="(@array='yes') or (@safearray='yes')">
                      <xsl:text>[]</xsl:text>
                    </xsl:if>
                  </programlisting>
                  <xsl:if test="( ($attrtype=($G_setSuppressedInterfaces/@name)) )">
                    <para><note><para>
                      This attribute is not supported in the web service.
                    </para></note></para>
                  </xsl:if>
                  <xsl:apply-templates select="desc" />
                </sect3>
              </xsl:for-each>
            </sect2>
          </xsl:if>

          <xsl:if test="method">
<!--             <sect2> -->
<!--               <title>Methods</title> -->
              <xsl:for-each select="method">
                <xsl:sort select="@name" />
                <xsl:variable name="returnidltype" select="param[@dir='return']/@type" />
                <sect2>
                  <xsl:attribute name="id">
                    <xsl:call-template name="makeLinkId">
                      <xsl:with-param name="ifname" select="$ifname" />
                      <xsl:with-param name="member" select="@name" />
                    </xsl:call-template>
                  </xsl:attribute>
                  <title>
                    <xsl:value-of select="@name" />
                  </title>
                  <xsl:if test="   (param[@type=($G_setSuppressedInterfaces/@name)])
                                or (param[@mod='ptr'])" >
                    <para><note><para>
                      This method is not supported in the web service.
                    </para></note></para>
                  </xsl:if>
                  <!-- make a set of all parameters with in and out direction -->
                  <xsl:variable name="paramsinout" select="param[@dir='in' or @dir='out']" />
                  <programlisting>
                    <!--emit return type-->
                    <xsl:call-template name="emitType">
                      <xsl:with-param name="type" select="$returnidltype" />
                    </xsl:call-template>
                    <xsl:if test="(param[@dir='return']/@array='yes') or (param[@dir='return']/@safearray='yes')">
                      <xsl:text>[]</xsl:text>
                    </xsl:if>
                    <xsl:value-of select="concat(' ', $ifname, '::', @name, '(')" />
                    <xsl:if test="$paramsinout">
                      <xsl:for-each select="$paramsinout">
                        <xsl:text>&#10;</xsl:text>
                        <xsl:value-of select="concat('           [', @dir, '] ')" />
                        <xsl:if test="@mod = 'ptr'">
                          <xsl:text>[ptr] </xsl:text>
                        </xsl:if>
                        <xsl:call-template name="emitType">
                          <xsl:with-param name="type" select="@type" />
                        </xsl:call-template>
                        <emphasis role="bold">
                          <xsl:value-of select="concat(' ', @name)" />
                        </emphasis>
                        <xsl:if test="(@array='yes') or (@safearray='yes')">
                          <xsl:text>[]</xsl:text>
                        </xsl:if>
                        <xsl:if test="not(position()=last())">
                          <xsl:text>, </xsl:text>
                        </xsl:if>
                      </xsl:for-each>
                    </xsl:if>
                    <xsl:text>)</xsl:text>
                  </programlisting>

                  <xsl:if test="$paramsinout">
                    <glosslist>
                      <xsl:for-each select="$paramsinout">
                        <glossentry>
                          <glossterm>
                            <xsl:value-of select="@name" />
                          </glossterm>
                          <glossdef>
                            <xsl:if test="not(desc)">
                              <para/>
                            </xsl:if>
                            <xsl:apply-templates select="desc" />
                          </glossdef>
                        </glossentry>
                      </xsl:for-each>
                    </glosslist>
                  </xsl:if>

                  <!-- dump the description here -->
                  <xsl:apply-templates select="desc" />

                  <xsl:if test="desc/result">
                    <para>If this method fails, the following error codes may be reported:</para>
                    <itemizedlist>
                      <xsl:for-each select="desc/result">
                        <listitem>
                          <para><code><xsl:value-of select="@name" />: </code>
                            <xsl:apply-templates />
                          </para>
                        </listitem>
                      </xsl:for-each>
                    </itemizedlist>
                  </xsl:if>
                </sect2>
              </xsl:for-each>
<!--             </sect2> -->
          </xsl:if>

        </sect1>
      </xsl:if>
    </xsl:for-each>
  </chapter>

  <chapter id="sdkref_enums">
    <title>Enumerations (enums)</title>
    <xsl:for-each select="//enum">
      <xsl:sort select="@name"/>

      <xsl:variable name="ifname" select="@name" />
      <xsl:variable name="wsmap" select="@wsmap" />
      <xsl:variable name="wscpp" select="@wscpp" />

      <sect1>
        <xsl:attribute name="id">
          <xsl:value-of select="$ifname" />
        </xsl:attribute>
        <title><xsl:value-of select="$ifname" /></title>

        <xsl:apply-templates select="desc" />

        <glosslist>
          <xsl:for-each select="const">
            <glossentry>
              <glossterm>
                <xsl:attribute name="id">
                  <xsl:call-template name="makeLinkId">
                    <xsl:with-param name="ifname" select="$ifname" />
                    <xsl:with-param name="member" select="@name" />
                  </xsl:call-template>
                </xsl:attribute>
                <xsl:value-of select="@name" />
              </glossterm>
              <glossdef>
                <xsl:if test="not(desc)">
                  <para/>
                </xsl:if>
                <xsl:apply-templates select="desc" />
              </glossdef>
            </glossentry>
          </xsl:for-each>
        </glosslist>
      </sect1>
    </xsl:for-each>
  </chapter>
 </book>
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
     result
     - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="result">
  <!--  ignore this, we handle them explicitly in method loops -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  library
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="library">
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
  <!-- todo: wrapping the entire content in a single para is actually not
       entirely correct, as it contains empty lines denoting new paragraphs -->
  <para>
  <xsl:apply-templates />
  </para>
</xsl:template>

<xsl:template name="getCurrentInterface">
  <xsl:for-each select="ancestor-or-self::*">
    <xsl:if test="name()='interface'">
      <xsl:value-of select="@name"/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<!-- <link to="DeviceType::HardDisk"/> -->
<xsl:template match="link">
  <link>
    <xsl:variable name="tmp" select="@to" />
    <xsl:variable name="enumNameFromCombinedName">
        <xsl:value-of select="substring-before($tmp, '_')" />
    </xsl:variable>
    <xsl:variable name="enumValueFromCombinedName">
        <xsl:value-of select="substring-after($tmp, '_')" />
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="count(key('G_keyInterfacesByName',$tmp)) > 0 or count(key('G_keyEnumsByName',$tmp)) > 0"><!-- link to interface only -->
        <xsl:attribute name="linkend"><xsl:value-of select="@to" /></xsl:attribute>
        <xsl:value-of select="$tmp" />
      </xsl:when>
      <xsl:when test="count(key('G_keyEnumsByName',$enumNameFromCombinedName)) > 0">
        <xsl:attribute name="linkend">
          <xsl:value-of select="concat($enumNameFromCombinedName, '__', $enumValueFromCombinedName)" />
        </xsl:attribute>
        <xsl:value-of select="$enumValueFromCombinedName" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="currentif">
          <xsl:call-template name="getCurrentInterface" />
        </xsl:variable>
        <xsl:variable name="if"><!-- interface -->
          <xsl:choose>
            <xsl:when test="contains(@to, '#')">
              <xsl:value-of select="$currentif" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="substring-before(@to, '::')" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="member"><!-- member in that interface -->
          <xsl:choose>
            <xsl:when test="contains(@to, '#')">
              <xsl:value-of select="substring-after(@to, '#')" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="substring-after(@to, '::')" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:attribute name="linkend"><xsl:value-of select="concat($if, '__', $member)" /></xsl:attribute>
        <xsl:variable name="autotextsuffix">
          <xsl:choose>
            <!-- if link points to a method, append "()" -->
            <xsl:when test="key('G_keyInterfacesByName',$if)/method[@name=$member]">
              <xsl:value-of select="'()'" />
            </xsl:when>
            <!-- if link points to a safearray attribute, append "[]" -->
            <xsl:when test="key('G_keyInterfacesByName',$if)/attribute[@name=$member]/@safearray = 'yes'">
              <xsl:value-of select="'[]'" />
            </xsl:when>
            <xsl:when test="key('G_keyInterfacesByName',$if)/attribute[@name=$member]"/>
            <xsl:when test="key('G_keyEnumsByName',$if)/const[@name=$member]"/>
            <xsl:when test="count(key('G_keyResultsByName',$tmp)) > 0"/>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat('Invalid link pointing to &quot;', $tmp, '&quot;')" />
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="./text()"><!-- link text given in source -->
            <xsl:apply-templates />
          </xsl:when>
          <xsl:when test="$if=$currentif"><!-- "near" link to method or attribute in current interface -->
            <xsl:value-of select="concat($member, $autotextsuffix)" />
          </xsl:when>
          <xsl:otherwise><!-- "far" link to other method or attribute -->
            <xsl:value-of select="concat($if, '::', $member, $autotextsuffix)" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </link>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  note
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="note">
  <xsl:if test="not(@internal='yes')">
    <note><para>
      <xsl:apply-templates />
    </para></note>
  </xsl:if>
</xsl:template>

<xsl:template match="tt">
  <computeroutput>
    <xsl:apply-templates />
  </computeroutput>
</xsl:template>

<xsl:template match="b">
  <emphasis role="bold">
    <xsl:apply-templates />
  </emphasis>
</xsl:template>

<xsl:template match="i">
  <emphasis>
    <xsl:apply-templates />
  </emphasis>
</xsl:template>

<xsl:template match="see">
  <xsl:text>See also: </xsl:text>
  <xsl:apply-templates />
</xsl:template>

<xsl:template match="ul">
  <itemizedlist>
    <xsl:apply-templates />
  </itemizedlist>
</xsl:template>

<xsl:template match="ol">
  <orderedlist>
    <xsl:apply-templates />
  </orderedlist>
</xsl:template>

<xsl:template match="li">
  <listitem>
    <para>
      <xsl:apply-templates />
    </para>
  </listitem>
</xsl:template>

<xsl:template match="h3">
  <emphasis role="bold">
    <xsl:apply-templates />
  </emphasis>
</xsl:template>

<xsl:template match="pre">
  <screen><xsl:apply-templates /></screen>
</xsl:template>

<xsl:template match="table">
  <xsl:apply-templates /> <!-- todo -->
</xsl:template>

</xsl:stylesheet>
