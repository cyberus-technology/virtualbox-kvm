<?xml version="1.0"?>

<!--
    apiwrap-server.xsl:
        XSLT stylesheet that generates C++ API wrappers (server side) from
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

<xsl:stylesheet
    version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

<xsl:output method="text"/>

<xsl:strip-space elements="*"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_xsltFilename" select="'apiwrap-server.xsl'"/>

<xsl:variable name="G_root" select="/"/>

<xsl:include href="typemap-shared.inc.xsl"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
templates for file separation
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="interface" mode="startfile">
    <xsl:param name="file"/>

    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:value-of select="concat($G_sNewLine, '// ##### BEGINFILE &quot;', $file, '&quot;', $G_sNewLine)"/>
</xsl:template>

<xsl:template match="interface" mode="endfile">
    <xsl:param name="file"/>

    <xsl:value-of select="concat($G_sNewLine, '// ##### ENDFILE &quot;', $file, '&quot;', $G_sNewLine)"/>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
templates for file headers/footers
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template name="fileheader">
    <xsl:param name="class"/>
    <xsl:param name="name"/>
    <xsl:param name="type"/>

    <xsl:text>/** @file
</xsl:text>
    <xsl:value-of select="concat(' * VirtualBox API class wrapper ', $type, ' for I', $class, '.')"/>
    <xsl:text>
 *
 * DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl
 * Generator: src/VBox/Main/idl/apiwrap-server.xsl
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

<!-- Emits COM_INTERFACE_ENTRY statements for the current interface node and whatever it inherits from. -->
<xsl:template name="emitCOMInterfaces">
    <xsl:value-of select="concat('        COM_INTERFACE_ENTRY(', @name, ')' , $G_sNewLine)"/>
    <!-- now recurse to emit all base interfaces -->
    <xsl:variable name="extends" select="@extends"/>
    <xsl:if test="$extends and not($extends='$unknown') and not($extends='$errorinfo')">
        <xsl:for-each select="key('G_keyInterfacesByName', $extends)">
            <xsl:call-template name="emitCOMInterfaces"/>
        </xsl:for-each>
    </xsl:if>
</xsl:template>

<xsl:template match="interface" mode="classheader">
    <xsl:param name="addinterfaces"/>
    <xsl:value-of select="concat('#ifndef ', substring(@name, 2), 'Wrap_H_', $G_sNewLine)"/>
    <xsl:value-of select="concat('#define ', substring(@name, 2), 'Wrap_H_')"/>
    <xsl:text>
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VirtualBoxBase.h"
#include "Wrapper.h"

</xsl:text>
    <xsl:value-of select="concat('class ATL_NO_VTABLE ', substring(@name, 2), 'Wrap')"/>
    <xsl:text>
    : public VirtualBoxBase
</xsl:text>
    <xsl:value-of select="concat('    , VBOX_SCRIPTABLE_IMPL(', @name, ')')"/>
    <xsl:value-of select="$G_sNewLine"/>
    <xsl:for-each select="exsl:node-set($addinterfaces)/token">
        <xsl:value-of select="concat('    , VBOX_SCRIPTABLE_IMPL(', text(), ')')"/>
        <xsl:value-of select="$G_sNewLine"/>
    </xsl:for-each>
    <xsl:text>{
    Q_OBJECT

public:
</xsl:text>
    <xsl:value-of select="concat('    VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(', substring(@name, 2), 'Wrap, ', @name, ')', $G_sNewLine)"/>
    <xsl:value-of select="concat('    DECLARE_NOT_AGGREGATABLE(', substring(@name, 2), 'Wrap)', $G_sNewLine)"/>
    <xsl:text>    DECLARE_PROTECT_FINAL_CONSTRUCT()

</xsl:text>
    <xsl:value-of select="concat('    BEGIN_COM_MAP(', substring(@name, 2), 'Wrap)', $G_sNewLine)"/>
    <xsl:text>        COM_INTERFACE_ENTRY(ISupportErrorInfo)
</xsl:text>
    <xsl:call-template name="emitCOMInterfaces"/>
    <xsl:value-of select="concat('        COM_INTERFACE_ENTRY2(IDispatch, ', @name, ')', $G_sNewLine)"/>
    <xsl:variable name="manualAddInterfaces">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'manualaddinterfaces'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:if test="not($manualAddInterfaces = 'true')">
        <xsl:for-each select="exsl:node-set($addinterfaces)/token">
            <!-- This is super tricky, as the for-each switches to the node set,
                 which means the normal document isn't available any more.  We get
                 the data we need, uses a for-each to switch document and then a
                 key() to look up the interface by name. -->
            <xsl:variable name="addifname">
                <xsl:value-of select="string(.)"/>
            </xsl:variable>
            <xsl:for-each select="$G_root">
                <xsl:for-each select="key('G_keyInterfacesByName', $addifname)">
                    <xsl:call-template name="emitCOMInterfaces"/>
                </xsl:for-each>
            </xsl:for-each>
        </xsl:for-each>
    </xsl:if>
    <xsl:value-of select="concat('        VBOX_TWEAK_INTERFACE_ENTRY(', @name, ')', $G_sNewLine)"/>
    <xsl:text>    END_COM_MAP()

</xsl:text>
    <xsl:value-of select="concat('    DECLARE_COMMON_CLASS_METHODS(', substring(@name, 2), 'Wrap)', $G_sNewLine)"/>
</xsl:template>

<xsl:template match="interface" mode="classfooter">
    <xsl:param name="addinterfaces"/>
    <xsl:if test="@wrap-gen-hook = 'yes'">
        <xsl:text>
public:
    virtual void i_callHook(const char *a_pszFunction) { RT_NOREF_PV(a_pszFunction); }
</xsl:text>
    </xsl:if>
    <xsl:text>
private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(</xsl:text>
    <xsl:value-of select="concat(substring(@name, 2),'Wrap')"/>
    <xsl:text>); /* Shuts up MSC warning C4625. */

};

</xsl:text>
    <xsl:value-of select="concat('#endif // !', substring(@name, 2), 'Wrap_H_', $G_sNewLine)"/>
</xsl:template>

<xsl:template match="interface" mode="codeheader">
    <xsl:param name="addinterfaces"/>
    <xsl:value-of select="concat('#define LOG_GROUP LOG_GROUP_MAIN_', translate(substring(@name, 2), $G_lowerCase, $G_upperCase), $G_sNewLine, $G_sNewLine)"/>
    <xsl:value-of select="concat('#include &quot;', substring(@name, 2), 'Wrap.h&quot;', $G_sNewLine)"/>
    <xsl:text>#include "LoggingNew.h"
#ifdef VBOX_WITH_DTRACE_R3_MAIN
# include "dtrace/VBoxAPI.h"
#endif

</xsl:text>
</xsl:template>

<xsl:template name="emitISupports">
    <xsl:param name="classname"/>
    <xsl:param name="extends"/>
    <xsl:param name="addinterfaces"/>
    <xsl:param name="depth"/>
    <xsl:param name="interfacelist"/>

    <xsl:choose>
        <xsl:when test="$extends and not($extends='$unknown') and not($extends='$errorinfo')">
            <xsl:variable name="newextends" select="key('G_keyInterfacesByName', $extends)/@extends"/>
            <xsl:variable name="newiflist" select="concat($interfacelist, ', ', $extends)"/>
            <xsl:call-template name="emitISupports">
                <xsl:with-param name="classname" select="$classname"/>
                <xsl:with-param name="extends" select="$newextends"/>
                <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
                <xsl:with-param name="depth" select="$depth + 1"/>
                <xsl:with-param name="interfacelist" select="$newiflist"/>
            </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
            <xsl:variable name="addinterfaces_ns" select="exsl:node-set($addinterfaces)"/>
            <xsl:choose>
                <xsl:when test="count($addinterfaces_ns/token) > 0">
                    <xsl:variable name="addifname" select="$addinterfaces_ns/token[1]"/>
                    <xsl:variable name="addif" select="key('G_keyInterfacesByName', $addifname)/@extends"/>
                    <xsl:variable name="newextends" select="$addif/@extends"/>
                    <xsl:variable name="newaddinterfaces" select="$addinterfaces_ns/token[position() > 1]"/>
                    <xsl:variable name="newiflist" select="concat($interfacelist, ', ', $addifname)"/>
                    <xsl:call-template name="emitISupports">
                        <xsl:with-param name="classname" select="$classname"/>
                        <xsl:with-param name="extends" select="$newextends"/>
                        <xsl:with-param name="addinterfaces" select="$newaddinterfaces"/>
                        <xsl:with-param name="depth" select="$depth + 1"/>
                        <xsl:with-param name="interfacelist" select="$newiflist"/>
                    </xsl:call-template>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="concat('NS_IMPL_THREADSAFE_ISUPPORTS', $depth, '_CI(', $classname, ', ', $interfacelist, ')', $G_sNewLine)"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template match="interface" mode="codefooter">
    <xsl:param name="addinterfaces"/>
    <xsl:text>#ifdef VBOX_WITH_XPCOM
</xsl:text>
    <xsl:value-of select="concat('NS_DECL_CLASSINFO(', substring(@name, 2), 'Wrap)', $G_sNewLine)"/>

    <xsl:variable name="manualAddInterfaces">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'manualaddinterfaces'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:choose>
        <xsl:when test="$manualAddInterfaces = 'true'">
            <xsl:variable name="nulladdinterfaces"></xsl:variable>
            <xsl:call-template name="emitISupports">
                <xsl:with-param name="classname" select="concat(substring(@name, 2), 'Wrap')"/>
                <xsl:with-param name="extends" select="@extends"/>
                <xsl:with-param name="addinterfaces" select="$nulladdinterfaces"/>
                <xsl:with-param name="depth" select="1"/>
                <xsl:with-param name="interfacelist" select="@name"/>
            </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
            <xsl:call-template name="emitISupports">
                <xsl:with-param name="classname" select="concat(substring(@name, 2), 'Wrap')"/>
                <xsl:with-param name="extends" select="@extends"/>
                <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
                <xsl:with-param name="depth" select="1"/>
                <xsl:with-param name="interfacelist" select="@name"/>
            </xsl:call-template>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:text>#endif // VBOX_WITH_XPCOM
</xsl:text>
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  templates for dealing with names and parameters
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template name="tospace">
    <xsl:param name="str"/>
    <xsl:value-of select="translate($str, 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_', '                                                               ')"/>
</xsl:template>

<xsl:template name="checkoption">
    <xsl:param name="optionlist"/>
    <xsl:param name="option"/>
    <xsl:value-of select="string-length($option) > 0 and contains(concat(',', translate($optionlist, ' ', ''), ','), concat(',', $option, ','))"/>
</xsl:template>

