/** @file
 * IPRT - DWARF constants.
 *
 * @note dwarf.mac is generated from this file by running 'kmk incs' in the root.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_formats_dwarf_h
#define IPRT_INCLUDED_formats_dwarf_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/** @name Standard DWARF Line Number Opcodes
 * @{ */
#define DW_LNS_extended                    UINT8_C(0x00)
#define DW_LNS_copy                        UINT8_C(0x01)
#define DW_LNS_advance_pc                  UINT8_C(0x02)
#define DW_LNS_advance_line                UINT8_C(0x03)
#define DW_LNS_set_file                    UINT8_C(0x04)
#define DW_LNS_set_column                  UINT8_C(0x05)
#define DW_LNS_negate_stmt                 UINT8_C(0x06)
#define DW_LNS_set_basic_block             UINT8_C(0x07)
#define DW_LNS_const_add_pc                UINT8_C(0x08)
#define DW_LNS_fixed_advance_pc            UINT8_C(0x09)
#define DW_LNS_set_prologue_end            UINT8_C(0x0a)
#define DW_LNS_set_epilogue_begin          UINT8_C(0x0b)
#define DW_LNS_set_isa                     UINT8_C(0x0c)
#define DW_LNS_what_question_mark          UINT8_C(0x0d)
/** @} */


/** @name Extended DWARF Line Number Opcodes
 * @{ */
#define DW_LNE_end_sequence                 UINT8_C(1)
#define DW_LNE_set_address                  UINT8_C(2)
#define DW_LNE_define_file                  UINT8_C(3)
#define DW_LNE_set_descriminator            UINT8_C(4)
/** @} */


/** @name DIE Tags.
 * @{ */
