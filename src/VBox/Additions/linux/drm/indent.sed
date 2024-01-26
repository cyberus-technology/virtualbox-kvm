# Oracle VM VirtualBox
# VirtualBox to Linux kernel coding style conversion script.

#
# Copyright (C) 2017-2023 Oracle and/or its affiliates.
#
# This file is part of VirtualBox base platform packages, as
# available from https://www.virtualbox.org.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, in version 3 of the
# License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses>.
#
# SPDX-License-Identifier: GPL-3.0-only
#

# This script is for converting code inside the vboxvideo module to Linux
# kernel coding style.  It assumes correct VirtualBox coding style, will break
# break if the coding style is wrong (e.g. tab instead of spaces) and is not
# indended to be a generic solution: for example, identifiers will be
# translated case by case, not algorithmically.  It also assumes that any
# flexibility in either coding style will be used where possible to make the
# code conform to both at once.

# Replace up to six leading groups of four spaces with tabs.
s/^                         */\t\t\t\t\t\t/g
s/^                    /\t\t\t\t\t/g
s/^                /\t\t\t\t/g
s/^            /\t\t\t/g
s/^        /\t\t/g
s/^    /\t/g

# Change various symbols and file names to fit kernel conventions.

# Miscellaneous:
# Remove @file headers.
\|/\*\* @file| {
:start
  N
  s|\*/|\*/|g
  T start
  N
  d
}
/^\/\* \$Id:.*\*\/$/d
/^typedef .* HGSMIOFFSET;/d
/^typedef .* HGSMISIZE;/d
s/^#\( *\)include <\([^/]*\)>/#\1include "\2"/g

# File names:
s/\bHGSMIBase\.h\b/vbox_drv.h/g
s/\bHGSMIChannels\.h\b/hgsmi_channels.h/g
s/\bHGSMIChSetup\.h\b/hgsmi_ch_setup.h/g
s/\bHGSMIContext\.h\b/hgsmi_context.h/g
s/\bHGSMIDefs\.h\b/hgsmi_defs.h/g
s/\bVBoxVideoGuest\.h\b/vboxvideo_guest.h/g
s/\bVBoxVideo\.h\b/vboxvideo.h/g
s/\bVBoxVideoIPRT\.h\b/vbox_err.h/g
s/\bVBoxVideoVBE\.h\b/vboxvideo_vbe.h/g

# Function names:
s/\btestQueryConf\b/hgsmi_test_query_conf/g
s/\bVBoxHGSMIBufferAlloc\b/hgsmi_buffer_alloc/g
s/\bVBoxHGSMIBufferFree\b/hgsmi_buffer_free/g
s/\bVBoxHGSMIBufferSubmit\b/hgsmi_buffer_submit/g
s/\bVBoxHGSMICursorPosition\b/hgsmi_cursor_position/g
s/\bVBoxHGSMIGetModeHints\b/hgsmi_get_mode_hints/g
s/\bVBoxHGSMIProcessDisplayInfo\b/hgsmi_process_display_info/g
s/\bVBoxHGSMIReportFlagsLocation\b/hgsmi_report_flags_location/g
s/\bVBoxHGSMISendCapsInfo\b/hgsmi_send_caps_info/g
s/\bVBoxHGSMIUpdateInputMapping\b/hgsmi_update_input_mapping/g
s/\bVBoxHGSMIUpdatePointerShape\b/hgsmi_update_pointer_shape/g
s/\bvboxHwBufferAvail\b/vbva_buffer_available/g
s/\bvboxHwBufferEndUpdate\b/vbva_buffer_end_update/g
s/\bvboxHwBufferFlush\b/vbva_buffer_flush/g
s/\bvboxHwBufferPlaceDataAt\b/vbva_buffer_place_data_at/g
s/\bvboxHwBufferWrite\b/vbva_write/g
s/\bVBoxQueryConfHGSMI\b/hgsmi_query_conf/g
s/\bVBoxVBVABufferBeginUpdate\b/vbva_buffer_begin_update/g
s/\bVBoxVBVABufferEndUpdate\b/vbva_buffer_end_update/g
s/\bVBoxVBVADisable\b/vbva_disable/g
s/\bVBoxVBVAEnable\b/vbva_enable/g
s/\bvboxVBVAInformHost\b/vbva_inform_host/g
s/\bvboxVBVASetupBufferContext\b/vbva_setup_buffer_context/g
s/\bVBVO_PORT_READ_U8\b/inb/g
s/\bVBVO_PORT_READ_U16\b/inw/g
s/\bVBVO_PORT_READ_U32\b/inl/g
s/\bVBVO_PORT_WRITE_U8\b *( *\(\b[^(),]*\b\) *, *\(\b[^(),]*\b\) *)/outb(\2, \1)/g
s/\bVBVO_PORT_WRITE_U16\b *( *\(\b[^(),]*\b\) *, *\(\b[^(),]*\b\) *)/outw(\2, \1)/g
s/\bVBVO_PORT_WRITE_U32\b *( *\(\b[^(),]*\b\) *, *\(\b[^(),]*\b\) *)/outl(\2, \1)/g
s/\bVBVO_PORT_WRITE_U[0-9]*\b/VBVO_PORT_WRITE_statement_should_be_on_one_line/g

