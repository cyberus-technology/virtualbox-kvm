/* $Xorg: Oid.h,v 1.3 2000/08/17 19:48:06 cpqbld Exp $ */
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _Xp_Oid_h
#define _Xp_Oid_h

#include <X11/Xproto.h>

/*
 * include the auto-generated XpOid enum definition
 */
#include "OidDefs.h"

/*
 * messages
 */
#define XPMSG_WARN_MSS "Syntax error parsing medium-source-sizes"
#define XPMSG_WARN_ITM "Syntax error parsing input-trays-medium"
#define XPMSG_WARN_DOC_FMT "Syntax error parsing document format"
#define XPMSG_WARN_DOCFMT_LIST "Syntax error parsing document format list"
#define XPMSG_WARN_CARD_LIST "Syntax error parsing cardinal list"

/*
 * macros for memory allocation
 */
#define XpOidMalloc(size) ((char*)Xalloc((unsigned long)(size)))
#define XpOidCalloc(count, size) \
	((char*)Xcalloc((unsigned long)((count)*(size))))
#define XpOidFree(mem) (Xfree((unsigned long*)(mem)))

/*
 * list of object identifiers
 */
typedef struct _XpOidList
{
    XpOid* list;
    int count;
} XpOidList;

/*
 * linked list of object identifiers
 */
typedef struct XpOidNodeStruct
{
    XpOid oid;
    struct XpOidNodeStruct* next;
} *XpOidNode;

typedef struct _XpOidLinkedList
{
    XpOidNode head;
    XpOidNode tail;
    XpOidNode current;
    int count;
} XpOidLinkedList;

/*
 * XpOidMediumSourceSize and related definitions
 */
typedef struct
{
    float minimum_x;
    float maximum_x;
    float minimum_y;
    float maximum_y;
} XpOidArea;

typedef struct
{
    float lower_bound;
    float upper_bound;
} XpOidRealRange; 

typedef struct
{
    XpOidRealRange range_across_feed;
    float increment_across_feed;
    XpOidRealRange range_in_feed;
    float increment_in_feed;
    BOOL long_edge_feeds;
    XpOidArea assured_reproduction_area;
} XpOidMediumContinuousSize;

typedef struct
{
    XpOid page_size;
    BOOL long_edge_feeds;
    XpOidArea assured_reproduction_area;
} XpOidMediumDiscreteSize;

typedef struct 
{
    XpOidMediumDiscreteSize* list;
    int count;
} XpOidMediumDiscreteSizeList;

typedef struct
{
    XpOid input_tray; /* may be set to xpoid_none or xpoid_unspecified */
    enum { XpOidMediumSS_DISCRETE, XpOidMediumSS_CONTINUOUS } mstag;
    union
    {
	XpOidMediumDiscreteSizeList* discrete;
	XpOidMediumContinuousSize* continuous_size;
    } ms; /* "ms" is short for medium-size */

} XpOidMediumSourceSize;

typedef struct
{
    XpOidMediumSourceSize* mss;
    int count;
} XpOidMediumSS;


typedef struct
{
    XpOid input_tray; /* may be set to xpoid_none */
    XpOid medium;
} XpOidTrayMedium;

typedef struct
{
    XpOidTrayMedium* list;
    int count;
} XpOidTrayMediumList;

typedef enum {
    XPOID_NOTIFY_UNSUPPORTED,
    XPOID_NOTIFY_NONE,
    XPOID_NOTIFY_EMAIL
} XpOidNotify;

typedef struct
{
    unsigned long *list;
    int count;
} XpOidCardList;

typedef struct
{
    char* format;
    char* variant;
    char* version;
} XpOidDocFmt;

typedef struct
{
    XpOidDocFmt* list;
    int count;
} XpOidDocFmtList;


/*
 * XpOid public methods
 */
const char* XpOidString(XpOid);
int XpOidStringLength(XpOid);
XpOid XpOidFromString(const char* value);
BOOL XpOidTrayMediumListHasTray(const XpOidTrayMediumList* list, XpOid tray);

/*
 * XpOidList public methods
 */
XpOidList* XpOidListNew(const char* value_string,
			       const XpOidList* valid_oids);