#define DW_TAG_array_type                   UINT16_C(0x0001)
#define DW_TAG_class_type                   UINT16_C(0x0002)
#define DW_TAG_entry_point                  UINT16_C(0x0003)
#define DW_TAG_enumeration_type             UINT16_C(0x0004)
#define DW_TAG_formal_parameter             UINT16_C(0x0005)
#define DW_TAG_imported_declaration         UINT16_C(0x0008)
#define DW_TAG_label                        UINT16_C(0x000a)
#define DW_TAG_lexical_block                UINT16_C(0x000b)
#define DW_TAG_member                       UINT16_C(0x000d)
#define DW_TAG_pointer_type                 UINT16_C(0x000f)
#define DW_TAG_reference_type               UINT16_C(0x0010)
#define DW_TAG_compile_unit                 UINT16_C(0x0011)
#define DW_TAG_string_type                  UINT16_C(0x0012)
#define DW_TAG_structure_type               UINT16_C(0x0013)
#define DW_TAG_subroutine_type              UINT16_C(0x0015)
#define DW_TAG_typedef                      UINT16_C(0x0016)
#define DW_TAG_union_type                   UINT16_C(0x0017)
#define DW_TAG_unspecified_parameters       UINT16_C(0x0018)
#define DW_TAG_variant                      UINT16_C(0x0019)
#define DW_TAG_common_block                 UINT16_C(0x001a)
#define DW_TAG_common_inclusion             UINT16_C(0x001b)
#define DW_TAG_inheritance                  UINT16_C(0x001c)
#define DW_TAG_inlined_subroutine           UINT16_C(0x001d)
#define DW_TAG_module                       UINT16_C(0x001e)
#define DW_TAG_ptr_to_member_type           UINT16_C(0x001f)
#define DW_TAG_set_type                     UINT16_C(0x0020)
#define DW_TAG_subrange_type                UINT16_C(0x0021)
#define DW_TAG_with_stmt                    UINT16_C(0x0022)
#define DW_TAG_access_declaration           UINT16_C(0x0023)
#define DW_TAG_base_type                    UINT16_C(0x0024)
#define DW_TAG_catch_block                  UINT16_C(0x0025)
#define DW_TAG_const_type                   UINT16_C(0x0026)
#define DW_TAG_constant                     UINT16_C(0x0027)
#define DW_TAG_enumerator                   UINT16_C(0x0028)
#define DW_TAG_file_type                    UINT16_C(0x0029)
#define DW_TAG_friend                       UINT16_C(0x002a)
#define DW_TAG_namelist                     UINT16_C(0x002b)
#define DW_TAG_namelist_item                UINT16_C(0x002c)
#define DW_TAG_packed_type                  UINT16_C(0x002d)
#define DW_TAG_subprogram                   UINT16_C(0x002e)
#define DW_TAG_template_type_parameter      UINT16_C(0x002f)
#define DW_TAG_template_value_parameter     UINT16_C(0x0030)
#define DW_TAG_thrown_type                  UINT16_C(0x0031)
#define DW_TAG_try_block                    UINT16_C(0x0032)
#define DW_TAG_variant_part                 UINT16_C(0x0033)
#define DW_TAG_variable                     UINT16_C(0x0034)
#define DW_TAG_volatile_type                UINT16_C(0x0035)
#define DW_TAG_dwarf_procedure              UINT16_C(0x0036)
#define DW_TAG_restrict_type                UINT16_C(0x0037)
#define DW_TAG_interface_type               UINT16_C(0x0038)
#define DW_TAG_namespace                    UINT16_C(0x0039)
#define DW_TAG_imported_module              UINT16_C(0x003a)
#define DW_TAG_unspecified_type             UINT16_C(0x003b)
#define DW_TAG_partial_unit                 UINT16_C(0x003c)
#define DW_TAG_imported_unit                UINT16_C(0x003d)
#define DW_TAG_condition                    UINT16_C(0x003f)
#define DW_TAG_shared_type                  UINT16_C(0x0040)
#define DW_TAG_type_unit                    UINT16_C(0x0041)
#define DW_TAG_rvalue_reference_type        UINT16_C(0x0042)
#define DW_TAG_template_alias               UINT16_C(0x0043)
#define DW_TAG_lo_user                      UINT16_C(0x4080)
#define DW_TAG_GNU_call_site                UINT16_C(0x4109)
#define DW_TAG_GNU_call_site_parameter      UINT16_C(0x410a)
#define DW_TAG_WATCOM_address_class_type    UINT16_C(0x4100) /**< Watcom extension. */
#define DW_TAG_WATCOM_namespace             UINT16_C(0x4101) /**< Watcom extension. */
#define DW_TAG_hi_user                      UINT16_C(0xffff)
/** @} */


/** @name Has children or not (follows DW_TAG_xxx in .debug_abbrev).
 * @{ */
#define DW_CHILDREN_yes 1
#define DW_CHILDREN_no  0
/** @} */


/** @name DIE Attributes.
 * @{ */