<xsl:template name="getattrlist">
    <xsl:param name="val"/>
    <xsl:param name="separator" select="','"/>

    <xsl:if test="$val and $val != ''">
        <xsl:choose>
            <xsl:when test="contains($val,$separator)">
                <token>
                    <xsl:value-of select="substring-before($val,$separator)"/>
                </token>
                <xsl:call-template name="getattrlist">
                    <xsl:with-param name="val" select="substring-after($val,$separator)"/>
                    <xsl:with-param name="separator" select="$separator"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
                <token><xsl:value-of select="$val"/></token>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:if>
</xsl:template>

<xsl:template name="translatepublictype">
    <xsl:param name="type"/>
    <xsl:param name="dir"/>
    <xsl:param name="mod"/>

    <xsl:choose>
        <xsl:when test="$type='wstring' or $type='uuid'">
            <xsl:if test="$dir='in'">
                <xsl:text>IN_</xsl:text>
            </xsl:if>
            <xsl:text>BSTR</xsl:text>
        </xsl:when>

        <xsl:when test="$type='$unknown'">
            <xsl:text>IUnknown *</xsl:text>
        </xsl:when>

        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:value-of select="concat($type, ' *')"/>
        </xsl:when>

        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
            <xsl:value-of select="concat($type, '_T')"/>
        </xsl:when>

        <!-- Micro optimizations: Put off wraptypefield calculation as long as possible; Check interfaces before enums. -->
        <xsl:otherwise>
            <!-- get C++ glue type from IDL type from table in typemap-shared.inc.xsl -->
            <xsl:variable name="gluetypefield" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@gluename"/>
            <xsl:choose>
                <xsl:when test="string-length($gluetypefield)">
                    <xsl:value-of select="$gluetypefield"/>
                </xsl:when>

                <xsl:otherwise>
                    <xsl:call-template name="fatalError">
                        <xsl:with-param name="msg" select="concat('translatepublictype: Type &quot;', $type, '&quot; is not supported.')"/>
                    </xsl:call-template>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$mod='ptr'">
        <xsl:text> *</xsl:text>
    </xsl:if>
</xsl:template>

<xsl:template name="translatewrappedtype">
    <xsl:param name="type"/>
    <xsl:param name="dir"/>
    <xsl:param name="mod"/>
    <xsl:param name="safearray"/>

    <xsl:choose>
        <xsl:when test="$type='wstring'">
            <xsl:if test="$dir='in' and not($safearray='yes')">
                <xsl:text>const </xsl:text>
            </xsl:if>
            <xsl:text>com::Utf8Str &amp;</xsl:text>
        </xsl:when>

        <xsl:when test="$type='uuid'">
            <xsl:if test="$dir='in'">
                <xsl:text>const </xsl:text>
            </xsl:if>
            <xsl:text>com::Guid &amp;</xsl:text>
        </xsl:when>

        <xsl:when test="$type='$unknown'">
            <xsl:if test="$dir='in' and not($safearray='yes')">
                <xsl:text>const </xsl:text>
            </xsl:if>
            <xsl:text>ComPtr&lt;IUnknown&gt; &amp;</xsl:text>
        </xsl:when>

        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:if test="$dir='in' and not($safearray='yes')">
                <xsl:text>const </xsl:text>
            </xsl:if>
            <xsl:value-of select="concat('ComPtr&lt;', $type, '&gt; &amp;')"/>
        </xsl:when>

        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
            <xsl:value-of select="concat($type, '_T')"/>
        </xsl:when>

        <!-- Micro optimizations: Put off wraptypefield calculation as long as possible; Check interfaces before enums. -->
        <xsl:otherwise>
            <!-- get C++ wrap type from IDL type from table in typemap-shared.inc.xsl -->
            <xsl:variable name="wraptypefield" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@gluename"/>
            <xsl:choose>
                <xsl:when test="string-length($wraptypefield)">
                    <xsl:value-of select="$wraptypefield"/>
                </xsl:when>

                <xsl:otherwise>
                    <xsl:call-template name="fatalError">
                        <xsl:with-param name="msg" select="concat('translatewrappedtype: Type &quot;', $type, '&quot; is not supported.')"/>
                    </xsl:call-template>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$mod='ptr'">
        <xsl:text> *</xsl:text>
    </xsl:if>
</xsl:template>

<xsl:template name="translatefmtspectype">
    <xsl:param name="type"/>
    <xsl:param name="dir"/>
    <xsl:param name="mod"/>
    <xsl:param name="safearray"/>
    <xsl:param name="isref"/>

    <!-- get C format string for IDL type from table in typemap-shared.inc.xsl -->
    <xsl:variable name="wrapfmt" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@gluefmt"/>
    <xsl:choose>
        <xsl:when test="$mod='ptr' or ($isref='yes' and $dir!='in')">
            <xsl:text>%p</xsl:text>
        </xsl:when>
        <xsl:when test="$safearray='yes'">
            <xsl:text>%zu</xsl:text>
        </xsl:when>
        <xsl:when test="string-length($wrapfmt)">
            <xsl:value-of select="$wrapfmt"/>
        </xsl:when>
        <xsl:when test="$type='$unknown'">
            <xsl:text>%p</xsl:text>
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
            <xsl:text>%RU32</xsl:text>
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:text>%p</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:call-template name="fatalError">
                <xsl:with-param name="msg" select="concat('translatefmtcpectype: Type &quot;', $type, '&quot; is not supported.')"/>
            </xsl:call-template>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template name="translatedtracetype">
    <xsl:param name="type"/>
    <xsl:param name="dir"/>
    <xsl:param name="mod"/>

    <!-- get dtrace probe type from IDL type from table in typemap-shared.inc.xsl -->
    <xsl:variable name="dtracetypefield" select="exsl:node-set($G_aSharedTypes)/type[@idlname=$type]/@dtracename"/>
    <xsl:choose>
        <xsl:when test="string-length($dtracetypefield)">
            <xsl:value-of select="$dtracetypefield"/>
        </xsl:when>
        <xsl:when test="$type='$unknown'">
            <!-- <xsl:text>struct IUnknown *</xsl:text> -->
            <xsl:text>void *</xsl:text>
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
            <!-- <xsl:value-of select="concat($type, '_T')"/> - later we can emit enums into dtrace the library -->
            <xsl:text>int</xsl:text>
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
            <!--
            <xsl:value-of select="concat('struct ', $type, ' *')"/>
            -->
            <xsl:text>void *</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:call-template name="fatalError">
                <xsl:with-param name="msg" select="concat('translatedtracetype Type &quot;', $type, '&quot; is not supported.')"/>
            </xsl:call-template>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$mod='ptr'">
        <xsl:text> *</xsl:text>
    </xsl:if>
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  templates for handling entire interfaces and their contents
 - - - - - - - - - - - - - - - - - - - - - - -->

<!-- Called on interface node. -->
<xsl:template name="emitInterface">
    <!-- sources, headers and dtrace-probes all needs attribute lists -->
    <xsl:variable name="addinterfaces">
        <xsl:call-template name="getattrlist">
            <xsl:with-param name="val" select="@wrap-hint-server-addinterfaces"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- interface sanity check, prevents crashes -->
    <xsl:if test="(count(attribute) + count(method) + sum(@reservedMethods[number()= number()]) + sum(@reservedAttributes[number()= number()])) = 0">
        <xsl:message terminate="yes">
            Interface <xsl:value-of select="@name"/> is empty which causes midl generated proxy
            stubs to crash. Please add a dummy:<xsl:value-of select="$G_sNewLine"/>
              &lt;attribute name="midlDoesNotLikeEmptyInterfaces" readonly="yes" type="boolean"/&gt;
        </xsl:message>
    </xsl:if>

    <xsl:choose>
        <xsl:when test="$generating = 'sources'">
            <xsl:if test="(position() mod 2) = $reminder">
                <xsl:call-template name="emitCode">
                    <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
                </xsl:call-template>
            </xsl:if>
        </xsl:when>
        <xsl:when test="$generating = 'headers'">
            <xsl:call-template name="emitHeader">
                <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
            </xsl:call-template>
        </xsl:when>
        <xsl:when test="$generating = 'dtrace-probes'">
            <xsl:call-template name="emitDTraceProbes">
                <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
            </xsl:call-template>
        </xsl:when>
        <xsl:otherwise><xsl:message terminate="yes">Otherwise oops in emitInterface</xsl:message></xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!-- Called on a method param or attribute node. -->
