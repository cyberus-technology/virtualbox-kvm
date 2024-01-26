/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.h,v 1.7 2003/10/08 14:58:27 dawes Exp $ */

/*
 * Copyright (c) 1997-2000 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _xf86_config_h
#define _xf86_config_h

#ifdef HAVE_PARSER_DECLS
/*
 * global structure that holds the result of parsing the config file
 */
extern XF86ConfigPtr xf86configptr;
#endif

typedef enum _ConfigStatus {
    CONFIG_OK = 0,
    CONFIG_PARSE_ERROR,
    CONFIG_NOFILE
} ConfigStatus;

/*
 * prototypes
 */
char ** xf86ModulelistFromConfig(pointer **);
char ** xf86DriverlistFromConfig(void);
char ** xf86DriverlistFromCompile(void);
char ** xf86InputDriverlistFromConfig(void);
char ** xf86InputDriverlistFromCompile(void);
Bool xf86BuiltinInputDriver(const char *);
ConfigStatus xf86HandleConfigFile(Bool);

Bool xf86AutoConfig(void);

#endif /* _xf86_config_h */
