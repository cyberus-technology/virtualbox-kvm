# Copyright © 2017 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

gen_xml_files = [
  'gen4.xml',
  'gen45.xml',
  'gen5.xml',
  'gen6.xml',
  'gen7.xml',
  'gen75.xml',
  'gen8.xml',
  'gen9.xml',
  'gen11.xml',
  'gen12.xml',
  'gen125.xml',
]

genX_xml_h = custom_target(
  'genX_xml.h',
  input : ['gen_zipped_file.py', gen_xml_files],
  output : 'genX_xml.h',
  command : [prog_python, '@INPUT@'],
  capture : true,
)

genX_bits_h = custom_target(
  'genX_bits.h',
  input : ['gen_bits_header.py', gen_xml_files],
  output : 'genX_bits.h',
  command : [prog_python, '@INPUT@', '-o', '@OUTPUT@'],
)

gen_xml_pack = []
foreach f : gen_xml_files + ['gen_rt.xml']
  _name = '@0@_pack.h'.format(f.split('.')[0])
  gen_xml_pack += custom_target(
    _name,
    input : ['gen_pack_header.py', f],
    output : _name,
    command : [prog_python, '@INPUT@'],
    capture : true,
  )
endforeach

gen_pack_header_py = files('gen_pack_header.py')

idep_genxml = declare_dependency(sources : [gen_xml_pack, genX_bits_h, genX_xml_h])
