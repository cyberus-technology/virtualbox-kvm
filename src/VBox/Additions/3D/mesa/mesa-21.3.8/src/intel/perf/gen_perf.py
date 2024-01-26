# Copyright (c) 2015-2017 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import argparse
import os
import sys
import textwrap

import xml.etree.ElementTree as et

hashed_funcs = {}

c_file = None
_c_indent = 0

def c(*args):
    code = ' '.join(map(str,args))
    for line in code.splitlines():
        text = ''.rjust(_c_indent) + line
        c_file.write(text.rstrip() + "\n")

# indented, but no trailing newline...
def c_line_start(code):
    c_file.write(''.rjust(_c_indent) + code)
def c_raw(code):
    c_file.write(code)

def c_indent(n):
    global _c_indent
    _c_indent = _c_indent + n
def c_outdent(n):
    global _c_indent
    _c_indent = _c_indent - n

header_file = None
_h_indent = 0

def h(*args):
    code = ' '.join(map(str,args))
    for line in code.splitlines():
        text = ''.rjust(_h_indent) + line
        header_file.write(text.rstrip() + "\n")

def h_indent(n):
    global _c_indent
    _h_indent = _h_indent + n
def h_outdent(n):
    global _c_indent
    _h_indent = _h_indent - n


def emit_fadd(tmp_id, args):
    c("double tmp{0} = {1} + {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

# Be careful to check for divide by zero...
def emit_fdiv(tmp_id, args):
    c("double tmp{0} = {1};".format(tmp_id, args[1]))
    c("double tmp{0} = {1};".format(tmp_id + 1, args[0]))
    c("double tmp{0} = tmp{1} ? tmp{2} / tmp{1} : 0;".format(tmp_id + 2, tmp_id + 1, tmp_id))
    return tmp_id + 3

def emit_fmax(tmp_id, args):
    c("double tmp{0} = {1};".format(tmp_id, args[1]))
    c("double tmp{0} = {1};".format(tmp_id + 1, args[0]))
    c("double tmp{0} = MAX(tmp{1}, tmp{2});".format(tmp_id + 2, tmp_id, tmp_id + 1))
    return tmp_id + 3

def emit_fmul(tmp_id, args):
    c("double tmp{0} = {1} * {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

def emit_fsub(tmp_id, args):
    c("double tmp{0} = {1} - {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

def emit_read(tmp_id, args):
    type = args[1].lower()
    c("uint64_t tmp{0} = results->accumulator[query->{1}_offset + {2}];".format(tmp_id, type, args[0]))
    return tmp_id + 1

def emit_uadd(tmp_id, args):
    c("uint64_t tmp{0} = {1} + {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

# Be careful to check for divide by zero...
def emit_udiv(tmp_id, args):
    c("uint64_t tmp{0} = {1};".format(tmp_id, args[1]))
    c("uint64_t tmp{0} = {1};".format(tmp_id + 1, args[0]))
    if args[0].isdigit():
        assert int(args[0]) > 0
        c("uint64_t tmp{0} = tmp{2} / tmp{1};".format(tmp_id + 2, tmp_id + 1, tmp_id))
    else:
        c("uint64_t tmp{0} = tmp{1} ? tmp{2} / tmp{1} : 0;".format(tmp_id + 2, tmp_id + 1, tmp_id))
    return tmp_id + 3

def emit_umul(tmp_id, args):
    c("uint64_t tmp{0} = {1} * {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

def emit_usub(tmp_id, args):
    c("uint64_t tmp{0} = {1} - {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

def emit_umin(tmp_id, args):
    c("uint64_t tmp{0} = MIN({1}, {2});".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

def emit_lshft(tmp_id, args):
    c("uint64_t tmp{0} = {1} << {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

def emit_rshft(tmp_id, args):
    c("uint64_t tmp{0} = {1} >> {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

def emit_and(tmp_id, args):
    c("uint64_t tmp{0} = {1} & {2};".format(tmp_id, args[1], args[0]))
    return tmp_id + 1

ops = {}
#             (n operands, emitter)
ops["FADD"] = (2, emit_fadd)
ops["FDIV"] = (2, emit_fdiv)
ops["FMAX"] = (2, emit_fmax)
ops["FMUL"] = (2, emit_fmul)
ops["FSUB"] = (2, emit_fsub)
ops["READ"] = (2, emit_read)
ops["UADD"] = (2, emit_uadd)
ops["UDIV"] = (2, emit_udiv)
ops["UMUL"] = (2, emit_umul)
ops["USUB"] = (2, emit_usub)
ops["UMIN"] = (2, emit_umin)
ops["<<"]   = (2, emit_lshft)
ops[">>"]   = (2, emit_rshft)
ops["AND"]  = (2, emit_and)

def brkt(subexp):
    if " " in subexp:
        return "(" + subexp + ")"
    else:
        return subexp

def splice_bitwise_and(args):
    return brkt(args[1]) + " & " + brkt(args[0])

def splice_logical_and(args):
    return brkt(args[1]) + " && " + brkt(args[0])

def splice_ult(args):
    return brkt(args[1]) + " < " + brkt(args[0])

def splice_ugte(args):
    return brkt(args[1]) + " >= " + brkt(args[0])

exp_ops = {}
#                 (n operands, splicer)
exp_ops["AND"]  = (2, splice_bitwise_and)
exp_ops["UGTE"] = (2, splice_ugte)
exp_ops["ULT"]  = (2, splice_ult)
exp_ops["&&"]   = (2, splice_logical_and)


hw_vars = {}
hw_vars["$EuCoresTotalCount"] = "perf->sys_vars.n_eus"
hw_vars["$EuSlicesTotalCount"] = "perf->sys_vars.n_eu_slices"
hw_vars["$EuSubslicesTotalCount"] = "perf->sys_vars.n_eu_sub_slices"
hw_vars["$EuThreadsCount"] = "perf->sys_vars.eu_threads_count"
hw_vars["$SliceMask"] = "perf->sys_vars.slice_mask"
# subslice_mask is interchangeable with subslice/dual-subslice since Gfx12+
# only has dual subslices which can be assimilated with 16EUs subslices.
hw_vars["$SubsliceMask"] = "perf->sys_vars.subslice_mask"
hw_vars["$DualSubsliceMask"] = "perf->sys_vars.subslice_mask"
hw_vars["$GpuTimestampFrequency"] = "perf->sys_vars.timestamp_frequency"
hw_vars["$GpuMinFrequency"] = "perf->sys_vars.gt_min_freq"
hw_vars["$GpuMaxFrequency"] = "perf->sys_vars.gt_max_freq"
hw_vars["$SkuRevisionId"] = "perf->sys_vars.revision"
hw_vars["$QueryMode"] = "perf->sys_vars.query_mode"

def output_rpn_equation_code(set, counter, equation):
    c("/* RPN equation: " + equation + " */")
    tokens = equation.split()
    stack = []
    tmp_id = 0
    tmp = None

    for token in tokens:
        stack.append(token)
        while stack and stack[-1] in ops:
            op = stack.pop()
            argc, callback = ops[op]
            args = []
            for i in range(0, argc):
                operand = stack.pop()
                if operand[0] == "$":
                    if operand in hw_vars:
                        operand = hw_vars[operand]
                    elif operand in set.counter_vars:
                        reference = set.counter_vars[operand]
                        operand = set.read_funcs[operand[1:]] + "(perf, query, results)"
                    else:
                        raise Exception("Failed to resolve variable " + operand + " in equation " + equation + " for " + set.name + " :: " + counter.get('name'));
                args.append(operand)

            tmp_id = callback(tmp_id, args)

            tmp = "tmp{0}".format(tmp_id - 1)
            stack.append(tmp)

    if len(stack) != 1:
        raise Exception("Spurious empty rpn code for " + set.name + " :: " +
                counter.get('name') + ".\nThis is probably due to some unhandled RPN function, in the equation \"" +
                equation + "\"")

    value = stack[-1]

    if value in hw_vars:
        value = hw_vars[value]
    if value in set.counter_vars:
        value = set.read_funcs[value[1:]] + "(perf, query, results)"

    c("\nreturn " + value + ";")

def splice_rpn_expression(set, counter, expression):
    tokens = expression.split()
    stack = []

    for token in tokens:
        stack.append(token)
        while stack and stack[-1] in exp_ops:
            op = stack.pop()
            argc, callback = exp_ops[op]
            args = []
            for i in range(0, argc):
                operand = stack.pop()
                if operand[0] == "$":
                    if operand in hw_vars:
                        operand = hw_vars[operand]
                    else:
                        raise Exception("Failed to resolve variable " + operand + " in expression " + expression + " for " + set.name + " :: " + counter.get('name'));
                args.append(operand)

            subexp = callback(args)

            stack.append(subexp)

    if len(stack) != 1:
        raise Exception("Spurious empty rpn expression for " + set.name + " :: " +
                counter.get('name') + ".\nThis is probably due to some unhandled RPN operation, in the expression \"" +
                expression + "\"")

    return stack[-1]

def output_counter_read(gen, set, counter):
    c("\n")
    c("/* {0} :: {1} */".format(set.name, counter.get('name')))

    if counter.read_hash in hashed_funcs:
        c("#define %s \\" % counter.read_sym)
        c_indent(3)
        c("%s" % hashed_funcs[counter.read_hash])
        c_outdent(3)
    else:
        ret_type = counter.get('data_type')
        if ret_type == "uint64":
            ret_type = "uint64_t"

        read_eq = counter.get('equation')

        c("static " + ret_type)
        c(counter.read_sym + "(UNUSED struct intel_perf_config *perf,\n")
        c_indent(len(counter.read_sym) + 1)
        c("const struct intel_perf_query_info *query,\n")
        c("const struct intel_perf_query_result *results)\n")
        c_outdent(len(counter.read_sym) + 1)

        c("{")
        c_indent(3)
        output_rpn_equation_code(set, counter, read_eq)
        c_outdent(3)
        c("}")

        hashed_funcs[counter.read_hash] = counter.read_sym


def output_counter_max(gen, set, counter):
    max_eq = counter.get('max_equation')

    if not counter.has_max_func():
        return

    c("\n")
    c("/* {0} :: {1} */".format(set.name, counter.get('name')))

    if counter.max_hash in hashed_funcs:
        c("#define %s \\" % counter.max_sym())
        c_indent(3)
        c("%s" % hashed_funcs[counter.max_hash])
        c_outdent(3)
    else:
        ret_type = counter.get('data_type')
        if ret_type == "uint64":
            ret_type = "uint64_t"

        c("static " + ret_type)
        c(counter.max_sym() + "(struct intel_perf_config *perf)\n")
        c("{")
        c_indent(3)
        output_rpn_equation_code(set, counter, max_eq)
        c_outdent(3)
        c("}")

        hashed_funcs[counter.max_hash] = counter.max_sym()


c_type_sizes = { "uint32_t": 4, "uint64_t": 8, "float": 4, "double": 8, "bool": 4 }
def sizeof(c_type):
    return c_type_sizes[c_type]

def pot_align(base, pot_alignment):
    return (base + pot_alignment - 1) & ~(pot_alignment - 1);

semantic_type_map = {
    "duration": "raw",
    "ratio": "event"
    }

def output_availability(set, availability, counter_name):
    expression = splice_rpn_expression(set, counter_name, availability)
    lines = expression.split(' && ')
    n_lines = len(lines)
    if n_lines == 1:
        c("if (" + lines[0] + ") {")
    else:
        c("if (" + lines[0] + " &&")
        c_indent(4)
        for i in range(1, (n_lines - 1)):
            c(lines[i] + " &&")
        c(lines[(n_lines - 1)] + ") {")
        c_outdent(4)


def output_units(unit):
    return unit.replace(' ', '_').upper()


# should a unit be visible in description?
units_map = {
    "bytes" : True,
    "cycles" : True,
    "eu atomic requests to l3 cache lines" : False,
    "eu bytes per l3 cache line" : False,
    "eu requests to l3 cache lines" : False,
    "eu sends to l3 cache lines" : False,
    "events" : True,
    "hz" : True,
    "messages" : True,
    "ns" : True,
    "number" : False,
    "percent" : True,
    "pixels" : True,
    "texels" : True,
    "threads" : True,
    "us" : True,
    "utilization" : False,
    }


def desc_units(unit):
    val = units_map.get(unit)
    if val is None:
        raise Exception("Unknown unit: " + unit)
    if val == False:
        return ""
    if unit == 'hz':
        unit = 'Hz'
    return " Unit: " + unit + "."


def output_counter_report(set, counter, current_offset):
    data_type = counter.get('data_type')
    data_type_uc = data_type.upper()
    c_type = data_type

    if "uint" in c_type:
        c_type = c_type + "_t"

    semantic_type = counter.get('semantic_type')
    if semantic_type in semantic_type_map:
        semantic_type = semantic_type_map[semantic_type]

    semantic_type_uc = semantic_type.upper()

    c("\n")

    availability = counter.get('availability')
    if availability:
        output_availability(set, availability, counter.get('name'))
        c_indent(3)

    c("counter = &query->counters[query->n_counters++];\n")
    c("counter->oa_counter_read_" + data_type + " = " + set.read_funcs[counter.get('symbol_name')] + ";\n")
    c("counter->name = \"" + counter.get('name') + "\";\n")
    c("counter->desc = \"" + counter.get('description') + desc_units(counter.get('units')) + "\";\n")
    c("counter->symbol_name = \"" + counter.get('symbol_name') + "\";\n")
    c("counter->category = \"" + counter.get('mdapi_group') + "\";\n")
    c("counter->type = INTEL_PERF_COUNTER_TYPE_" + semantic_type_uc + ";\n")
    c("counter->data_type = INTEL_PERF_COUNTER_DATA_TYPE_" + data_type_uc + ";\n")
    c("counter->units = INTEL_PERF_COUNTER_UNITS_" + output_units(counter.get('units')) + ";\n")
    c("counter->raw_max = " + set.max_values[counter.get('symbol_name')] + ";\n")

    current_offset = pot_align(current_offset, sizeof(c_type))
    c("counter->offset = " + str(current_offset) + ";\n")

    if availability:
        c_outdent(3);
        c("}")

    return current_offset + sizeof(c_type)


register_types = {
    'FLEX': 'flex_regs',
    'NOA': 'mux_regs',
    'OA': 'b_counter_regs',
}

def compute_register_lengths(set):
    register_lengths = {}
    register_configs = set.findall('register_config')
    for register_config in register_configs:
        t = register_types[register_config.get('type')]
        if t not in register_lengths:
            register_lengths[t] = len(register_config.findall('register'))
        else:
            register_lengths[t] += len(register_config.findall('register'))

    return register_lengths


def generate_register_configs(set):
    register_configs = set.findall('register_config')

    for register_config in register_configs:
        t = register_types[register_config.get('type')]

        availability = register_config.get('availability')
        if availability:
            output_availability(set, availability, register_config.get('type') + ' register config')
            c_indent(3)

        registers = register_config.findall('register')
        c("static const struct intel_perf_query_register_prog %s[] = {" % t)
        c_indent(3)
        for register in registers:
            c("{ .reg = %s, .val = %s }," % (register.get('address'), register.get('value')))
        c_outdent(3)
        c("};")
        c("query->config.%s = %s;" % (t, t))
        c("query->config.n_%s = ARRAY_SIZE(%s);" % (t, t))

        if availability:
            c_outdent(3)
            c("}")
        c("\n")


# Wraps a <counter> element from the oa-*.xml files.
class Counter:
    def __init__(self, set, xml):
        self.xml = xml
        self.set = set
        self.read_hash = None
        self.max_hash = None

        self.read_sym = "{0}__{1}__{2}__read".format(self.set.gen.chipset,
                                                     self.set.underscore_name,
                                                     self.xml.get('underscore_name'))

    def get(self, prop):
        return self.xml.get(prop)

    # Compute the hash of a counter's equation by expanding (including all the
    # sub-equations it depends on)
    def compute_hashes(self):
        if self.read_hash is not None:
            return

        def replace_token(token):
            if token[0] != "$":
                return token
            if token not in self.set.counter_vars:
                return token
            self.set.counter_vars[token].compute_hashes()
            return self.set.counter_vars[token].read_hash

        read_eq = self.xml.get('equation')
        self.read_hash = ' '.join(map(replace_token, read_eq.split()))

        max_eq = self.xml.get('max_equation')
        if max_eq:
            self.max_hash = ' '.join(map(replace_token, max_eq.split()))

    def has_max_func(self):
        max_eq = self.xml.get('max_equation')
        if not max_eq:
            return False

        try:
            val = float(max_eq)
            return False
        except ValueError:
            pass

        for token in max_eq.split():
            if token[0] == '$' and token not in hw_vars:
                return False
        return True

    def max_sym(self):
        assert self.has_max_func()
        return "{0}__{1}__{2}__max".format(self.set.gen.chipset,
                                           self.set.underscore_name,
                                           self.xml.get('underscore_name'))

    def max_value(self):
        max_eq = self.xml.get('max_equation')
        if not max_eq:
            return "0 /* undefined */"

        try:
            return "{0}".format(float(max_eq))
        except ValueError:
            pass

        for token in max_eq.split():
            if token[0] == '$' and token not in hw_vars:
                return "0 /* unsupported (varies over time) */"

        return "{0}__{1}__{2}__max(perf)".format(self.set.gen.chipset,
                                                 self.set.underscore_name,
                                                 self.xml.get('underscore_name'))

# Wraps a <set> element from the oa-*.xml files.
class Set:
    def __init__(self, gen, xml):
        self.gen = gen
        self.xml = xml

        self.counter_vars = {}
        self.max_values = {}
        self.read_funcs = {}

        xml_counters = self.xml.findall("counter")
        self.counters = []
        for xml_counter in xml_counters:
            counter = Counter(self, xml_counter)
            self.counters.append(counter)
            self.counter_vars["$" + counter.get('symbol_name')] = counter
            self.read_funcs[counter.get('symbol_name')] = counter.read_sym
            self.max_values[counter.get('symbol_name')] = counter.max_value()

        for counter in self.counters:
            counter.compute_hashes()

    @property
    def hw_config_guid(self):
        return self.xml.get('hw_config_guid')

    @property
    def name(self):
        return self.xml.get('name')

    @property
    def symbol_name(self):
        return self.xml.get('symbol_name')

    @property
    def underscore_name(self):
        return self.xml.get('underscore_name')

    def findall(self, path):
        return self.xml.findall(path)

    def find(self, path):
        return self.xml.find(path)


# Wraps an entire oa-*.xml file.
class Gen:
    def __init__(self, filename):
        self.filename = filename
        self.xml = et.parse(self.filename)
        self.chipset = self.xml.find('.//set').get('chipset').lower()
        self.sets = []

        for xml_set in self.xml.findall(".//set"):
            self.sets.append(Set(self, xml_set))


def main():
    global c_file
    global header_file

    parser = argparse.ArgumentParser()
    parser.add_argument("--header", help="Header file to write", required=True)
    parser.add_argument("--code", help="C file to write", required=True)
    parser.add_argument("xml_files", nargs='+', help="List of xml metrics files to process")

    args = parser.parse_args()

    c_file = open(args.code, 'w')
    header_file = open(args.header, 'w')

    gens = []
    for xml_file in args.xml_files:
        gens.append(Gen(xml_file))


    copyright = textwrap.dedent("""\
        /* Autogenerated file, DO NOT EDIT manually! generated by {}
         *
         * Copyright (c) 2015 Intel Corporation
         *
         * Permission is hereby granted, free of charge, to any person obtaining a
         * copy of this software and associated documentation files (the "Software"),
         * to deal in the Software without restriction, including without limitation
         * the rights to use, copy, modify, merge, publish, distribute, sublicense,
         * and/or sell copies of the Software, and to permit persons to whom the
         * Software is furnished to do so, subject to the following conditions:
         *
         * The above copyright notice and this permission notice (including the next
         * paragraph) shall be included in all copies or substantial portions of the
         * Software.
         *
         * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
         * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
         * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
         * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
         * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
         * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
         * DEALINGS IN THE SOFTWARE.
         */

        """).format(os.path.basename(__file__))

    h(copyright)
    h(textwrap.dedent("""\
        #pragma once

        struct intel_perf_config;

        """))

    c(copyright)
    c(textwrap.dedent("""\
        #include <stdint.h>
        #include <stdbool.h>

        #include <drm-uapi/i915_drm.h>

        #include "util/hash_table.h"
        #include "util/ralloc.h"

        """))

    c("#include \"" + os.path.basename(args.header) + "\"")

    c(textwrap.dedent("""\
        #include "perf/intel_perf.h"


        #define MIN(a, b) ((a < b) ? (a) : (b))
        #define MAX(a, b) ((a > b) ? (a) : (b))


        """))

    # Print out all equation functions.
    for gen in gens:
        for set in gen.sets:
            for counter in set.counters:
                output_counter_read(gen, set, counter)
                output_counter_max(gen, set, counter)

    # Print out all metric sets registration functions for each set in each
    # generation.
    for gen in gens:
        for set in gen.sets:
            counters = set.counters

            c("\n")
            c("\nstatic void\n")
            c("{0}_register_{1}_counter_query(struct intel_perf_config *perf)\n".format(gen.chipset, set.underscore_name))
            c("{\n")
            c_indent(3)

            c("struct intel_perf_query_info *query = rzalloc(perf, struct intel_perf_query_info);\n")
            c("\n")
            c("query->perf = perf;\n")
            c("query->kind = INTEL_PERF_QUERY_TYPE_OA;\n")
            c("query->name = \"" + set.name + "\";\n")
            c("query->symbol_name = \"" + set.symbol_name + "\";\n")
            c("query->guid = \"" + set.hw_config_guid + "\";\n")

            c("query->counters = rzalloc_array(query, struct intel_perf_query_counter, %u);" % len(counters))
            c("query->n_counters = 0;")
            c("query->oa_metrics_set_id = 0; /* determined at runtime, via sysfs */")

            if gen.chipset == "hsw":
                c(textwrap.dedent("""\
                    query->oa_format = I915_OA_FORMAT_A45_B8_C8;
                    /* Accumulation buffer offsets... */
                    query->gpu_time_offset = 0;
                    query->a_offset = query->gpu_time_offset + 1;
                    query->b_offset = query->a_offset + 45;
                    query->c_offset = query->b_offset + 8;
                    query->perfcnt_offset = query->c_offset + 8;
                    query->rpstat_offset = query->perfcnt_offset + 2;
                """))
            else:
                c(textwrap.dedent("""\
                    query->oa_format = I915_OA_FORMAT_A32u40_A4u32_B8_C8;
                    /* Accumulation buffer offsets... */
                    query->gpu_time_offset = 0;
                    query->gpu_clock_offset = query->gpu_time_offset + 1;
                    query->a_offset = query->gpu_clock_offset + 1;
                    query->b_offset = query->a_offset + 36;
                    query->c_offset = query->b_offset + 8;
                    query->perfcnt_offset = query->c_offset + 8;
                    query->rpstat_offset = query->perfcnt_offset + 2;
                """))


            c("\n")
            c("struct intel_perf_query_counter *counter = query->counters;\n")

            c("\n")
            c("/* Note: we're assuming there can't be any variation in the definition ")
            c(" * of a query between contexts so it's ok to describe a query within a ")
            c(" * global variable which only needs to be initialized once... */")
            c("\nif (!query->data_size) {")
            c_indent(3)

            generate_register_configs(set)

            offset = 0
            for counter in counters:
                offset = output_counter_report(set, counter, offset)


            c("\nquery->data_size = counter->offset + intel_perf_query_counter_get_size(counter);\n")

            c_outdent(3)
            c("}");

            c("\n_mesa_hash_table_insert(perf->oa_metrics_table, query->guid, query);")

            c_outdent(3)
            c("}\n")

        h("void intel_oa_register_queries_" + gen.chipset + "(struct intel_perf_config *perf);\n")

        c("\nvoid")
        c("intel_oa_register_queries_" + gen.chipset + "(struct intel_perf_config *perf)")
        c("{")
        c_indent(3)

        for set in gen.sets:
            c("{0}_register_{1}_counter_query(perf);".format(gen.chipset, set.underscore_name))

        c_outdent(3)
        c("}")


if __name__ == '__main__':
    main()
