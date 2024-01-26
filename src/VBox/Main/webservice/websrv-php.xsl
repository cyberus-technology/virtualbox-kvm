<xsl:stylesheet version = '1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:vbox="http://www.virtualbox.org/">

<!--
    websrv-php.xsl:
        XSLT stylesheet that generates vboxServiceWrappers.php from
        VirtualBox.xidl. This PHP file represents our
        web service API. Depends on WSDL file for actual SOAP bindings.

     Contributed by James Lucas (mjlucas at eng.uts.edu.au).
-->
<!--
    Copyright (C) 2008-2023 Oracle and/or its affiliates.

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

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<xsl:variable name="G_setSuppressedInterfaces"
              select="//interface[@wsmap='suppress']" />

<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>

<xsl:template name="emitOutParam">
  <xsl:param name="type" />
  <xsl:param name="value" />
  <xsl:param name="safearray" />

  <xsl:choose>
    <xsl:when test="$type='wstring' or $type='uuid'">
      <xsl:call-template name="emitPrimitive">
        <xsl:with-param name="type">string</xsl:with-param>
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$type='boolean'">
      <xsl:call-template name="emitPrimitive">
        <xsl:with-param name="type">bool</xsl:with-param>
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$type='short' or $type='unsigned short' or $type='long' or $type='octet'">
      <xsl:call-template name="emitPrimitive">
        <xsl:with-param name="type">int</xsl:with-param>
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$type='double' or $type='float' or $type='unsigned long' or $type='long long' or $type='unsigned long long'">
      <xsl:call-template name="emitPrimitive">
        <xsl:with-param name="type">float</xsl:with-param>
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$type='$unknown'">
      <xsl:call-template name="emitObject">
        <xsl:with-param name="type">VBox_ManagedObject</xsl:with-param>
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
   </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="emitObject">
        <xsl:with-param name="type" select="$type" />
        <xsl:with-param name="value" select="$value" />
        <xsl:with-param name="safearray" select="$safearray"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="emitObject">
  <xsl:param name="type" />
  <xsl:param name="value" />
  <xsl:param name="safearray" />
  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:text>new </xsl:text><xsl:value-of select="$type" />Collection ($this->connection, (array)<xsl:value-of select="$value"/><xsl:text>)</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>new </xsl:text><xsl:value-of select="$type" /> ($this->connection, <xsl:value-of select="$value"/><xsl:text>)</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="emitPrimitive">
  <xsl:param name="type" />
  <xsl:param name="value" />
  <xsl:param name="safearray" />
  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:text>(array)</xsl:text><xsl:value-of select="$value"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>(</xsl:text><xsl:value-of select="$type" /><xsl:text>)</xsl:text><xsl:value-of select="$value"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="emitGetAttribute">
  <xsl:param name="ifname" />
  <xsl:param name="attrname" />
  <xsl:param name="attrtype" />
  <xsl:param name="attrsafearray" />
  <xsl:variable name="fname"><xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="$attrname"/></xsl:call-template> </xsl:variable>
    public function <xsl:value-of select="$fname"/>()
    {
        $request = new stdClass();
        $request->_this = $this->handle;
        $response = $this->connection->__soapCall('<xsl:value-of select="$ifname"/>_<xsl:value-of select="$fname"/>', array((array)$request));
        <xsl:text>return </xsl:text>
        <xsl:call-template name="emitOutParam">
            <xsl:with-param name="type" select="$attrtype" />
            <xsl:with-param name="value" select="concat('$response->','returnval')" />
            <xsl:with-param name="safearray" select="@safearray"/>
          </xsl:call-template><xsl:text>;</xsl:text>
    }
</xsl:template>

