/* $Xorg: OidDefs.h,v 1.4 2001/03/14 18:45:13 pookie Exp $ */
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/
/* This is an automatically-generated file. Do not edit. */

typedef enum {
    xpoid_none,
    xpoid_unspecified,
    xpoid_att_descriptor,
    xpoid_att_content_orientation,
    xpoid_att_copy_count,
    xpoid_att_default_printer_resolution,
    xpoid_att_default_input_tray,
    xpoid_att_default_medium,
    xpoid_att_document_format,
    xpoid_att_plex,
    xpoid_att_xp_listfonts_modes,
    xpoid_att_job_name,
    xpoid_att_job_owner,
    xpoid_att_notification_profile,
    xpoid_att_xp_setup_state,
    xpoid_att_xp_spooler_command_options,
    xpoid_att_content_orientations_supported,
    xpoid_att_document_formats_supported,
    xpoid_att_dt_pdm_command,
    xpoid_att_input_trays_medium,
    xpoid_att_medium_source_sizes_supported,
    xpoid_att_plexes_supported,
    xpoid_att_printer_model,
    xpoid_att_printer_name,
    xpoid_att_printer_resolutions_supported,
    xpoid_att_xp_embedded_formats_supported,
    xpoid_att_xp_listfonts_modes_supported,
    xpoid_att_xp_page_attributes_supported,
    xpoid_att_xp_raw_formats_supported,
    xpoid_att_xp_setup_proviso,
    xpoid_att_document_attributes_supported,
    xpoid_att_job_attributes_supported,
    xpoid_att_locale,
    xpoid_att_multiple_documents_supported,
    xpoid_att_available_compression,
    xpoid_att_available_compressions_supported,
    xpoid_val_content_orientation_portrait,
    xpoid_val_content_orientation_landscape,
    xpoid_val_content_orientation_reverse_portrait,
    xpoid_val_content_orientation_reverse_landscape,
    xpoid_val_medium_size_iso_a0,
    xpoid_val_medium_size_iso_a1,
    xpoid_val_medium_size_iso_a2,
    xpoid_val_medium_size_iso_a3,
    xpoid_val_medium_size_iso_a4,
    xpoid_val_medium_size_iso_a5,
    xpoid_val_medium_size_iso_a6,
    xpoid_val_medium_size_iso_a7,
    xpoid_val_medium_size_iso_a8,
    xpoid_val_medium_size_iso_a9,
    xpoid_val_medium_size_iso_a10,
    xpoid_val_medium_size_iso_b0,
    xpoid_val_medium_size_iso_b1,
    xpoid_val_medium_size_iso_b2,
    xpoid_val_medium_size_iso_b3,
    xpoid_val_medium_size_iso_b4,
    xpoid_val_medium_size_iso_b5,
    xpoid_val_medium_size_iso_b6,
    xpoid_val_medium_size_iso_b7,
    xpoid_val_medium_size_iso_b8,
    xpoid_val_medium_size_iso_b9,
    xpoid_val_medium_size_iso_b10,
    xpoid_val_medium_size_na_letter,
    xpoid_val_medium_size_na_legal,
    xpoid_val_medium_size_executive,
    xpoid_val_medium_size_folio,
    xpoid_val_medium_size_invoice,
    xpoid_val_medium_size_ledger,
    xpoid_val_medium_size_quarto,
    xpoid_val_medium_size_iso_c3,
    xpoid_val_medium_size_iso_c4,
    xpoid_val_medium_size_iso_c5,
    xpoid_val_medium_size_iso_c6,
    xpoid_val_medium_size_iso_designated_long,
    xpoid_val_medium_size_na_10x13_envelope,
    xpoid_val_medium_size_na_9x12_envelope,
    xpoid_val_medium_size_na_number_10_envelope,
    xpoid_val_medium_size_na_7x9_envelope,
    xpoid_val_medium_size_na_9x11_envelope,
    xpoid_val_medium_size_na_10x14_envelope,
    xpoid_val_medium_size_na_number_9_envelope,
    xpoid_val_medium_size_na_6x9_envelope,
    xpoid_val_medium_size_na_10x15_envelope,
    xpoid_val_medium_size_monarch_envelope,
    xpoid_val_medium_size_a,
    xpoid_val_medium_size_b,
    xpoid_val_medium_size_c,
    xpoid_val_medium_size_d,
    xpoid_val_medium_size_e,
    xpoid_val_medium_size_jis_b0,
    xpoid_val_medium_size_jis_b1,
    xpoid_val_medium_size_jis_b2,
    xpoid_val_medium_size_jis_b3,
    xpoid_val_medium_size_jis_b4,
    xpoid_val_medium_size_jis_b5,
    xpoid_val_medium_size_jis_b6,
    xpoid_val_medium_size_jis_b7,
    xpoid_val_medium_size_jis_b8,
    xpoid_val_medium_size_jis_b9,
    xpoid_val_medium_size_jis_b10,
    xpoid_val_medium_size_hp_2x_postcard,
    xpoid_val_medium_size_hp_european_edp,
    xpoid_val_medium_size_hp_mini,
    xpoid_val_medium_size_hp_postcard,
    xpoid_val_medium_size_hp_tabloid,
    xpoid_val_medium_size_hp_us_edp,
    xpoid_val_medium_size_hp_us_government_legal,
    xpoid_val_medium_size_hp_us_government_letter,
    xpoid_val_plex_simplex,
    xpoid_val_plex_duplex,
    xpoid_val_plex_tumble,
    xpoid_val_input_tray_top,
    xpoid_val_input_tray_middle,
    xpoid_val_input_tray_bottom,
    xpoid_val_input_tray_envelope,
    xpoid_val_input_tray_manual,
    xpoid_val_input_tray_large_capacity,
    xpoid_val_input_tray_main,
    xpoid_val_input_tray_side,
    xpoid_val_event_report_job_completed,
    xpoid_val_delivery_method_electronic_mail,
    xpoid_val_xp_setup_mandatory,
    xpoid_val_xp_setup_optional,
    xpoid_val_xp_setup_ok,
    xpoid_val_xp_setup_incomplete,
    xpoid_val_xp_list_glyph_fonts,
    xpoid_val_xp_list_internal_printer_fonts,
    xpoid_val_available_compressions_0,
    xpoid_val_available_compressions_01,
    xpoid_val_available_compressions_02,
    xpoid_val_available_compressions_03,
    xpoid_val_available_compressions_012,
    xpoid_val_available_compressions_013,
    xpoid_val_available_compressions_023,
    xpoid_val_available_compressions_0123
} XpOid;