<xsl:template name="emitPublicParameter">
    <xsl:param name="dir"/>

    <xsl:variable name="gluetype">
        <xsl:call-template name="translatepublictype">
            <xsl:with-param name="type" select="@type"/>
            <xsl:with-param name="dir" select="$dir"/>
            <xsl:with-param name="mod" select="@mod"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
        <xsl:when test="@safearray='yes'">
            <xsl:choose>
                <xsl:when test="$dir='in'">
                    <xsl:text>ComSafeArrayIn(</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>ComSafeArrayOut(</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
            <xsl:value-of select="$gluetype"/>
            <xsl:text>, a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:value-of select="$gluetype"/>
            <xsl:if test="substring($gluetype,string-length($gluetype))!='*'">
                <xsl:text> </xsl:text>
            </xsl:if>
            <xsl:if test="$dir != 'in'">
                <xsl:text>*</xsl:text>
            </xsl:if>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template match="attribute/@type | param/@type" mode="wrapped">
    <xsl:param name="dir"/>

    <xsl:variable name="wraptype">
        <xsl:call-template name="translatewrappedtype">
            <xsl:with-param name="type" select="."/>
            <xsl:with-param name="dir" select="$dir"/>
            <xsl:with-param name="mod" select="../@mod"/>
            <xsl:with-param name="safearray" select="../@safearray"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="lastchar">
        <xsl:value-of select="substring($wraptype, string-length($wraptype))"/>
    </xsl:variable>

    <xsl:choose>
        <xsl:when test="../@safearray='yes'">
            <xsl:if test="$dir='in'">
                <xsl:text>const </xsl:text>
            </xsl:if>
            <xsl:text>std::vector&lt;</xsl:text>
            <xsl:choose>
                <xsl:when test="$lastchar = '&amp;'">
                    <xsl:variable name="wraptype2">
                        <xsl:value-of select="substring($wraptype, 1, string-length($wraptype)-2)"/>
                    </xsl:variable>
                    <xsl:value-of select="$wraptype2"/>
                    <xsl:if test="substring($wraptype2,string-length($wraptype2)) = '&gt;'">
                        <xsl:text> </xsl:text>
                    </xsl:if>
                </xsl:when>
                <xsl:when test="lastchar = '&gt;'">
                    <xsl:value-of select="concat($wraptype, ' ')"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="$wraptype"/>
                </xsl:otherwise>
            </xsl:choose>
            <xsl:text>&gt; &amp;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:value-of select="$wraptype"/>
            <xsl:if test="$lastchar != '&amp;'">
                <xsl:if test="$lastchar != '*'">
                    <xsl:text> </xsl:text>
                </xsl:if>
                <xsl:if test="$dir != 'in'">
                    <xsl:text>*</xsl:text>
                </xsl:if>
            </xsl:if>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:text>a</xsl:text>
    <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="../@name"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="attribute/@type | param/@type" mode="logparamtext">
    <xsl:param name="dir"/>
    <xsl:param name="isref"/>

    <xsl:if test="$isref!='yes' and ($dir='out' or $dir='ret')">
        <xsl:text>*</xsl:text>
    </xsl:if>
    <xsl:text>a</xsl:text>
    <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="../@name"/>
    </xsl:call-template>
    <xsl:text>=</xsl:text>
    <xsl:call-template name="translatefmtspectype">
        <xsl:with-param name="type" select="."/>
        <xsl:with-param name="dir" select="$dir"/>
        <xsl:with-param name="mod" select="../@mod"/>
        <xsl:with-param name="safearray" select="../@safearray"/>
        <xsl:with-param name="isref" select="$isref"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="attribute/@type | param/@type" mode="logparamval">
    <xsl:param name="dir"/>
    <xsl:param name="isref"/>

    <xsl:choose>
        <xsl:when test="../@safearray='yes' and $isref!='yes'">
            <xsl:text>ComSafeArraySize(</xsl:text>
            <xsl:if test="$isref!='yes' and $dir!='in'">
                <xsl:text>*</xsl:text>
            </xsl:if>
        </xsl:when>
        <xsl:when test="$isref!='yes' and $dir!='in'">
            <xsl:text>*</xsl:text>
        </xsl:when>
    </xsl:choose>
    <xsl:text>a</xsl:text>
    <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="../@name"/>
    </xsl:call-template>
    <xsl:choose>
        <xsl:when test="../@safearray='yes' and $isref!='yes'">
            <xsl:text>)</xsl:text>
        </xsl:when>
    </xsl:choose>
</xsl:template>

<!-- Emits the DTrace probe parameter value (using tmps), invoked on param or attribute node. -->
<xsl:template name="emitDTraceParamValue">
    <xsl:param name="dir"/>

    <xsl:variable name="viatmpvar">
        <xsl:for-each select="@type">
            <xsl:call-template name="paramconversionviatmp">
                <xsl:with-param name="dir" select="$dir"/>
            </xsl:call-template>
        </xsl:for-each>
    </xsl:variable>

    <xsl:variable name="type" select="@type"/>
    <xsl:choose>
        <!-- Doesn't help to inline paramconversionviatmp: <xsl:when test="$type = 'wstring' or $type = '$unknown' or $type = 'uuid' or @safearray = 'yes' or count(key('G_keyInterfacesByName', $type)) > 0"> -->
        <xsl:when test="$viatmpvar = 'yes'">
            <xsl:variable name="tmpname">
                <xsl:text>Tmp</xsl:text>
                <xsl:call-template name="capitalize">
                    <xsl:with-param name="str" select="@name"/>
                </xsl:call-template>
            </xsl:variable>

            <xsl:choose>
                <xsl:when test="@safearray = 'yes'">
                    <xsl:text>(uint32_t)</xsl:text>
                    <xsl:value-of select="$tmpname"/>
                    <xsl:text>.array().size(), </xsl:text>
                    <!-- Later:
                    <xsl:value-of select="concat($tmpname, '.array().data(), ')"/>
                    -->
                    <xsl:text>NULL /*for now*/</xsl:text>
                </xsl:when>
                <xsl:when test="$type = 'wstring'">
                    <xsl:value-of select="$tmpname"/>
                    <xsl:text>.str().c_str()</xsl:text>
                </xsl:when>
                <xsl:when test="$type = 'uuid'">
                    <xsl:value-of select="$tmpname"/>
                    <xsl:text>.uuid().toStringCurly().c_str()</xsl:text>
                </xsl:when>
                <xsl:when test="$type = '$unknown'">
                    <xsl:text>(void *)</xsl:text>
                    <xsl:value-of select="$tmpname"/>
                    <xsl:text>.ptr()</xsl:text>
                </xsl:when>
                <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
                    <xsl:text>(void *)</xsl:text>
                    <xsl:value-of select="$tmpname"/>
                    <xsl:text>.ptr()</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="$tmpname"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:when>

        <xsl:otherwise>
            <xsl:if test="$dir != 'in'">
                <xsl:text>*</xsl:text>
            </xsl:if>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>

            <xsl:if test="$type = 'boolean'">
                <xsl:text> != FALSE</xsl:text>
            </xsl:if>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--
Same as emitDTraceParamValue except no temporary variables are used (they are out of scope).
Note! There are two other instances of this code with different @dir values, see below.
-->
<xsl:template name="emitDTraceParamValNoTmp">
    <!-- To speed this up, the logic of paramconversionviatmp has been duplicated/inlined here. -->
    <xsl:variable name="type" select="@type"/>
    <xsl:choose>
        <xsl:when test="@safearray = 'yes'">
            <xsl:text>0, 0</xsl:text>
        </xsl:when>
        <xsl:when test="$type = 'wstring' or $type = '$unknown' or $type = 'uuid' or count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:if test="@dir != 'in'">
                <xsl:text>*</xsl:text>
            </xsl:if>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:if test="$type = 'boolean'">
                <xsl:text> != FALSE</xsl:text>
            </xsl:if>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!-- Copy of emitDTraceParamValNoTmp with @dir = 'in' for speeding up the code (noticable difference). -->
<xsl:template name="emitDTraceParamValNoTmp-DirIn">
    <xsl:variable name="type" select="@type"/>
    <xsl:choose>
        <xsl:when test="@safearray = 'yes'">
            <xsl:text>0, 0</xsl:text>
        </xsl:when>
        <xsl:when test="$type = 'wstring' or $type = '$unknown' or $type = 'uuid' or count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:if test="$type = 'boolean'">
                <xsl:text> != FALSE</xsl:text>
            </xsl:if>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!-- Copy of emitDTraceParamValNoTmp with @dir != 'in' for speeding up attributes (noticable difference). -->
<xsl:template name="emitDTraceParamValNoTmp-DirNotIn">
    <xsl:variable name="type" select="@type"/>
    <xsl:choose>
        <xsl:when test="@safearray = 'yes'">
            <xsl:text>0, 0</xsl:text>
        </xsl:when>
        <xsl:when test="$type = 'wstring' or $type = '$unknown' or $type = 'uuid' or count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>*a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:if test="$type = 'boolean'">
                <xsl:text> != FALSE</xsl:text>
            </xsl:if>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template match="attribute/@type | param/@type" mode="dtraceparamdecl">
    <xsl:param name="dir"/>

    <xsl:variable name="gluetype">
        <xsl:call-template name="translatedtracetype">
            <xsl:with-param name="type" select="."/>
            <xsl:with-param name="dir" select="$dir"/>
            <xsl:with-param name="mod" select="../@mod"/>
        </xsl:call-template>
    </xsl:variable>

    <!-- Safe arrays get an extra size parameter. -->
    <xsl:if test="../@safearray='yes'">
        <xsl:text>uint32_t a_c</xsl:text>
        <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="../@name"/>
        </xsl:call-template>
        <xsl:text>, </xsl:text>
    </xsl:if>

    <xsl:value-of select="$gluetype"/>
    <xsl:choose>
        <xsl:when test="../@safearray='yes'">
            <xsl:text> *a_pa</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:if test="substring($gluetype,string-length($gluetype))!='*'">
                <xsl:text> </xsl:text>
            </xsl:if>
            <xsl:text>a_</xsl:text>
        </xsl:otherwise>
    </xsl:choose>

    <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="../@name"/>
    </xsl:call-template>
</xsl:template>

<!-- Call this to determine whether a temporary conversion variable is used for the current parameter.
Returns empty if not needed, non-empty ('yes') if needed. -->
<xsl:template name="paramconversionviatmp">
    <xsl:param name="dir"/>
    <xsl:variable name="type" select="."/>
    <xsl:choose>
        <xsl:when test="$type = 'wstring' or $type = '$unknown' or $type = 'uuid'">
            <xsl:text>yes</xsl:text>
        </xsl:when>
        <xsl:when test="../@safearray = 'yes'">
            <xsl:text>yes</xsl:text>
        </xsl:when>
        <xsl:when test="$type = 'boolean' or $type = 'long' or $type = 'long' or $type = 'long long'"/> <!-- XXX: drop this? -->
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:text>yes</xsl:text>
        </xsl:when>
    </xsl:choose>
</xsl:template>

<!-- Call this to get the argument conversion class, if any is needed. -->
<xsl:template name="paramconversionclass">
    <xsl:param name="dir"/>

    <xsl:variable name="type" select="."/>
    <xsl:choose>
        <xsl:when test="$type='$unknown'">
            <xsl:if test="../@safearray='yes'">
                <xsl:text>Array</xsl:text>
            </xsl:if>
            <xsl:choose>
                <xsl:when test="$dir='in'">
                    <xsl:text>ComTypeInConverter&lt;IUnknown&gt;</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>ComTypeOutConverter&lt;IUnknown&gt;</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:when>

        <xsl:when test="$type='wstring'">
            <xsl:if test="../@safearray='yes'">
                <xsl:text>Array</xsl:text>
            </xsl:if>
            <xsl:choose>
                <xsl:when test="$dir='in'">
                    <xsl:text>BSTRInConverter</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>BSTROutConverter</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:when>

        <xsl:when test="$type='uuid'">
            <xsl:if test="../@safearray='yes'">
                <xsl:text>Array</xsl:text>
            </xsl:if>
            <xsl:choose>
                <xsl:when test="$dir='in'">
                    <xsl:text>UuidInConverter</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>UuidOutConverter</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:when>

        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
            <xsl:if test="../@safearray='yes'">
                <xsl:text>Array</xsl:text>
            </xsl:if>
            <xsl:choose>
                <xsl:when test="$dir='in'">
                    <xsl:text>ComTypeInConverter</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>ComTypeOutConverter</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
            <xsl:value-of select="concat('&lt;', $type, '&gt;')"/>
        </xsl:when>

        <xsl:when test="../@safearray='yes'">
            <xsl:text>Array</xsl:text>
            <xsl:choose>
                <xsl:when test="$dir='in'">
                    <xsl:text>InConverter</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>OutConverter</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
            <xsl:variable name="gluetype">
                <xsl:call-template name="translatepublictype">
                    <xsl:with-param name="type" select="."/>
                    <xsl:with-param name="dir" select="$dir"/>
                    <xsl:with-param name="mod" select="../@mod"/>
                </xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="concat('&lt;', $gluetype, '&gt;')"/>
        </xsl:when>
    </xsl:choose>
</xsl:template>

<!-- Emits code for converting the parameter to a temporary variable. -->
<xsl:template match="attribute/@type | param/@type" mode="paramvalconversion2tmpvar">
    <xsl:param name="dir"/>

    <xsl:variable name="conversionclass">
        <xsl:call-template name="paramconversionclass">
            <xsl:with-param name="dir" select="$dir"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:if test="$conversionclass != ''">
        <xsl:value-of select="$conversionclass"/>
        <xsl:text> Tmp</xsl:text>
        <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="../@name"/>
        </xsl:call-template>
        <xsl:text>(</xsl:text>
        <xsl:if test="../@safearray = 'yes'">
            <xsl:choose>
                <xsl:when test="$dir = 'in'">
                    <xsl:text>ComSafeArrayInArg(</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>ComSafeArrayOutArg(</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:if>
        <xsl:text>a</xsl:text>
        <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="../@name"/>
        </xsl:call-template>
        <xsl:if test="../@safearray = 'yes'">
            <xsl:text>)</xsl:text>
        </xsl:if>
        <xsl:text>);</xsl:text>
    </xsl:if>

</xsl:template>

<!-- Partner to paramvalconversion2tmpvar that emits the parameter when calling call the internal worker method. -->
<xsl:template match="attribute/@type | param/@type" mode="paramvalconversionusingtmp">
    <xsl:param name="dir"/>

    <xsl:variable name="viatmpvar">
        <xsl:call-template name="paramconversionviatmp">
            <xsl:with-param name="dir" select="$dir"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="type" select="."/>

    <xsl:choose>
        <xsl:when test="$viatmpvar = 'yes'">
            <xsl:text>Tmp</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="../@name"/>
            </xsl:call-template>

            <xsl:choose>
                <xsl:when test="../@safearray='yes'">
                    <xsl:text>.array()</xsl:text>
                </xsl:when>
                <xsl:when test="$type = 'wstring'">
                    <xsl:text>.str()</xsl:text>
                </xsl:when>
                <xsl:when test="$type = 'uuid'">
                    <xsl:text>.uuid()</xsl:text>
                </xsl:when>
                <xsl:when test="$type = '$unknown'">
                    <xsl:text>.ptr()</xsl:text>
                </xsl:when>
                <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
                    <xsl:text>.ptr()</xsl:text>
                </xsl:when>
                <xsl:otherwise><xsl:message terminate="yes">Oops #1</xsl:message></xsl:otherwise>
            </xsl:choose>
        </xsl:when>

        <xsl:otherwise>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="../@name"/>
            </xsl:call-template>

            <!-- Make sure BOOL values we pass down are either TRUE or FALSE. -->
            <xsl:if test="$type = 'boolean' and $dir = 'in'">
                <xsl:text> != FALSE</xsl:text>
            </xsl:if>
        </xsl:otherwise>
    </xsl:choose>

</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit attribute
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="attribute" mode="public">
    <xsl:param name="target"/>

    <xsl:call-template name="emitTargetBegin">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:variable name="attrbasename">
        <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:value-of select="concat('    STDMETHOD(COMGETTER(', $attrbasename, '))(')"/>
    <xsl:call-template name="emitPublicParameter">
        <xsl:with-param name="dir">out</xsl:with-param>
    </xsl:call-template>
    <xsl:text>);
</xsl:text>

    <xsl:if test="not(@readonly) or @readonly!='yes'">
        <xsl:value-of select="concat('    STDMETHOD(COMSETTER(', $attrbasename, '))(')"/>
        <xsl:call-template name="emitPublicParameter">
            <xsl:with-param name="dir">in</xsl:with-param>
        </xsl:call-template>
        <xsl:text>);
</xsl:text>
    </xsl:if>

    <xsl:call-template name="emitTargetEnd">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="attribute" mode="wrapped">
    <xsl:param name="target"/>

    <xsl:call-template name="emitTargetBegin">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:variable name="attrbasename">
        <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:if test="$attrbasename = 'MidlDoesNotLikeEmptyInterfaces'">
        <xsl:text>    //</xsl:text>
    </xsl:if>

    <xsl:value-of select="concat('    virtual HRESULT get', $attrbasename, '(')"/>
    <xsl:variable name="passAutoCaller">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'passcaller'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$passAutoCaller = 'true'">
        <xsl:text>AutoCaller &amp;aAutoCaller, </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="@type" mode="wrapped">
        <xsl:with-param name="dir" select="'out'"/>
    </xsl:apply-templates>
    <xsl:text>) = 0;
