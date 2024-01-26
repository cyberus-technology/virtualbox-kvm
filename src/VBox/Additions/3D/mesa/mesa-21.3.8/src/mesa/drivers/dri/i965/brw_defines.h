/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */

#ifndef BRW_DEFINES_H
#define BRW_DEFINES_H

#include "util/macros.h"

#define INTEL_MASK(high, low) (((1u<<((high)-(low)+1))-1)<<(low))
/* Using the GNU statement expression extension */
#define SET_FIELD(value, field)                                         \
   ({                                                                   \
      uint32_t fieldval = (uint32_t)(value) << field ## _SHIFT;         \
      assert((fieldval & ~ field ## _MASK) == 0);                       \
      fieldval & field ## _MASK;                                        \
   })

#define GET_BITS(data, high, low) ((data & INTEL_MASK((high), (low))) >> (low))
#define GET_FIELD(word, field) (((word)  & field ## _MASK) >> field ## _SHIFT)

/**
 * For use with masked MMIO registers where the upper 16 bits control which
 * of the lower bits are committed to the register.
 */
#define REG_MASK(value) ((value) << 16)

/* 3D state:
 */
#define CMD_3D_PRIM                                 0x7b00 /* 3DPRIMITIVE */
/* DW0 */
# define GFX4_3DPRIM_TOPOLOGY_TYPE_SHIFT            10
# define GFX4_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL (0 << 15)
# define GFX4_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM     (1 << 15)
# define GFX7_3DPRIM_INDIRECT_PARAMETER_ENABLE      (1 << 10)
# define GFX7_3DPRIM_PREDICATE_ENABLE               (1 << 8)
/* DW1 */
# define GFX7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL (0 << 8)
# define GFX7_3DPRIM_VERTEXBUFFER_ACCESS_RANDOM     (1 << 8)

#define BRW_ANISORATIO_2     0
#define BRW_ANISORATIO_4     1
#define BRW_ANISORATIO_6     2
#define BRW_ANISORATIO_8     3
#define BRW_ANISORATIO_10    4
#define BRW_ANISORATIO_12    5
#define BRW_ANISORATIO_14    6
#define BRW_ANISORATIO_16    7

#define BRW_BLENDFACTOR_ONE                 0x1
#define BRW_BLENDFACTOR_SRC_COLOR           0x2
#define BRW_BLENDFACTOR_SRC_ALPHA           0x3
#define BRW_BLENDFACTOR_DST_ALPHA           0x4
#define BRW_BLENDFACTOR_DST_COLOR           0x5
#define BRW_BLENDFACTOR_SRC_ALPHA_SATURATE  0x6
#define BRW_BLENDFACTOR_CONST_COLOR         0x7
#define BRW_BLENDFACTOR_CONST_ALPHA         0x8
#define BRW_BLENDFACTOR_SRC1_COLOR          0x9
#define BRW_BLENDFACTOR_SRC1_ALPHA          0x0A
#define BRW_BLENDFACTOR_ZERO                0x11
#define BRW_BLENDFACTOR_INV_SRC_COLOR       0x12
#define BRW_BLENDFACTOR_INV_SRC_ALPHA       0x13
#define BRW_BLENDFACTOR_INV_DST_ALPHA       0x14
#define BRW_BLENDFACTOR_INV_DST_COLOR       0x15
#define BRW_BLENDFACTOR_INV_CONST_COLOR     0x17
#define BRW_BLENDFACTOR_INV_CONST_ALPHA     0x18
#define BRW_BLENDFACTOR_INV_SRC1_COLOR      0x19
#define BRW_BLENDFACTOR_INV_SRC1_ALPHA      0x1A

#define BRW_BLENDFUNCTION_ADD               0
#define BRW_BLENDFUNCTION_SUBTRACT          1
#define BRW_BLENDFUNCTION_REVERSE_SUBTRACT  2
#define BRW_BLENDFUNCTION_MIN               3
#define BRW_BLENDFUNCTION_MAX               4

#define BRW_ALPHATEST_FORMAT_UNORM8         0
#define BRW_ALPHATEST_FORMAT_FLOAT32        1

#define BRW_CHROMAKEY_KILL_ON_ANY_MATCH  0
#define BRW_CHROMAKEY_REPLACE_BLACK      1

#define BRW_CLIP_API_OGL     0
#define BRW_CLIP_API_DX      1

#define BRW_CLIP_NDCSPACE     0
#define BRW_CLIP_SCREENSPACE  1

#define BRW_COMPAREFUNCTION_ALWAYS       0
#define BRW_COMPAREFUNCTION_NEVER        1
#define BRW_COMPAREFUNCTION_LESS         2
#define BRW_COMPAREFUNCTION_EQUAL        3
#define BRW_COMPAREFUNCTION_LEQUAL       4
#define BRW_COMPAREFUNCTION_GREATER      5
#define BRW_COMPAREFUNCTION_NOTEQUAL     6
#define BRW_COMPAREFUNCTION_GEQUAL       7

#define BRW_COVERAGE_PIXELS_HALF     0
#define BRW_COVERAGE_PIXELS_1        1
#define BRW_COVERAGE_PIXELS_2        2
#define BRW_COVERAGE_PIXELS_4        3

#define BRW_CULLMODE_BOTH        0
#define BRW_CULLMODE_NONE        1
#define BRW_CULLMODE_FRONT       2
#define BRW_CULLMODE_BACK        3

#define BRW_DEFAULTCOLOR_R8G8B8A8_UNORM      0
#define BRW_DEFAULTCOLOR_R32G32B32A32_FLOAT  1

#define BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT     0
#define BRW_DEPTHFORMAT_D32_FLOAT                1
#define BRW_DEPTHFORMAT_D24_UNORM_S8_UINT        2
#define BRW_DEPTHFORMAT_D24_UNORM_X8_UINT        3 /* GFX5 */
#define BRW_DEPTHFORMAT_D16_UNORM                5

#define BRW_FLOATING_POINT_IEEE_754        0
#define BRW_FLOATING_POINT_NON_IEEE_754    1

#define BRW_FRONTWINDING_CW      0
#define BRW_FRONTWINDING_CCW     1

#define BRW_CUT_INDEX_ENABLE     (1 << 10)

#define BRW_INDEX_BYTE     0
#define BRW_INDEX_WORD     1
#define BRW_INDEX_DWORD    2

#define BRW_LOGICOPFUNCTION_CLEAR            0
#define BRW_LOGICOPFUNCTION_NOR              1
#define BRW_LOGICOPFUNCTION_AND_INVERTED     2
#define BRW_LOGICOPFUNCTION_COPY_INVERTED    3
#define BRW_LOGICOPFUNCTION_AND_REVERSE      4
#define BRW_LOGICOPFUNCTION_INVERT           5
#define BRW_LOGICOPFUNCTION_XOR              6
#define BRW_LOGICOPFUNCTION_NAND             7
#define BRW_LOGICOPFUNCTION_AND              8
#define BRW_LOGICOPFUNCTION_EQUIV            9
#define BRW_LOGICOPFUNCTION_NOOP             10
#define BRW_LOGICOPFUNCTION_OR_INVERTED      11
#define BRW_LOGICOPFUNCTION_COPY             12
#define BRW_LOGICOPFUNCTION_OR_REVERSE       13
#define BRW_LOGICOPFUNCTION_OR               14
#define BRW_LOGICOPFUNCTION_SET              15

#define BRW_MAPFILTER_NEAREST        0x0
#define BRW_MAPFILTER_LINEAR         0x1
#define BRW_MAPFILTER_ANISOTROPIC    0x2

#define BRW_MIPFILTER_NONE        0
#define BRW_MIPFILTER_NEAREST     1
#define BRW_MIPFILTER_LINEAR      3

#define BRW_ADDRESS_ROUNDING_ENABLE_U_MAG	0x20
#define BRW_ADDRESS_ROUNDING_ENABLE_U_MIN	0x10
#define BRW_ADDRESS_ROUNDING_ENABLE_V_MAG	0x08
#define BRW_ADDRESS_ROUNDING_ENABLE_V_MIN	0x04
#define BRW_ADDRESS_ROUNDING_ENABLE_R_MAG	0x02
#define BRW_ADDRESS_ROUNDING_ENABLE_R_MIN	0x01

#define BRW_PREFILTER_ALWAYS     0x0
#define BRW_PREFILTER_NEVER      0x1
#define BRW_PREFILTER_LESS       0x2
#define BRW_PREFILTER_EQUAL      0x3
#define BRW_PREFILTER_LEQUAL     0x4
#define BRW_PREFILTER_GREATER    0x5
#define BRW_PREFILTER_NOTEQUAL   0x6
#define BRW_PREFILTER_GEQUAL     0x7

#define BRW_PROVOKING_VERTEX_0    0
#define BRW_PROVOKING_VERTEX_1    1
#define BRW_PROVOKING_VERTEX_2    2

#define BRW_RASTRULE_UPPER_LEFT  0
#define BRW_RASTRULE_UPPER_RIGHT 1
/* These are listed as "Reserved, but not seen as useful"
 * in Intel documentation (page 212, "Point Rasterization Rule",
 * section 7.4 "SF Pipeline State Summary", of document
 * "Intel® 965 Express Chipset Family and Intel® G35 Express
 * Chipset Graphics Controller Programmer's Reference Manual,
 * Volume 2: 3D/Media", Revision 1.0b as of January 2008,
 * available at
 *     https://01.org/linuxgraphics/documentation/hardware-specification-prms
 * at the time of this writing).
 *
 * These appear to be supported on at least some
 * i965-family devices, and the BRW_RASTRULE_LOWER_RIGHT
 * is useful when using OpenGL to render to a FBO
 * (which has the pixel coordinate Y orientation inverted
 * with respect to the normal OpenGL pixel coordinate system).
 */
#define BRW_RASTRULE_LOWER_LEFT  2
#define BRW_RASTRULE_LOWER_RIGHT 3

#define BRW_RENDERTARGET_CLAMPRANGE_UNORM    0
#define BRW_RENDERTARGET_CLAMPRANGE_SNORM    1
#define BRW_RENDERTARGET_CLAMPRANGE_FORMAT   2

#define BRW_STENCILOP_KEEP               0
#define BRW_STENCILOP_ZERO               1
#define BRW_STENCILOP_REPLACE            2
#define BRW_STENCILOP_INCRSAT            3
#define BRW_STENCILOP_DECRSAT            4
#define BRW_STENCILOP_INCR               5
#define BRW_STENCILOP_DECR               6
#define BRW_STENCILOP_INVERT             7

/* Surface state DW0 */
#define GFX8_SURFACE_IS_ARRAY                       (1 << 28)
#define GFX8_SURFACE_VALIGN_4                       (1 << 16)
#define GFX8_SURFACE_VALIGN_8                       (2 << 16)
#define GFX8_SURFACE_VALIGN_16                      (3 << 16)
#define GFX8_SURFACE_HALIGN_4                       (1 << 14)
#define GFX8_SURFACE_HALIGN_8                       (2 << 14)
#define GFX8_SURFACE_HALIGN_16                      (3 << 14)
#define GFX8_SURFACE_TILING_NONE                    (0 << 12)
#define GFX8_SURFACE_TILING_W                       (1 << 12)
#define GFX8_SURFACE_TILING_X                       (2 << 12)
#define GFX8_SURFACE_TILING_Y                       (3 << 12)
#define GFX8_SURFACE_SAMPLER_L2_BYPASS_DISABLE      (1 << 9)
#define BRW_SURFACE_RC_READ_WRITE	(1 << 8)
#define BRW_SURFACE_MIPLAYOUT_SHIFT	10
#define BRW_SURFACE_MIPMAPLAYOUT_BELOW   0
#define BRW_SURFACE_MIPMAPLAYOUT_RIGHT   1
#define BRW_SURFACE_CUBEFACE_ENABLES	0x3f
#define BRW_SURFACE_BLEND_ENABLED	(1 << 13)
#define BRW_SURFACE_WRITEDISABLE_B_SHIFT	14
#define BRW_SURFACE_WRITEDISABLE_G_SHIFT	15
#define BRW_SURFACE_WRITEDISABLE_R_SHIFT	16
#define BRW_SURFACE_WRITEDISABLE_A_SHIFT	17

#define GFX9_SURFACE_ASTC_HDR_FORMAT_BIT                 0x100

#define BRW_SURFACE_FORMAT_SHIFT	18
#define BRW_SURFACE_FORMAT_MASK		INTEL_MASK(26, 18)

#define BRW_SURFACERETURNFORMAT_FLOAT32  0
#define BRW_SURFACERETURNFORMAT_S1       1

#define BRW_SURFACE_TYPE_SHIFT		29
#define BRW_SURFACE_TYPE_MASK		INTEL_MASK(31, 29)
#define BRW_SURFACE_1D      0
#define BRW_SURFACE_2D      1
#define BRW_SURFACE_3D      2
#define BRW_SURFACE_CUBE    3
#define BRW_SURFACE_BUFFER  4
#define BRW_SURFACE_NULL    7

#define GFX7_SURFACE_IS_ARRAY           (1 << 28)
#define GFX7_SURFACE_VALIGN_2           (0 << 16)
#define GFX7_SURFACE_VALIGN_4           (1 << 16)
#define GFX7_SURFACE_HALIGN_4           (0 << 15)
#define GFX7_SURFACE_HALIGN_8           (1 << 15)
#define GFX7_SURFACE_TILING_NONE        (0 << 13)
#define GFX7_SURFACE_TILING_X           (2 << 13)
#define GFX7_SURFACE_TILING_Y           (3 << 13)
#define GFX7_SURFACE_ARYSPC_FULL	(0 << 10)
#define GFX7_SURFACE_ARYSPC_LOD0	(1 << 10)

/* Surface state DW1 */
#define GFX8_SURFACE_MOCS_SHIFT         24
#define GFX8_SURFACE_MOCS_MASK          INTEL_MASK(30, 24)
#define GFX8_SURFACE_QPITCH_SHIFT       0
#define GFX8_SURFACE_QPITCH_MASK        INTEL_MASK(14, 0)

/* Surface state DW2 */
#define BRW_SURFACE_HEIGHT_SHIFT	19
#define BRW_SURFACE_HEIGHT_MASK		INTEL_MASK(31, 19)
#define BRW_SURFACE_WIDTH_SHIFT		6
#define BRW_SURFACE_WIDTH_MASK		INTEL_MASK(18, 6)
#define BRW_SURFACE_LOD_SHIFT		2
#define BRW_SURFACE_LOD_MASK		INTEL_MASK(5, 2)
#define GFX7_SURFACE_HEIGHT_SHIFT       16
#define GFX7_SURFACE_HEIGHT_MASK        INTEL_MASK(29, 16)
#define GFX7_SURFACE_WIDTH_SHIFT        0
#define GFX7_SURFACE_WIDTH_MASK         INTEL_MASK(13, 0)

/* Surface state DW3 */
#define BRW_SURFACE_DEPTH_SHIFT		21
#define BRW_SURFACE_DEPTH_MASK		INTEL_MASK(31, 21)
#define BRW_SURFACE_PITCH_SHIFT		3
#define BRW_SURFACE_PITCH_MASK		INTEL_MASK(19, 3)
#define BRW_SURFACE_TILED		(1 << 1)
#define BRW_SURFACE_TILED_Y		(1 << 0)
#define HSW_SURFACE_IS_INTEGER_FORMAT   (1 << 18)

/* Surface state DW4 */
#define BRW_SURFACE_MIN_LOD_SHIFT	28
#define BRW_SURFACE_MIN_LOD_MASK	INTEL_MASK(31, 28)
#define BRW_SURFACE_MIN_ARRAY_ELEMENT_SHIFT	17
#define BRW_SURFACE_MIN_ARRAY_ELEMENT_MASK	INTEL_MASK(27, 17)
#define BRW_SURFACE_RENDER_TARGET_VIEW_EXTENT_SHIFT	8
#define BRW_SURFACE_RENDER_TARGET_VIEW_EXTENT_MASK	INTEL_MASK(16, 8)
#define BRW_SURFACE_MULTISAMPLECOUNT_1  (0 << 4)
#define BRW_SURFACE_MULTISAMPLECOUNT_4  (2 << 4)
#define GFX7_SURFACE_MULTISAMPLECOUNT_1         (0 << 3)
#define GFX8_SURFACE_MULTISAMPLECOUNT_2         (1 << 3)
#define GFX7_SURFACE_MULTISAMPLECOUNT_4         (2 << 3)
#define GFX7_SURFACE_MULTISAMPLECOUNT_8         (3 << 3)
#define GFX8_SURFACE_MULTISAMPLECOUNT_16        (4 << 3)
#define GFX7_SURFACE_MSFMT_MSS                  (0 << 6)
#define GFX7_SURFACE_MSFMT_DEPTH_STENCIL        (1 << 6)
#define GFX7_SURFACE_MIN_ARRAY_ELEMENT_SHIFT	18
#define GFX7_SURFACE_MIN_ARRAY_ELEMENT_MASK     INTEL_MASK(28, 18)
#define GFX7_SURFACE_RENDER_TARGET_VIEW_EXTENT_SHIFT	7
#define GFX7_SURFACE_RENDER_TARGET_VIEW_EXTENT_MASK   INTEL_MASK(17, 7)

/* Surface state DW5 */
#define BRW_SURFACE_X_OFFSET_SHIFT		25
#define BRW_SURFACE_X_OFFSET_MASK		INTEL_MASK(31, 25)
#define BRW_SURFACE_VERTICAL_ALIGN_ENABLE	(1 << 24)
#define BRW_SURFACE_Y_OFFSET_SHIFT		20
#define BRW_SURFACE_Y_OFFSET_MASK		INTEL_MASK(23, 20)
#define GFX7_SURFACE_MIN_LOD_SHIFT              4
#define GFX7_SURFACE_MIN_LOD_MASK               INTEL_MASK(7, 4)
#define GFX8_SURFACE_Y_OFFSET_SHIFT		21
#define GFX8_SURFACE_Y_OFFSET_MASK		INTEL_MASK(23, 21)

#define GFX7_SURFACE_MOCS_SHIFT                 16
#define GFX7_SURFACE_MOCS_MASK                  INTEL_MASK(19, 16)

#define GFX9_SURFACE_MIP_TAIL_START_LOD_SHIFT      8
#define GFX9_SURFACE_MIP_TAIL_START_LOD_MASK       INTEL_MASK(11, 8)

/* Surface state DW6 */
#define GFX7_SURFACE_MCS_ENABLE                 (1 << 0)
#define GFX7_SURFACE_MCS_PITCH_SHIFT            3
#define GFX7_SURFACE_MCS_PITCH_MASK             INTEL_MASK(11, 3)
#define GFX8_SURFACE_AUX_QPITCH_SHIFT           16
#define GFX8_SURFACE_AUX_QPITCH_MASK            INTEL_MASK(30, 16)
#define GFX8_SURFACE_AUX_PITCH_SHIFT            3
#define GFX8_SURFACE_AUX_PITCH_MASK             INTEL_MASK(11, 3)
#define GFX8_SURFACE_AUX_MODE_MASK              INTEL_MASK(2, 0)

#define GFX8_SURFACE_AUX_MODE_NONE              0
#define GFX8_SURFACE_AUX_MODE_MCS               1
#define GFX8_SURFACE_AUX_MODE_APPEND            2
#define GFX8_SURFACE_AUX_MODE_HIZ               3
#define GFX9_SURFACE_AUX_MODE_CCS_E             5

/* Surface state DW7 */
#define GFX9_SURFACE_RT_COMPRESSION_SHIFT       30
#define GFX9_SURFACE_RT_COMPRESSION_MASK        INTEL_MASK(30, 30)
#define GFX7_SURFACE_CLEAR_COLOR_SHIFT		28
#define GFX7_SURFACE_SCS_R_SHIFT                25
#define GFX7_SURFACE_SCS_R_MASK                 INTEL_MASK(27, 25)
#define GFX7_SURFACE_SCS_G_SHIFT                22
#define GFX7_SURFACE_SCS_G_MASK                 INTEL_MASK(24, 22)
#define GFX7_SURFACE_SCS_B_SHIFT                19
#define GFX7_SURFACE_SCS_B_MASK                 INTEL_MASK(21, 19)
#define GFX7_SURFACE_SCS_A_SHIFT                16
#define GFX7_SURFACE_SCS_A_MASK                 INTEL_MASK(18, 16)

/* The actual swizzle values/what channel to use */
#define HSW_SCS_ZERO                     0
#define HSW_SCS_ONE                      1
#define HSW_SCS_RED                      4
#define HSW_SCS_GREEN                    5
#define HSW_SCS_BLUE                     6
#define HSW_SCS_ALPHA                    7

/* SAMPLER_STATE DW0 */
#define BRW_SAMPLER_DISABLE                     (1 << 31)
#define BRW_SAMPLER_LOD_PRECLAMP_ENABLE         (1 << 28)
#define GFX6_SAMPLER_MIN_MAG_NOT_EQUAL          (1 << 27) /* Gfx6 only */
#define BRW_SAMPLER_BASE_MIPLEVEL_MASK          INTEL_MASK(26, 22)
#define BRW_SAMPLER_BASE_MIPLEVEL_SHIFT         22
#define BRW_SAMPLER_MIP_FILTER_MASK             INTEL_MASK(21, 20)
#define BRW_SAMPLER_MIP_FILTER_SHIFT            20
#define BRW_SAMPLER_MAG_FILTER_MASK             INTEL_MASK(19, 17)
#define BRW_SAMPLER_MAG_FILTER_SHIFT            17
#define BRW_SAMPLER_MIN_FILTER_MASK             INTEL_MASK(16, 14)
#define BRW_SAMPLER_MIN_FILTER_SHIFT            14
#define GFX4_SAMPLER_LOD_BIAS_MASK              INTEL_MASK(13, 3)
#define GFX4_SAMPLER_LOD_BIAS_SHIFT             3
#define GFX4_SAMPLER_SHADOW_FUNCTION_MASK       INTEL_MASK(2, 0)
#define GFX4_SAMPLER_SHADOW_FUNCTION_SHIFT      0

#define GFX7_SAMPLER_LOD_BIAS_MASK              INTEL_MASK(13, 1)
#define GFX7_SAMPLER_LOD_BIAS_SHIFT             1
#define GFX7_SAMPLER_EWA_ANISOTROPIC_ALGORITHM  (1 << 0)

/* SAMPLER_STATE DW1 */
#define GFX4_SAMPLER_MIN_LOD_MASK               INTEL_MASK(31, 22)
#define GFX4_SAMPLER_MIN_LOD_SHIFT              22
#define GFX4_SAMPLER_MAX_LOD_MASK               INTEL_MASK(21, 12)
#define GFX4_SAMPLER_MAX_LOD_SHIFT              12
#define GFX4_SAMPLER_CUBE_CONTROL_OVERRIDE      (1 << 9)
/* Wrap modes are in DW1 on Gfx4-6 and DW3 on Gfx7+ */
#define BRW_SAMPLER_TCX_WRAP_MODE_MASK          INTEL_MASK(8, 6)
#define BRW_SAMPLER_TCX_WRAP_MODE_SHIFT         6
#define BRW_SAMPLER_TCY_WRAP_MODE_MASK          INTEL_MASK(5, 3)
#define BRW_SAMPLER_TCY_WRAP_MODE_SHIFT         3
#define BRW_SAMPLER_TCZ_WRAP_MODE_MASK          INTEL_MASK(2, 0)
#define BRW_SAMPLER_TCZ_WRAP_MODE_SHIFT         0

#define GFX7_SAMPLER_MIN_LOD_MASK               INTEL_MASK(31, 20)
#define GFX7_SAMPLER_MIN_LOD_SHIFT              20
#define GFX7_SAMPLER_MAX_LOD_MASK               INTEL_MASK(19, 8)
#define GFX7_SAMPLER_MAX_LOD_SHIFT              8
#define GFX7_SAMPLER_SHADOW_FUNCTION_MASK       INTEL_MASK(3, 1)
#define GFX7_SAMPLER_SHADOW_FUNCTION_SHIFT      1
#define GFX7_SAMPLER_CUBE_CONTROL_OVERRIDE      (1 << 0)

/* SAMPLER_STATE DW2 - border color pointer */

/* SAMPLER_STATE DW3 */
#define BRW_SAMPLER_MAX_ANISOTROPY_MASK         INTEL_MASK(21, 19)
#define BRW_SAMPLER_MAX_ANISOTROPY_SHIFT        19
#define BRW_SAMPLER_ADDRESS_ROUNDING_MASK       INTEL_MASK(18, 13)
#define BRW_SAMPLER_ADDRESS_ROUNDING_SHIFT      13
#define GFX7_SAMPLER_NON_NORMALIZED_COORDINATES (1 << 10)
/* Gfx7+ wrap modes reuse the same BRW_SAMPLER_TC*_WRAP_MODE enums. */
#define GFX6_SAMPLER_NON_NORMALIZED_COORDINATES (1 << 0)

enum brw_wrap_mode {
   BRW_TEXCOORDMODE_WRAP         = 0,
   BRW_TEXCOORDMODE_MIRROR       = 1,
   BRW_TEXCOORDMODE_CLAMP        = 2,
   BRW_TEXCOORDMODE_CUBE         = 3,
   BRW_TEXCOORDMODE_CLAMP_BORDER = 4,
   BRW_TEXCOORDMODE_MIRROR_ONCE  = 5,
   GFX8_TEXCOORDMODE_HALF_BORDER = 6,
};

#define BRW_THREAD_PRIORITY_NORMAL   0
#define BRW_THREAD_PRIORITY_HIGH     1

#define BRW_TILEWALK_XMAJOR                 0
#define BRW_TILEWALK_YMAJOR                 1

#define BRW_VERTEX_SUBPIXEL_PRECISION_8BITS  0
#define BRW_VERTEX_SUBPIXEL_PRECISION_4BITS  1


#define CMD_URB_FENCE                 0x6000
#define CMD_CS_URB_STATE              0x6001
#define CMD_CONST_BUFFER              0x6002

#define CMD_STATE_BASE_ADDRESS        0x6101
#define CMD_STATE_SIP                 0x6102
#define CMD_PIPELINE_SELECT_965       0x6104
#define CMD_PIPELINE_SELECT_GM45      0x6904

#define _3DSTATE_PIPELINED_POINTERS		0x7800
#define _3DSTATE_BINDING_TABLE_POINTERS		0x7801
# define GFX6_BINDING_TABLE_MODIFY_VS	(1 << 8)
# define GFX6_BINDING_TABLE_MODIFY_GS	(1 << 9)
# define GFX6_BINDING_TABLE_MODIFY_PS	(1 << 12)

#define _3DSTATE_BINDING_TABLE_POINTERS_VS	0x7826 /* GFX7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_HS	0x7827 /* GFX7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_DS	0x7828 /* GFX7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_GS	0x7829 /* GFX7+ */
#define _3DSTATE_BINDING_TABLE_POINTERS_PS	0x782A /* GFX7+ */

#define _3DSTATE_SAMPLER_STATE_POINTERS		0x7802 /* GFX6+ */
# define PS_SAMPLER_STATE_CHANGE				(1 << 12)
# define GS_SAMPLER_STATE_CHANGE				(1 << 9)
# define VS_SAMPLER_STATE_CHANGE				(1 << 8)
/* DW1: VS */
/* DW2: GS */
/* DW3: PS */

#define _3DSTATE_SAMPLER_STATE_POINTERS_VS	0x782B /* GFX7+ */
#define _3DSTATE_SAMPLER_STATE_POINTERS_HS	0x782C /* GFX7+ */
#define _3DSTATE_SAMPLER_STATE_POINTERS_DS	0x782D /* GFX7+ */
#define _3DSTATE_SAMPLER_STATE_POINTERS_GS	0x782E /* GFX7+ */
#define _3DSTATE_SAMPLER_STATE_POINTERS_PS	0x782F /* GFX7+ */

#define _3DSTATE_VERTEX_BUFFERS       0x7808
# define BRW_VB0_INDEX_SHIFT		27
# define GFX6_VB0_INDEX_SHIFT		26
# define BRW_VB0_ACCESS_VERTEXDATA	(0 << 26)
# define BRW_VB0_ACCESS_INSTANCEDATA	(1 << 26)
# define GFX6_VB0_ACCESS_VERTEXDATA	(0 << 20)
# define GFX6_VB0_ACCESS_INSTANCEDATA	(1 << 20)
# define GFX7_VB0_ADDRESS_MODIFYENABLE  (1 << 14)
# define BRW_VB0_PITCH_SHIFT		0

#define _3DSTATE_VERTEX_ELEMENTS      0x7809
# define BRW_VE0_INDEX_SHIFT		27
# define GFX6_VE0_INDEX_SHIFT		26
# define BRW_VE0_FORMAT_SHIFT		16
# define BRW_VE0_VALID			(1 << 26)
# define GFX6_VE0_VALID			(1 << 25)
# define GFX6_VE0_EDGE_FLAG_ENABLE	(1 << 15)
# define BRW_VE0_SRC_OFFSET_SHIFT	0
# define BRW_VE1_COMPONENT_NOSTORE	0
# define BRW_VE1_COMPONENT_STORE_SRC	1
# define BRW_VE1_COMPONENT_STORE_0	2
# define BRW_VE1_COMPONENT_STORE_1_FLT	3
# define BRW_VE1_COMPONENT_STORE_1_INT	4
# define BRW_VE1_COMPONENT_STORE_VID	5
# define BRW_VE1_COMPONENT_STORE_IID	6
# define BRW_VE1_COMPONENT_STORE_PID	7
# define BRW_VE1_COMPONENT_0_SHIFT	28
# define BRW_VE1_COMPONENT_1_SHIFT	24
# define BRW_VE1_COMPONENT_2_SHIFT	20
# define BRW_VE1_COMPONENT_3_SHIFT	16
# define BRW_VE1_DST_OFFSET_SHIFT	0

#define CMD_INDEX_BUFFER              0x780a
#define GFX4_3DSTATE_VF_STATISTICS		0x780b
#define GM45_3DSTATE_VF_STATISTICS		0x680b
#define _3DSTATE_CC_STATE_POINTERS		0x780e /* GFX6+ */
#define _3DSTATE_BLEND_STATE_POINTERS		0x7824 /* GFX7+ */
#define _3DSTATE_DEPTH_STENCIL_STATE_POINTERS	0x7825 /* GFX7+ */

#define _3DSTATE_URB				0x7805 /* GFX6 */
# define GFX6_URB_VS_SIZE_SHIFT				16
# define GFX6_URB_VS_ENTRIES_SHIFT			0
# define GFX6_URB_GS_ENTRIES_SHIFT			8
# define GFX6_URB_GS_SIZE_SHIFT				0

#define _3DSTATE_VF                             0x780c /* GFX7.5+ */
#define HSW_CUT_INDEX_ENABLE                            (1 << 8)

#define _3DSTATE_VF_INSTANCING                  0x7849 /* GFX8+ */
# define GFX8_VF_INSTANCING_ENABLE                      (1 << 8)

#define _3DSTATE_VF_SGVS                        0x784a /* GFX8+ */
# define GFX8_SGVS_ENABLE_INSTANCE_ID                   (1 << 31)
# define GFX8_SGVS_INSTANCE_ID_COMPONENT_SHIFT          29
# define GFX8_SGVS_INSTANCE_ID_ELEMENT_OFFSET_SHIFT     16
# define GFX8_SGVS_ENABLE_VERTEX_ID                     (1 << 15)
# define GFX8_SGVS_VERTEX_ID_COMPONENT_SHIFT            13
# define GFX8_SGVS_VERTEX_ID_ELEMENT_OFFSET_SHIFT       0

#define _3DSTATE_VF_TOPOLOGY                    0x784b /* GFX8+ */

#define _3DSTATE_WM_CHROMAKEY			0x784c /* GFX8+ */

#define _3DSTATE_URB_VS                         0x7830 /* GFX7+ */
#define _3DSTATE_URB_HS                         0x7831 /* GFX7+ */
#define _3DSTATE_URB_DS                         0x7832 /* GFX7+ */
#define _3DSTATE_URB_GS                         0x7833 /* GFX7+ */
# define GFX7_URB_ENTRY_SIZE_SHIFT                      16
# define GFX7_URB_STARTING_ADDRESS_SHIFT                25

#define _3DSTATE_PUSH_CONSTANT_ALLOC_VS         0x7912 /* GFX7+ */
#define _3DSTATE_PUSH_CONSTANT_ALLOC_HS         0x7913 /* GFX7+ */
#define _3DSTATE_PUSH_CONSTANT_ALLOC_DS         0x7914 /* GFX7+ */
#define _3DSTATE_PUSH_CONSTANT_ALLOC_GS         0x7915 /* GFX7+ */
#define _3DSTATE_PUSH_CONSTANT_ALLOC_PS         0x7916 /* GFX7+ */
# define GFX7_PUSH_CONSTANT_BUFFER_OFFSET_SHIFT         16

#define _3DSTATE_VIEWPORT_STATE_POINTERS	0x780d /* GFX6+ */
# define GFX6_CC_VIEWPORT_MODIFY			(1 << 12)
# define GFX6_SF_VIEWPORT_MODIFY			(1 << 11)
# define GFX6_CLIP_VIEWPORT_MODIFY			(1 << 10)
# define GFX6_NUM_VIEWPORTS				16

#define _3DSTATE_VIEWPORT_STATE_POINTERS_CC	0x7823 /* GFX7+ */
#define _3DSTATE_VIEWPORT_STATE_POINTERS_SF_CL	0x7821 /* GFX7+ */

#define _3DSTATE_SCISSOR_STATE_POINTERS		0x780f /* GFX6+ */

#define _3DSTATE_VS				0x7810 /* GFX6+ */
/* DW2 */
# define GFX6_VS_SPF_MODE				(1 << 31)
# define GFX6_VS_VECTOR_MASK_ENABLE			(1 << 30)
# define GFX6_VS_SAMPLER_COUNT_SHIFT			27
# define GFX6_VS_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GFX6_VS_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GFX6_VS_FLOATING_POINT_MODE_ALT		(1 << 16)
# define HSW_VS_UAV_ACCESS_ENABLE                       (1 << 12)
/* DW4 */
# define GFX6_VS_DISPATCH_START_GRF_SHIFT		20
# define GFX6_VS_URB_READ_LENGTH_SHIFT			11
# define GFX6_VS_URB_ENTRY_READ_OFFSET_SHIFT		4
/* DW5 */
# define GFX6_VS_MAX_THREADS_SHIFT			25
# define HSW_VS_MAX_THREADS_SHIFT			23
# define GFX6_VS_STATISTICS_ENABLE			(1 << 10)
# define GFX6_VS_CACHE_DISABLE				(1 << 1)
# define GFX6_VS_ENABLE					(1 << 0)
/* Gfx8+ DW7 */
# define GFX8_VS_SIMD8_ENABLE                           (1 << 2)
/* Gfx8+ DW8 */
# define GFX8_VS_URB_ENTRY_OUTPUT_OFFSET_SHIFT          21
# define GFX8_VS_URB_OUTPUT_LENGTH_SHIFT                16
# define GFX8_VS_USER_CLIP_DISTANCE_SHIFT               8

#define _3DSTATE_GS		      		0x7811 /* GFX6+ */
/* DW2 */
# define GFX6_GS_SPF_MODE				(1 << 31)
# define GFX6_GS_VECTOR_MASK_ENABLE			(1 << 30)
# define GFX6_GS_SAMPLER_COUNT_SHIFT			27
# define GFX6_GS_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GFX6_GS_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GFX6_GS_FLOATING_POINT_MODE_ALT		(1 << 16)
# define HSW_GS_UAV_ACCESS_ENABLE       		(1 << 12)
/* DW4 */
# define GFX7_GS_OUTPUT_VERTEX_SIZE_SHIFT		23
# define GFX7_GS_OUTPUT_TOPOLOGY_SHIFT			17
# define GFX6_GS_URB_READ_LENGTH_SHIFT			11
# define GFX7_GS_INCLUDE_VERTEX_HANDLES		        (1 << 10)
# define GFX6_GS_URB_ENTRY_READ_OFFSET_SHIFT		4
# define GFX6_GS_DISPATCH_START_GRF_SHIFT		0
/* DW5 */
# define GFX6_GS_MAX_THREADS_SHIFT			25
# define HSW_GS_MAX_THREADS_SHIFT			24
# define IVB_GS_CONTROL_DATA_FORMAT_SHIFT		24
# define GFX7_GS_CONTROL_DATA_FORMAT_GSCTL_CUT		0
# define GFX7_GS_CONTROL_DATA_FORMAT_GSCTL_SID		1
# define GFX7_GS_CONTROL_DATA_HEADER_SIZE_SHIFT		20
# define GFX7_GS_INSTANCE_CONTROL_SHIFT			15
# define GFX7_GS_DISPATCH_MODE_SHIFT                    11
# define GFX7_GS_DISPATCH_MODE_MASK                     INTEL_MASK(12, 11)
# define GFX6_GS_STATISTICS_ENABLE			(1 << 10)
# define GFX6_GS_SO_STATISTICS_ENABLE			(1 << 9)
# define GFX6_GS_RENDERING_ENABLE			(1 << 8)
# define GFX7_GS_INCLUDE_PRIMITIVE_ID			(1 << 4)
# define GFX7_GS_REORDER_TRAILING			(1 << 2)
# define GFX7_GS_ENABLE					(1 << 0)
/* DW6 */
# define HSW_GS_CONTROL_DATA_FORMAT_SHIFT		31
# define GFX6_GS_REORDER				(1 << 30)
# define GFX6_GS_DISCARD_ADJACENCY			(1 << 29)
# define GFX6_GS_SVBI_PAYLOAD_ENABLE			(1 << 28)
# define GFX6_GS_SVBI_POSTINCREMENT_ENABLE		(1 << 27)
# define GFX6_GS_SVBI_POSTINCREMENT_VALUE_SHIFT		16
# define GFX6_GS_SVBI_POSTINCREMENT_VALUE_MASK		INTEL_MASK(25, 16)
# define GFX6_GS_ENABLE					(1 << 15)

/* Gfx8+ DW8 */
# define GFX8_GS_STATIC_OUTPUT                          (1 << 30)
# define GFX8_GS_STATIC_VERTEX_COUNT_SHIFT              16
# define GFX8_GS_STATIC_VERTEX_COUNT_MASK               INTEL_MASK(26, 16)

/* Gfx8+ DW9 */
# define GFX8_GS_URB_ENTRY_OUTPUT_OFFSET_SHIFT          21
# define GFX8_GS_URB_OUTPUT_LENGTH_SHIFT                16
# define GFX8_GS_USER_CLIP_DISTANCE_SHIFT               8

# define BRW_GS_EDGE_INDICATOR_0			(1 << 8)
# define BRW_GS_EDGE_INDICATOR_1			(1 << 9)

#define _3DSTATE_HS                             0x781B /* GFX7+ */
/* DW1 */
# define GFX7_HS_SAMPLER_COUNT_MASK                     INTEL_MASK(29, 27)
# define GFX7_HS_SAMPLER_COUNT_SHIFT                    27
# define GFX7_HS_BINDING_TABLE_ENTRY_COUNT_MASK         INTEL_MASK(25, 18)
# define GFX7_HS_BINDING_TABLE_ENTRY_COUNT_SHIFT        18
# define GFX7_HS_FLOATING_POINT_MODE_IEEE_754           (0 << 16)
# define GFX7_HS_FLOATING_POINT_MODE_ALT                (1 << 16)
# define GFX7_HS_MAX_THREADS_SHIFT                      0
/* DW2 */
# define GFX7_HS_ENABLE                                 (1 << 31)
# define GFX7_HS_STATISTICS_ENABLE                      (1 << 29)
# define GFX8_HS_MAX_THREADS_SHIFT                      8
# define GFX7_HS_INSTANCE_COUNT_MASK                    INTEL_MASK(3, 0)
# define GFX7_HS_INSTANCE_COUNT_SHIFT                   0
/* DW5 */
# define GFX7_HS_SINGLE_PROGRAM_FLOW                    (1 << 27)
# define GFX7_HS_VECTOR_MASK_ENABLE                     (1 << 26)
# define HSW_HS_ACCESSES_UAV                            (1 << 25)
# define GFX7_HS_INCLUDE_VERTEX_HANDLES                 (1 << 24)
# define GFX7_HS_DISPATCH_START_GRF_MASK                INTEL_MASK(23, 19)
# define GFX7_HS_DISPATCH_START_GRF_SHIFT               19
# define GFX7_HS_URB_READ_LENGTH_MASK                   INTEL_MASK(16, 11)
# define GFX7_HS_URB_READ_LENGTH_SHIFT                  11
# define GFX7_HS_URB_ENTRY_READ_OFFSET_MASK             INTEL_MASK(9, 4)
# define GFX7_HS_URB_ENTRY_READ_OFFSET_SHIFT            4

#define _3DSTATE_TE                             0x781C /* GFX7+ */
/* DW1 */
# define GFX7_TE_PARTITIONING_SHIFT                     12
# define GFX7_TE_OUTPUT_TOPOLOGY_SHIFT                  8
# define GFX7_TE_DOMAIN_SHIFT                           4
//# define GFX7_TE_MODE_SW                                (1 << 1)
# define GFX7_TE_ENABLE                                 (1 << 0)

#define _3DSTATE_DS                             0x781D /* GFX7+ */
/* DW2 */
# define GFX7_DS_SINGLE_DOMAIN_POINT_DISPATCH           (1 << 31)
# define GFX7_DS_VECTOR_MASK_ENABLE                     (1 << 30)
# define GFX7_DS_SAMPLER_COUNT_MASK                     INTEL_MASK(29, 27)
# define GFX7_DS_SAMPLER_COUNT_SHIFT                    27
# define GFX7_DS_BINDING_TABLE_ENTRY_COUNT_MASK         INTEL_MASK(25, 18)
# define GFX7_DS_BINDING_TABLE_ENTRY_COUNT_SHIFT        18
# define GFX7_DS_FLOATING_POINT_MODE_IEEE_754           (0 << 16)
# define GFX7_DS_FLOATING_POINT_MODE_ALT                (1 << 16)
# define HSW_DS_ACCESSES_UAV                            (1 << 14)
/* DW4 */
# define GFX7_DS_DISPATCH_START_GRF_MASK                INTEL_MASK(24, 20)
# define GFX7_DS_DISPATCH_START_GRF_SHIFT               20
# define GFX7_DS_URB_READ_LENGTH_MASK                   INTEL_MASK(17, 11)
# define GFX7_DS_URB_READ_LENGTH_SHIFT                  11
# define GFX7_DS_URB_ENTRY_READ_OFFSET_MASK             INTEL_MASK(9, 4)
# define GFX7_DS_URB_ENTRY_READ_OFFSET_SHIFT            4
/* DW5 */
# define GFX7_DS_MAX_THREADS_SHIFT                      25
# define HSW_DS_MAX_THREADS_SHIFT                       21
# define GFX7_DS_STATISTICS_ENABLE                      (1 << 10)
# define GFX7_DS_SIMD8_DISPATCH_ENABLE                  (1 << 3)
# define GFX7_DS_COMPUTE_W_COORDINATE_ENABLE            (1 << 2)
# define GFX7_DS_CACHE_DISABLE                          (1 << 1)
# define GFX7_DS_ENABLE                                 (1 << 0)
/* Gfx8+ DW8 */
# define GFX8_DS_URB_ENTRY_OUTPUT_OFFSET_MASK           INTEL_MASK(26, 21)
# define GFX8_DS_URB_ENTRY_OUTPUT_OFFSET_SHIFT          21
# define GFX8_DS_URB_OUTPUT_LENGTH_MASK                 INTEL_MASK(20, 16)
# define GFX8_DS_URB_OUTPUT_LENGTH_SHIFT                16
# define GFX8_DS_USER_CLIP_DISTANCE_MASK                INTEL_MASK(15, 8)
# define GFX8_DS_USER_CLIP_DISTANCE_SHIFT               8
# define GFX8_DS_USER_CULL_DISTANCE_MASK                INTEL_MASK(7, 0)
# define GFX8_DS_USER_CULL_DISTANCE_SHIFT               0


#define _3DSTATE_CLIP				0x7812 /* GFX6+ */
/* DW1 */
# define GFX7_CLIP_WINDING_CW                           (0 << 20)
# define GFX7_CLIP_WINDING_CCW                          (1 << 20)
# define GFX7_CLIP_VERTEX_SUBPIXEL_PRECISION_8          (0 << 19)
# define GFX7_CLIP_VERTEX_SUBPIXEL_PRECISION_4          (1 << 19)
# define GFX7_CLIP_EARLY_CULL                           (1 << 18)
# define GFX8_CLIP_FORCE_USER_CLIP_DISTANCE_BITMASK     (1 << 17)
# define GFX7_CLIP_CULLMODE_BOTH                        (0 << 16)
# define GFX7_CLIP_CULLMODE_NONE                        (1 << 16)
# define GFX7_CLIP_CULLMODE_FRONT                       (2 << 16)
# define GFX7_CLIP_CULLMODE_BACK                        (3 << 16)
# define GFX6_CLIP_STATISTICS_ENABLE			(1 << 10)
/**
 * Just does cheap culling based on the clip distance.  Bits must be
 * disjoint with USER_CLIP_CLIP_DISTANCE bits.
 */
# define GFX6_USER_CLIP_CULL_DISTANCES_SHIFT		0
/* DW2 */
# define GFX6_CLIP_ENABLE				(1 << 31)
# define GFX6_CLIP_API_OGL				(0 << 30)
# define GFX6_CLIP_API_D3D				(1 << 30)
# define GFX6_CLIP_XY_TEST				(1 << 28)
# define GFX6_CLIP_Z_TEST				(1 << 27)
# define GFX6_CLIP_GB_TEST				(1 << 26)
/** 8-bit field of which user clip distances to clip aganist. */
# define GFX6_USER_CLIP_CLIP_DISTANCES_SHIFT		16
# define GFX6_CLIP_MODE_NORMAL				(0 << 13)
# define GFX6_CLIP_MODE_REJECT_ALL			(3 << 13)
# define GFX6_CLIP_MODE_ACCEPT_ALL			(4 << 13)
# define GFX6_CLIP_PERSPECTIVE_DIVIDE_DISABLE		(1 << 9)
# define GFX6_CLIP_NON_PERSPECTIVE_BARYCENTRIC_ENABLE	(1 << 8)
# define GFX6_CLIP_TRI_PROVOKE_SHIFT			4
# define GFX6_CLIP_LINE_PROVOKE_SHIFT			2
# define GFX6_CLIP_TRIFAN_PROVOKE_SHIFT			0
/* DW3 */
# define GFX6_CLIP_MIN_POINT_WIDTH_SHIFT		17
# define GFX6_CLIP_MAX_POINT_WIDTH_SHIFT		6
# define GFX6_CLIP_FORCE_ZERO_RTAINDEX			(1 << 5)
# define GFX6_CLIP_MAX_VP_INDEX_MASK			INTEL_MASK(3, 0)

#define _3DSTATE_SF				0x7813 /* GFX6+ */
/* DW1 (for gfx6) */
# define GFX6_SF_NUM_OUTPUTS_SHIFT			22
# define GFX6_SF_SWIZZLE_ENABLE				(1 << 21)
# define GFX6_SF_POINT_SPRITE_UPPERLEFT			(0 << 20)
# define GFX6_SF_POINT_SPRITE_LOWERLEFT			(1 << 20)
# define GFX9_SF_LINE_WIDTH_SHIFT			12 /* U11.7 */
# define GFX6_SF_URB_ENTRY_READ_LENGTH_SHIFT		11
# define GFX6_SF_URB_ENTRY_READ_OFFSET_SHIFT		4
/* DW2 */
# define GFX6_SF_LEGACY_GLOBAL_DEPTH_BIAS		(1 << 11)
# define GFX6_SF_STATISTICS_ENABLE			(1 << 10)
# define GFX6_SF_GLOBAL_DEPTH_OFFSET_SOLID		(1 << 9)
# define GFX6_SF_GLOBAL_DEPTH_OFFSET_WIREFRAME		(1 << 8)
# define GFX6_SF_GLOBAL_DEPTH_OFFSET_POINT		(1 << 7)
# define GFX6_SF_FRONT_SOLID				(0 << 5)
# define GFX6_SF_FRONT_WIREFRAME			(1 << 5)
# define GFX6_SF_FRONT_POINT				(2 << 5)
# define GFX6_SF_BACK_SOLID				(0 << 3)
# define GFX6_SF_BACK_WIREFRAME				(1 << 3)
# define GFX6_SF_BACK_POINT				(2 << 3)
# define GFX6_SF_VIEWPORT_TRANSFORM_ENABLE		(1 << 1)
# define GFX6_SF_WINDING_CCW				(1 << 0)
/* DW3 */
# define GFX6_SF_LINE_AA_ENABLE				(1 << 31)
# define GFX6_SF_CULL_BOTH				(0 << 29)
# define GFX6_SF_CULL_NONE				(1 << 29)
# define GFX6_SF_CULL_FRONT				(2 << 29)
# define GFX6_SF_CULL_BACK				(3 << 29)
# define GFX6_SF_LINE_WIDTH_SHIFT			18 /* U3.7 */
# define GFX6_SF_LINE_END_CAP_WIDTH_0_5			(0 << 16)
# define GFX6_SF_LINE_END_CAP_WIDTH_1_0			(1 << 16)
# define GFX6_SF_LINE_END_CAP_WIDTH_2_0			(2 << 16)
# define GFX6_SF_LINE_END_CAP_WIDTH_4_0			(3 << 16)
# define GFX6_SF_SCISSOR_ENABLE				(1 << 11)
# define GFX6_SF_MSRAST_OFF_PIXEL			(0 << 8)
# define GFX6_SF_MSRAST_OFF_PATTERN			(1 << 8)
# define GFX6_SF_MSRAST_ON_PIXEL			(2 << 8)
# define GFX6_SF_MSRAST_ON_PATTERN			(3 << 8)
/* DW4 */
# define GFX6_SF_TRI_PROVOKE_SHIFT			29
# define GFX6_SF_LINE_PROVOKE_SHIFT			27
# define GFX6_SF_TRIFAN_PROVOKE_SHIFT			25
# define GFX6_SF_LINE_AA_MODE_MANHATTAN			(0 << 14)
# define GFX6_SF_LINE_AA_MODE_TRUE			(1 << 14)
# define GFX6_SF_VERTEX_SUBPIXEL_8BITS			(0 << 12)
# define GFX6_SF_VERTEX_SUBPIXEL_4BITS			(1 << 12)
# define GFX6_SF_USE_STATE_POINT_WIDTH			(1 << 11)
# define GFX6_SF_POINT_WIDTH_SHIFT			0 /* U8.3 */
/* DW5: depth offset constant */
/* DW6: depth offset scale */
/* DW7: depth offset clamp */
/* DW8 */
# define ATTRIBUTE_1_OVERRIDE_W				(1 << 31)
# define ATTRIBUTE_1_OVERRIDE_Z				(1 << 30)
# define ATTRIBUTE_1_OVERRIDE_Y				(1 << 29)
# define ATTRIBUTE_1_OVERRIDE_X				(1 << 28)
# define ATTRIBUTE_1_CONST_SOURCE_SHIFT			25
# define ATTRIBUTE_1_SWIZZLE_SHIFT			22
# define ATTRIBUTE_1_SOURCE_SHIFT			16
# define ATTRIBUTE_0_OVERRIDE_W				(1 << 15)
# define ATTRIBUTE_0_OVERRIDE_Z				(1 << 14)
# define ATTRIBUTE_0_OVERRIDE_Y				(1 << 13)
# define ATTRIBUTE_0_OVERRIDE_X				(1 << 12)
# define ATTRIBUTE_0_CONST_SOURCE_SHIFT			9
#  define ATTRIBUTE_CONST_0000				0
#  define ATTRIBUTE_CONST_0001_FLOAT			1
#  define ATTRIBUTE_CONST_1111_FLOAT			2
#  define ATTRIBUTE_CONST_PRIM_ID			3
# define ATTRIBUTE_0_SWIZZLE_SHIFT			6
# define ATTRIBUTE_0_SOURCE_SHIFT			0

# define ATTRIBUTE_SWIZZLE_INPUTATTR                    0
# define ATTRIBUTE_SWIZZLE_INPUTATTR_FACING             1
# define ATTRIBUTE_SWIZZLE_INPUTATTR_W                  2
# define ATTRIBUTE_SWIZZLE_INPUTATTR_FACING_W           3
# define ATTRIBUTE_SWIZZLE_SHIFT                        6

/* DW16: Point sprite texture coordinate enables */
/* DW17: Constant interpolation enables */
/* DW18: attr 0-7 wrap shortest enables */
/* DW19: attr 8-16 wrap shortest enables */

/* On GFX7, many fields of 3DSTATE_SF were split out into a new command:
 * 3DSTATE_SBE.  The remaining fields live in different DWords, but retain
 * the same bit-offset.  The only new field:
 */
/* GFX7/DW1: */
# define GFX7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT	12
/* GFX7/DW2: */
# define HSW_SF_LINE_STIPPLE_ENABLE			(1 << 14)

# define GFX8_SF_SMOOTH_POINT_ENABLE                    (1 << 13)

#define _3DSTATE_SBE				0x781F /* GFX7+ */
/* DW1 */
# define GFX8_SBE_FORCE_URB_ENTRY_READ_LENGTH           (1 << 29)
# define GFX8_SBE_FORCE_URB_ENTRY_READ_OFFSET           (1 << 28)
# define GFX7_SBE_SWIZZLE_CONTROL_MODE			(1 << 28)
# define GFX7_SBE_NUM_OUTPUTS_SHIFT			22
# define GFX7_SBE_SWIZZLE_ENABLE			(1 << 21)
# define GFX7_SBE_POINT_SPRITE_LOWERLEFT		(1 << 20)
# define GFX7_SBE_URB_ENTRY_READ_LENGTH_SHIFT		11
# define GFX7_SBE_URB_ENTRY_READ_OFFSET_SHIFT		4
# define GFX8_SBE_URB_ENTRY_READ_OFFSET_SHIFT		5
/* DW2-9: Attribute setup (same as DW8-15 of gfx6 _3DSTATE_SF) */
/* DW10: Point sprite texture coordinate enables */
/* DW11: Constant interpolation enables */
/* DW12: attr 0-7 wrap shortest enables */
/* DW13: attr 8-16 wrap shortest enables */

/* DW4-5: Attribute active components (gfx9) */
#define GFX9_SBE_ACTIVE_COMPONENT_NONE			0
#define GFX9_SBE_ACTIVE_COMPONENT_XY			1
#define GFX9_SBE_ACTIVE_COMPONENT_XYZ			2
#define GFX9_SBE_ACTIVE_COMPONENT_XYZW			3

#define _3DSTATE_SBE_SWIZ                       0x7851 /* GFX8+ */

#define _3DSTATE_RASTER                         0x7850 /* GFX8+ */
/* DW1 */
# define GFX9_RASTER_VIEWPORT_Z_FAR_CLIP_TEST_ENABLE    (1 << 26)
# define GFX9_RASTER_CONSERVATIVE_RASTERIZATION_ENABLE  (1 << 24)
# define GFX8_RASTER_FRONT_WINDING_CCW                  (1 << 21)
# define GFX8_RASTER_CULL_BOTH                          (0 << 16)
# define GFX8_RASTER_CULL_NONE                          (1 << 16)
# define GFX8_RASTER_CULL_FRONT                         (2 << 16)
# define GFX8_RASTER_CULL_BACK                          (3 << 16)
# define GFX8_RASTER_SMOOTH_POINT_ENABLE                (1 << 13)
# define GFX8_RASTER_API_MULTISAMPLE_ENABLE             (1 << 12)
# define GFX8_RASTER_LINE_AA_ENABLE                     (1 << 2)
# define GFX8_RASTER_SCISSOR_ENABLE                     (1 << 1)
# define GFX8_RASTER_VIEWPORT_Z_CLIP_TEST_ENABLE        (1 << 0)
# define GFX9_RASTER_VIEWPORT_Z_NEAR_CLIP_TEST_ENABLE   (1 << 0)

/* Gfx8 BLEND_STATE */
/* DW0 */
#define GFX8_BLEND_ALPHA_TO_COVERAGE_ENABLE             (1 << 31)
#define GFX8_BLEND_INDEPENDENT_ALPHA_BLEND_ENABLE       (1 << 30)
#define GFX8_BLEND_ALPHA_TO_ONE_ENABLE                  (1 << 29)
#define GFX8_BLEND_ALPHA_TO_COVERAGE_DITHER_ENABLE      (1 << 28)
#define GFX8_BLEND_ALPHA_TEST_ENABLE                    (1 << 27)
#define GFX8_BLEND_ALPHA_TEST_FUNCTION_MASK             INTEL_MASK(26, 24)
#define GFX8_BLEND_ALPHA_TEST_FUNCTION_SHIFT            24
#define GFX8_BLEND_COLOR_DITHER_ENABLE                  (1 << 23)
#define GFX8_BLEND_X_DITHER_OFFSET_MASK                 INTEL_MASK(22, 21)
#define GFX8_BLEND_X_DITHER_OFFSET_SHIFT                21
#define GFX8_BLEND_Y_DITHER_OFFSET_MASK                 INTEL_MASK(20, 19)
#define GFX8_BLEND_Y_DITHER_OFFSET_SHIFT                19
/* DW1 + 2n */
#define GFX8_BLEND_COLOR_BUFFER_BLEND_ENABLE            (1 << 31)
#define GFX8_BLEND_SRC_BLEND_FACTOR_MASK                INTEL_MASK(30, 26)
#define GFX8_BLEND_SRC_BLEND_FACTOR_SHIFT               26
#define GFX8_BLEND_DST_BLEND_FACTOR_MASK                INTEL_MASK(25, 21)
#define GFX8_BLEND_DST_BLEND_FACTOR_SHIFT               21
#define GFX8_BLEND_COLOR_BLEND_FUNCTION_MASK            INTEL_MASK(20, 18)
#define GFX8_BLEND_COLOR_BLEND_FUNCTION_SHIFT           18
#define GFX8_BLEND_SRC_ALPHA_BLEND_FACTOR_MASK          INTEL_MASK(17, 13)
#define GFX8_BLEND_SRC_ALPHA_BLEND_FACTOR_SHIFT         13
#define GFX8_BLEND_DST_ALPHA_BLEND_FACTOR_MASK          INTEL_MASK(12, 8)
#define GFX8_BLEND_DST_ALPHA_BLEND_FACTOR_SHIFT         8
#define GFX8_BLEND_ALPHA_BLEND_FUNCTION_MASK            INTEL_MASK(7, 5)
#define GFX8_BLEND_ALPHA_BLEND_FUNCTION_SHIFT           5
#define GFX8_BLEND_WRITE_DISABLE_ALPHA                  (1 << 3)
#define GFX8_BLEND_WRITE_DISABLE_RED                    (1 << 2)
#define GFX8_BLEND_WRITE_DISABLE_GREEN                  (1 << 1)
#define GFX8_BLEND_WRITE_DISABLE_BLUE                   (1 << 0)
/* DW1 + 2n + 1 */
#define GFX8_BLEND_LOGIC_OP_ENABLE                      (1 << 31)
#define GFX8_BLEND_LOGIC_OP_FUNCTION_MASK               INTEL_MASK(30, 27)
#define GFX8_BLEND_LOGIC_OP_FUNCTION_SHIFT              27
#define GFX8_BLEND_PRE_BLEND_SRC_ONLY_CLAMP_ENABLE      (1 << 4)
#define GFX8_BLEND_COLOR_CLAMP_RANGE_RTFORMAT           (2 << 2)
#define GFX8_BLEND_PRE_BLEND_COLOR_CLAMP_ENABLE         (1 << 1)
#define GFX8_BLEND_POST_BLEND_COLOR_CLAMP_ENABLE        (1 << 0)

#define _3DSTATE_WM_HZ_OP                       0x7852 /* GFX8+ */
/* DW1 */
# define GFX8_WM_HZ_STENCIL_CLEAR                       (1 << 31)
# define GFX8_WM_HZ_DEPTH_CLEAR                         (1 << 30)
# define GFX8_WM_HZ_DEPTH_RESOLVE                       (1 << 28)
# define GFX8_WM_HZ_HIZ_RESOLVE                         (1 << 27)
# define GFX8_WM_HZ_PIXEL_OFFSET_ENABLE                 (1 << 26)
# define GFX8_WM_HZ_FULL_SURFACE_DEPTH_CLEAR            (1 << 25)
# define GFX8_WM_HZ_STENCIL_CLEAR_VALUE_MASK            INTEL_MASK(23, 16)
# define GFX8_WM_HZ_STENCIL_CLEAR_VALUE_SHIFT           16
# define GFX8_WM_HZ_NUM_SAMPLES_MASK                    INTEL_MASK(15, 13)
# define GFX8_WM_HZ_NUM_SAMPLES_SHIFT                   13
/* DW2 */
# define GFX8_WM_HZ_CLEAR_RECTANGLE_Y_MIN_MASK          INTEL_MASK(31, 16)
# define GFX8_WM_HZ_CLEAR_RECTANGLE_Y_MIN_SHIFT         16
# define GFX8_WM_HZ_CLEAR_RECTANGLE_X_MIN_MASK          INTEL_MASK(15, 0)
# define GFX8_WM_HZ_CLEAR_RECTANGLE_X_MIN_SHIFT         0
/* DW3 */
# define GFX8_WM_HZ_CLEAR_RECTANGLE_Y_MAX_MASK          INTEL_MASK(31, 16)
# define GFX8_WM_HZ_CLEAR_RECTANGLE_Y_MAX_SHIFT         16
# define GFX8_WM_HZ_CLEAR_RECTANGLE_X_MAX_MASK          INTEL_MASK(15, 0)
# define GFX8_WM_HZ_CLEAR_RECTANGLE_X_MAX_SHIFT         0
/* DW4 */
# define GFX8_WM_HZ_SAMPLE_MASK_MASK                    INTEL_MASK(15, 0)
# define GFX8_WM_HZ_SAMPLE_MASK_SHIFT                   0


#define _3DSTATE_PS_BLEND                       0x784D /* GFX8+ */
/* DW1 */
# define GFX8_PS_BLEND_ALPHA_TO_COVERAGE_ENABLE         (1 << 31)
# define GFX8_PS_BLEND_HAS_WRITEABLE_RT                 (1 << 30)
# define GFX8_PS_BLEND_COLOR_BUFFER_BLEND_ENABLE        (1 << 29)
# define GFX8_PS_BLEND_SRC_ALPHA_BLEND_FACTOR_MASK      INTEL_MASK(28, 24)
# define GFX8_PS_BLEND_SRC_ALPHA_BLEND_FACTOR_SHIFT     24
# define GFX8_PS_BLEND_DST_ALPHA_BLEND_FACTOR_MASK      INTEL_MASK(23, 19)
# define GFX8_PS_BLEND_DST_ALPHA_BLEND_FACTOR_SHIFT     19
# define GFX8_PS_BLEND_SRC_BLEND_FACTOR_MASK            INTEL_MASK(18, 14)
# define GFX8_PS_BLEND_SRC_BLEND_FACTOR_SHIFT           14
# define GFX8_PS_BLEND_DST_BLEND_FACTOR_MASK            INTEL_MASK(13, 9)
# define GFX8_PS_BLEND_DST_BLEND_FACTOR_SHIFT           9
# define GFX8_PS_BLEND_ALPHA_TEST_ENABLE                (1 << 8)
# define GFX8_PS_BLEND_INDEPENDENT_ALPHA_BLEND_ENABLE   (1 << 7)

#define _3DSTATE_WM_DEPTH_STENCIL               0x784E /* GFX8+ */
/* DW1 */
# define GFX8_WM_DS_STENCIL_FAIL_OP_SHIFT               29
# define GFX8_WM_DS_Z_FAIL_OP_SHIFT                     26
# define GFX8_WM_DS_Z_PASS_OP_SHIFT                     23
# define GFX8_WM_DS_BF_STENCIL_FUNC_SHIFT               20
# define GFX8_WM_DS_BF_STENCIL_FAIL_OP_SHIFT            17
# define GFX8_WM_DS_BF_Z_FAIL_OP_SHIFT                  14
# define GFX8_WM_DS_BF_Z_PASS_OP_SHIFT                  11
# define GFX8_WM_DS_STENCIL_FUNC_SHIFT                  8
# define GFX8_WM_DS_DEPTH_FUNC_SHIFT                    5
# define GFX8_WM_DS_DOUBLE_SIDED_STENCIL_ENABLE         (1 << 4)
# define GFX8_WM_DS_STENCIL_TEST_ENABLE                 (1 << 3)
# define GFX8_WM_DS_STENCIL_BUFFER_WRITE_ENABLE         (1 << 2)
# define GFX8_WM_DS_DEPTH_TEST_ENABLE                   (1 << 1)
# define GFX8_WM_DS_DEPTH_BUFFER_WRITE_ENABLE           (1 << 0)
/* DW2 */
# define GFX8_WM_DS_STENCIL_TEST_MASK_MASK              INTEL_MASK(31, 24)
# define GFX8_WM_DS_STENCIL_TEST_MASK_SHIFT             24
# define GFX8_WM_DS_STENCIL_WRITE_MASK_MASK             INTEL_MASK(23, 16)
# define GFX8_WM_DS_STENCIL_WRITE_MASK_SHIFT            16
# define GFX8_WM_DS_BF_STENCIL_TEST_MASK_MASK           INTEL_MASK(15, 8)
# define GFX8_WM_DS_BF_STENCIL_TEST_MASK_SHIFT          8
# define GFX8_WM_DS_BF_STENCIL_WRITE_MASK_MASK          INTEL_MASK(7, 0)
# define GFX8_WM_DS_BF_STENCIL_WRITE_MASK_SHIFT         0
/* DW3 */
# define GFX9_WM_DS_STENCIL_REF_MASK                    INTEL_MASK(15, 8)
# define GFX9_WM_DS_STENCIL_REF_SHIFT                   8
# define GFX9_WM_DS_BF_STENCIL_REF_MASK                 INTEL_MASK(7, 0)
# define GFX9_WM_DS_BF_STENCIL_REF_SHIFT                0

enum brw_pixel_shader_coverage_mask_mode {
   BRW_PSICMS_OFF     = 0, /* PS does not use input coverage masks. */
   BRW_PSICMS_NORMAL  = 1, /* Input Coverage masks based on outer conservatism
                            * and factors in SAMPLE_MASK.  If Pixel is
                            * conservatively covered, all samples are enabled.
                            */

   BRW_PSICMS_INNER   = 2, /* Input Coverage masks based on inner conservatism
                            * and factors in SAMPLE_MASK.  If Pixel is
                            * conservatively *FULLY* covered, all samples are
                            * enabled.
                            */
   BRW_PCICMS_DEPTH   = 3,
};

#define _3DSTATE_PS_EXTRA                       0x784F /* GFX8+ */
/* DW1 */
# define GFX8_PSX_PIXEL_SHADER_VALID                    (1 << 31)
# define GFX8_PSX_PIXEL_SHADER_NO_RT_WRITE              (1 << 30)
# define GFX8_PSX_OMASK_TO_RENDER_TARGET                (1 << 29)
# define GFX8_PSX_KILL_ENABLE                           (1 << 28)
# define GFX8_PSX_COMPUTED_DEPTH_MODE_SHIFT             26
# define GFX8_PSX_FORCE_COMPUTED_DEPTH                  (1 << 25)
# define GFX8_PSX_USES_SOURCE_DEPTH                     (1 << 24)
# define GFX8_PSX_USES_SOURCE_W                         (1 << 23)
# define GFX8_PSX_ATTRIBUTE_ENABLE                      (1 << 8)
# define GFX8_PSX_SHADER_DISABLES_ALPHA_TO_COVERAGE     (1 << 7)
# define GFX8_PSX_SHADER_IS_PER_SAMPLE                  (1 << 6)
# define GFX9_PSX_SHADER_COMPUTES_STENCIL               (1 << 5)
# define GFX9_PSX_SHADER_PULLS_BARY                     (1 << 3)
# define GFX8_PSX_SHADER_HAS_UAV                        (1 << 2)
# define GFX8_PSX_SHADER_USES_INPUT_COVERAGE_MASK       (1 << 1)
# define GFX9_PSX_SHADER_NORMAL_COVERAGE_MASK_SHIFT     0

#define _3DSTATE_WM				0x7814 /* GFX6+ */
/* DW1: kernel pointer */
/* DW2 */
# define GFX6_WM_SPF_MODE				(1 << 31)
# define GFX6_WM_VECTOR_MASK_ENABLE			(1 << 30)
# define GFX6_WM_SAMPLER_COUNT_SHIFT			27
# define GFX6_WM_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GFX6_WM_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GFX6_WM_FLOATING_POINT_MODE_ALT		(1 << 16)
/* DW3: scratch space */
/* DW4 */
# define GFX6_WM_STATISTICS_ENABLE			(1 << 31)
# define GFX6_WM_DEPTH_CLEAR				(1 << 30)
# define GFX6_WM_DEPTH_RESOLVE				(1 << 28)
# define GFX6_WM_HIERARCHICAL_DEPTH_RESOLVE		(1 << 27)
# define GFX6_WM_DISPATCH_START_GRF_SHIFT_0		16
# define GFX6_WM_DISPATCH_START_GRF_SHIFT_1		8
# define GFX6_WM_DISPATCH_START_GRF_SHIFT_2		0
/* DW5 */
# define GFX6_WM_MAX_THREADS_SHIFT			25
# define GFX6_WM_KILL_ENABLE				(1 << 22)
# define GFX6_WM_COMPUTED_DEPTH				(1 << 21)
# define GFX6_WM_USES_SOURCE_DEPTH			(1 << 20)
# define GFX6_WM_DISPATCH_ENABLE			(1 << 19)
# define GFX6_WM_LINE_END_CAP_AA_WIDTH_0_5		(0 << 16)
# define GFX6_WM_LINE_END_CAP_AA_WIDTH_1_0		(1 << 16)
# define GFX6_WM_LINE_END_CAP_AA_WIDTH_2_0		(2 << 16)
# define GFX6_WM_LINE_END_CAP_AA_WIDTH_4_0		(3 << 16)
# define GFX6_WM_LINE_AA_WIDTH_0_5			(0 << 14)
# define GFX6_WM_LINE_AA_WIDTH_1_0			(1 << 14)
# define GFX6_WM_LINE_AA_WIDTH_2_0			(2 << 14)
# define GFX6_WM_LINE_AA_WIDTH_4_0			(3 << 14)
# define GFX6_WM_POLYGON_STIPPLE_ENABLE			(1 << 13)
# define GFX6_WM_LINE_STIPPLE_ENABLE			(1 << 11)
# define GFX6_WM_OMASK_TO_RENDER_TARGET			(1 << 9)
# define GFX6_WM_USES_SOURCE_W				(1 << 8)
# define GFX6_WM_DUAL_SOURCE_BLEND_ENABLE		(1 << 7)
# define GFX6_WM_32_DISPATCH_ENABLE			(1 << 2)
# define GFX6_WM_16_DISPATCH_ENABLE			(1 << 1)
# define GFX6_WM_8_DISPATCH_ENABLE			(1 << 0)
/* DW6 */
# define GFX6_WM_NUM_SF_OUTPUTS_SHIFT			20
# define GFX6_WM_POSOFFSET_NONE				(0 << 18)
# define GFX6_WM_POSOFFSET_CENTROID			(2 << 18)
# define GFX6_WM_POSOFFSET_SAMPLE			(3 << 18)
# define GFX6_WM_POSITION_ZW_PIXEL			(0 << 16)
# define GFX6_WM_POSITION_ZW_CENTROID			(2 << 16)
# define GFX6_WM_POSITION_ZW_SAMPLE			(3 << 16)
# define GFX6_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC	(1 << 15)
# define GFX6_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 14)
# define GFX6_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC	(1 << 13)
# define GFX6_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC		(1 << 12)
# define GFX6_WM_PERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 11)
# define GFX6_WM_PERSPECTIVE_PIXEL_BARYCENTRIC		(1 << 10)
# define GFX6_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT   10
# define GFX6_WM_POINT_RASTRULE_UPPER_RIGHT		(1 << 9)
# define GFX6_WM_MSRAST_OFF_PIXEL			(0 << 1)
# define GFX6_WM_MSRAST_OFF_PATTERN			(1 << 1)
# define GFX6_WM_MSRAST_ON_PIXEL			(2 << 1)
# define GFX6_WM_MSRAST_ON_PATTERN			(3 << 1)
# define GFX6_WM_MSDISPMODE_PERSAMPLE			(0 << 0)
# define GFX6_WM_MSDISPMODE_PERPIXEL			(1 << 0)
/* DW7: kernel 1 pointer */
/* DW8: kernel 2 pointer */

#define _3DSTATE_CONSTANT_VS		      0x7815 /* GFX6+ */
#define _3DSTATE_CONSTANT_GS		      0x7816 /* GFX6+ */
#define _3DSTATE_CONSTANT_PS		      0x7817 /* GFX6+ */
# define GFX6_CONSTANT_BUFFER_3_ENABLE			(1 << 15)
# define GFX6_CONSTANT_BUFFER_2_ENABLE			(1 << 14)
# define GFX6_CONSTANT_BUFFER_1_ENABLE			(1 << 13)
# define GFX6_CONSTANT_BUFFER_0_ENABLE			(1 << 12)

#define _3DSTATE_CONSTANT_HS                  0x7819 /* GFX7+ */
#define _3DSTATE_CONSTANT_DS                  0x781A /* GFX7+ */

#define _3DSTATE_STREAMOUT                    0x781e /* GFX7+ */
/* DW1 */
# define SO_FUNCTION_ENABLE				(1 << 31)
# define SO_RENDERING_DISABLE				(1 << 30)
/* This selects which incoming rendering stream goes down the pipeline.  The
 * rendering stream is 0 if not defined by special cases in the GS state.
 */
# define SO_RENDER_STREAM_SELECT_SHIFT			27
# define SO_RENDER_STREAM_SELECT_MASK			INTEL_MASK(28, 27)
/* Controls reordering of TRISTRIP_* elements in stream output (not rendering).
 */
# define SO_REORDER_TRAILING				(1 << 26)
/* Controls SO_NUM_PRIMS_WRITTEN_* and SO_PRIM_STORAGE_* */
# define SO_STATISTICS_ENABLE				(1 << 25)
# define SO_BUFFER_ENABLE(n)				(1 << (8 + (n)))
/* DW2 */
# define SO_STREAM_3_VERTEX_READ_OFFSET_SHIFT		29
# define SO_STREAM_3_VERTEX_READ_OFFSET_MASK		INTEL_MASK(29, 29)
# define SO_STREAM_3_VERTEX_READ_LENGTH_SHIFT		24
# define SO_STREAM_3_VERTEX_READ_LENGTH_MASK		INTEL_MASK(28, 24)
# define SO_STREAM_2_VERTEX_READ_OFFSET_SHIFT		21
# define SO_STREAM_2_VERTEX_READ_OFFSET_MASK		INTEL_MASK(21, 21)
# define SO_STREAM_2_VERTEX_READ_LENGTH_SHIFT		16
# define SO_STREAM_2_VERTEX_READ_LENGTH_MASK		INTEL_MASK(20, 16)
# define SO_STREAM_1_VERTEX_READ_OFFSET_SHIFT		13
# define SO_STREAM_1_VERTEX_READ_OFFSET_MASK		INTEL_MASK(13, 13)
# define SO_STREAM_1_VERTEX_READ_LENGTH_SHIFT		8
# define SO_STREAM_1_VERTEX_READ_LENGTH_MASK		INTEL_MASK(12, 8)
# define SO_STREAM_0_VERTEX_READ_OFFSET_SHIFT		5
# define SO_STREAM_0_VERTEX_READ_OFFSET_MASK		INTEL_MASK(5, 5)
# define SO_STREAM_0_VERTEX_READ_LENGTH_SHIFT		0
# define SO_STREAM_0_VERTEX_READ_LENGTH_MASK		INTEL_MASK(4, 0)

/* 3DSTATE_WM for Gfx7 */
/* DW1 */
# define GFX7_WM_STATISTICS_ENABLE			(1 << 31)
# define GFX7_WM_DEPTH_CLEAR				(1 << 30)
# define GFX7_WM_DISPATCH_ENABLE			(1 << 29)
# define GFX7_WM_DEPTH_RESOLVE				(1 << 28)
# define GFX7_WM_HIERARCHICAL_DEPTH_RESOLVE		(1 << 27)
# define GFX7_WM_KILL_ENABLE				(1 << 25)
# define GFX7_WM_COMPUTED_DEPTH_MODE_SHIFT              23
# define GFX7_WM_USES_SOURCE_DEPTH			(1 << 20)
# define GFX7_WM_EARLY_DS_CONTROL_NORMAL                (0 << 21)
# define GFX7_WM_EARLY_DS_CONTROL_PSEXEC                (1 << 21)
# define GFX7_WM_EARLY_DS_CONTROL_PREPS                 (2 << 21)
# define GFX7_WM_USES_SOURCE_W			        (1 << 19)
# define GFX7_WM_POSITION_ZW_PIXEL			(0 << 17)
# define GFX7_WM_POSITION_ZW_CENTROID			(2 << 17)
# define GFX7_WM_POSITION_ZW_SAMPLE			(3 << 17)
# define GFX7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT   11
# define GFX7_WM_USES_INPUT_COVERAGE_MASK	        (1 << 10)
# define GFX7_WM_LINE_END_CAP_AA_WIDTH_0_5		(0 << 8)
# define GFX7_WM_LINE_END_CAP_AA_WIDTH_1_0		(1 << 8)
# define GFX7_WM_LINE_END_CAP_AA_WIDTH_2_0		(2 << 8)
# define GFX7_WM_LINE_END_CAP_AA_WIDTH_4_0		(3 << 8)
# define GFX7_WM_LINE_AA_WIDTH_0_5			(0 << 6)
# define GFX7_WM_LINE_AA_WIDTH_1_0			(1 << 6)
# define GFX7_WM_LINE_AA_WIDTH_2_0			(2 << 6)
# define GFX7_WM_LINE_AA_WIDTH_4_0			(3 << 6)
# define GFX7_WM_POLYGON_STIPPLE_ENABLE			(1 << 4)
# define GFX7_WM_LINE_STIPPLE_ENABLE			(1 << 3)
# define GFX7_WM_POINT_RASTRULE_UPPER_RIGHT		(1 << 2)
# define GFX7_WM_MSRAST_OFF_PIXEL			(0 << 0)
# define GFX7_WM_MSRAST_OFF_PATTERN			(1 << 0)
# define GFX7_WM_MSRAST_ON_PIXEL			(2 << 0)
# define GFX7_WM_MSRAST_ON_PATTERN			(3 << 0)
/* DW2 */
# define GFX7_WM_MSDISPMODE_PERSAMPLE			(0 << 31)
# define GFX7_WM_MSDISPMODE_PERPIXEL			(1 << 31)
# define HSW_WM_UAV_ONLY                                (1 << 30)

#define _3DSTATE_PS				0x7820 /* GFX7+ */
/* DW1: kernel pointer */
/* DW2 */
# define GFX7_PS_SPF_MODE				(1 << 31)
# define GFX7_PS_VECTOR_MASK_ENABLE			(1 << 30)
# define GFX7_PS_SAMPLER_COUNT_SHIFT			27
# define GFX7_PS_SAMPLER_COUNT_MASK                     INTEL_MASK(29, 27)
# define GFX7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GFX7_PS_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GFX7_PS_FLOATING_POINT_MODE_ALT		(1 << 16)
/* DW3: scratch space */
/* DW4 */
# define IVB_PS_MAX_THREADS_SHIFT			24
# define HSW_PS_MAX_THREADS_SHIFT			23
# define HSW_PS_SAMPLE_MASK_SHIFT		        12
# define HSW_PS_SAMPLE_MASK_MASK			INTEL_MASK(19, 12)
# define GFX7_PS_PUSH_CONSTANT_ENABLE		        (1 << 11)
# define GFX7_PS_ATTRIBUTE_ENABLE		        (1 << 10)
# define GFX7_PS_OMASK_TO_RENDER_TARGET			(1 << 9)
# define GFX7_PS_RENDER_TARGET_FAST_CLEAR_ENABLE	(1 << 8)
# define GFX7_PS_DUAL_SOURCE_BLEND_ENABLE		(1 << 7)
# define GFX7_PS_RENDER_TARGET_RESOLVE_ENABLE		(1 << 6)
# define GFX9_PS_RENDER_TARGET_RESOLVE_FULL             (3 << 6)
# define HSW_PS_UAV_ACCESS_ENABLE			(1 << 5)
# define GFX7_PS_POSOFFSET_NONE				(0 << 3)
# define GFX7_PS_POSOFFSET_CENTROID			(2 << 3)
# define GFX7_PS_POSOFFSET_SAMPLE			(3 << 3)
# define GFX7_PS_32_DISPATCH_ENABLE			(1 << 2)
# define GFX7_PS_16_DISPATCH_ENABLE			(1 << 1)
# define GFX7_PS_8_DISPATCH_ENABLE			(1 << 0)
/* DW5 */
# define GFX7_PS_DISPATCH_START_GRF_SHIFT_0		16
# define GFX7_PS_DISPATCH_START_GRF_SHIFT_1		8
# define GFX7_PS_DISPATCH_START_GRF_SHIFT_2		0
/* DW6: kernel 1 pointer */
/* DW7: kernel 2 pointer */

#define _3DSTATE_SAMPLE_MASK			0x7818 /* GFX6+ */

#define _3DSTATE_DRAWING_RECTANGLE		0x7900
#define _3DSTATE_BLEND_CONSTANT_COLOR		0x7901
#define _3DSTATE_CHROMA_KEY			0x7904
#define _3DSTATE_DEPTH_BUFFER			0x7905 /* GFX4-6 */
#define _3DSTATE_POLY_STIPPLE_OFFSET		0x7906
#define _3DSTATE_POLY_STIPPLE_PATTERN		0x7907
#define _3DSTATE_LINE_STIPPLE_PATTERN		0x7908
#define _3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP	0x7909
#define _3DSTATE_AA_LINE_PARAMETERS		0x790a /* G45+ */

#define _3DSTATE_GS_SVB_INDEX			0x790b /* CTG+ */
/* DW1 */
# define SVB_INDEX_SHIFT				29
# define SVB_LOAD_INTERNAL_VERTEX_COUNT			(1 << 0) /* SNB+ */
/* DW2: SVB index */
/* DW3: SVB maximum index */

#define _3DSTATE_MULTISAMPLE			0x790d /* GFX6+ */
#define GFX8_3DSTATE_MULTISAMPLE		0x780d /* GFX8+ */
/* DW1 */
# define MS_PIXEL_LOCATION_CENTER			(0 << 4)
# define MS_PIXEL_LOCATION_UPPER_LEFT			(1 << 4)
# define MS_NUMSAMPLES_1				(0 << 1)
# define MS_NUMSAMPLES_2				(1 << 1)
# define MS_NUMSAMPLES_4				(2 << 1)
# define MS_NUMSAMPLES_8				(3 << 1)
# define MS_NUMSAMPLES_16				(4 << 1)

#define _3DSTATE_SAMPLE_PATTERN                 0x791c

#define _3DSTATE_STENCIL_BUFFER			0x790e /* ILK, SNB */
#define _3DSTATE_HIER_DEPTH_BUFFER		0x790f /* ILK, SNB */

#define GFX7_3DSTATE_CLEAR_PARAMS		0x7804
#define GFX7_3DSTATE_DEPTH_BUFFER		0x7805
#define GFX7_3DSTATE_STENCIL_BUFFER		0x7806
# define HSW_STENCIL_ENABLED                            (1 << 31)
#define GFX7_3DSTATE_HIER_DEPTH_BUFFER		0x7807

#define _3DSTATE_CLEAR_PARAMS			0x7910 /* ILK, SNB */
# define GFX5_DEPTH_CLEAR_VALID				(1 << 15)
/* DW1: depth clear value */
/* DW2 */
# define GFX7_DEPTH_CLEAR_VALID				(1 << 0)

#define _3DSTATE_SO_DECL_LIST			0x7917 /* GFX7+ */
/* DW1 */
# define SO_STREAM_TO_BUFFER_SELECTS_3_SHIFT		12
# define SO_STREAM_TO_BUFFER_SELECTS_3_MASK		INTEL_MASK(15, 12)
# define SO_STREAM_TO_BUFFER_SELECTS_2_SHIFT		8
# define SO_STREAM_TO_BUFFER_SELECTS_2_MASK		INTEL_MASK(11, 8)
# define SO_STREAM_TO_BUFFER_SELECTS_1_SHIFT		4
# define SO_STREAM_TO_BUFFER_SELECTS_1_MASK		INTEL_MASK(7, 4)
# define SO_STREAM_TO_BUFFER_SELECTS_0_SHIFT		0
# define SO_STREAM_TO_BUFFER_SELECTS_0_MASK		INTEL_MASK(3, 0)
/* DW2 */
# define SO_NUM_ENTRIES_3_SHIFT				24
# define SO_NUM_ENTRIES_3_MASK				INTEL_MASK(31, 24)
# define SO_NUM_ENTRIES_2_SHIFT				16
# define SO_NUM_ENTRIES_2_MASK				INTEL_MASK(23, 16)
# define SO_NUM_ENTRIES_1_SHIFT				8
# define SO_NUM_ENTRIES_1_MASK				INTEL_MASK(15, 8)
# define SO_NUM_ENTRIES_0_SHIFT				0
# define SO_NUM_ENTRIES_0_MASK				INTEL_MASK(7, 0)

/* SO_DECL DW0 */
# define SO_DECL_OUTPUT_BUFFER_SLOT_SHIFT		12
# define SO_DECL_OUTPUT_BUFFER_SLOT_MASK		INTEL_MASK(13, 12)
# define SO_DECL_HOLE_FLAG				(1 << 11)
# define SO_DECL_REGISTER_INDEX_SHIFT			4
# define SO_DECL_REGISTER_INDEX_MASK			INTEL_MASK(9, 4)
# define SO_DECL_COMPONENT_MASK_SHIFT			0
# define SO_DECL_COMPONENT_MASK_MASK			INTEL_MASK(3, 0)

#define _3DSTATE_SO_BUFFER                    0x7918 /* GFX7+ */
/* DW1 */
# define GFX8_SO_BUFFER_ENABLE                          (1 << 31)
# define SO_BUFFER_INDEX_SHIFT				29
# define SO_BUFFER_INDEX_MASK				INTEL_MASK(30, 29)
# define GFX8_SO_BUFFER_OFFSET_WRITE_ENABLE             (1 << 21)
# define GFX8_SO_BUFFER_OFFSET_ADDRESS_ENABLE           (1 << 20)
# define SO_BUFFER_PITCH_SHIFT				0
# define SO_BUFFER_PITCH_MASK				INTEL_MASK(11, 0)
/* DW2: start address */
/* DW3: end address. */

#define _3DSTATE_3D_MODE                     0x791e
# define SLICE_HASHING_TABLE_ENABLE          (1 << 6)
# define SLICE_HASHING_TABLE_ENABLE_MASK     REG_MASK(1 << 6)

#define _3DSTATE_SLICE_TABLE_STATE_POINTERS  0x7920

#define CMD_MI_FLUSH                  0x0200

# define BLT_X_SHIFT					0
# define BLT_X_MASK					INTEL_MASK(15, 0)
# define BLT_Y_SHIFT					16
# define BLT_Y_MASK					INTEL_MASK(31, 16)

#define GFX5_MI_REPORT_PERF_COUNT ((0x26 << 23) | (3 - 2))
/* DW0 */
# define GFX5_MI_COUNTER_SET_0      (0 << 6)
# define GFX5_MI_COUNTER_SET_1      (1 << 6)
/* DW1 */
# define MI_COUNTER_ADDRESS_GTT     (1 << 0)
/* DW2: a user-defined report ID (written to the buffer but can be anything) */

#define GFX6_MI_REPORT_PERF_COUNT ((0x28 << 23) | (3 - 2))

#define GFX8_MI_REPORT_PERF_COUNT ((0x28 << 23) | (4 - 2))

/* Maximum number of entries that can be addressed using a binding table
 * pointer of type SURFTYPE_BUFFER
 */
#define BRW_MAX_NUM_BUFFER_ENTRIES	(1 << 27)

#define MEDIA_VFE_STATE                         0x7000
/* GFX7 DW2, GFX8+ DW3 */
# define MEDIA_VFE_STATE_MAX_THREADS_SHIFT      16
# define MEDIA_VFE_STATE_MAX_THREADS_MASK       INTEL_MASK(31, 16)
# define MEDIA_VFE_STATE_URB_ENTRIES_SHIFT      8
# define MEDIA_VFE_STATE_URB_ENTRIES_MASK       INTEL_MASK(15, 8)
# define MEDIA_VFE_STATE_RESET_GTW_TIMER_SHIFT  7
# define MEDIA_VFE_STATE_RESET_GTW_TIMER_MASK   INTEL_MASK(7, 7)
# define MEDIA_VFE_STATE_BYPASS_GTW_SHIFT       6
# define MEDIA_VFE_STATE_BYPASS_GTW_MASK        INTEL_MASK(6, 6)
# define GFX7_MEDIA_VFE_STATE_GPGPU_MODE_SHIFT  2
# define GFX7_MEDIA_VFE_STATE_GPGPU_MODE_MASK   INTEL_MASK(2, 2)
/* GFX7 DW4, GFX8+ DW5 */
# define MEDIA_VFE_STATE_URB_ALLOC_SHIFT        16
# define MEDIA_VFE_STATE_URB_ALLOC_MASK         INTEL_MASK(31, 16)
# define MEDIA_VFE_STATE_CURBE_ALLOC_SHIFT      0
# define MEDIA_VFE_STATE_CURBE_ALLOC_MASK       INTEL_MASK(15, 0)

#define MEDIA_CURBE_LOAD                        0x7001
#define MEDIA_INTERFACE_DESCRIPTOR_LOAD         0x7002
/* GFX7 DW4, GFX8+ DW5 */
# define MEDIA_CURBE_READ_LENGTH_SHIFT          16
# define MEDIA_CURBE_READ_LENGTH_MASK           INTEL_MASK(31, 16)
# define MEDIA_CURBE_READ_OFFSET_SHIFT          0
# define MEDIA_CURBE_READ_OFFSET_MASK           INTEL_MASK(15, 0)
/* GFX7 DW5, GFX8+ DW6 */
# define MEDIA_BARRIER_ENABLE_SHIFT             21
# define MEDIA_BARRIER_ENABLE_MASK              INTEL_MASK(21, 21)
# define MEDIA_SHARED_LOCAL_MEMORY_SIZE_SHIFT   16
# define MEDIA_SHARED_LOCAL_MEMORY_SIZE_MASK    INTEL_MASK(20, 16)
# define MEDIA_GPGPU_THREAD_COUNT_SHIFT         0
# define MEDIA_GPGPU_THREAD_COUNT_MASK          INTEL_MASK(7, 0)
# define GFX8_MEDIA_GPGPU_THREAD_COUNT_SHIFT    0
# define GFX8_MEDIA_GPGPU_THREAD_COUNT_MASK     INTEL_MASK(9, 0)
/* GFX7 DW6, GFX8+ DW7 */
# define CROSS_THREAD_READ_LENGTH_SHIFT         0
# define CROSS_THREAD_READ_LENGTH_MASK          INTEL_MASK(7, 0)
#define MEDIA_STATE_FLUSH                       0x7004
#define GPGPU_WALKER                            0x7105
/* GFX7 DW0 */
# define GFX7_GPGPU_INDIRECT_PARAMETER_ENABLE   (1 << 10)
# define GFX7_GPGPU_PREDICATE_ENABLE            (1 << 8)
/* GFX8+ DW2 */
# define GPGPU_WALKER_INDIRECT_LENGTH_SHIFT     0
# define GPGPU_WALKER_INDIRECT_LENGTH_MASK      INTEL_MASK(15, 0)
/* GFX7 DW2, GFX8+ DW4 */
# define GPGPU_WALKER_SIMD_SIZE_SHIFT           30
# define GPGPU_WALKER_SIMD_SIZE_MASK            INTEL_MASK(31, 30)
# define GPGPU_WALKER_THREAD_DEPTH_MAX_SHIFT    16
# define GPGPU_WALKER_THREAD_DEPTH_MAX_MASK     INTEL_MASK(21, 16)
# define GPGPU_WALKER_THREAD_HEIGHT_MAX_SHIFT   8
# define GPGPU_WALKER_THREAD_HEIGHT_MAX_MASK    INTEL_MASK(31, 8)
# define GPGPU_WALKER_THREAD_WIDTH_MAX_SHIFT    0
# define GPGPU_WALKER_THREAD_WIDTH_MAX_MASK     INTEL_MASK(5, 0)

#define CMD_MI				(0x0 << 29)
#define CMD_2D				(0x2 << 29)
#define CMD_3D				(0x3 << 29)

#define MI_NOOP				(CMD_MI | 0)

#define MI_BATCH_BUFFER_END		(CMD_MI | 0xA << 23)

#define MI_FLUSH			(CMD_MI | (4 << 23))
#define FLUSH_MAP_CACHE				(1 << 0)
#define INHIBIT_FLUSH_RENDER_CACHE		(1 << 2)

#define MI_STORE_DATA_IMM		(CMD_MI | (0x20 << 23))
#define MI_LOAD_REGISTER_IMM		(CMD_MI | (0x22 << 23))
#define MI_LOAD_REGISTER_REG		(CMD_MI | (0x2A << 23))

#define MI_FLUSH_DW			(CMD_MI | (0x26 << 23))

#define MI_STORE_REGISTER_MEM		(CMD_MI | (0x24 << 23))
# define MI_STORE_REGISTER_MEM_USE_GGTT		(1 << 22)
# define MI_STORE_REGISTER_MEM_PREDICATE	(1 << 21)

/* Load a value from memory into a register.  Only available on Gfx7+. */
#define GFX7_MI_LOAD_REGISTER_MEM	(CMD_MI | (0x29 << 23))
# define MI_LOAD_REGISTER_MEM_USE_GGTT		(1 << 22)

/* Manipulate the predicate bit based on some register values. Only on Gfx7+ */
#define GFX7_MI_PREDICATE		(CMD_MI | (0xC << 23))
# define MI_PREDICATE_LOADOP_KEEP		(0 << 6)
# define MI_PREDICATE_LOADOP_LOAD		(2 << 6)
# define MI_PREDICATE_LOADOP_LOADINV		(3 << 6)
# define MI_PREDICATE_COMBINEOP_SET		(0 << 3)
# define MI_PREDICATE_COMBINEOP_AND		(1 << 3)
# define MI_PREDICATE_COMBINEOP_OR		(2 << 3)
# define MI_PREDICATE_COMBINEOP_XOR		(3 << 3)
# define MI_PREDICATE_COMPAREOP_TRUE		(0 << 0)
# define MI_PREDICATE_COMPAREOP_FALSE		(1 << 0)
# define MI_PREDICATE_COMPAREOP_SRCS_EQUAL	(2 << 0)
# define MI_PREDICATE_COMPAREOP_DELTAS_EQUAL	(3 << 0)

#define HSW_MI_MATH			(CMD_MI | (0x1a << 23))

#define MI_MATH_ALU2(opcode, operand1, operand2) \
   ( ((MI_MATH_OPCODE_##opcode) << 20) | ((MI_MATH_OPERAND_##operand1) << 10) | \
     ((MI_MATH_OPERAND_##operand2) << 0) )

#define MI_MATH_ALU1(opcode, operand1) \
   ( ((MI_MATH_OPCODE_##opcode) << 20) | ((MI_MATH_OPERAND_##operand1) << 10) )

#define MI_MATH_ALU0(opcode) \
   ( ((MI_MATH_OPCODE_##opcode) << 20) )

#define MI_MATH_OPCODE_NOOP      0x000
#define MI_MATH_OPCODE_LOAD      0x080
#define MI_MATH_OPCODE_LOADINV   0x480
#define MI_MATH_OPCODE_LOAD0     0x081
#define MI_MATH_OPCODE_LOAD1     0x481
#define MI_MATH_OPCODE_ADD       0x100
#define MI_MATH_OPCODE_SUB       0x101
#define MI_MATH_OPCODE_AND       0x102
#define MI_MATH_OPCODE_OR        0x103
#define MI_MATH_OPCODE_XOR       0x104
#define MI_MATH_OPCODE_STORE     0x180
#define MI_MATH_OPCODE_STOREINV  0x580

#define MI_MATH_OPERAND_R0   0x00
#define MI_MATH_OPERAND_R1   0x01
#define MI_MATH_OPERAND_R2   0x02
#define MI_MATH_OPERAND_R3   0x03
#define MI_MATH_OPERAND_R4   0x04
#define MI_MATH_OPERAND_SRCA 0x20
#define MI_MATH_OPERAND_SRCB 0x21
#define MI_MATH_OPERAND_ACCU 0x31
#define MI_MATH_OPERAND_ZF   0x32
#define MI_MATH_OPERAND_CF   0x33

#define XY_SETUP_BLT_CMD		(CMD_2D | (0x01 << 22))

#define XY_COLOR_BLT_CMD		(CMD_2D | (0x50 << 22))

#define XY_SRC_COPY_BLT_CMD             (CMD_2D | (0x53 << 22))

#define XY_FAST_COPY_BLT_CMD             (CMD_2D | (0x42 << 22))

#define XY_TEXT_IMMEDIATE_BLIT_CMD	(CMD_2D | (0x31 << 22))
# define XY_TEXT_BYTE_PACKED		(1 << 16)

/* BR00 */
#define XY_BLT_WRITE_ALPHA	(1 << 21)
#define XY_BLT_WRITE_RGB	(1 << 20)
#define XY_SRC_TILED		(1 << 15)
#define XY_DST_TILED		(1 << 11)

/* BR00 */
#define XY_FAST_SRC_TILED_64K        (3 << 20)
#define XY_FAST_SRC_TILED_Y          (2 << 20)
#define XY_FAST_SRC_TILED_X          (1 << 20)

#define XY_FAST_DST_TILED_64K        (3 << 13)
#define XY_FAST_DST_TILED_Y          (2 << 13)
#define XY_FAST_DST_TILED_X          (1 << 13)

/* BR13 */
#define BR13_8			(0x0 << 24)
#define BR13_565		(0x1 << 24)
#define BR13_8888		(0x3 << 24)
#define BR13_16161616		(0x4 << 24)
#define BR13_32323232		(0x5 << 24)

#define GFX6_SO_PRIM_STORAGE_NEEDED     0x2280
#define GFX7_SO_PRIM_STORAGE_NEEDED(n)  (0x5240 + (n) * 8)

#define GFX6_SO_NUM_PRIMS_WRITTEN       0x2288
#define GFX7_SO_NUM_PRIMS_WRITTEN(n)    (0x5200 + (n) * 8)

#define GFX7_SO_WRITE_OFFSET(n)         (0x5280 + (n) * 4)

#define TIMESTAMP                       0x2358

#define BCS_SWCTRL                      0x22200
# define BCS_SWCTRL_SRC_Y               (1 << 0)
# define BCS_SWCTRL_DST_Y               (1 << 1)

#define OACONTROL                       0x2360
# define OACONTROL_COUNTER_SELECT_SHIFT  2
# define OACONTROL_ENABLE_COUNTERS       (1 << 0)

/* Auto-Draw / Indirect Registers */
#define GFX7_3DPRIM_END_OFFSET          0x2420
#define GFX7_3DPRIM_START_VERTEX        0x2430
#define GFX7_3DPRIM_VERTEX_COUNT        0x2434
#define GFX7_3DPRIM_INSTANCE_COUNT      0x2438
#define GFX7_3DPRIM_START_INSTANCE      0x243C
#define GFX7_3DPRIM_BASE_VERTEX         0x2440

/* Auto-Compute / Indirect Registers */
#define GFX7_GPGPU_DISPATCHDIMX         0x2500
#define GFX7_GPGPU_DISPATCHDIMY         0x2504
#define GFX7_GPGPU_DISPATCHDIMZ         0x2508

#define GFX7_CACHE_MODE_0               0x7000
#define GFX7_CACHE_MODE_1               0x7004
# define GFX9_FLOAT_BLEND_OPTIMIZATION_ENABLE (1 << 4)
# define GFX9_MSC_RAW_HAZARD_AVOIDANCE_BIT    (1 << 9)
# define GFX8_HIZ_NP_PMA_FIX_ENABLE        (1 << 11)
# define GFX8_HIZ_NP_EARLY_Z_FAILS_DISABLE (1 << 13)
# define GFX9_PARTIAL_RESOLVE_DISABLE_IN_VC (1 << 1)
# define GFX8_HIZ_PMA_MASK_BITS \
   REG_MASK(GFX8_HIZ_NP_PMA_FIX_ENABLE | GFX8_HIZ_NP_EARLY_Z_FAILS_DISABLE)
# define GFX11_DISABLE_REPACKING_FOR_COMPRESSION (1 << 15)

#define GFX7_GT_MODE                    0x7008
# define GFX9_SUBSLICE_HASHING_8x8      (0 << 8)
# define GFX9_SUBSLICE_HASHING_16x4     (1 << 8)
# define GFX9_SUBSLICE_HASHING_8x4      (2 << 8)
# define GFX9_SUBSLICE_HASHING_16x16    (3 << 8)
# define GFX9_SUBSLICE_HASHING_MASK_BITS REG_MASK(3 << 8)
# define GFX9_SLICE_HASHING_NORMAL      (0 << 11)
# define GFX9_SLICE_HASHING_DISABLED    (1 << 11)
# define GFX9_SLICE_HASHING_32x16       (2 << 11)
# define GFX9_SLICE_HASHING_32x32       (3 << 11)
# define GFX9_SLICE_HASHING_MASK_BITS REG_MASK(3 << 11)

/* Predicate registers */
#define MI_PREDICATE_SRC0               0x2400
#define MI_PREDICATE_SRC1               0x2408
#define MI_PREDICATE_DATA               0x2410
#define MI_PREDICATE_RESULT             0x2418
#define MI_PREDICATE_RESULT_1           0x241C
#define MI_PREDICATE_RESULT_2           0x2214

#define HSW_CS_GPR(n) (0x2600 + (n) * 8)

/* L3 cache control registers. */
#define GFX7_L3SQCREG1                     0xb010
/* L3SQ general and high priority credit initialization. */
# define IVB_L3SQCREG1_SQGHPCI_DEFAULT     0x00730000
# define VLV_L3SQCREG1_SQGHPCI_DEFAULT     0x00d30000
# define HSW_L3SQCREG1_SQGHPCI_DEFAULT     0x00610000
# define GFX7_L3SQCREG1_CONV_DC_UC         (1 << 24)
# define GFX7_L3SQCREG1_CONV_IS_UC         (1 << 25)
# define GFX7_L3SQCREG1_CONV_C_UC          (1 << 26)
# define GFX7_L3SQCREG1_CONV_T_UC          (1 << 27)

#define GFX7_L3CNTLREG2                    0xb020
# define GFX7_L3CNTLREG2_SLM_ENABLE        (1 << 0)
# define GFX7_L3CNTLREG2_URB_ALLOC_SHIFT   1
# define GFX7_L3CNTLREG2_URB_ALLOC_MASK    INTEL_MASK(6, 1)
# define GFX7_L3CNTLREG2_URB_LOW_BW        (1 << 7)
# define GFX7_L3CNTLREG2_ALL_ALLOC_SHIFT   8
# define GFX7_L3CNTLREG2_ALL_ALLOC_MASK    INTEL_MASK(13, 8)
# define GFX7_L3CNTLREG2_RO_ALLOC_SHIFT    14
# define GFX7_L3CNTLREG2_RO_ALLOC_MASK     INTEL_MASK(19, 14)
# define GFX7_L3CNTLREG2_RO_LOW_BW         (1 << 20)
# define GFX7_L3CNTLREG2_DC_ALLOC_SHIFT    21
# define GFX7_L3CNTLREG2_DC_ALLOC_MASK     INTEL_MASK(26, 21)
# define GFX7_L3CNTLREG2_DC_LOW_BW         (1 << 27)

#define GFX7_L3CNTLREG3                    0xb024
# define GFX7_L3CNTLREG3_IS_ALLOC_SHIFT    1
# define GFX7_L3CNTLREG3_IS_ALLOC_MASK     INTEL_MASK(6, 1)
# define GFX7_L3CNTLREG3_IS_LOW_BW         (1 << 7)
# define GFX7_L3CNTLREG3_C_ALLOC_SHIFT     8
# define GFX7_L3CNTLREG3_C_ALLOC_MASK      INTEL_MASK(13, 8)
# define GFX7_L3CNTLREG3_C_LOW_BW          (1 << 14)
# define GFX7_L3CNTLREG3_T_ALLOC_SHIFT     15
# define GFX7_L3CNTLREG3_T_ALLOC_MASK      INTEL_MASK(20, 15)
# define GFX7_L3CNTLREG3_T_LOW_BW          (1 << 21)

#define HSW_SCRATCH1                       0xb038
#define HSW_SCRATCH1_L3_ATOMIC_DISABLE     (1 << 27)

#define HSW_ROW_CHICKEN3                   0xe49c
#define HSW_ROW_CHICKEN3_L3_ATOMIC_DISABLE (1 << 6)

#define GFX8_L3CNTLREG                     0x7034
# define GFX8_L3CNTLREG_SLM_ENABLE         (1 << 0)
# define GFX8_L3CNTLREG_URB_ALLOC_SHIFT    1
# define GFX8_L3CNTLREG_URB_ALLOC_MASK     INTEL_MASK(7, 1)
# define GFX8_L3CNTLREG_RO_ALLOC_SHIFT     11
# define GFX8_L3CNTLREG_RO_ALLOC_MASK      INTEL_MASK(17, 11)
# define GFX8_L3CNTLREG_DC_ALLOC_SHIFT     18
# define GFX8_L3CNTLREG_DC_ALLOC_MASK      INTEL_MASK(24, 18)
# define GFX8_L3CNTLREG_ALL_ALLOC_SHIFT    25
# define GFX8_L3CNTLREG_ALL_ALLOC_MASK     INTEL_MASK(31, 25)
# define GFX8_L3CNTLREG_EDBC_NO_HANG       (1 << 9)
# define GFX11_L3CNTLREG_USE_FULL_WAYS     (1 << 10)

#define GFX10_CACHE_MODE_SS            0x0e420
#define GFX10_FLOAT_BLEND_OPTIMIZATION_ENABLE (1 << 4)

#define INSTPM                             0x20c0
# define INSTPM_CONSTANT_BUFFER_ADDRESS_OFFSET_DISABLE (1 << 6)

#define CS_DEBUG_MODE2                     0x20d8 /* Gfx9+ */
# define CSDBG2_CONSTANT_BUFFER_ADDRESS_OFFSET_DISABLE (1 << 4)

#define SLICE_COMMON_ECO_CHICKEN1          0x731c /* Gfx9+ */
# define GLK_SCEC_BARRIER_MODE_GPGPU       (0 << 7)
# define GLK_SCEC_BARRIER_MODE_3D_HULL     (1 << 7)
# define GLK_SCEC_BARRIER_MODE_MASK        REG_MASK(1 << 7)
# define GFX11_STATE_CACHE_REDIRECT_TO_CS_SECTION_ENABLE (1 << 11)

#define HALF_SLICE_CHICKEN7                0xE194
# define TEXEL_OFFSET_FIX_ENABLE           (1 << 1)
# define TEXEL_OFFSET_FIX_MASK             REG_MASK(1 << 1)

#define GFX11_SAMPLER_MODE                                  0xE18C
# define HEADERLESS_MESSAGE_FOR_PREEMPTABLE_CONTEXTS        (1 << 5)
# define HEADERLESS_MESSAGE_FOR_PREEMPTABLE_CONTEXTS_MASK   REG_MASK(1 << 5)

#define CS_CHICKEN1                        0x2580 /* Gfx9+ */
# define GFX9_REPLAY_MODE_MIDBUFFER             (0 << 0)
# define GFX9_REPLAY_MODE_MIDOBJECT             (1 << 0)
# define GFX9_REPLAY_MODE_MASK                  REG_MASK(1 << 0)

#endif
