/* Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * of the copyright holder.
 */

#ifndef _XORG_SUN_KBD_H_
#define _XORG_SUN_KBD_H_

/*
 * Keyboard common implementation routines shared by "keyboard" driver
 * in sun_io.c and "kbd" driver in sun_kbd.c
 */

typedef struct {
    int			kbdFD;
    const char *	devName;
    int 		ktype;		/* Keyboard type from KIOCTYPE */
    Bool		kbdActive;	/* Have we set kbd modes for X? */
    int 		otranslation;	/* Original translation mode */
    int 		odirect;	/* Original "direct" mode setting */
    unsigned char	oleds;		/* Original LED state */
    const char *	strmod;		/* Streams module pushed on kbd device */
    const char *	audioDevName;	/* Audio device path to use for bell
					   or NULL to use keyboard beeper */
    enum {AB_INITIALIZING, AB_NORMAL} audioState;
    const unsigned char *keyMap;
} sunKbdPrivRec, *sunKbdPrivPtr;

/* sun_kbd.c */
extern int  sunKbdOpen	(const char *devName, pointer options);
extern int  sunKbdInit	(sunKbdPrivPtr priv, int kbdFD,
			 const char *devName, pointer options);
extern int  sunKbdOn	(sunKbdPrivPtr priv);
extern int  sunKbdOff	(sunKbdPrivPtr priv);
    
extern void sunKbdSoundBell 	(sunKbdPrivPtr priv,
				 int loudness, int pitch, int duration);

extern void sunKbdSetLeds 	(sunKbdPrivPtr priv, int leds);
extern int  sunKbdGetLeds 	(sunKbdPrivPtr priv);
extern void sunKbdSetRepeat 	(sunKbdPrivPtr priv, char rad);

/* sun_kbdEv.c */
#include <sys/vuid_event.h>
extern void sunPostKbdEvent	(int ktype, Firm_event *event);

extern const unsigned char *sunGetKbdMapping(int ktype);

#endif
