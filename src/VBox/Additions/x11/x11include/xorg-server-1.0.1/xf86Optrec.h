/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/xf86Optrec.h,v 1.11 2003/08/24 17:37:08 dawes Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */
/*
 * Copyright (c) 1997-2001 by The XFree86 Project, Inc.
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


/* 
 * This file contains the Option Record that is passed between the Parser,
 * and Module setup procs.
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _xf86Optrec_h_
#define _xf86Optrec_h_
#include <stdio.h>

/* 
 * all records that need to be linked lists should contain a GenericList as
 * their first field.
 */
typedef struct generic_list_rec
{
	void *next;
}
GenericListRec, *GenericListPtr, *glp;

/*
 * All options are stored using this data type.
 */
typedef struct
{
	GenericListRec list;
	char *opt_name;
	char *opt_val;
	int opt_used;
	char *opt_comment;
}
XF86OptionRec, *XF86OptionPtr;


XF86OptionPtr xf86addNewOption(XF86OptionPtr head, char *name, char *val);
XF86OptionPtr xf86optionListDup(XF86OptionPtr opt);
void xf86optionListFree(XF86OptionPtr opt);
char *xf86optionName(XF86OptionPtr opt);
char *xf86optionValue(XF86OptionPtr opt);
XF86OptionPtr xf86newOption(char *name, char *value);
XF86OptionPtr xf86nextOption(XF86OptionPtr list);
XF86OptionPtr xf86findOption(XF86OptionPtr list, const char *name);
char *xf86findOptionValue(XF86OptionPtr list, const char *name);
int xf86findOptionBoolean (XF86OptionPtr, const char *, int);
XF86OptionPtr xf86optionListCreate(const char **options, int count, int used);
XF86OptionPtr xf86optionListMerge(XF86OptionPtr head, XF86OptionPtr tail);
char *xf86configStrdup (const char *s);
int xf86nameCompare (const char *s1, const char *s2);
char *xf86uLongToString(unsigned long i);
void xf86debugListOptions(XF86OptionPtr);
XF86OptionPtr xf86parseOption(XF86OptionPtr head);
void xf86printOptionList(FILE *fp, XF86OptionPtr list, int tabs);


#endif /* _xf86Optrec_h_ */