</xsl:text>

    <xsl:if test="not(@readonly) or @readonly!='yes'">
        <xsl:value-of select="concat('    virtual HRESULT set', $attrbasename, '(')"/>
        <xsl:if test="$passAutoCaller = 'true'">
            <xsl:text>AutoCaller &amp;aAutoCaller, </xsl:text>
        </xsl:if>
        <xsl:apply-templates select="@type" mode="wrapped">
            <xsl:with-param name="dir" select="'in'"/>
        </xsl:apply-templates>
        <xsl:text>) = 0;
</xsl:text>
    </xsl:if>

    <xsl:call-template name="emitTargetEnd">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="attribute" mode="code">
    <xsl:param name="topclass"/>
    <xsl:param name="dtracetopclass"/>
    <xsl:param name="target"/>

    <xsl:call-template name="emitTargetBegin">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:variable name="attrbasename">
        <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="limitedAutoCaller">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'limitedcaller'"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="dtraceattrname">
        <xsl:choose>
            <xsl:when test="@dtracename">
                <xsl:value-of select="@dtracename"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$attrbasename"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:variable>

    <xsl:value-of select="concat('STDMETHODIMP ', $topclass, 'Wrap::COMGETTER(', $attrbasename, ')(')"/>
    <xsl:call-template name="emitPublicParameter">
        <xsl:with-param name="dir">out</xsl:with-param>
    </xsl:call-template>
    <xsl:text>)
{</xsl:text>
    <xsl:if test="$attrbasename = 'MidlDoesNotLikeEmptyInterfaces'">
        <xsl:text>
#if 0 /* This is a dummy attribute */</xsl:text>
    </xsl:if>
    <xsl:text>
    LogRelFlow(("{%p} %s: enter </xsl:text>
    <xsl:apply-templates select="@type" mode="logparamtext">
        <xsl:with-param name="dir" select="'out'"/>
        <xsl:with-param name="isref" select="'yes'"/>
    </xsl:apply-templates>
    <xsl:text>\n", this, </xsl:text>
    <xsl:value-of select="concat('&quot;', $topclass, '::get', $attrbasename, '&quot;, ')"/>
    <xsl:apply-templates select="@type" mode="logparamval">
        <xsl:with-param name="dir" select="'out'"/>
        <xsl:with-param name="isref" select="'yes'"/>
    </xsl:apply-templates>
    <xsl:text>));
</xsl:text>
    <xsl:if test="ancestor::interface[@wrap-gen-hook = 'yes']">
        <xsl:text>
    i_callHook(__FUNCTION__);</xsl:text>
    </xsl:if>
<xsl:text>
    // Clear error info, to make in-process calls behave the same as
    // cross-apartment calls or out-of-process calls.
    VirtualBoxBase::clearError();

    HRESULT hrc;

    try
    {
        CheckComArgOutPointerValidThrow(a</xsl:text>
    <xsl:value-of select="$attrbasename"/>
    <xsl:text>);
        </xsl:text>
    <xsl:apply-templates select="@type" mode="paramvalconversion2tmpvar">
        <xsl:with-param name="dir" select="'out'"/>
    </xsl:apply-templates>
    <xsl:if test="$attrbasename != 'MidlDoesNotLikeEmptyInterfaces'">
        <xsl:text>
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
        <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_GET_', $dtraceattrname, '_ENTER('), $G_lowerCase, $G_upperCase)"/>
        <xsl:text>this);
#endif</xsl:text>
    </xsl:if>
    <xsl:text>
        </xsl:text>
    <xsl:choose>
      <xsl:when test="$limitedAutoCaller = 'true'">
        <xsl:text>AutoLimitedCaller</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>AutoCaller</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text> autoCaller(this);
        hrc = autoCaller.hrc();
        if (SUCCEEDED(hrc))
        {
</xsl:text>
    <xsl:value-of select="concat('            hrc = get', $attrbasename, '(')"/>
    <xsl:variable name="passAutoCaller">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'passcaller'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$passAutoCaller = 'true'">
        <xsl:text>autoCaller, </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="@type" mode="paramvalconversionusingtmp">
        <xsl:with-param name="dir" select="'out'"/>
    </xsl:apply-templates>
    <xsl:text>);
        }</xsl:text>
    <xsl:if test="$attrbasename != 'MidlDoesNotLikeEmptyInterfaces'">
        <xsl:text>
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
        <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_GET_', $dtraceattrname, '_RETURN('), $G_lowerCase, $G_upperCase)"/>
        <xsl:text>this, hrc, 0 /*normal*/,</xsl:text>
        <xsl:call-template name="emitDTraceParamValue">
            <xsl:with-param name="dir">out</xsl:with-param>
        </xsl:call-template>
        <xsl:text>);
#endif</xsl:text>
    </xsl:if>
    <xsl:text>
    }
    catch (HRESULT hrc2)
    {
        hrc = hrc2;</xsl:text>
    <xsl:if test="$attrbasename != 'MidlDoesNotLikeEmptyInterfaces'">
        <xsl:text>
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_GET_', $dtraceattrname, '_RETURN('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this, hrc, 1 /*hrc exception*/,</xsl:text>
    <xsl:call-template name="emitDTraceParamValNoTmp-DirNotIn"/>
    <xsl:text>);
#endif</xsl:text>
    </xsl:if>
    <xsl:text>
    }
    catch (...)
    {
        hrc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);</xsl:text>
    <xsl:if test="$attrbasename != 'MidlDoesNotLikeEmptyInterfaces'">
        <xsl:text>
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_GET_', $dtraceattrname, '_RETURN('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this, hrc, 9 /*unhandled exception*/,</xsl:text>
    <xsl:call-template name="emitDTraceParamValNoTmp-DirNotIn"/>
    <xsl:text>);
