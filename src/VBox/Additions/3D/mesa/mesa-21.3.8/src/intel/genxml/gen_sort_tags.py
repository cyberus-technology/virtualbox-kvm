#!/usr/bin/env python3
# Copyright © 2019 Intel Corporation
# SPDX-License-Identifier: MIT

from collections import OrderedDict
import os
import pathlib
import re
import xml.etree.ElementTree as et

def get_filename(element):
    return element.attrib['filename']

def get_name(element):
    return element.attrib['name']

def get_value(element):
    return int(element.attrib['value'], 0)

def get_start(element):
    return int(element.attrib['start'], 0)


base_types = [
    'address',
    'offset',
    'int',
    'uint',
    'bool',
    'float',
]

ufixed_pattern = re.compile(r"u(\d+)\.(\d+)")
sfixed_pattern = re.compile(r"s(\d+)\.(\d+)")

def is_base_type(name):
    return name in base_types or sfixed_pattern.match(name) or ufixed_pattern.match(name)

def add_struct_refs(items, node):
    if node.tag == 'field':
        if 'type' in node.attrib and not is_base_type(node.attrib['type']):
            t = node.attrib['type']
            items[t] = True
        return
    if node.tag != 'struct' and node.tag != 'group':
        return
    for c in node:
        add_struct_refs(items, c)


class Struct(object):
    def __init__(self, xml):
        self.xml = xml
        self.name = xml.attrib['name']
        self.deps = OrderedDict()

    def find_deps(self, struct_dict, enum_dict):
        deps = OrderedDict()
        add_struct_refs(deps, self.xml)
        for d in deps.keys():
            if d in struct_dict:
                self.deps[d] = struct_dict[d]
            else:
                assert(d in enum_dict)

    def add_xml(self, items):
        for d in self.deps.values():
            d.add_xml(items)
        items[self.name] = self.xml


# ordering of the various tag attributes
genxml_desc = {
    'genxml'      : [ 'name', 'gen', ],
    'enum'        : [ 'name', 'value', 'prefix', ],
    'struct'      : [ 'name', 'length', ],
    'field'       : [ 'name', 'start', 'end', 'type', 'default', 'prefix', ],
    'instruction' : [ 'name', 'bias', 'length', 'engine', ],
    'value'       : [ 'name', 'value', ],
    'group'       : [ 'count', 'start', 'size', ],
    'register'    : [ 'name', 'length', 'num', ],
}

space_delta = 2

def print_node(f, offset, node):
    if node.tag in [ 'enum', 'struct', 'instruction', 'register' ]:
        f.write('\n')
    spaces = ''.rjust(offset * space_delta)
    f.write('{0}<{1}'.format(spaces, node.tag))
    attribs = genxml_desc[node.tag]
    for a in node.attrib:
        assert(a in attribs)
    for a in attribs:
        if a in node.attrib:
            f.write(' {0}="{1}"'.format(a, node.attrib[a]))
    children = list(node)
    if len(children) > 0:
        f.write('>\n')
        for c in children:
            print_node(f, offset + 1, c)
        f.write('{0}</{1}>\n'.format(spaces, node.tag))
    else:
        f.write('/>\n')


def process(filename):
    xml = et.parse(filename)
    genxml = xml.getroot()

    enums = sorted(genxml.findall('enum'), key=get_name)
    enum_dict = {}
    for e in enums:
        values = e.findall('./value')
        e[:] = sorted(e, key=get_value)
        enum_dict[e.attrib['name']] = e

    # Structs are a bit annoying because they can refer to each other. We sort
    # them alphabetically and then build a graph of depedencies. Finally we go
    # through the alphabetically sorted list and print out dependencies first.
    structs = sorted(xml.findall('./struct'), key=get_name)
    wrapped_struct_dict = {}
    for s in structs:
        s[:] = sorted(s, key=get_start)
        ws = Struct(s)
        wrapped_struct_dict[ws.name] = ws

    for s in wrapped_struct_dict:
        wrapped_struct_dict[s].find_deps(wrapped_struct_dict, enum_dict)

    sorted_structs = OrderedDict()
    for _s in structs:
        s = wrapped_struct_dict[_s.attrib['name']]
        s.add_xml(sorted_structs)

    instructions = sorted(xml.findall('./instruction'), key=get_name)
    for i in instructions:
        i[:] = sorted(i, key=get_start)

    registers = sorted(xml.findall('./register'), key=get_name)
    for r in registers:
        r[:] = sorted(r, key=get_start)

    genxml[:] = enums + list(sorted_structs.values()) + instructions + registers

    with open(filename, 'w') as f:
        f.write('<?xml version="1.0" ?>\n')
        print_node(f, 0, genxml)


if __name__ == '__main__':
    folder = pathlib.Path('.')
    for f in folder.glob('*.xml'):
        print('Processing {}... '.format(f), end='', flush=True)
        process(f)
        print('done.')
