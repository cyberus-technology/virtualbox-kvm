/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
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
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/loader.h,v 1.6 2001/07/07 01:43:58 paulo Exp $
 */

#ifdef USE_MODULES
#ifndef LOADER_PRIVATE
#include "config.h"
#include "stubs.h"

#else

#ifndef XFree86LOADER
#define XFree86LOADER		/* not really */
#endif
#define IN_LOADER

#include "xf86.h"
#include "xf86str.h"
#include "xf86Opt.h"
#include "xf86Module.h"

#ifndef XINPUT
#define XINPUT
#endif
#include "xf86Xinput.h"

#include <X11/fonts/fontmod.h>
#include "loaderProcs.h"

#include <sym.h>

void LoaderDefaultFunc(void);
#endif

#ifndef _xf86cfg_loader_h
#define _xf86cfg_loader_h

void xf86cfgLoaderInit(void);
void xf86cfgLoaderInitList(int);
void xf86cfgLoaderFreeList(void);
int xf86cfgCheckModule(void);

#ifndef LOADER_PRIVATE
/* common/xf86Opt.h */
typedef struct {
    double freq;
    int units;
} OptFrequency;

typedef union {
    unsigned long       num;
    char *              str;
    double              realnum;
    Bool		xbool;
    OptFrequency	freq;
} ValueUnion;

typedef enum {
    OPTV_NONE = 0,
    OPTV_INTEGER,
    OPTV_STRING,                /* a non-empty string */
    OPTV_ANYSTR,                /* Any string, including an empty one */
    OPTV_REAL,
    OPTV_BOOLEAN,
    OPTV_FREQ
} OptionValueType;

typedef enum {
    OPTUNITS_HZ = 1,
    OPTUNITS_KHZ,
    OPTUNITS_MHZ
} OptFreqUnits;

typedef struct {
    int                 token;
    const char*         name;
    OptionValueType     type;
    ValueUnion          value;
    Bool                found;
} OptionInfoRec, *OptionInfoPtr;

/* fontmod.h */
typedef void (*InitFont)(void);

typedef struct {
    InitFont	initFunc;
    char *	name;
    void	*module;
} FontModule;

extern FontModule *FontModuleList;

typedef struct {
    int                 token;          /* id of the token */
    const char *        name;           /* token name */
} SymTabRec, *SymTabPtr;
#endif	/* !LOADER_PRIVATE */

typedef enum {
    NullModule = 0,
    VideoModule,
    InputModule,
    GenericModule,
    FontRendererModule
} ModuleType;

typedef struct _xf86cfgModuleOptions {
    char *name;
    ModuleType type;
    OptionInfoPtr option;
    int vendor;
    SymTabPtr chipsets;
    struct _xf86cfgModuleOptions *next;
} xf86cfgModuleOptions;

extern xf86cfgModuleOptions *module_options;

/* When adding a new code to the LEGEND, also update checkerLegend
 * in loader.c
 */
extern char **checkerLegend;
extern int *checkerErrors;
#define	CHECKER_OPTIONS_FILE_MISSING			1
#define	CHECKER_OPTION_DESCRIPTION_MISSING		2
#define CHECKER_LOAD_FAILED				3
#define CHECKER_RECOGNIZED_AS				4
#define CHECKER_NO_OPTIONS_AVAILABLE			5
#define CHECKER_NO_VENDOR_CHIPSET			6
#define CHECKER_CANNOT_VERIFY_CHIPSET			7
#define	CHECKER_OPTION_UNUSED				8
#define CHECKER_NOMATCH_CHIPSET_STRINGS			9
#define CHECKER_CHIPSET_NOT_LISTED			10
#define CHECKER_CHIPSET_NOT_SUPPORTED			11
#define CHECKER_CHIPSET_NO_VENDOR			12
#define CHECKER_NO_CHIPSETS				13
#define CHECKER_FILE_MODULE_NAME_MISMATCH		14

#define CHECKER_LAST_MESSAGE				14

extern void CheckMsg(int, char*, ...);

#ifndef LOADER_PRIVATE
int LoaderInitializeOptions(void);
#endif
#endif /* USE_MODULES */

#endif /* _xf86cfg_loader_h */
