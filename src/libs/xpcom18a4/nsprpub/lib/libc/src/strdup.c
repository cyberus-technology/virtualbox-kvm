/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape Portable Runtime (NSPR).
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "plstr.h"
#include "prmem.h"
#include <string.h>
#ifdef VBOX_USE_IPRT_IN_NSPR
#include <iprt/mem.h>
#endif

PR_IMPLEMENT(char *)
PL_strdup(const char *s)
{
    char *rv;
    size_t n;

    if( (const char *)0 == s )
        s = "";

    n = strlen(s) + 1;

#ifdef VBOX_USE_IPRT_IN_NSPR
    rv = (char *)RTMemAlloc(n);
#else
    rv = (char *)malloc(n);
#endif
    if( (char *)0 == rv ) return rv;

    (void)memcpy(rv, s, n);

    return rv;
}

PR_IMPLEMENT(void)
PL_strfree(char *s)
{
#ifdef VBOX_USE_IPRT_IN_NSPR
    RTMemFree(s);
#else
    free(s);
#endif
}

PR_IMPLEMENT(char *)
PL_strndup(const char *s, PRUint32 max)
{
    char *rv;
    size_t l;

    if( (const char *)0 == s )
        s = "";

    l = PL_strnlen(s, max);

#ifdef VBOX_USE_IPRT_IN_NSPR
    rv = (char *)RTMemAlloc(l+1);
#else
    rv = (char *)malloc(l+1);
#endif
    if( (char *)0 == rv ) return rv;

    (void)memcpy(rv, s, l);
    rv[l] = '\0';

    return rv;
}