#endif</xsl:text>
    </xsl:if>
    <xsl:text>
    }

    LogRelFlow(("{%p} %s: leave </xsl:text>
    <xsl:apply-templates select="@type" mode="logparamtext">
        <xsl:with-param name="dir" select="'out'"/>
        <xsl:with-param name="isref" select="''"/>
    </xsl:apply-templates>
    <xsl:text> hrc=%Rhrc\n", this, </xsl:text>
    <xsl:value-of select="concat('&quot;', $topclass, '::get', $dtraceattrname, '&quot;, ')"/>
    <xsl:apply-templates select="@type" mode="logparamval">
        <xsl:with-param name="dir" select="'out'"/>
        <xsl:with-param name="isref" select="''"/>
    </xsl:apply-templates>
    <xsl:text>, hrc));
    return hrc;</xsl:text>
    <xsl:if test="$attrbasename = 'MidlDoesNotLikeEmptyInterfaces'">
        <xsl:text>
#else  /* dummy attribute */
    NOREF(aMidlDoesNotLikeEmptyInterfaces);
    return E_FAIL;
#endif /* dummy attribute */</xsl:text>
    </xsl:if>
    <xsl:text>
}
</xsl:text>
    <xsl:if test="not(@readonly) or @readonly!='yes'">
        <xsl:text>
</xsl:text>
        <xsl:value-of select="concat('STDMETHODIMP ', $topclass, 'Wrap::COMSETTER(', $attrbasename, ')(')"/>
        <xsl:call-template name="emitPublicParameter">
            <xsl:with-param name="dir">in</xsl:with-param>
        </xsl:call-template>
        <!-- @todo check in parameters if possible -->
        <xsl:text>)
{
    LogRelFlow(("{%p} %s: enter </xsl:text>
        <xsl:apply-templates select="@type" mode="logparamtext">
            <xsl:with-param name="dir" select="'in'"/>
            <xsl:with-param name="isref" select="''"/>
        </xsl:apply-templates>
        <xsl:text>\n", this, </xsl:text>
        <xsl:value-of select="concat('&quot;', $topclass, '::set', $attrbasename, '&quot;, ')"/>
        <xsl:apply-templates select="@type" mode="logparamval">
            <xsl:with-param name="dir" select="'in'"/>
            <xsl:with-param name="isref" select="''"/>
        </xsl:apply-templates>
        <xsl:text>));
</xsl:text>
    <xsl:if test="ancestor::interface[@wrap-gen-hook = 'yes']">
        <xsl:text>
    i_callHook(__FUNCTION__);</xsl:text>
    </xsl:if>
<xsl:text>
    // Clear error info, to make in-process calls behave the same as
    // cross-apartment calls or out-of-process calls.
    VirtualBoxBase::clearError();

    HRESULT hrc;

    try
    {
        </xsl:text>
        <xsl:apply-templates select="@type" mode="paramvalconversion2tmpvar">
            <xsl:with-param name="dir" select="'in'"/>
        </xsl:apply-templates>
        <xsl:text>

#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
        <xsl:value-of select="translate(concat('VBOXAPI_', $topclass, '_SET_', $dtraceattrname, '_ENTER('), $G_lowerCase, $G_upperCase)"/>
        <xsl:text>this, </xsl:text>
        <xsl:call-template name="emitDTraceParamValue">
            <xsl:with-param name="dir">in</xsl:with-param>
        </xsl:call-template>
        <xsl:text>);
#endif
        </xsl:text>
        <xsl:choose>
          <xsl:when test="$limitedAutoCaller = 'true'">
            <xsl:text>AutoLimitedCaller</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>AutoCaller</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text> autoCaller(this);
        hrc = autoCaller.hrc();
        if (SUCCEEDED(hrc))
        {
</xsl:text>
        <xsl:value-of select="concat('            hrc = set', $attrbasename, '(')"/>
        <xsl:if test="$passAutoCaller = 'true'">
            <xsl:text>autoCaller, </xsl:text>
        </xsl:if>
        <xsl:apply-templates select="@type" mode="paramvalconversionusingtmp">
            <xsl:with-param name="dir" select="'in'"/>
        </xsl:apply-templates>
        <xsl:text>);
        }
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
        <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_SET_', $dtraceattrname, '_RETURN('), $G_lowerCase, $G_upperCase)"/>
        <xsl:text>this, hrc, 0 /*normal*/,</xsl:text>
        <xsl:call-template name="emitDTraceParamValue">
            <xsl:with-param name="dir">in</xsl:with-param>
        </xsl:call-template>
        <xsl:text>);
#endif
    }
    catch (HRESULT hrc2)
    {
        hrc = hrc2;
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_SET_', $dtraceattrname, '_RETURN('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this, hrc, 1 /*hrc exception*/,</xsl:text>
    <xsl:call-template name="emitDTraceParamValNoTmp-DirIn"/>
    <xsl:text>);
#endif
    }
    catch (...)
    {
        hrc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_SET_', $dtraceattrname, '_RETURN('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this, hrc, 9 /*unhandled exception*/,</xsl:text>
    <xsl:call-template name="emitDTraceParamValNoTmp-DirIn"/>
    <xsl:text>);
#endif
    }

    LogRelFlow(("{%p} %s: leave hrc=%Rhrc\n", this, </xsl:text>
        <xsl:value-of select="concat('&quot;', $topclass, '::set', $attrbasename, '&quot;, ')"/>
        <xsl:text>hrc));
    return hrc;
}
</xsl:text>
    </xsl:if>

    <xsl:call-template name="emitTargetEnd">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:call-template name="xsltprocNewlineOutputHack"/>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
   Emit DTrace probes for the given attribute.
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template match="attribute" mode="dtrace-probes">
    <xsl:param name="topclass"/>
    <xsl:param name="dtracetopclass"/>
    <xsl:param name="target"/>

    <xsl:variable name="dtraceattrname">
        <xsl:choose>
            <xsl:when test="@dtracename">
                <xsl:value-of select="@dtracename"/>
            </xsl:when>
            <xsl:otherwise>
                <!-- attrbasename -->
                <xsl:call-template name="capitalize">
                    <xsl:with-param name="str" select="@name"/>
                </xsl:call-template>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:variable>

    <xsl:if test="@name != 'midlDoesNotLikeEmptyInterfaces'">
        <xsl:text>    probe </xsl:text>
        <!-- <xsl:value-of select="concat($dtracetopclass, '__get__', $dtraceattrname, '__enter(struct ', $topclass)"/> -->
        <xsl:value-of select="concat($dtracetopclass, '__get__', $dtraceattrname, '__enter(void')"/>
        <xsl:text> *a_pThis);
    probe </xsl:text>
        <!-- <xsl:value-of select="concat($dtracetopclass, '__get__', $dtraceattrname, '__return(struct ', $topclass, ' *a_pThis')"/> -->
        <xsl:value-of select="concat($dtracetopclass, '__get__', $dtraceattrname, '__return(void *a_pThis')"/>
        <xsl:text>, uint32_t a_hrc, int32_t enmWhy, </xsl:text>
        <xsl:apply-templates select="@type" mode="dtraceparamdecl">
            <xsl:with-param name="dir">out</xsl:with-param>
        </xsl:apply-templates>
        <xsl:text>);
</xsl:text>
    </xsl:if>
    <xsl:if test="(not(@readonly) or @readonly!='yes') and @name != 'midlDoesNotLikeEmptyInterfaces'">
        <xsl:text>    probe </xsl:text>
        <!-- <xsl:value-of select="concat($topclass, '__set__', $dtraceattrname, '__enter(struct ', $topclass, ' *a_pThis, ')"/>-->
        <xsl:value-of select="concat($topclass, '__set__', $dtraceattrname, '__enter(void *a_pThis, ')"/>
        <xsl:apply-templates select="@type" mode="dtraceparamdecl">
            <xsl:with-param name="dir" select="'in'"/>
        </xsl:apply-templates>
        <xsl:text>);
    probe </xsl:text>
        <!-- <xsl:value-of select="concat($dtracetopclass, '__set__', $dtraceattrname, '__return(struct ', $topclass, ' *a_pThis')"/> -->
        <xsl:value-of select="concat($dtracetopclass, '__set__', $dtraceattrname, '__return(void *a_pThis')"/>
        <xsl:text>, uint32_t a_hrc, int32_t enmWhy, </xsl:text>
        <xsl:apply-templates select="@type" mode="dtraceparamdecl">
            <xsl:with-param name="dir">in</xsl:with-param>
        </xsl:apply-templates>
        <xsl:text>);
</xsl:text>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  Emit all attributes of an interface (current node).
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitAttributes">
    <xsl:param name="topclass"/>
    <xsl:param name="dtracetopclass"/>
    <xsl:param name="pmode"/>

    <xsl:variable name="name" select="@name"/>
    <!-- first recurse to emit all base interfaces -->
    <xsl:variable name="extends" select="@extends"/>
    <xsl:if test="$extends and not($extends='$unknown') and not($extends='$errorinfo')">
        <xsl:for-each select="key('G_keyInterfacesByName', $extends)">
            <xsl:call-template name="emitAttributes">
                <xsl:with-param name="topclass" select="$topclass"/>
                <xsl:with-param name="pmode" select="$pmode"/>
                <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
            </xsl:call-template>
        </xsl:for-each>
    </xsl:if>

    <xsl:choose>
        <xsl:when test="$pmode='code'">
            <xsl:text>//
</xsl:text>
            <xsl:value-of select="concat('// ', $name, ' properties')"/>
            <xsl:text>
//