#define DW_AT_sibling                       UINT16_C(0x0001)
#define DW_AT_location                      UINT16_C(0x0002)
#define DW_AT_name                          UINT16_C(0x0003)
#define DW_AT_ordering                      UINT16_C(0x0009)
#define DW_AT_byte_size                     UINT16_C(0x000b)
#define DW_AT_bit_offset                    UINT16_C(0x000c)
#define DW_AT_bit_size                      UINT16_C(0x000d)
#define DW_AT_stmt_list                     UINT16_C(0x0010)
#define DW_AT_low_pc                        UINT16_C(0x0011)
#define DW_AT_high_pc                       UINT16_C(0x0012)
#define DW_AT_language                      UINT16_C(0x0013)
#define DW_AT_discr                         UINT16_C(0x0015)
#define DW_AT_discr_value                   UINT16_C(0x0016)
#define DW_AT_visibility                    UINT16_C(0x0017)
#define DW_AT_import                        UINT16_C(0x0018)
#define DW_AT_string_length                 UINT16_C(0x0019)
#define DW_AT_common_reference              UINT16_C(0x001a)
#define DW_AT_comp_dir                      UINT16_C(0x001b)
#define DW_AT_const_value                   UINT16_C(0x001c)
#define DW_AT_containing_type               UINT16_C(0x001d)
#define DW_AT_default_value                 UINT16_C(0x001e)
#define DW_AT_inline                        UINT16_C(0x0020)
#define DW_AT_is_optional                   UINT16_C(0x0021)
#define DW_AT_lower_bound                   UINT16_C(0x0022)
#define DW_AT_producer                      UINT16_C(0x0025)
#define DW_AT_prototyped                    UINT16_C(0x0027)
#define DW_AT_return_addr                   UINT16_C(0x002a)
#define DW_AT_start_scope                   UINT16_C(0x002c)
#define DW_AT_bit_stride                    UINT16_C(0x002e)
#define DW_AT_upper_bound                   UINT16_C(0x002f)
#define DW_AT_abstract_origin               UINT16_C(0x0031)
#define DW_AT_accessibility                 UINT16_C(0x0032)
#define DW_AT_address_class                 UINT16_C(0x0033)
#define DW_AT_artificial                    UINT16_C(0x0034)
#define DW_AT_base_types                    UINT16_C(0x0035)
#define DW_AT_calling_convention            UINT16_C(0x0036)
#define DW_AT_count                         UINT16_C(0x0037)
#define DW_AT_data_member_location          UINT16_C(0x0038)
#define DW_AT_decl_column                   UINT16_C(0x0039)
#define DW_AT_decl_file                     UINT16_C(0x003a)
#define DW_AT_decl_line                     UINT16_C(0x003b)
#define DW_AT_declaration                   UINT16_C(0x003c)
#define DW_AT_discr_list                    UINT16_C(0x003d)
#define DW_AT_encoding                      UINT16_C(0x003e)
#define DW_AT_external                      UINT16_C(0x003f)
#define DW_AT_frame_base                    UINT16_C(0x0040)
#define DW_AT_friend                        UINT16_C(0x0041)
#define DW_AT_identifier_case               UINT16_C(0x0042)
#define DW_AT_macro_info                    UINT16_C(0x0043)
#define DW_AT_namelist_item                 UINT16_C(0x0044)
#define DW_AT_priority                      UINT16_C(0x0045)
#define DW_AT_segment                       UINT16_C(0x0046)
#define DW_AT_specification                 UINT16_C(0x0047)
#define DW_AT_static_link                   UINT16_C(0x0048)
#define DW_AT_type                          UINT16_C(0x0049)
#define DW_AT_use_location                  UINT16_C(0x004a)
#define DW_AT_variable_parameter            UINT16_C(0x004b)
#define DW_AT_virtuality                    UINT16_C(0x004c)
#define DW_AT_vtable_elem_location          UINT16_C(0x004d)
#define DW_AT_allocated                     UINT16_C(0x004e)
#define DW_AT_associated                    UINT16_C(0x004f)
#define DW_AT_data_location                 UINT16_C(0x0050)
#define DW_AT_byte_stride                   UINT16_C(0x0051)
#define DW_AT_entry_pc                      UINT16_C(0x0052)
#define DW_AT_use_UTF8                      UINT16_C(0x0053)
#define DW_AT_extension                     UINT16_C(0x0054)
#define DW_AT_ranges                        UINT16_C(0x0055)
#define DW_AT_trampoline                    UINT16_C(0x0056)
#define DW_AT_call_column                   UINT16_C(0x0057)
#define DW_AT_call_file                     UINT16_C(0x0058)
#define DW_AT_call_line                     UINT16_C(0x0059)
#define DW_AT_description                   UINT16_C(0x005a)
#define DW_AT_binary_scale                  UINT16_C(0x005b)
#define DW_AT_decimal_scale                 UINT16_C(0x005c)
#define DW_AT_small                         UINT16_C(0x005d)
#define DW_AT_decimal_sign                  UINT16_C(0x005e)
#define DW_AT_digit_count                   UINT16_C(0x005f)
#define DW_AT_picture_string                UINT16_C(0x0060)
#define DW_AT_mutable                       UINT16_C(0x0061)
#define DW_AT_threads_scaled                UINT16_C(0x0062)
#define DW_AT_explicit                      UINT16_C(0x0063)
#define DW_AT_object_pointer                UINT16_C(0x0064)
#define DW_AT_endianity                     UINT16_C(0x0065)
#define DW_AT_elemental                     UINT16_C(0x0066)
#define DW_AT_pure                          UINT16_C(0x0067)
#define DW_AT_recursive                     UINT16_C(0x0068)
#define DW_AT_signature                     UINT16_C(0x0069)
#define DW_AT_main_subprogram               UINT16_C(0x006a)
#define DW_AT_data_bit_offset               UINT16_C(0x006b)
#define DW_AT_const_expr                    UINT16_C(0x006c)
#define DW_AT_enum_class                    UINT16_C(0x006d)
#define DW_AT_linkage_name                  UINT16_C(0x006e)
#define DW_AT_lo_user                       UINT16_C(0x2000)
/** Used by GCC and others, same as DW_AT_linkage_name. See http://wiki.dwarfstd.org/index.php?title=DW_AT_linkage_name*/
#define DW_AT_MIPS_linkage_name             UINT16_C(0x2007)
#define DW_AT_WATCOM_memory_model           UINT16_C(0x2082) /**< Watcom extension. */
#define DW_AT_WATCOM_references_start       UINT16_C(0x2083) /**< Watcom extension. */
#define DW_AT_WATCOM_parm_entry             UINT16_C(0x2084) /**< Watcom extension. */
#define DW_AT_hi_user                       UINT16_C(0x3fff)
/** @} */

