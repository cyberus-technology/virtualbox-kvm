/* $Xorg: AttrValid.h,v 1.4 2001/03/14 18:43:40 pookie Exp $ */
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

#ifndef _Xp_AttrValid_h
#define _Xp_AttrValid_h

#include <X11/extensions/Printstr.h>
#include "Oid.h"

#define XpNumber(a) (sizeof(a) / sizeof(*(a)))

/*
 * Attribute pool validation valid values and defaults
 */
typedef struct
{
    XpOidList* valid_content_orientations_supported;
    XpOidList* default_content_orientations_supported;

    XpOidDocFmtList* valid_document_formats_supported;
    XpOidDocFmtList* default_document_formats_supported;

    XpOidList* valid_input_trays;
    XpOidList* valid_medium_sizes;

    XpOidList* valid_plexes_supported;
    XpOidList* default_plexes_supported;

    XpOidCardList* valid_printer_resolutions_supported;
    XpOidCardList* default_printer_resolutions_supported;
    
    XpOidDocFmtList* valid_xp_embedded_formats_supported;
    XpOidDocFmtList* default_xp_embedded_formats_supported;

    XpOidList* valid_xp_listfonts_modes_supported;
    XpOidList* default_xp_listfonts_modes_supported;

    XpOidDocFmtList* valid_xp_raw_formats_supported;
    XpOidDocFmtList* default_xp_raw_formats_supported;

    XpOidList* valid_xp_setup_proviso;

    XpOidDocFmt* default_document_format;
    XpOidList* valid_available_compressions_supported;
    XpOidList* default_available_compressions_supported;
    
} XpValidatePoolsRec;

/*
 * XpOid resource access
 */
#define XpGetStringAttr(pContext, pool, oid) \
    (const char*)XpGetOneAttribute(pContext, pool, (char*)XpOidString(oid))
#define XpPutStringAttr(pContext, pool, oid, value) \
    XpPutOneAttribute(pContext, pool, XpOidString(oid), value)

#ifdef _XP_PRINT_SERVER_       /* needed for XpContextPtr in Printstr.h */

/*
 * XpOid-valued attribute access
 */
XpOid XpGetOidAttr(XpContextPtr pContext,
		   XPAttributes pool,
		   XpOid oid,
		   const XpOidList* valid_oid_list);
void XpPutOidAttr(XpContextPtr pContext,
		  XPAttributes pool,
		  XpOid oid,
		  XpOid value_oid);
void XpValidateOidAttr(XpContextPtr pContext,
		       XPAttributes pool,
		       XpOid oid,
		       const XpOidList* valid_oids,
		       XpOid default_oid);
/*
 * cardinal-valued attribute access
 */
unsigned long XpGetCardAttr(XpContextPtr pContext,
			    XPAttributes pool,
			    XpOid oid,
			    const XpOidCardList* valid_card_list);
void XpPutCardAttr(XpContextPtr pContext,
		   XPAttributes pool,
		   XpOid oid,
		   unsigned long value_card);
void XpValidateCardAttr(XpContextPtr pContext,
			XPAttributes pool,
			XpOid oid,
			const XpOidCardList* valid_cards,
			unsigned long default_card);
/*
 * XpOidList-valued attribute access
 */
XpOidList* XpGetListAttr(XpContextPtr pContext,
			 XPAttributes pool,
			 XpOid oid,
			 const XpOidList* valid_oid_list);
void XpPutListAttr(XpContextPtr pContext,
		   XPAttributes pool,
		   XpOid oid,
		   const XpOidList* list);
void XpValidateListAttr(XpContextPtr pContext,
			XPAttributes pool,
			XpOid oid,
			const XpOidList* valid_oids,
			const XpOidList* default_oids);
/*
 * XpOidCardList-valued attribute access
 */
XpOidCardList* XpGetCardListAttr(XpContextPtr pContext,
				 XPAttributes pool,
				 XpOid oid,
				 const XpOidCardList* valid_card_list);
void XpPutCardListAttr(XpContextPtr pContext,
		       XPAttributes pool,
		       XpOid oid,
		       const XpOidCardList* list);
void XpValidateCardListAttr(XpContextPtr pContext,
			    XPAttributes pool,
			    XpOid oid,
			    const XpOidCardList* valid_cards,
			    const XpOidCardList* default_cards);
/*
 * XpOidDocFmtList-valued attribute access
 */
XpOidDocFmtList* XpGetDocFmtListAttr(XpContextPtr pContext,
				     XPAttributes pool,
				     XpOid oid,
				     const XpOidDocFmtList* valid_fmt_list);
void XpPutDocFmtListAttr(XpContextPtr pContext,
			 XPAttributes pool,
			 XpOid oid,
			 const XpOidDocFmtList* list);
void XpValidateDocFmtListAttr(XpContextPtr pContext,
			      XPAttributes pool,
			      XpOid oid,
			      const XpOidDocFmtList* valid_fmts,
			      const XpOidDocFmtList* default_fmts);
/*
 * XpOidMediumSS-valued attribute access
 */
XpOidMediumSS* XpGetMediumSSAttr(XpContextPtr pContext,
				 XPAttributes pool,
				 XpOid oid,
				 const XpOidList* valid_trays,
				 const XpOidList* valid_sizes);
void XpPutMediumSSAttr(XpContextPtr pContext,
		       XPAttributes pool,
		       XpOid oid,
		       const XpOidMediumSS* msss);
const XpOidMediumSS* XpGetDefaultMediumSS(void);

/*
 * XpOidTrayMediumList-valued attribute access
 */
XpOidTrayMediumList* XpGetTrayMediumListAttr(XpContextPtr pContext,
					     XPAttributes pool,
					     XpOid oid,
					     const XpOidList* valid_trays,
					     const XpOidMediumSS* msss);
void XpPutTrayMediumListAttr(XpContextPtr pContext,
			     XPAttributes pool,
			     XpOid oid,
			     const XpOidTrayMediumList* tm);
/*
 * Attribute pool validation
 */
void XpValidateAttributePool(XpContextPtr pContext,
			     XPAttributes pool,
			     const XpValidatePoolsRec* vpr);
void XpValidatePrinterPool(XpContextPtr pContext,
			   const XpValidatePoolsRec* vpr);
void XpValidateJobPool(XpContextPtr pContext,
		       const XpValidatePoolsRec* vpr);
void XpValidateDocumentPool(XpContextPtr pContext,
			    const XpValidatePoolsRec* vpr);
void XpValidatePagePool(XpContextPtr pContext,
			const XpValidatePoolsRec* vpr);

#endif /* _XP_PRINT_SERVER_ */

#endif /* _Xp_AttrValid_h - don't add anything after this line */