# Macros:
s/\b_1K\b/1024/g
s/\b_4M\b/4*1024*1024/g
s/\bAssert\b\([^;]*\);/WARN_ON_ONCE(!(\1));/g
s/\bAssertCompile\b/assert_compile/g
s/\bAssertCompileSize\b/assert_compile_size/g
s/\bAssertPtr\b\([^;]*\);/WARN_ON_ONCE(!(\1));/g
s/\bAssertPtrReturn\b/assert_ptr_return/g
/AssertPtrNullReturnVoid/d
s/\bAssertRC\b\([^;]*\);/WARN_ON_ONCE(RT_FAILURE\1);/g
s/\bAssertRC\b/Assert_RC_statement_should_be_on_one_line/g
s/\bDECLCALLBACK\b(\([^)]*\))/\1/g
  s/\bDECLCALLBACKTYPE\b(\([^,)]*\), *\([^,)]*\), *(\([^;)]*\) *) *)/\1 \2(\3)/g
s/\bDECLCALLBACKMEMBER\b(\([^,)]*\), *\([^,)]*\), *(\([^;)]*\) *) *)/\1 (*\2)(\3)/g
s/^\bDECLHIDDEN\b(\([^)]*\))/\1/g
s/\bDECLINLINE\b(\([^)]*\))/static inline \1/g
s/\bRT_BIT\b/BIT/g
s/\bRT_BOOL\b(\([^)]*\))/(!!(\1))/g
/RT_C_DECLS/d
s/\bUINT16_MAX\b/U16_MAX/g
s/\bUINT32_MAX\b/U32_MAX/g
s/\bUINT32_C\b(\(.*\))/\1u/g
s/!RT_VALID_PTR(/WARN_ON(!/g
s/\bRT_UNTRUSTED_VOLATILE_HOST\b//g
s/\bRT_UNTRUSTED_VOLATILE_GUEST\b//g
s/\bRT_UNTRUSTED_VOLATILE_HSTGST\b//g

# Type names:
s/\bint32_t\b/s32/g
s/\buint8_t\b/u8/g
s/\buint16_t\b/u16/g
s/\buint32_t\b/u32/g
s/(HGSMIBUFFERLOCATION \*)//g  # Remove C++ casts from void.
s/typedef struct HGSMIBUFFERLOCATION/struct hgsmi_buffer_location/g
s/struct HGSMIBUFFERLOCATION/struct hgsmi_buffer_location/g
s/} HGSMIBUFFERLOCATION/}/g
s/\bHGSMIBUFFERLOCATION\b/struct hgsmi_buffer_location/g
s/\([^*] *\)\bPHGSMIGUESTCOMMANDCONTEXT\b/\1struct gen_pool */g
s/(HGSMIHOSTFLAGS \*)//g  # Remove C++ casts from void.
s/typedef struct HGSMIHOSTFLAGS/struct hgsmi_host_flags/g
s/struct HGSMIHOSTFLAGS/struct hgsmi_host_flags/g
s/} HGSMIHOSTFLAGS/}/g
s/\bHGSMIHOSTFLAGS\b/struct hgsmi_host_flags/g
s/\bHGSMIOFFSET\b/u32/g
s/\bHGSMISIZE\b/u32/g
s/\bRTRECT\b/void/g
s/(VBVABUFFERCONTEXT \*)//g  # Remove C++ casts from void.
s/struct VBVABUFFERCONTEXT/struct vbva_buf_context/g
s/} VBVABUFFERCONTEXT/} vbva_buf_context/g
s/\bVBVABUFFERCONTEXT\b/struct vbva_buf_context/g
s/\([^*] *\)\bPVBVABUFFERCONTEXT\b/\1struct vbva_buf_context */g
s/(VBVACAPS \*)//g  # Remove C++ casts from void.
s/struct VBVACAPS/struct vbva_caps/g
s/} VBVACAPS/} vbva_caps/g
s/\bVBVACAPS\b/struct vbva_caps/g
s/(VBVACONF32 \*)//g  # Remove C++ casts from void.
s/struct VBVACONF32/struct vbva_conf32/g
s/} VBVACONF32/} vbva_conf32/g
s/\bVBVACONF32\b/struct vbva_conf32/g
s/(VBVACURSORPOSITION \*)//g  # Remove C++ casts from void.
s/struct VBVACURSORPOSITION/struct vbva_cursor_position/g
s/} VBVACURSORPOSITION/} vbva_cursor_position/g
s/\bVBVACURSORPOSITION\b/struct vbva_cursor_position/g
s/(VBVAENABLE_EX \*)//g  # Remove C++ casts from void.
s/struct VBVAENABLE_EX/struct vbva_enable_ex/g
s/} VBVAENABLE_EX/} vbva_enable_ex/g
s/\bVBVAENABLE_EX\b/struct vbva_enable_ex/g
s/(VBVAMOUSEPOINTERSHAPE \*)//g  # Remove C++ casts from void.
s/struct VBVAMOUSEPOINTERSHAPE/struct vbva_mouse_pointer_shape/g
s/} VBVAMOUSEPOINTERSHAPE/} vbva_mouse_pointer_shape/g
s/\bVBVAMOUSEPOINTERSHAPE\b/struct vbva_mouse_pointer_shape/g
s/(VBVAMODEHINT \*)//g  # Remove C++ casts from void.
s/struct VBVAMODEHINT/struct vbva_modehint/g
s/} VBVAMODEHINT/} vbva_modehint/g
s/\bVBVAMODEHINT\b/struct vbva_modehint/g
s/(VBVAQUERYMODEHINTS \*)//g  # Remove C++ casts from void.
s/struct VBVAQUERYMODEHINTS/struct vbva_query_mode_hints/g
s/} VBVAQUERYMODEHINTS/} vbva_query_mode_hints/g
s/\bVBVAQUERYMODEHINTS\b/struct vbva_query_mode_hints/g
s/(VBVAREPORTINPUTMAPPING \*)//g  # Remove C++ casts from void.
s/struct VBVAREPORTINPUTMAPPING/struct vbva_report_input_mapping/g
s/} VBVAREPORTINPUTMAPPING/} vbva_report_input_mapping/g
s/\bVBVAREPORTINPUTMAPPING\b/struct vbva_report_input_mapping/g