/** @name DIE Forms.
 * @{ */
#define DW_FORM_addr                        UINT16_C(0x01)
/* 0x02 was FORM_REF in DWARF v1, obsolete now. */
#define DW_FORM_block2                      UINT16_C(0x03)
#define DW_FORM_block4                      UINT16_C(0x04)
#define DW_FORM_data2                       UINT16_C(0x05)
#define DW_FORM_data4                       UINT16_C(0x06)
#define DW_FORM_data8                       UINT16_C(0x07)
#define DW_FORM_string                      UINT16_C(0x08)
#define DW_FORM_block                       UINT16_C(0x09)
#define DW_FORM_block1                      UINT16_C(0x0a)
#define DW_FORM_data1                       UINT16_C(0x0b)
#define DW_FORM_flag                        UINT16_C(0x0c)
#define DW_FORM_sdata                       UINT16_C(0x0d)
#define DW_FORM_strp                        UINT16_C(0x0e)
#define DW_FORM_udata                       UINT16_C(0x0f)
#define DW_FORM_ref_addr                    UINT16_C(0x10)
#define DW_FORM_ref1                        UINT16_C(0x11)
#define DW_FORM_ref2                        UINT16_C(0x12)
#define DW_FORM_ref4                        UINT16_C(0x13)
#define DW_FORM_ref8                        UINT16_C(0x14)
#define DW_FORM_ref_udata                   UINT16_C(0x15)
#define DW_FORM_indirect                    UINT16_C(0x16)
#define DW_FORM_sec_offset                  UINT16_C(0x17)
#define DW_FORM_exprloc                     UINT16_C(0x18)
#define DW_FORM_flag_present                UINT16_C(0x19)
#define DW_FORM_ref_sig8                    UINT16_C(0x20)
/** @} */

/** @name Address classes.
 * @{ */
#define DW_ADDR_none            UINT8_C(0)
#define DW_ADDR_i386_near16     UINT8_C(1)
#define DW_ADDR_i386_far16      UINT8_C(2)
#define DW_ADDR_i386_huge16     UINT8_C(3)
#define DW_ADDR_i386_near32     UINT8_C(4)
#define DW_ADDR_i386_far32      UINT8_C(5)
/** @} */


/** @name Location Expression Opcodes
 * @{ */