</xsl:text>
        </xsl:when>
        <xsl:when test="$pmode != 'dtrace-probes'">
            <xsl:value-of select="concat($G_sNewLine, '    /** @name ', translate(substring($pmode, 1, 1), $G_lowerCase, $G_upperCase), substring($pmode,2), ' ', $name, ' properties', $G_sNewLine)"/>
            <xsl:text>     * @{ */
</xsl:text>
        </xsl:when>
    </xsl:choose>
    <xsl:choose>
        <xsl:when test="$pmode='public'">
            <xsl:apply-templates select="./attribute | ./if" mode="public">
                <xsl:with-param name="emitmode" select="'attribute'"/>
            </xsl:apply-templates>
            <xsl:variable name="reservedAttributes" select="@reservedAttributes"/>
            <xsl:if test="$reservedAttributes > 0">
                <!-- tricky way to do a "for" loop without recursion -->
                <xsl:for-each select="(//*)[position() &lt;= $reservedAttributes]">
                    <xsl:text>    STDMETHOD(COMGETTER(InternalAndReservedAttribute</xsl:text>
                    <xsl:value-of select="concat(position(), $name)"/>
                    <xsl:text>))(ULONG *aReserved);&#x0A;</xsl:text>
                </xsl:for-each>
            </xsl:if>
        </xsl:when>
        <xsl:when test="$pmode='wrapped'">
            <xsl:apply-templates select="./attribute | ./if" mode="wrapped">
                <xsl:with-param name="emitmode" select="'attribute'"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:when test="$pmode='code'">
            <xsl:apply-templates select="./attribute | ./if" mode="code">
                <xsl:with-param name="topclass" select="$topclass"/>
                <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                <xsl:with-param name="emitmode" select="'attribute'"/>
            </xsl:apply-templates>
            <xsl:variable name="reservedAttributes" select="@reservedAttributes"/>
            <xsl:if test="$reservedAttributes > 0">
                <!-- tricky way to do a "for" loop without recursion -->
                <xsl:for-each select="(//*)[position() &lt;= $reservedAttributes]">
                    <xsl:value-of select="concat('STDMETHODIMP ', $topclass, 'Wrap::COMGETTER(InternalAndReservedAttribute', position(), $name, ')(ULONG *aReserved)&#x0A;')"/>
                    <xsl:text>{
    NOREF(aReserved);
    return E_NOTIMPL;
}

</xsl:text>
                </xsl:for-each>
            </xsl:if>
        </xsl:when>
        <xsl:when test="$pmode = 'dtrace-probes'">
            <xsl:apply-templates select="./attribute | ./if" mode="dtrace-probes">
                <xsl:with-param name="topclass" select="$topclass"/>
                <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                <xsl:with-param name="emitmode" select="'attribute'"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise><xsl:message terminate="yes">Otherwise oops in emitAttributes</xsl:message></xsl:otherwise>
    </xsl:choose>

    <!-- close doxygen @name -->
    <xsl:if test="($pmode != 'code') and ($pmode != 'dtrace-probes')" >
            <xsl:text>    /** @} */
</xsl:text>
    </xsl:if>
</xsl:template>

<xsl:template name="emitTargetBegin">
    <xsl:param name="target"/>

    <xsl:choose>
        <xsl:when test="$target = ''"/>
        <xsl:when test="$target = 'xpidl'">
            <xsl:text>#ifdef VBOX_WITH_XPCOM
</xsl:text>
        </xsl:when>
        <xsl:when test="$target = 'midl'">
            <xsl:text>#ifndef VBOX_WITH_XPCOM
</xsl:text>
        </xsl:when>
        <xsl:otherwise><xsl:message terminate="yes">Otherwise oops in emitTargetBegin: target=<xsl:value-of select="$target"/></xsl:message></xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template name="emitTargetEnd">
    <xsl:param name="target"/>

    <xsl:choose>
        <xsl:when test="$target = ''"/>
        <xsl:when test="$target = 'xpidl'">
            <xsl:text>#endif /* VBOX_WITH_XPCOM */
</xsl:text>
        </xsl:when>
        <xsl:when test="$target = 'midl'">
            <xsl:text>#endif /* !VBOX_WITH_XPCOM */
</xsl:text>
        </xsl:when>
        <xsl:otherwise><xsl:message terminate="yes">Otherwise oops in emitTargetEnd target=<xsl:value-of select="$target"/></xsl:message></xsl:otherwise>
    </xsl:choose>
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit method
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="method" mode="public">
    <xsl:param name="target"/>

    <xsl:call-template name="emitTargetBegin">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:variable name="methodindent">
      <xsl:call-template name="tospace">
          <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:text>    STDMETHOD(</xsl:text>
    <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name"/>
    </xsl:call-template>
    <xsl:text>)(</xsl:text>
    <xsl:for-each select="param">
        <xsl:call-template name="emitPublicParameter">
            <xsl:with-param name="dir" select="@dir"/>
        </xsl:call-template>
        <xsl:if test="not(position()=last())">
            <xsl:text>,
                </xsl:text>
            <xsl:value-of select="$methodindent"/>
        </xsl:if>
    </xsl:for-each>
    <xsl:text>);
</xsl:text>

    <xsl:call-template name="emitTargetEnd">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="method" mode="wrapped">
    <xsl:param name="target"/>

    <xsl:call-template name="emitTargetBegin">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:variable name="methodindent">
        <xsl:call-template name="tospace">
            <xsl:with-param name="str" select="@name"/>
        </xsl:call-template>
    </xsl:variable>

    <xsl:text>    virtual HRESULT </xsl:text>
    <xsl:call-template name="uncapitalize">
        <xsl:with-param name="str" select="@name"/>
    </xsl:call-template>
    <xsl:text>(</xsl:text>
    <xsl:variable name="passAutoCaller">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'passcaller'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$passAutoCaller = 'true'">
        <xsl:text>AutoCaller &amp;aAutoCaller</xsl:text>
        <xsl:if test="count(param) > 0">
            <xsl:text>,
                     </xsl:text>
            <xsl:value-of select="$methodindent"/>
        </xsl:if>
    </xsl:if>
    <xsl:for-each select="param">
        <xsl:apply-templates select="@type" mode="wrapped">
            <xsl:with-param name="dir" select="@dir"/>
        </xsl:apply-templates>
        <xsl:if test="not(position()=last())">
            <xsl:text>,
                     </xsl:text>
            <xsl:value-of select="$methodindent"/>
        </xsl:if>
    </xsl:for-each>
    <xsl:text>) = 0;
</xsl:text>

    <xsl:call-template name="emitTargetEnd">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="method" mode="code">
    <xsl:param name="topclass"/>
    <xsl:param name="dtracetopclass"/>
    <xsl:param name="target"/>

    <xsl:call-template name="emitTargetBegin">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:variable name="methodindent">
      <xsl:call-template name="tospace">
          <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="methodclassindent">
      <xsl:call-template name="tospace">
          <xsl:with-param name="str" select="concat($topclass, @name)"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="methodbasename">
        <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="limitedAutoCaller">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'limitedcaller'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="dtracemethodname">
        <xsl:choose>
            <xsl:when test="@dtracename">
                <xsl:value-of select="@dtracename"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="@name"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:variable>
    <xsl:variable name="dtracenamehack"> <!-- Ugly hack to deal with Session::assignMachine and similar. -->
        <xsl:if test="name(..) = 'if'">
            <xsl:value-of select="concat('__', ../@target)"/>
        </xsl:if>
    </xsl:variable>

    <xsl:value-of select="concat('STDMETHODIMP ', $topclass, 'Wrap::', $methodbasename, '(')"/>
    <xsl:for-each select="param">
        <xsl:call-template name="emitPublicParameter">
            <xsl:with-param name="dir" select="@dir"/>
        </xsl:call-template>
        <xsl:if test="not(position()=last())">
            <xsl:text>,
                    </xsl:text>
            <xsl:value-of select="$methodclassindent"/>
        </xsl:if>
    </xsl:for-each>
    <xsl:text>)
{
    LogRelFlow(("{%p} %s: enter</xsl:text>
    <xsl:for-each select="param">
        <xsl:text> </xsl:text>
        <xsl:apply-templates select="@type" mode="logparamtext">
            <xsl:with-param name="dir" select="@dir"/>
            <xsl:with-param name="isref" select="'yes'"/>
        </xsl:apply-templates>
    </xsl:for-each>
    <xsl:text>\n", this</xsl:text>
    <xsl:value-of select="concat(', &quot;', $topclass, '::', @name, '&quot;')"/>
    <xsl:for-each select="param">
        <xsl:text>, </xsl:text>
        <xsl:apply-templates select="@type" mode="logparamval">
            <xsl:with-param name="dir" select="@dir"/>
            <xsl:with-param name="isref" select="'yes'"/>
        </xsl:apply-templates>
    </xsl:for-each>
    <xsl:text>));
</xsl:text>
    <xsl:if test="ancestor::interface[@wrap-gen-hook = 'yes']">
        <xsl:text>
    i_callHook(__FUNCTION__);</xsl:text>
    </xsl:if>
<xsl:text>
    // Clear error info, to make in-process calls behave the same as
    // cross-apartment calls or out-of-process calls.
    VirtualBoxBase::clearError();

    HRESULT hrc;

    try
    {
</xsl:text>
    <!-- @todo check in parameters if possible -->
    <xsl:for-each select="param">
        <xsl:if test="@dir!='in'">
            <xsl:text>        CheckComArgOutPointerValidThrow(a</xsl:text>
            <xsl:call-template name="capitalize">
                <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:text>);
</xsl:text>
        </xsl:if>
    </xsl:for-each>
<xsl:text>
</xsl:text>
    <xsl:for-each select="param">
        <xsl:text>
        </xsl:text>
        <xsl:apply-templates select="@type" mode="paramvalconversion2tmpvar">
            <xsl:with-param name="dir" select="@dir"/>
        </xsl:apply-templates>
    </xsl:for-each>
    <xsl:text>

#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_', $dtracemethodname, substring($dtracenamehack, 2), '_ENTER('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this</xsl:text>
    <xsl:for-each select="param[@dir='in']">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="emitDTraceParamValue">
            <xsl:with-param name="dir" select="@dir"/>
        </xsl:call-template>
    </xsl:for-each>
    <xsl:text>);
#endif
        </xsl:text>
    <xsl:choose>
      <xsl:when test="$limitedAutoCaller = 'true'">
        <xsl:text>AutoLimitedCaller</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>AutoCaller</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text> autoCaller(this);
        hrc = autoCaller.hrc();
        if (SUCCEEDED(hrc))
        {
</xsl:text>
    <xsl:value-of select="concat('            hrc = ', @name, '(')"/>
    <xsl:variable name="passAutoCaller">
        <xsl:call-template name="checkoption">
            <xsl:with-param name="optionlist" select="@wrap-hint-server"/>
            <xsl:with-param name="option" select="'passcaller'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$passAutoCaller = 'true'">
        <xsl:text>autoCaller</xsl:text>
        <xsl:if test="count(param) > 0">
            <xsl:text>,
               </xsl:text>
            <xsl:value-of select="$methodindent"/>
        </xsl:if>
    </xsl:if>
    <xsl:for-each select="param">
        <xsl:apply-templates select="@type" mode="paramvalconversionusingtmp">
            <xsl:with-param name="dir" select="@dir"/>
        </xsl:apply-templates>
        <xsl:if test="not(position()=last())">
            <xsl:text>,
                   </xsl:text>
            <xsl:value-of select="$methodindent"/>
        </xsl:if>
    </xsl:for-each>
    <xsl:text>);
        }
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_', $dtracemethodname, substring($dtracenamehack, 2), '_RETURN('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this, hrc, 0 /*normal*/</xsl:text>
    <xsl:for-each select="param">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="emitDTraceParamValue">
            <xsl:with-param name="dir" select="@dir"/>
        </xsl:call-template>
    </xsl:for-each>
    <xsl:text>);