# Variable and parameter names:
s/\baRecords\b/records/g
s/\bau8Data\b/data/g
s/\bau32Reserved\b/reserved/g
s/\bBase\b/base/g
s/\bbEnable\b/enable/g
s/\bbRc\b/ret/g
s/\bcb\b/len/g
s/\bcbBuffer\b/buffer_length/g
s/\bcbChunk\b/chunk/g
s/\bcbData\b/data_len/g
s/\bcbHintsStructureGuest\b/hints_structure_guest_size/g
s/\bcbHwBufferAvail\b/available/g
s/\bcbLength\b/len/g
s/\bcbLocation\b/buf_len/g
s/\bcbPartialWriteThreshold\b/partial_write_tresh/g  ## @todo fix this?
s/\bcbPitch\b/pitch/g
s/\bcbPixels\b/pixel_len/g
s/\bcBPP\b/bpp/g
s/\bcbRecord\b/len_and_flags/g  ## @todo fix this?
s/\bcDisplay\b/display/g
s/\bcHeight\b/height/g
s/\bcHintsQueried\b/hints_queried_count/g
s/\bcHotX\b/hot_x/g
s/\bcHotY\b/hot_y/g
s/\bcOriginX\b/origin_x/g
s/\bcOriginY\b/origin_y/g
s/\bcScreen\b/screen/g
s/\bcScreens\b/screens/g
s/\bcWidth\b/width/g
s/\bfCaps\b/caps/g
s/\bfFlags\b/flags/g
s/\bfHwBufferOverflow\b/buffer_overflow/g
s/\bfReportPosition\b/report_position/g
s/\bfu32Flags\b/flags/g
s/\bhostFlags\b/host_flags/g
s/\bi32Diff\b/diff/g
s/\bi32OriginX\b/origin_x/g
s/\bi32OriginY\b/origin_y/g
s/\bi32Result\b/result/g
s/\bindexRecordFirst\b/first_record_index/g
s/\bindexRecordFree\b/free_record_index/g
s/\bindexRecordNext\b/next/g
s/\boff32Data\b/data_offset/g
s/\boff32Free\b/free_offset/g
s/\boffLocation\b/location/g
s/\boffStart\b/start_offset/g
s/\boffVRAMBuffer\b/buffer_offset/g
s/\bpaHints\b/hints/g
s/\bpCtx\b/ctx/g
s/\bpPixels\b/pixels/g
s/\bpRecord\b/record/g
s/\bpulValue\b/value_ret/g
s/\bpVBVA\b/vbva/g
s/\bpxHost\b/x_host/g
s/\bpyHost\b/y_host/g
s/\bu16BitsPerPixel\b/bits_per_pixel/g
s/\bu16Flags\b/flags/g
s/\bu32BytesTillBoundary\b/bytes_till_boundary/g
s/\bu32Flags\b/flags/g
s/\bu32Height\b/height/g
s/\bu32HostEvents\b/host_events/g
s/\bu32HostFlags\b/host_flags/g
s/\bu32HotX\b/hot_x/g
s/\bu32HotY\b/hot_y/g
s/\bu32Index\b/index/g
s/\bu32LineSize\b/line_size/g
s/\bu32Offset\b/offset/g
s/\bu32Reserved\b/reserved/g
s/\bu32ScreenId\b/screen_id/g
s/\bu32StartOffset\b/start_offset/g
s/\bu32SupportedOrders\b/supported_orders/g
s/\bu32Value\b/value/g
s/\bu32ViewIndex\b/view_index/g
s/\bu32Width\b/width/g
s/\bulValue\b/value/g

# Header file guard:
s/__HGSMIChannels_h__/__HGSMI_CHANNELS_H__/g
s/VBOX_INCLUDED_Graphics_HGSMIChSetup_h/__HGSMI_CH_SETUP_H__/g

# And move braces.  This must be the last expression as it jumps to the next
# line.
/..*$/ {
  N
  s/^\([\t ][\t ]*\)} *\n[\t ]*else/\1} else/g
  t continue_else
  b try_brace
:continue_else
  N
:try_brace
  s/^\([\t ].*\)\n[\t ][\t ]*{/\1 {/g
  s/^\([^#()]*\)\n[\t ]*{/\1 {/g
  t done_brace
  P
  D
:done_brace
  p
  d
}