#define DW_OP_addr              UINT8_C(0x03) /**< 1 operand, a constant address (size target specific). */
#define DW_OP_deref             UINT8_C(0x06) /**< 0 operands. */
#define DW_OP_const1u           UINT8_C(0x08) /**< 1 operand, a 1-byte constant. */
#define DW_OP_const1s           UINT8_C(0x09) /**< 1 operand, a 1-byte constant. */
#define DW_OP_const2u           UINT8_C(0x0a) /**< 1 operand, a 2-byte constant. */
#define DW_OP_const2s           UINT8_C(0x0b) /**< 1 operand, a 2-byte constant. */
#define DW_OP_const4u           UINT8_C(0x0c) /**< 1 operand, a 4-byte constant. */
#define DW_OP_const4s           UINT8_C(0x0d) /**< 1 operand, a 4-byte constant. */
#define DW_OP_const8u           UINT8_C(0x0e) /**< 1 operand, a 8-byte constant. */
#define DW_OP_const8s           UINT8_C(0x0f) /**< 1 operand, a 8-byte constant. */
#define DW_OP_constu            UINT8_C(0x10) /**< 1 operand, a ULEB128 constant. */
#define DW_OP_consts            UINT8_C(0x11) /**< 1 operand, a SLEB128 constant. */
#define DW_OP_dup               UINT8_C(0x12) /**< 0 operands. */
#define DW_OP_drop              UINT8_C(0x13) /**< 0 operands. */
#define DW_OP_over              UINT8_C(0x14) /**< 0 operands. */
#define DW_OP_pick              UINT8_C(0x15) /**< 1 operands, a 1-byte stack index. */
#define DW_OP_swap              UINT8_C(0x16) /**< 0 operands. */
#define DW_OP_rot               UINT8_C(0x17) /**< 0 operands. */
#define DW_OP_xderef            UINT8_C(0x18) /**< 0 operands. */
#define DW_OP_abs               UINT8_C(0x19) /**< 0 operands. */
#define DW_OP_and               UINT8_C(0x1a) /**< 0 operands. */
#define DW_OP_div               UINT8_C(0x1b) /**< 0 operands. */
#define DW_OP_minus             UINT8_C(0x1c) /**< 0 operands. */
#define DW_OP_mod               UINT8_C(0x1d) /**< 0 operands. */
#define DW_OP_mul               UINT8_C(0x1e) /**< 0 operands. */
#define DW_OP_neg               UINT8_C(0x1f) /**< 0 operands. */
#define DW_OP_not               UINT8_C(0x20) /**< 0 operands. */
#define DW_OP_or                UINT8_C(0x21) /**< 0 operands. */
#define DW_OP_plus              UINT8_C(0x22) /**< 0 operands. */
#define DW_OP_plus_uconst       UINT8_C(0x23) /**< 1 operands, a ULEB128 addend. */
#define DW_OP_shl               UINT8_C(0x24) /**< 0 operands. */
#define DW_OP_shr               UINT8_C(0x25) /**< 0 operands. */
#define DW_OP_shra              UINT8_C(0x26) /**< 0 operands. */
#define DW_OP_xor               UINT8_C(0x27) /**< 0 operands. */
#define DW_OP_skip              UINT8_C(0x2f) /**< 1 signed 2-byte constant. */
#define DW_OP_bra               UINT8_C(0x28) /**< 1 signed 2-byte constant. */
#define DW_OP_eq                UINT8_C(0x29) /**< 0 operands. */
#define DW_OP_ge                UINT8_C(0x2a) /**< 0 operands. */
#define DW_OP_gt                UINT8_C(0x2b) /**< 0 operands. */
#define DW_OP_le                UINT8_C(0x2c) /**< 0 operands. */
#define DW_OP_lt                UINT8_C(0x2d) /**< 0 operands. */
#define DW_OP_ne                UINT8_C(0x2e) /**< 0 operands. */
#define DW_OP_lit0              UINT8_C(0x30) /**< 0 operands - literals 0..31 */
#define DW_OP_lit31             UINT8_C(0x4f) /**< last litteral. */
#define DW_OP_reg0              UINT8_C(0x50) /**< 0 operands - reg 0..31. */
#define DW_OP_reg31             UINT8_C(0x6f) /**< last register. */
#define DW_OP_breg0             UINT8_C(0x70) /**< 1 operand, a SLEB128 offset. */
#define DW_OP_breg31            UINT8_C(0x8f) /**< last branch register. */
#define DW_OP_regx              UINT8_C(0x90) /**< 1 operand, a ULEB128 register. */
#define DW_OP_fbreg             UINT8_C(0x91) /**< 1 operand, a SLEB128 offset. */
#define DW_OP_bregx             UINT8_C(0x92) /**< 2 operands, a ULEB128 register followed by a SLEB128 offset. */
#define DW_OP_piece             UINT8_C(0x93) /**< 1 operand, a ULEB128 size of piece addressed. */
#define DW_OP_deref_size        UINT8_C(0x94) /**< 1 operand, a 1-byte size of data retrieved. */
#define DW_OP_xderef_size       UINT8_C(0x95) /**< 1 operand, a 1-byte size of data retrieved. */
#define DW_OP_nop               UINT8_C(0x96) /**< 0 operands. */
#define DW_OP_lo_user           UINT8_C(0xe0) /**< First user opcode */
#define DW_OP_hi_user           UINT8_C(0xff) /**< Last user opcode. */
/** @} */