#endif
    }
    catch (HRESULT hrc2)
    {
        hrc = hrc2;
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_', $dtracemethodname, substring($dtracenamehack, 2), '_RETURN('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this, hrc, 1 /*hrc exception*/</xsl:text>
    <xsl:for-each select="param">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="emitDTraceParamValNoTmp"/>
    </xsl:for-each>
    <xsl:text>);
#endif
    }
    catch (...)
    {
        hrc = VirtualBoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
#ifdef VBOX_WITH_DTRACE_R3_MAIN
        </xsl:text>
    <xsl:value-of select="translate(concat('VBOXAPI_', $dtracetopclass, '_', $dtracemethodname, substring($dtracenamehack, 2), '_RETURN('), $G_lowerCase, $G_upperCase)"/>
    <xsl:text>this, hrc, 9 /*unhandled exception*/</xsl:text>
    <xsl:for-each select="param">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="emitDTraceParamValNoTmp"/>
    </xsl:for-each>
    <xsl:text>);
#endif
    }

    LogRelFlow(("{%p} %s: leave</xsl:text>
    <xsl:for-each select="param">
        <xsl:if test="@dir!='in'">
            <xsl:text> </xsl:text>
            <xsl:apply-templates select="@type" mode="logparamtext">
                <xsl:with-param name="dir" select="@dir"/>
                <xsl:with-param name="isref" select="''"/>
            </xsl:apply-templates>
        </xsl:if>
    </xsl:for-each>
    <xsl:text> hrc=%Rhrc\n", this</xsl:text>
    <xsl:value-of select="concat(', &quot;', $topclass, '::', @name, '&quot;')"/>
    <xsl:for-each select="param">
        <xsl:if test="@dir!='in'">
            <xsl:text>, </xsl:text>
            <xsl:apply-templates select="@type" mode="logparamval">
                <xsl:with-param name="dir" select="@dir"/>
                <xsl:with-param name="isref" select="''"/>
            </xsl:apply-templates>
        </xsl:if>
    </xsl:for-each>
    <xsl:text>, hrc));
    return hrc;
}
</xsl:text>

    <xsl:call-template name="emitTargetEnd">
        <xsl:with-param name="target" select="$target"/>
    </xsl:call-template>

    <xsl:text>
</xsl:text>
</xsl:template>

<!--  - - - - - - - - - - - - - - - - - - - - - -
  Emits the DTrace probes for a method.
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template match="method" mode="dtrace-probes">
    <xsl:param name="topclass"/>
    <xsl:param name="dtracetopclass"/>
    <xsl:param name="target"/>

    <xsl:variable name="dtracemethodname">
        <xsl:choose>
            <xsl:when test="@dtracename">
                <xsl:value-of select="@dtracename"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="@name"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:variable>
    <xsl:variable name="dtracenamehack"> <!-- Ugly hack to deal with Session::assignMachine and similar. -->
        <xsl:if test="name(..) = 'if'">
            <xsl:value-of select="concat('__', ../@target)"/>
        </xsl:if>
    </xsl:variable>

    <xsl:text>    probe </xsl:text>
    <!-- <xsl:value-of select="concat($dtracetopclass, '__', $dtracemethodname, $dtracenamehack, '__enter(struct ', $dtracetopclass, ' *a_pThis')"/> -->
    <xsl:value-of select="concat($dtracetopclass, '__', $dtracemethodname, $dtracenamehack, '__enter(void *a_pThis')"/>
    <xsl:for-each select="param[@dir='in']">
        <xsl:text>, </xsl:text>
        <xsl:apply-templates select="@type" mode="dtraceparamdecl">
            <xsl:with-param name="dir" select="'@dir'"/>
        </xsl:apply-templates>
    </xsl:for-each>
    <xsl:text>);
    probe </xsl:text>
    <!-- <xsl:value-of select="concat($dtracetopclass, '__', $dtracemethodname, '__return(struct ', $dtracetopclass, ' *a_pThis')"/> -->
    <xsl:value-of select="concat($dtracetopclass, '__', $dtracemethodname, $dtracenamehack, '__return(void *a_pThis')"/>
    <xsl:text>, uint32_t a_hrc, int32_t enmWhy</xsl:text>
    <xsl:for-each select="param">
        <xsl:text>, </xsl:text>
        <xsl:apply-templates select="@type" mode="dtraceparamdecl">
            <xsl:with-param name="dir" select="'@dir'"/>
        </xsl:apply-templates>
    </xsl:for-each>
    <xsl:text>);
</xsl:text>

</xsl:template>


<xsl:template name="emitIf">
    <xsl:param name="passmode"/>
    <xsl:param name="target"/>
    <xsl:param name="topclass"/>
    <xsl:param name="emitmode"/>
    <xsl:param name="dtracetopclass"/>

    <xsl:if test="($target = 'xpidl') or ($target = 'midl')">
        <xsl:choose>
            <xsl:when test="$passmode='public'">
                <xsl:choose>
                    <xsl:when test="$emitmode='method'">
                        <xsl:apply-templates select="method" mode="public">
                            <xsl:with-param name="target" select="$target"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:when test="$emitmode='attribute'">
                        <xsl:apply-templates select="attribute" mode="public">
                            <xsl:with-param name="target" select="$target"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:otherwise/>
                </xsl:choose>
            </xsl:when>
            <xsl:when test="$passmode='wrapped'">
                <xsl:choose>
                    <xsl:when test="$emitmode='method'">
                        <xsl:apply-templates select="method" mode="wrapped">
                            <xsl:with-param name="target" select="$target"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:when test="$emitmode='attribute'">
                        <xsl:apply-templates select="attribute" mode="wrapped">
                            <xsl:with-param name="target" select="$target"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:otherwise/>
                </xsl:choose>
            </xsl:when>
            <xsl:when test="$passmode='code'">
                <xsl:choose>
                    <xsl:when test="$emitmode='method'">
                        <xsl:apply-templates select="method" mode="code">
                            <xsl:with-param name="target" select="$target"/>
                            <xsl:with-param name="topclass" select="$topclass"/>
                            <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:when test="$emitmode='attribute'">
                        <xsl:apply-templates select="attribute" mode="code">
                            <xsl:with-param name="target" select="$target"/>
                            <xsl:with-param name="topclass" select="$topclass"/>
                            <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:otherwise/>
                </xsl:choose>
            </xsl:when>
            <xsl:when test="$passmode = 'dtrace-probes'">
                <xsl:choose>
                    <xsl:when test="$emitmode = 'method'">
                        <xsl:apply-templates select="method" mode="dtrace-probes">
                            <xsl:with-param name="target" select="$target"/>
                            <xsl:with-param name="topclass" select="$topclass"/>
                            <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:when test="$emitmode = 'attribute'">
                        <xsl:apply-templates select="attribute" mode="dtrace-probes">
                            <xsl:with-param name="target" select="$target"/>
                            <xsl:with-param name="topclass" select="$topclass"/>
                            <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:otherwise/>
                </xsl:choose>
            </xsl:when>
            <xsl:otherwise/>
        </xsl:choose>
    </xsl:if>
</xsl:template>

<xsl:template match="if" mode="public">
    <xsl:param name="emitmode"/>

    <xsl:call-template name="emitIf">
        <xsl:with-param name="passmode" select="'public'"/>
        <xsl:with-param name="target" select="@target"/>
        <xsl:with-param name="emitmode" select="$emitmode"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="if" mode="wrapped">
    <xsl:param name="emitmode"/>

    <xsl:call-template name="emitIf">
        <xsl:with-param name="passmode" select="'wrapped'"/>
        <xsl:with-param name="target" select="@target"/>
        <xsl:with-param name="emitmode" select="$emitmode"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="if" mode="code">
    <xsl:param name="topclass"/>
    <xsl:param name="emitmode"/>
    <xsl:param name="dtracetopclass"/>

    <xsl:call-template name="emitIf">
        <xsl:with-param name="passmode" select="'code'"/>
        <xsl:with-param name="target" select="@target"/>
        <xsl:with-param name="emitmode" select="$emitmode"/>
        <xsl:with-param name="topclass" select="$topclass"/>
        <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="if" mode="dtrace-probes">
    <xsl:param name="topclass"/>
    <xsl:param name="emitmode"/>
    <xsl:param name="dtracetopclass"/>

    <xsl:call-template name="emitIf">
        <xsl:with-param name="passmode" select="'dtrace-probes'"/>
        <xsl:with-param name="target" select="@target"/>
        <xsl:with-param name="emitmode" select="$emitmode"/>
        <xsl:with-param name="topclass" select="$topclass"/>
        <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
    </xsl:call-template>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit all methods of the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitMethods">
    <xsl:param name="topclass"/>
    <xsl:param name="pmode"/>
    <xsl:param name="dtracetopclass"/>

    <xsl:variable name="name" select="@name"/>
    <!-- first recurse to emit all base interfaces -->
    <xsl:variable name="extends" select="@extends"/>
    <xsl:if test="$extends and not($extends='$unknown') and not($extends='$errorinfo')">
        <xsl:for-each select="key('G_keyInterfacesByName', $extends)">
            <xsl:call-template name="emitMethods">
                <xsl:with-param name="topclass" select="$topclass"/>
                <xsl:with-param name="pmode" select="$pmode"/>
                <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
            </xsl:call-template>
        </xsl:for-each>
    </xsl:if>

    <xsl:choose>
        <xsl:when test="$pmode='code'">
            <xsl:text>//
</xsl:text>
            <xsl:value-of select="concat('// ', $name, ' methods')"/>
            <xsl:text>
//