<xsl:template name="emitSetAttribute">
  <xsl:param name="ifname" />
  <xsl:param name="attrname" />
  <xsl:param name="attrtype" />
  <xsl:param name="attrsafearray" />
  <xsl:variable name="fname"><xsl:call-template name="makeSetterName"><xsl:with-param name="attrname" select="$attrname"/></xsl:call-template></xsl:variable>
    public function <xsl:value-of select="$fname"/>($value)
    {
        $request = new stdClass();
        $request->_this = $this->handle;
<xsl:choose>
<xsl:when test="$attrsafearray='yes'">        if (is_array($value) || is_null($value) || is_scalar($value))</xsl:when>
<xsl:otherwise>        if (is_null($value) || is_scalar($value))</xsl:otherwise>
</xsl:choose>
        {
            $request-><xsl:value-of select="$attrname"/> = $value;
        }
        else
        {
            $request-><xsl:value-of select="$attrname"/> = $value->handle;
        }
        $this->connection->__soapCall('<xsl:value-of select="$ifname"/>_<xsl:value-of select="$fname"/>', array((array)$request));
    }
</xsl:template>

<xsl:template name="interface">
   <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
   <xsl:variable name="wsmap"><xsl:value-of select="@wsmap" /></xsl:variable>
   <xsl:variable name="extends"><xsl:value-of select="@extends" /></xsl:variable>
   <xsl:text>
/**
 * Generated VBoxWebService Interface Wrapper
 */
</xsl:text>
   <xsl:choose>
      <xsl:when test="($extends = '$unknown') or ($extends = '$errorinfo')">
         <xsl:value-of select="concat('class ', $ifname, ' extends VBox_ManagedObject&#10;{&#10;')" />
      </xsl:when>
      <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
         <xsl:value-of select="concat('class ', $ifname, ' extends ', $extends, '&#10;{&#10;')" />
      </xsl:when>
   </xsl:choose>
   <xsl:for-each select="method">
      <xsl:if test="not((param[@type=($G_setSuppressedInterfaces/@name)])
                      or (param[@mod='ptr']))" >
           <xsl:call-template name="method">
               <xsl:with-param name="wsmap" select="$wsmap" />
            </xsl:call-template>
      </xsl:if>
  </xsl:for-each>
  <xsl:for-each select="attribute">
      <xsl:variable name="attrname"><xsl:value-of select="@name" /></xsl:variable>
      <xsl:variable name="attrtype"><xsl:value-of select="@type" /></xsl:variable>
      <xsl:variable name="attrreadonly"><xsl:value-of select="@readonly" /></xsl:variable>
      <xsl:variable name="attrsafearray"><xsl:value-of select="@safearray" /></xsl:variable>
      <!-- skip this attribute if it has parameters of a type that has wsmap="suppress" -->
      <xsl:choose>
        <xsl:when test="( $attrtype=($G_setSuppressedInterfaces/@name) )">
          <xsl:comment><xsl:value-of select="concat('skipping attribute ', $attrtype, ' for it is of a suppressed type')" /></xsl:comment>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="@readonly='yes'">
              <xsl:comment> readonly attribute <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$attrname" /> </xsl:comment>
            </xsl:when>
            <xsl:otherwise>
              <xsl:comment> read/write attribute <xsl:copy-of select="$ifname" />::<xsl:copy-of select="$attrname" /> </xsl:comment>
            </xsl:otherwise>
          </xsl:choose>
          <!-- aa) get method: emit request and result -->
          <xsl:call-template name="emitGetAttribute">
            <xsl:with-param name="ifname" select="$ifname" />
            <xsl:with-param name="attrname" select="$attrname" />
            <xsl:with-param name="attrtype" select="$attrtype" />
            <xsl:with-param name="attrsafearray" select="$attrsafearray" />
          </xsl:call-template>
          <!-- bb) emit a set method if the attribute is read/write -->
          <xsl:if test="not($attrreadonly='yes')">
            <xsl:call-template name="emitSetAttribute">
              <xsl:with-param name="ifname" select="$ifname" />
              <xsl:with-param name="attrname" select="$attrname" />
              <xsl:with-param name="attrtype" select="$attrtype" />
              <xsl:with-param name="attrsafearray" select="$attrsafearray" />
            </xsl:call-template>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>
  </xsl:for-each>
  <xsl:text>}
