/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef VBOX
#pragma ident	"%Z%%M%	%I%	%E% SMI"
#endif

#include <ctf_impl.h>
#include <libctf.h>
#ifndef VBOX
#include <sys/mman.h>
#include <stdarg.h>
#else
# include <iprt/asm.h>
# include <iprt/log.h>
#endif

void *
ctf_data_alloc(size_t size)
{
#ifndef VBOX
	return (mmap(NULL, size, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANON, -1, 0));
#else
    void *pv = RTMemPageAlloc(size);
    return pv == NULL ? MAP_FAILED : pv;
#endif
}

void
ctf_data_free(void *buf, size_t size)
{
#ifndef VBOX
	(void) munmap(buf, size);
#else
    RTMemProtect(buf, size, RTMEM_PROT_WRITE | RTMEM_PROT_READ);
    RTMemPageFree(buf, size);
#endif
}

void
ctf_data_protect(void *buf, size_t size)
{
#ifndef VBOX
	(void) mprotect(buf, size, PROT_READ);
#else
    int rc = RTMemProtect(buf, size, RTMEM_PROT_READ);
    AssertRC(rc);
#endif
}

void *
ctf_alloc(size_t size)
{
#ifndef VBOX
	return (malloc(size));
#else
    return RTMemAlloc(size);
#endif
}

/*ARGSUSED*/
void
ctf_free(void *buf, size_t size)
{
#ifndef VBOX
	free(buf);
#else
    RT_NOREF1(size);
    RTMemFree(buf);
#endif
}

const char *
ctf_strerror(int err)
{
	return (strerror(err));
}

/*PRINTFLIKE1*/
void
ctf_dprintf(const char *format, ...)
{
	if (_libctf_debug) {
		va_list alist;

		va_start(alist, format);
#ifndef VBOX
		(void) fputs("libctf DEBUG: ", stderr);
		(void) vfprintf(stderr, format, alist);
#else
        RTLogPrintf("libctf DEBUG: %N", format, alist);
#endif
		va_end(alist);
	}
}