</xsl:text>
        </xsl:when>
        <xsl:when test="$pmode='dtrace-probes'"/>
        <xsl:otherwise>
            <xsl:value-of select="concat($G_sNewLine, '    /** @name ', translate(substring($pmode, 1, 1), $G_lowerCase, $G_upperCase), substring($pmode,2), ' ', $name, ' methods', $G_sNewLine)"/>
            <xsl:text>     * @{ */
</xsl:text>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
        <xsl:when test="$pmode='public'">
            <xsl:apply-templates select="./method | ./if" mode="public">
                <xsl:with-param name="emitmode" select="'method'"/>
            </xsl:apply-templates>
            <xsl:variable name="reservedMethods" select="@reservedMethods"/>
            <xsl:if test="$reservedMethods > 0">
                <!-- tricky way to do a "for" loop without recursion -->
                <xsl:for-each select="(//*)[position() &lt;= $reservedMethods]">
                    <xsl:text>    STDMETHOD(InternalAndReservedMethod</xsl:text>
                    <xsl:value-of select="concat(position(), $name)"/>
                    <xsl:text>)();&#x0A;</xsl:text>
                </xsl:for-each>
            </xsl:if>
        </xsl:when>
        <xsl:when test="$pmode='wrapped'">
            <xsl:apply-templates select="./method | ./if" mode="wrapped">
                <xsl:with-param name="emitmode" select="'method'"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:when test="$pmode='code'">
            <xsl:apply-templates select="./method | ./if" mode="code">
                <xsl:with-param name="topclass" select="$topclass"/>
                <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                <xsl:with-param name="emitmode" select="'method'"/>
            </xsl:apply-templates>
            <xsl:variable name="reservedMethods" select="@reservedMethods"/>
            <xsl:if test="$reservedMethods > 0">
                <!-- tricky way to do a "for" loop without recursion -->
                <xsl:for-each select="(//*)[position() &lt;= $reservedMethods]">
                    <xsl:value-of select="concat('STDMETHODIMP ', $topclass, 'Wrap::InternalAndReservedMethod', position(), $name, '()&#x0A;')"/>
                    <xsl:text>{
    return E_NOTIMPL;
}

</xsl:text>
                </xsl:for-each>
            </xsl:if>
        </xsl:when>
        <xsl:when test="$pmode='dtrace-probes'">
            <xsl:apply-templates select="./method | ./if" mode="dtrace-probes">
                <xsl:with-param name="topclass" select="$topclass"/>
                <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                <xsl:with-param name="emitmode" select="'method'"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise/>
    </xsl:choose>

    <!-- close doxygen @name -->
    <xsl:if test="($pmode != 'code') and ($pmode != 'dtrace-probes')" >
            <xsl:text>    /** @} */
</xsl:text>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit all attributes and methods declarations of the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitInterfaceDecls">
    <xsl:param name="pmode"/>

    <!-- attributes -->
    <xsl:call-template name="emitAttributes">
        <xsl:with-param name="pmode" select="$pmode"/>
    </xsl:call-template>

    <!-- methods -->
    <xsl:call-template name="emitMethods">
        <xsl:with-param name="pmode" select="$pmode"/>
    </xsl:call-template>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit auxiliary method declarations of the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitAuxMethodDecls">
    <!-- currently nothing, maybe later some generic FinalConstruct/... helper declaration for ComObjPtr -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit the header file of the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitHeader">
    <xsl:param name="addinterfaces"/>

    <xsl:variable name="filename" select="concat(substring(@name, 2), 'Wrap.h')"/>

    <xsl:apply-templates select="." mode="startfile">
        <xsl:with-param name="file" select="$filename"/>
    </xsl:apply-templates>
    <xsl:call-template name="fileheader">
        <xsl:with-param name="name" select="$filename"/>
        <xsl:with-param name="class" select="substring(@name, 2)"/>
        <xsl:with-param name="type" select="'header'"/>
    </xsl:call-template>
    <xsl:apply-templates select="." mode="classheader">
        <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
    </xsl:apply-templates>

    <!-- interface attributes/methods (public) -->
    <xsl:call-template name="emitInterfaceDecls">
        <xsl:with-param name="pmode" select="'public'"/>
    </xsl:call-template>

    <xsl:for-each select="exsl:node-set($addinterfaces)/token">
        <!-- This is super tricky, as the for-each switches to the node set,
             which means the normal document isn't available any more.  We get
             the data we need, uses a for-each to switch document and then a
             key() to look up the interface by name. -->
        <xsl:variable name="addifname">
            <xsl:value-of select="string(.)"/>
        </xsl:variable>
        <xsl:for-each select="$G_root">
            <xsl:for-each select="key('G_keyInterfacesByName', $addifname)">
                <xsl:call-template name="emitInterfaceDecls">
                    <xsl:with-param name="pmode" select="'public'"/>
                </xsl:call-template>
            </xsl:for-each>
        </xsl:for-each>
    </xsl:for-each>

    <!-- auxiliary methods (public) -->
    <xsl:call-template name="emitAuxMethodDecls"/>

    <!-- switch to private -->
    <xsl:text>
private:</xsl:text>

    <!-- wrapped interface attributes/methods (private) -->
    <xsl:call-template name="emitInterfaceDecls">
        <xsl:with-param name="pmode" select="'wrapped'"/>
    </xsl:call-template>

    <xsl:for-each select="exsl:node-set($addinterfaces)/token">
        <!-- This is super tricky, as the for-each switches to the node set,
             which means the normal document isn't available any more.  We get
             the data we need, uses a for-each to switch document and then a
             key() to look up the interface by name. -->
        <xsl:variable name="addifname">
            <xsl:value-of select="string(.)"/>
        </xsl:variable>
        <xsl:for-each select="$G_root">
            <xsl:for-each select="key('G_keyInterfacesByName', $addifname)">
                <xsl:call-template name="emitInterfaceDecls">
                    <xsl:with-param name="pmode" select="'wrapped'"/>
                </xsl:call-template>
            </xsl:for-each>
        </xsl:for-each>
    </xsl:for-each>

    <xsl:apply-templates select="." mode="classfooter"/>
    <xsl:apply-templates select="." mode="endfile">
        <xsl:with-param name="file" select="$filename"/>
    </xsl:apply-templates>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit all attributes and methods definitions (pmode=code) or probes (pmode=dtrace-probes) of the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitInterfaceDefs">
    <xsl:param name="addinterfaces"/>
    <xsl:param name="pmode" select="'code'"/>

    <xsl:variable name="topclass" select="substring(@name, 2)"/>
    <xsl:variable name="dtracetopclass">
        <xsl:choose>
            <xsl:when test="@dtracename"><xsl:value-of select="@dtracename"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="$topclass"/></xsl:otherwise>
        </xsl:choose>
    </xsl:variable>

    <xsl:if test="$pmode = 'code'">
        <xsl:value-of select="concat('DEFINE_EMPTY_CTOR_DTOR(', $topclass, 'Wrap)', $G_sNewLine, $G_sNewLine)"/>
    </xsl:if>

    <!-- attributes -->
    <xsl:call-template name="emitAttributes">
        <xsl:with-param name="topclass" select="$topclass"/>
        <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
        <xsl:with-param name="pmode" select="$pmode"/>
    </xsl:call-template>

    <xsl:for-each select="exsl:node-set($addinterfaces)/token">
        <!-- This is super tricky, as the for-each switches to the node set,
             which means the normal document isn't available any more.  We get
             the data we need, uses a for-each to switch document and then a
             key() to look up the interface by name. -->
        <xsl:variable name="addifname">
            <xsl:value-of select="string(.)"/>
        </xsl:variable>
        <xsl:for-each select="$G_root">
            <xsl:for-each select="key('G_keyInterfacesByName', $addifname)">
                <xsl:call-template name="emitAttributes">
                    <xsl:with-param name="topclass" select="$topclass"/>
                    <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                    <xsl:with-param name="pmode" select="$pmode"/>
                </xsl:call-template>
            </xsl:for-each>
        </xsl:for-each>
    </xsl:for-each>

    <!-- methods -->
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:call-template name="emitMethods">
        <xsl:with-param name="topclass" select="$topclass"/>
        <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
        <xsl:with-param name="pmode" select="$pmode"/>
    </xsl:call-template>

    <xsl:for-each select="exsl:node-set($addinterfaces)/token">
        <!-- This is super tricky, as the for-each switches to the node set,
             which means the normal document isn't available any more.  We get
             the data we need, uses a for-each to switch document and then a
             key() to look up the interface by name. -->
        <xsl:variable name="addifname">
            <xsl:value-of select="string(.)"/>
        </xsl:variable>
        <xsl:for-each select="$G_root">
            <xsl:for-each select="key('G_keyInterfacesByName', $addifname)">
                <xsl:call-template name="emitMethods">
                    <xsl:with-param name="topclass" select="$topclass"/>
                    <xsl:with-param name="dtracetopclass" select="$dtracetopclass"/>
                    <xsl:with-param name="pmode" select="$pmode"/>
                </xsl:call-template>
            </xsl:for-each>
        </xsl:for-each>
    </xsl:for-each>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit auxiliary method declarations of the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitAuxMethodDefs">
    <xsl:param name="pmode" select="'code'"/>
    <!-- currently nothing, maybe later some generic FinalConstruct/... implementation -->
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit the code file of the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitCode">
    <xsl:param name="addinterfaces"/>

    <xsl:variable name="filename" select="concat(substring(@name, 2), 'Wrap.cpp')"/>

    <xsl:apply-templates select="." mode="startfile">
        <xsl:with-param name="file" select="$filename"/>
    </xsl:apply-templates>
    <xsl:call-template name="fileheader">
        <xsl:with-param name="name" select="$filename"/>
        <xsl:with-param name="class" select="substring(@name, 2)"/>
        <xsl:with-param name="type" select="'code'"/>
    </xsl:call-template>
    <xsl:apply-templates select="." mode="codeheader">
        <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
    </xsl:apply-templates>

    <!-- interface attributes/methods (public) -->
    <xsl:call-template name="emitInterfaceDefs">
        <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
    </xsl:call-template>

    <!-- auxiliary methods (public) -->
    <xsl:call-template name="emitAuxMethodDefs"/>

    <xsl:apply-templates select="." mode="codefooter">
        <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="." mode="endfile">
        <xsl:with-param name="file" select="$filename"/>
    </xsl:apply-templates>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  emit the DTrace probes for the current interface
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitDTraceProbes">
    <xsl:param name="addinterfaces"/>

    <!-- interface attributes/methods (public) -->
    <xsl:call-template name="emitInterfaceDefs">
        <xsl:with-param name="addinterfaces" select="$addinterfaces"/>
        <xsl:with-param name="pmode">dtrace-probes</xsl:with-param>
    </xsl:call-template>

    <!-- auxiliary methods (public) -->
    <xsl:call-template name="emitAuxMethodDefs">
        <xsl:with-param name="pmode">dtrace-probes</xsl:with-param>
    </xsl:call-template>

</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  wildcard match, ignore everything which has no explicit match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="*"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  ignore all if tags except those for XPIDL or MIDL target
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="if">
    <xsl:if test="(@target = 'xpidl') or (@target = 'midl')">
        <xsl:apply-templates/>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  interface match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="interface">
    <xsl:if test="not(@internal='yes') and not(@autogen='VBoxEvent') and not(@supportsErrorInfo='no')">
        <xsl:call-template name="emitInterface"/>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  application match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="application">
    <xsl:apply-templates/>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  library match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="library">
    <xsl:apply-templates/>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  root match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="/idl">
    <xsl:choose>
        <xsl:when test="$generating = 'headers'">
            <xsl:apply-templates/>
        </xsl:when>
        <xsl:when test="$generating = 'sources'">
            <xsl:apply-templates/>
        </xsl:when>
        <xsl:when test="$generating = 'dtrace-probes'">
            <xsl:apply-templates/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:message terminate="yes">
                Unknown string parameter value: generating='<xsl:value-of select="$generating"/>'
            </xsl:message>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

</xsl:stylesheet>
<!-- vi: set tabstop=4 shiftwidth=4 expandtab: -->