/** @name Exception Handler Pointer Encodings (GCC/LSB).
 * @{ */
#define DW_EH_PE_FORMAT_MASK    UINT8_C(0x0f) /**< Format mask. */
#define DW_EH_PE_APPL_MASK      UINT8_C(0x70) /**< Application mask. */
#define DW_EH_PE_indirect       UINT8_C(0x80) /**< Flag: Indirect pointer. */
#define DW_EH_PE_omit           UINT8_C(0xff) /**< Special value: Omitted. */
#define DW_EH_PE_ptr            UINT8_C(0x00) /**< Format: pointer sized, unsigned. */
#define DW_EH_PE_uleb128        UINT8_C(0x01) /**< Format: unsigned LEB128. */
#define DW_EH_PE_udata2         UINT8_C(0x02) /**< Format: unsigned 16-bit. */
#define DW_EH_PE_udata4         UINT8_C(0x03) /**< Format: unsigned 32-bit. */
#define DW_EH_PE_udata8         UINT8_C(0x04) /**< Format: unsigned 64-bit. */
#define DW_EH_PE_sleb128        UINT8_C(0x09) /**< Format: signed LEB128. */
#define DW_EH_PE_sdata2         UINT8_C(0x0a) /**< Format: signed 16-bit. */
#define DW_EH_PE_sdata4         UINT8_C(0x0b) /**< Format: signed 32-bit. */
#define DW_EH_PE_sdata8         UINT8_C(0x0c) /**< Format: signed 64-bit. */
#define DW_EH_PE_absptr         UINT8_C(0x00) /**< Application: Absolute */
#define DW_EH_PE_pcrel          UINT8_C(0x10) /**< Application: PC relative, i.e. relative pointer address. */
#define DW_EH_PE_textrel        UINT8_C(0x20) /**< Application: text section relative. */
#define DW_EH_PE_datarel        UINT8_C(0x30) /**< Application: data section relative. */
#define DW_EH_PE_funcrel        UINT8_C(0x40) /**< Application: relative to start of function. */
#define DW_EH_PE_aligned        UINT8_C(0x50) /**< Application: aligned pointer. */
/** @} */

/** @name Call frame instructions.
 * @{  */
/** Mask to use to identify DW_CFA_advance_loc, DW_CFA_offset and DW_CFA_restore. */
#define DW_CFA_high_bit_mask        UINT8_C(0xc0)

#define DW_CFA_nop                  UINT8_C(0x00) /**< No operands. */

#define DW_CFA_advance_loc          UINT8_C(0x40) /**< low 6 bits: delta to advance. */
#define DW_CFA_set_loc              UINT8_C(0x01) /**< op1: address. */
#define DW_CFA_advance_loc1         UINT8_C(0x02) /**< op1: 1-byte delta. */
#define DW_CFA_advance_loc2         UINT8_C(0x03) /**< op1: 2-byte delta. */
#define DW_CFA_advance_loc4         UINT8_C(0x04) /**< op1: 4-byte delta. */