#define XpOidListInit(l, a, c) { (l)->list = (a); (l)->count = (c); }
void XpOidListDelete(XpOidList*);
#define XpOidListCount(l) ((l) ? (l)->count : 0)
#define XpOidListGetOid(l, i) ((l) ? (l)->list[(i)] : xpoid_none)
int XpOidListGetIndex(const XpOidList* list, XpOid oid);
BOOL XpOidListHasOid(const XpOidList* list, XpOid oid);
char* XpOidListString(const XpOidList*);


/*
 * XpOidLinkedList public methods
 */
XpOidLinkedList* XpOidLinkedListNew(void);
void XpOidLinkedListDelete(XpOidLinkedList*);
#define XpOidLinkedListCount(l) ((l) ? (l)->count : 0)
XpOid XpOidLinkedListGetOid(XpOidLinkedList* list, int i);
void XpOidLinkedListAddOid(XpOidLinkedList* list, XpOid oid);
int XpOidLinkedListGetIndex(XpOidLinkedList* list, XpOid oid);
BOOL XpOidLinkedListHasOid(XpOidLinkedList* list,
				  XpOid oid);
XpOid XpOidLinkedListFirstOid(XpOidLinkedList* list);
XpOid XpOidLinkedListNextOid(XpOidLinkedList* list);

/*
 * XpOidMediumSourceSize public methods
 */
XpOidMediumSS* XpOidMediumSSNew(const char* value_string,
				       const XpOidList* valid_trays,
				       const XpOidList* valid_medium_sizes);
void XpOidMediumSSDelete(XpOidMediumSS*);
#define XpOidMediumSSCount(me) ((me) ? (me)->count : 0)
BOOL XpOidMediumSSHasSize(XpOidMediumSS*, XpOid medium_size);
char* XpOidMediumSSString(const XpOidMediumSS*);

/*
 * XpOidTrayMediumList public methods
 */
XpOidTrayMediumList* XpOidTrayMediumListNew(const char* value_string,
					    const XpOidList* valid_trays,
					    const XpOidMediumSS* msss);
void XpOidTrayMediumListDelete(XpOidTrayMediumList* me);
#define XpOidTrayMediumListCount(me) ((me) ? (me)->count : 0)
#define XpOidTrayMediumListTray(me, i) \
    ((me) ? (me)->list[(i)].input_tray : xpoid_none)
#define XpOidTrayMediumListMedium(me, i) \
    ((me) ? (me)->list[(i)].medium : xpoid_none)
char* XpOidTrayMediumListString(const XpOidTrayMediumList*);

/*
 * XpOidNotify public methods
 */
XpOidNotify XpOidNotifyParse(const char* value_string);
const char* XpOidNotifyString(XpOidNotify notify);

/*
 * XpOidDocFmt public methods
 */
XpOidDocFmt* XpOidDocFmtNew(const char* value_string);
void XpOidDocFmtDelete(XpOidDocFmt*);
char* XpOidDocFmtString(XpOidDocFmt*);

/*
 * XpOidDocFmtList public methods
 */
XpOidDocFmtList* XpOidDocFmtListNew(const char* value_string,
				    const XpOidDocFmtList* valid_fmts);
void XpOidDocFmtListDelete(XpOidDocFmtList*);
char* XpOidDocFmtListString(const XpOidDocFmtList*);
#define XpOidDocFmtListCount(me) ((me) ? (me)->count : 0)
#define XpOidDocFmtListGetDocFmt(me, i) \
    ((me) ? &(me)->list[(i)] : (XpDocFmt*)NULL)
BOOL XpOidDocFmtListHasFmt(const XpOidDocFmtList* list,
			   const XpOidDocFmt* fmt);
/*
 * XpOidCardList public methods
 */
XpOidCardList* XpOidCardListNew(const char* value_string,
				       const XpOidCardList* valid_cards);
#define XpOidCardListInit(l, a, c) { (l)->list = (a); (l)->count = (c); }
void XpOidCardListDelete(XpOidCardList*);
char* XpOidCardListString(const XpOidCardList*);
#define XpOidCardListCount(me) ((me) ? (me)->count : 0)
#define XpOidCardListGetCard(me, i) ((me) ? (me)->list[(i)] : 0)
BOOL XpOidCardListHasCard(const XpOidCardList*, unsigned long);

/*
 * misc parsing functions
 */
BOOL XpOidParseUnsignedValue(const char* value_string,
			     const char** ptr_return,
			     unsigned long* unsigned_return);


#endif /* _Xp_Oid_h - don't add anything after this line */
