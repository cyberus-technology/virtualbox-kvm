/* $Xorg: PclDef.h,v 1.3 2000/08/17 19:48:08 cpqbld Exp $ */
/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		PclDef.h
**    *
**    *  Contents:  extran defines and includes for the Pcl driver
**    *             for a printing X server.
**    *
**    *  Created:	7/31/95
**    *
**    *********************************************************
** 
********************************************************************/
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

#ifndef _PCLDEF_H_
#define _PCLDEF_H_

#define DT_PRINT_JOB_HEADER "DT_PRINT_JOB_HEADER"
#define DT_PRINT_JOB_TRAILER "DT_PRINT_JOB_TRAILER"
#define DT_PRINT_JOB_COMMAND "DT_PRINT_JOB_COMMAND"
#define DT_PRINT_JOB_EXEC_COMMAND "DT_PRINT_JOB_EXEC_COMMAND"
#define DT_PRINT_JOB_EXEC_OPTIONS "DT_PRINT_JOB_EXEC_OPTION"
#define DT_PRINT_PAGE_HEADER "DT_PRINT_PAGE_HEADER"
#define DT_PRINT_PAGE_TRAILER "DT_PRINT_PAGE_TRAILER"
#define DT_PRINT_PAGE_COMMAND "DT_PRINT_PAGE_COMMAND"

#define DT_IN_FILE_STRING "%(InFile)%"
#define DT_OUT_FILE_STRING "%(OutFile)%"
#define DT_ALLOWED_COMMANDS_FILE "printCommands"

#endif  /* _PCLDEF_H_ */