#define DW_CFA_offset               UINT8_C(0x80) /**< low 6 bits: register; op1: ULEB128 offset. */
#define DW_CFA_offset_extended      UINT8_C(0x05) /**< op1: ULEB128 register; op2: ULEB128 offset. */
#define DW_CFA_offset_extended_sf   UINT8_C(0x11) /**< op1: ULEB128 register; op2: SLEB128 offset. */
#define DW_CFA_restore              UINT8_C(0xc0) /**< low 6 bits: register. */
#define DW_CFA_restore_extended     UINT8_C(0x06) /**< op1: ULEB128 register. */
#define DW_CFA_undefined            UINT8_C(0x07) /**< op1: ULEB128 register. */
#define DW_CFA_same_value           UINT8_C(0x08) /**< op1: ULEB128 register. */
#define DW_CFA_register             UINT8_C(0x09) /**< op1: ULEB128 destination register; op2: ULEB128 source register. */
#define DW_CFA_expression           UINT8_C(0x10) /**< op1: ULEB128 register; op2: BLOCK. */

#define DW_CFA_val_offset           UINT8_C(0x14) /**< op1: ULEB128 register; op2: ULEB128. */
#define DW_CFA_val_offset_sf        UINT8_C(0x15) /**< op1: ULEB128 register; op2: SLEB128. */
#define DW_CFA_val_expression       UINT8_C(0x16) /**< op1: ULEB128 register; op2: BLOCK. */

#define DW_CFA_remember_state       UINT8_C(0x0a) /**< No operands. */
#define DW_CFA_restore_state        UINT8_C(0x0b) /**< No operands. */

#define DW_CFA_def_cfa              UINT8_C(0x0c) /**< op1: ULEB128 register; op2: ULEB128 offset. */
#define DW_CFA_def_cfa_register     UINT8_C(0x0d) /**< op1: ULEB128 register. */
#define DW_CFA_def_cfa_offset       UINT8_C(0x0e) /**< op1: ULEB128 offset. */
#define DW_CFA_def_cfa_expression   UINT8_C(0x0f) /**< op1: BLOCK. */
#define DW_CFA_def_cfa_sf           UINT8_C(0x12) /**< op1: ULEB128 register; op2: SLEB128 offset. */
#define DW_CFA_def_cfa_offset_sf    UINT8_C(0x13) /**< op1: SLEB128 offset. */

#define DW_CFA_lo_user              UINT8_C(0x1c) /**< User defined operands. */
#define DW_CFA_MIPS_advance_loc8    UINT8_C(0x1d) /**< op1: 8-byte delta? */
#define DW_CFA_GNU_window_save      UINT8_C(0x2d) /**< op1: ??; op2: ?? */
#define DW_CFA_GNU_args_size        UINT8_C(0x2e) /**< op1: ??; op2: ?? */
#define DW_CFA_GNU_negative_offset_extended UINT8_C(0x2f) /**< op1: ??; op2: ?? */
#define DW_CFA_hi_user              UINT8_C(0x3f) /**< User defined operands. */
/** @} */


/** @name DWREG_X86_XXX - 386+ register number mappings.
 * @{  */
#define DWREG_X86_EAX       0
#define DWREG_X86_ECX       1
#define DWREG_X86_EDX       2
#define DWREG_X86_EBX       3
#define DWREG_X86_ESP       4
#define DWREG_X86_EBP       5
#define DWREG_X86_ESI       6
#define DWREG_X86_EDI       7
#define DWREG_X86_RA        8   /* return address (=EIP) */
#define DWREG_X86_EFLAGS    9
#define DWREG_X86_ST1       11
#define DWREG_X86_ST2       12
#define DWREG_X86_ST3       13
#define DWREG_X86_ST4       14
#define DWREG_X86_ST5       15
#define DWREG_X86_ST6       16
#define DWREG_X86_ST7       17
#define DWREG_X86_XMM0      21
#define DWREG_X86_XMM1      22
#define DWREG_X86_XMM2      23
#define DWREG_X86_XMM3      24
#define DWREG_X86_XMM4      25
#define DWREG_X86_XMM5      26
#define DWREG_X86_XMM6      27
#define DWREG_X86_XMM7      28
#define DWREG_X86_MM0       29
#define DWREG_X86_MM1       30
#define DWREG_X86_MM2       31
#define DWREG_X86_MM3       32
#define DWREG_X86_MM4       33
#define DWREG_X86_MM5       34
#define DWREG_X86_MM6       35
#define DWREG_X86_MM7       36
#define DWREG_X86_MXCSR     39
#define DWREG_X86_ES        40
#define DWREG_X86_CS        41
#define DWREG_X86_SS        42
#define DWREG_X86_DS        43
#define DWREG_X86_FS        44
#define DWREG_X86_GS        45
#define DWREG_X86_TR        48
#define DWREG_X86_LDTR      49
/** @} */