</xsl:text>
</xsl:template>

<xsl:template name="collection">
   <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
   <xsl:text>
/**
 * Generated VBoxWebService Managed Object Collection
 */</xsl:text>
class <xsl:value-of select="$ifname"/>Collection extends VBox_ManagedObjectCollection
{
    protected $_interfaceName = "<xsl:value-of select="$ifname"/>";
}
</xsl:template>

<xsl:template name="interfacestruct">
   <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
   <xsl:text>
/**
 * Generated VBoxWebService Struct
 */</xsl:text>
class <xsl:value-of select="$ifname"/> extends VBox_Struct
{
<xsl:for-each select="attribute">    protected $<xsl:value-of select="@name"/>;
</xsl:for-each>
    public function __construct($connection, $values)
    {
        $this->connection = $connection;
<xsl:for-each select="attribute">        $this-><xsl:value-of select="@name"/> = $values-><xsl:value-of select="@name"/>;
</xsl:for-each>    }

<xsl:for-each select="attribute">    public function <xsl:call-template name="makeGetterName"><xsl:with-param name="attrname" select="@name"/></xsl:call-template>()
    {
        <xsl:text>return </xsl:text>
        <xsl:call-template name="emitOutParam">
           <xsl:with-param name="type" select="@type" />
           <xsl:with-param name="value" select="concat('$this->',@name)" />
           <xsl:with-param name="safearray" select="@safearray"/>
         </xsl:call-template>;
    }
</xsl:for-each>}
</xsl:template>

<xsl:template name="structcollection">
   <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
   <xsl:text>
/**
 * Generated VBoxWebService Struct Collection
 */</xsl:text>
class <xsl:value-of select="$ifname"/>Collection extends VBox_StructCollection
{
    protected $_interfaceName = "<xsl:value-of select="$ifname"/>";
}
</xsl:template>

<xsl:template name="genreq">
    <xsl:param name="wsmap" />
    <xsl:text>        $request = new stdClass();
</xsl:text>
    <xsl:if test="$wsmap='managed'">        $request->_this = $this->handle;</xsl:if>
    <xsl:for-each select="param[@dir='in']">
        $request-><xsl:value-of select="@name" /> = $arg_<xsl:value-of select="@name" /><xsl:text>;</xsl:text>
    </xsl:for-each>
        $response = $this->connection->__soapCall('<xsl:value-of select="../@name"/>_<xsl:value-of select="@name"/>', array((array)$request));
        return <xsl:if test="param[@dir='out']">
                 <xsl:text>array(</xsl:text>
               </xsl:if>
         <xsl:for-each select="param[@dir='return']">
         <xsl:call-template name="emitOutParam">
           <xsl:with-param name="type" select="@type" />
           <xsl:with-param name="value" select="concat('$response->','returnval')" />
           <xsl:with-param name="safearray" select="@safearray"/>
         </xsl:call-template>
         <xsl:if test="../param[@dir='out']">
           <xsl:text>, </xsl:text>
         </xsl:if>
       </xsl:for-each>
       <xsl:for-each select="param[@dir='out']">
         <xsl:if test="not(position()=1)">
           <xsl:text>, </xsl:text>
         </xsl:if>
         <xsl:call-template name="emitOutParam">
           <xsl:with-param name="type" select="@type" />
           <xsl:with-param name="value" select="concat('$response->',@name)" />
           <xsl:with-param name="safearray" select="@safearray"/>
         </xsl:call-template>
       </xsl:for-each>
       <xsl:if test="param[@dir='out']">
           <xsl:text>)</xsl:text>
       </xsl:if>
       <xsl:text>;&#10;</xsl:text>
