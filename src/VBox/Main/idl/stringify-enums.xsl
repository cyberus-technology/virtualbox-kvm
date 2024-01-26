<?xml version="1.0"?>

<!--
    stringify-enums.xsl - Generates stringify functions for all the enums in VirtualBox.xidl.
-->
<!--
    Copyright (C) 2022-2023 Oracle and/or its affiliates.

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
  Parameters
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:param name="G_kind">source</xsl:param>


<!-- - - - - - - - - - - - - - - - - - - - - - -
templates for file headers
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template name="fileheader">
    <xsl:param name="class"/>
    <xsl:param name="name"/>
    <xsl:param name="type"/>

    <xsl:text>/** @file
 * VirtualBox API Enum Stringifier - </xsl:text>
    <xsl:choose>
        <xsl:when test="$G_kind = 'header'">Header</xsl:when>
        <xsl:when test="$G_kind = 'source'">Definition</xsl:when>
    </xsl:choose>
<xsl:text>.
 *
 * DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl
 * Generator: src/VBox/Main/idl/stringify-enums.xsl
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Emits a function prototype for the header file.
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitEnumStringifyPrototype">
    <xsl:text>const char *stringify</xsl:text><xsl:value-of select="@name"/><xsl:text>(</xsl:text>
    <xsl:value-of select="@name"/><xsl:text>_T aValue) RT_NOEXCEPT;
</xsl:text>
</xsl:template>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Emits a function definition for the source file.
  - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template name="emitEnumStringifyFunction">
    <xsl:text>

const char *stringify</xsl:text><xsl:value-of select="@name"/><xsl:text>(</xsl:text>
    <xsl:value-of select="@name"/><xsl:text>_T aValue) RT_NOEXCEPT
{
    switch (aValue)
    {
</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>        default:
            AssertMsgFailedReturn(("%d / %#x\n", aValue, aValue), formatUnknown("</xsl:text>
                <xsl:value-of select="@name"/><xsl:text>", (int)aValue));
    }
}
</xsl:text>
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
  const match - emits case statemtns
 - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template match="const">
    <!-- HACK ALERT! There are 4 enums in MachineState that have duplicate values,
                     we exploit the existing @wsmap="suppress" markers on these to
                     avoid generating code which doesn't compile. -->
    <xsl:if test="not(@wsmap) or @wsmap != 'suppress'">
        <xsl:text>        case </xsl:text><xsl:value-of select="../@name"/>
                                          <xsl:text>_</xsl:text>
                                          <xsl:value-of select="@name"/><xsl:text>:
            return "</xsl:text><xsl:value-of select="@name"/><xsl:text>";
</xsl:text>
    </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  enum match
 - - - - - - - - - - - - - - - - - - - - - - -->
<xsl:template match="enum">
    <xsl:choose>
        <xsl:when test="$G_kind = 'header'">
            <xsl:call-template name="emitEnumStringifyPrototype"/>
        </xsl:when>
        <xsl:when test="$G_kind = 'source'">
            <xsl:call-template name="emitEnumStringifyFunction"/>
        </xsl:when>
    </xsl:choose>
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
        <xsl:when test="$G_kind = 'header'">
            <xsl:call-template name="fileheader"/>
            <xsl:text>
#ifndef INCLUDED_GENERATED_StringifyEnums_h
#define INCLUDED_GENERATED_StringifyEnums_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBox/com/VirtualBox.h"

</xsl:text>
            <xsl:apply-templates/>
            <xsl:text>
#endif /* INCLUDED_GENERATED_StringifyEnums_h */
</xsl:text>
        </xsl:when>

        <xsl:when test="$G_kind = 'source'">
            <xsl:call-template name="fileheader"/>
            <xsl:text>

/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "StringifyEnums.h"

#include "iprt/asm.h"
#include "iprt/assert.h"
#include "iprt/string.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
typedef char UNKNOWNBUF[64];
static UNKNOWNBUF           s_aszUnknown[16];
static uint32_t volatile    s_iUnknown = 0;


static const char *formatUnknown(const char *pszName, int iValue)
{
    size_t iUnknown = ASMAtomicIncU32(&amp;s_iUnknown) % RT_ELEMENTS(s_aszUnknown);
    char *pszBuf = s_aszUnknown[iUnknown];
    RTStrPrintf(pszBuf, sizeof(UNKNOWNBUF), "Unk-%s-%#x", pszName, iValue);
    return pszBuf;
}

</xsl:text>
            <xsl:apply-templates/>
        </xsl:when>

        <xsl:otherwise>
            <xsl:message terminate="yes">
                Unknown string parameter value: G_kind='<xsl:value-of select="$G_kind"/>'
            </xsl:message>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

</xsl:stylesheet>
<!-- vi: set tabstop=4 shiftwidth=4 expandtab: -->
