<?xml version="1.0"?>

<!--
    websrv-nsmap.xsl:
        XSLT stylesheet that generates a vboxweb.nsmap file from
        VirtualBox.xidl, which gets included from C++ client and
        server code.
        See webservice/Makefile.kmk for an overview of all the things
        generated for the webservice.
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
  xmlns:xsd="http://www.w3.org/2001/XMLSchema">

  <xsl:output method="text"/>

  <xsl:strip-space elements="*"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_xsltFilename" select="'websrv-typemap.xsl'" />

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<!-- - - - - - - - - - - - - - - - - - - - - - -
  root match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="/idl">
  <xsl:text><![CDATA[
/* DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
 * Generator: src/VBox/Main/webservice/websrv-nsmap.xsl */

#include "soapH.h"
SOAP_NMAC struct Namespace namespaces[] =
{
    {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
    {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
    {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
]]></xsl:text>

  <xsl:value-of select="concat('    {&quot;vbox&quot;, &quot;', $G_targetNamespace, $G_targetNamespaceSeparator, '&quot;, NULL, NULL},')" />
  <xsl:call-template name="emitNewline" />

  <xsl:text><![CDATA[
    {NULL, NULL, NULL, NULL}
};

]]></xsl:text>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  if
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
 *  ignore all |if|s except those for WSDL target
-->
<xsl:template match="if">
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
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  note
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="note">
  <xsl:apply-templates />
</xsl:template>

<xsl:template match="interface | collection">
</xsl:template>

</xsl:stylesheet>
