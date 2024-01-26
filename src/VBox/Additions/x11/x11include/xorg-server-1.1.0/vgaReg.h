/* $XFree86: xc/programs/Xserver/hw/xfree86/xf4bpp/vgaReg.h,v 1.3 1999/06/06 08:49:07 dawes Exp $ */
/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/

/* $XConsortium: vgaReg.h /main/4 1996/02/21 17:59:02 kaleb $ */

#define SET_BYTE_REGISTER( ioport, value )	outb( ioport, value )
#define SET_INDEX_REGISTER( ioport, value ) SET_BYTE_REGISTER( ioport, value )
#define SET_DATA_REGISTER( ioport, value ) SET_BYTE_REGISTER( ioport, value )
/* GJA -- deleted RTIO and ATRIO case here, so that a PCIO #define became
 * superfluous.
 */
#define SET_INDEXED_REGISTER(RegGroup, Index, Value) \
	(SET_BYTE_REGISTER(RegGroup, Index), \
	 SET_BYTE_REGISTER((RegGroup) + 1, Value))

/* There is a jumper on the ega to change this to 0x200 instead !! */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#if 0	/* This is now a stack variable, as needed */
#define REGBASE				0x300
#endif

#define AttributeIndexRegister		REGBASE + 0xC0
#define AttributeDataWriteRegister	REGBASE + 0xC0
#define AttributeDataReadRegister	REGBASE + 0xC1
#define AttributeRegister		AttributeIndexRegister
#define AttributeModeIndex		0x30
#define OverScanColorIndex		0x31
#define ColorPlaneEnableIndex		0x32
#define HorizPelPanIndex		0x33
#define ColorSelectIndex		0x34
#ifndef	PC98_EGC
#define SetVideoAttributeIndex( index ) \
	SET_INDEX_REGISTER( AttributeIndexRegister, index )
#define SetVideoAttribute( index, value ) \
	SetVideoAttributeIndex( index ) ; \
	SET_BYTE_REGISTER( AttributeDataWriteRegister, value )
#endif

	/* Graphics Registers  03CE & 03CF */
#define GraphicsIndexRegister		REGBASE + 0xCE
#define GraphicsDataRegister		REGBASE + 0xCF
#define GraphicsRegister		GraphicsIndexRegister
#define Set_ResetIndex			0x00
#define Enb_Set_ResetIndex		0x01
#define Color_CompareIndex		0x02
#define Data_RotateIndex		0x03
#define Read_Map_SelectIndex		0x04
#define Graphics_ModeIndex		0x05
#define MiscellaneousIndex		0x06
#define Color_Dont_CareIndex		0x07
#define Bit_MaskIndex			0x08
#ifndef	PC98_EGC
#define SetVideoGraphicsIndex( index ) \
	SET_INDEX_REGISTER( GraphicsIndexRegister, index )
#define SetVideoGraphicsData( value ) \
	SET_INDEX_REGISTER( GraphicsDataRegister, value )
#define SetVideoGraphics( index, value ) \
	SET_INDEXED_REGISTER( GraphicsRegister, index, value )
#endif

/* Sequencer Registers  03C4 & 03C5 */
#define SequencerIndexRegister		REGBASE + 0xC4
#define SequencerDataRegister		REGBASE + 0xC5
#define SequencerRegister		SequencerIndexRegister
#define Seq_ResetIndex			00
#define Clock_ModeIndex			01
#define Mask_MapIndex			02
#define Char_Map_SelectIndex		03
#define Memory_ModeIndex		04
#ifndef	PC98_EGC
#define SetVideoSequencerIndex( index ) \
	SET_INDEX_REGISTER( SequencerIndexRegister, index )
#define SetVideoSequencer( index, value ) \
	SET_INDEXED_REGISTER( SequencerRegister, index, value )
#endif

/* BIT CONSTANTS FOR THE VGA/EGA HARDWARE */
/* for the Graphics' Data_Rotate Register */
#define VGA_ROTATE_FUNC_SHIFT 3
#define VGA_COPY_MODE	( 0 << VGA_ROTATE_FUNC_SHIFT ) /* 0x00 */
#define VGA_AND_MODE	( 1 << VGA_ROTATE_FUNC_SHIFT ) /* 0x08 */
#define VGA_OR_MODE	( 2 << VGA_ROTATE_FUNC_SHIFT ) /* 0x10 */
#define VGA_XOR_MODE	( 3 << VGA_ROTATE_FUNC_SHIFT ) /* 0x18 */
/* for the Graphics' Graphics_Mode Register */
#define VGA_READ_MODE_SHIFT 3
#define VGA_WRITE_MODE_0	0
#define VGA_WRITE_MODE_1	1
#define VGA_WRITE_MODE_2	2
#define VGA_WRITE_MODE_3	3
#define VGA_READ_MODE_0		( 0 << VGA_READ_MODE_SHIFT )
#define VGA_READ_MODE_1		( 1 << VGA_READ_MODE_SHIFT )

#ifdef	PC98_EGC
/* I/O port address define for extended EGC */
#define		EGC_PLANE	0x4a0	/* EGC active plane select */
#define		EGC_READ	0x4a2	/* EGC FGC,EGC,Read Plane  */
#define		EGC_MODE	0x4a4	/* EGC Mode register & ROP */
#define		EGC_FGC		0x4a6	/* EGC Forground color     */
#define		EGC_MASK	0x4a8	/* EGC Mask register       */
#define		EGC_BGC		0x4aa	/* EGC Background color    */
#define		EGC_ADD		0x4ac	/* EGC Dest/Source address */
#define		EGC_LENGTH	0x4ae	/* EGC Bit length          */

#define		PALETTE_ADD	0xa8	/* Palette address         */
#define		PALETTE_GRE	0xaa	/* Palette Green           */
#define		PALETTE_RED	0xac	/* Palette Red             */
#define		PALETTE_BLU	0xae	/* Palette Blue            */
					
#define EGC_AND_MODE		0x2c8c	/* (S&P&D)|(~S&D) */
#define EGC_AND_INV_MODE	0x2c2c	/* (S&P&~D)|(~S&D) */
#define EGC_OR_MODE		0x2cec	/* S&(P|D)|(~S&D) */
#define EGC_OR_INV_MODE		0x2cbc	/* S&(P|~D)|(~S&D) */
#define EGC_XOR_MODE		0x2c6c	/* (S&(P&~D|~P&D))|(~S&D) */
#define EGC_XOR_INV_MODE	0x2c9c	/* (S&(P&D)|(~P&~D))|(~S&D) */
#define EGC_COPY_MODE		0x2cac /* (S&P)|(~S&D) */
#endif