/** @name DWREG_AMD64_XXX - AMD64 register number mappings.
 * @note This for some braindead reason the first 8 GPR are in intel encoding
 *       order, unlike the DWREG_X86_XXX variant.  Utter stupidity.
 * @{ */
#define DWREG_AMD64_RAX     0
#define DWREG_AMD64_RDX     1
#define DWREG_AMD64_RCX     2
#define DWREG_AMD64_RBX     3
#define DWREG_AMD64_RSI     4
#define DWREG_AMD64_RDI     5
#define DWREG_AMD64_RBP     6
#define DWREG_AMD64_RSP     7
#define DWREG_AMD64_R8      8
#define DWREG_AMD64_R9      9
#define DWREG_AMD64_R10     10
#define DWREG_AMD64_R11     11
#define DWREG_AMD64_R12     12
#define DWREG_AMD64_R13     13
#define DWREG_AMD64_R14     14
#define DWREG_AMD64_R15     15
#define DWREG_AMD64_RA      16   /* return address (=RIP) */
#define DWREG_AMD64_XMM0    17
#define DWREG_AMD64_XMM1    18
#define DWREG_AMD64_XMM2    19
#define DWREG_AMD64_XMM3    20
#define DWREG_AMD64_XMM4    21
#define DWREG_AMD64_XMM5    22
#define DWREG_AMD64_XMM6    23
#define DWREG_AMD64_XMM7    24
#define DWREG_AMD64_XMM8    25
#define DWREG_AMD64_XMM9    26
#define DWREG_AMD64_XMM10   27
#define DWREG_AMD64_XMM11   28
#define DWREG_AMD64_XMM12   29
#define DWREG_AMD64_XMM13   30
#define DWREG_AMD64_XMM14   31
#define DWREG_AMD64_XMM15   32
#define DWREG_AMD64_ST0     33
#define DWREG_AMD64_ST1     34
#define DWREG_AMD64_ST2     35
#define DWREG_AMD64_ST3     36
#define DWREG_AMD64_ST4     37
#define DWREG_AMD64_ST5     38
#define DWREG_AMD64_ST6     39
#define DWREG_AMD64_ST7     40
#define DWREG_AMD64_MM0     41
#define DWREG_AMD64_MM1     42
#define DWREG_AMD64_MM2     43
#define DWREG_AMD64_MM3     44
#define DWREG_AMD64_MM4     45
#define DWREG_AMD64_MM5     46
#define DWREG_AMD64_MM6     47
#define DWREG_AMD64_MM7     48
#define DWREG_AMD64_RFLAGS  49
#define DWREG_AMD64_ES      50
#define DWREG_AMD64_CS      51
#define DWREG_AMD64_SS      52
#define DWREG_AMD64_DS      53
#define DWREG_AMD64_FS      54
#define DWREG_AMD64_GS      55
#define DWREG_AMD64_FS_BASE 58
#define DWREG_AMD64_GS_BASE 59
#define DWREG_AMD64_TR      62
#define DWREG_AMD64_LDTR    63
#define DWREG_AMD64_MXCSR   64
#define DWREG_AMD64_FCW     65
#define DWREG_AMD64_FSW     66
/** @} */

#endif /* !IPRT_INCLUDED_formats_dwarf_h */