</xsl:template>

<xsl:template name="method" >
    <xsl:param name="wsmap" />
    public function <xsl:value-of select="@name"/><xsl:text>(</xsl:text>
    <xsl:for-each select="param[@dir='in']">
      <xsl:if test="not(position()=1)">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:value-of select="concat('$arg_',@name)"/>
    </xsl:for-each> <xsl:text>)&#10;    {&#10;</xsl:text>
    <xsl:call-template name="genreq"><xsl:with-param name="wsmap" select="$wsmap" /></xsl:call-template>
    <xsl:text>    }&#10;</xsl:text>
</xsl:template>

<xsl:template name="enum">
  <xsl:text>
/**
 * Generated VBoxWebService ENUM
 */</xsl:text>
class <xsl:value-of select="@name"/> extends VBox_Enum
{
    public $NameMap = array(<xsl:for-each select="const"><xsl:if test="not(@wsmap='suppress')"><xsl:value-of select="@value"/> => '<xsl:value-of select="@name"/>'<xsl:if test="not(position()=last())">, </xsl:if></xsl:if></xsl:for-each>);
    public $ValueMap = array(<xsl:for-each select="const"><xsl:if test="not(@wsmap='suppress')">'<xsl:value-of select="@name"/>' => <xsl:value-of select="@value"/><xsl:if test="not(position()=last())">, </xsl:if></xsl:if></xsl:for-each>);
}
</xsl:template>

<xsl:template name="enumcollection">
   <xsl:variable name="ifname"><xsl:value-of select="@name" /></xsl:variable>
   <xsl:text>
/**
 * Generated VBoxWebService Enum Collection
 */</xsl:text>
class <xsl:value-of select="$ifname"/>Collection extends VBox_EnumCollection
{
    protected $_interfaceName = "<xsl:value-of select="$ifname"/>";
}
</xsl:template>

<xsl:template name="comResultCodes">
    const <xsl:value-of select="@name"/> = <xsl:value-of select="@value"/>;
</xsl:template>

<xsl:template match="/">
<xsl:text>&lt;?php

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
 *
 * This file is part of a free software library; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License version 2.1 as published by the Free Software
 * Foundation and shipped in the "COPYING.LIB" file with this library.
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
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */
/*
 * This file is autogenerated from VirtualBox.xidl, DO NOT EDIT!
 */

class VBox_ManagedObject
{
    protected $connection;
    protected $handle;

    public function __construct($soap, $handle = null)
    {
        $this->connection = $soap;
        $this->handle = $handle;
    }

    public function __toString()
    {
        return (string)$this->handle;
    }

    public function __set($attr, $value)
    {
        $methodName = "set" . $attr;
        if (method_exists($this, $methodName))
            $this->$methodName($value);
        else
            throw new Exception("Attribute does not exist");
    }

    public function __get($attr)
    {
        $methodName = "get" . $attr;
        if (method_exists($this, $methodName))
            return $this->$methodName();
        else
            throw new Exception("Attribute does not exist");
    }

    public function getHandle()
    {
        return $this->handle;
    }

    public function cast($class)
    {
        if (is_subclass_of($class, 'VBox_ManagedObject'))
        {
            return new $class($this->connection, $this->handle);
        }
        throw new Exception('Cannot cast VBox_ManagedObject to non-child class VBox_ManagedObject');
    }

    public function releaseRemote()
    {
        try
        {
            $request = new stdClass();
            $request->_this = $this->handle;
            $this->connection->__soapCall('IManagedObjectRef_release', array((array)$request));
        }
        catch (Exception $ex)
        {
        }
    }
}

abstract class VBox_Collection implements ArrayAccess, Iterator, Countable
{
    protected $_connection;
    protected $_values;
    protected $_objects;
    protected $_interfaceName;

    public function __construct($soap, array $values = array())
    {
        $this->_connection = $soap;
        $this->_values = $values;
        $this->_soapToObject();
    }

    protected function _soapToObject()
    {
        $this->_objects = array();
        foreach($this->_values as $value)
        {
            $this->_objects[] = new $this->_interfaceName($this->_connection, $value);
        }
    }

    /** ArrayAccess Functions **/
    public function offsetSet($offset, $value)
    {
        if ($value instanceof $this->_interfaceName)
        {
            if ($offset)
            {
                $this->_objects[$offset] = $value;
            }
            else
            {
                $this->_objects[] = $value;
            }
        }
        else
        {
            throw new Exception("Value must be a instance of " . $this->_interfaceName);
        }
    }

    public function offsetExists($offset)
    {
        return isset($this->_objects[$offset]);
    }

    public function offsetUnset($offset)
    {
        unset($this->_objects[$offset]);
    }

    public function offsetGet($offset)
    {
        return isset($this->_objects[$offset]) ? $this->_objects[$offset] : null;
    }

    /** Iterator Functions **/
    public function rewind()
    {
        reset($this->_objects);
    }

    public function current()
    {
        return current($this->_objects);
    }

    public function key()
    {
        return key($this->_objects);
    }

    public function next()
    {
        return next($this->_objects);
    }

    public function valid()
    {
        return ($this->current() !== false);
    }

    /** Countable Functions **/
    public function count()
    {
        return count($this->_objects);
    }
}

class VBox_ManagedObjectCollection extends VBox_Collection
{
    protected $_interfaceName = 'VBox_ManagedObject';

    // Result is undefined if this is called AFTER any call to VBox_Collection::offsetSet or VBox_Collection::offsetUnset
    public function setInterfaceName($interface)
    {
        if (!is_subclass_of($interface, 'VBox_ManagedObject'))
        {
            throw new Exception('Cannot set collection interface to non-child class of VBox_ManagedObject');
        }
        $this->_interfaceName = $interface;
        $this->_soapToObject();
    }
}

abstract class VBox_Struct
{
    protected $connection;

    public function __get($attr)
    {
        $methodName = "get" . $attr;
        if (method_exists($this, $methodName))
            return $this->$methodName();
        else
            throw new Exception("Attribute does not exist");
    }
}

abstract class VBox_StructCollection extends VBox_Collection
{

    public function __construct($soap, array $values = array())
    {
        if (!(array_values($values) === $values))
        {
            $values = array((object)$values); //Fix for when struct return value only contains one list item (e.g. one medium attachment)
        }
        parent::__construct($soap, $values);
    }
}

abstract class VBox_Enum
{
    protected $_handle;

    public function __construct($connection, $handle)
    {
        if (is_string($handle))
            $this->_handle = $this->ValueMap[$handle];
        else
            $this->_handle = $handle;
    }

    public function __toString()
    {
        return (string)$this->NameMap[$this->_handle];
    }
}

abstract class VBox_EnumCollection extends VBox_Collection
{
}

</xsl:text>

<xsl:text>
/**
 * VirtualBox COM result codes
 */
class VirtualBox_COM_result_codes
{
</xsl:text>
  <xsl:for-each select="/idl/library/result">
       <xsl:call-template name="comResultCodes"/>
  </xsl:for-each>
<xsl:text>
}
</xsl:text>
  <xsl:for-each select="//interface[@wsmap='managed' or @wsmap='global']">
       <xsl:call-template name="interface"/>
       <xsl:call-template name="collection"/>
  </xsl:for-each>
  <xsl:for-each select="//interface[@wsmap='struct']">
       <xsl:call-template name="interfacestruct"/>
       <xsl:call-template name="structcollection"/>
  </xsl:for-each>
  <xsl:for-each select="//enum">
       <xsl:call-template name="enum"/>
       <xsl:call-template name="enumcollection"/>
  </xsl:for-each>

</xsl:template>

</xsl:stylesheet>
