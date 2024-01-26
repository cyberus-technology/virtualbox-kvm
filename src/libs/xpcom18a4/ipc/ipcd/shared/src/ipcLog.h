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
 * The Original Code is Mozilla IPC.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@netscape.com>
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

#ifndef ipcLog_h__
#define ipcLog_h__

#include "nscore.h"
#include "prtypes.h"

#ifndef VBOX
#ifdef DEBUG
#define IPC_LOGGING
#endif
#endif

#ifdef IPC_LOGGING

#ifdef VBOX

/* Redefine logging group to IPC */
# ifdef LOG_GROUP
#  undef LOG_GROUP
# endif
# define LOG_GROUP LOG_GROUP_IPC

/* Ensure log macros are enabled */
# ifndef LOG_ENABLED
#  define LOG_ENABLED
# endif

#include <VBox/log.h>

extern NS_HIDDEN_(void) IPC_InitLog(const char *prefix);
extern NS_HIDDEN_(void) IPC_Log(const char *fmt, ...);
extern NS_HIDDEN_(void) IPC_LogBinary(const PRUint8 *data, PRUint32 len);

# define IPC_LOG(_args) \
    PR_BEGIN_MACRO \
        if (IPC_LOG_ENABLED()) \
            IPC_Log _args; \
    PR_END_MACRO

/* IPC_Log() internally uses LogFlow() so use LogIsFlowEnabled() below */
# define IPC_LOG_ENABLED() (LogIsFlowEnabled())

# define LOG(args)     IPC_LOG(args)

#else  /* !VBOX */

extern PRBool ipcLogEnabled;
extern NS_HIDDEN_(void) IPC_InitLog(const char *prefix);
extern NS_HIDDEN_(void) IPC_Log(const char *fmt, ...);
extern NS_HIDDEN_(void) IPC_LogBinary(const PRUint8 *data, PRUint32 len);

#define IPC_LOG(_args)         \
    PR_BEGIN_MACRO             \
        if (ipcLogEnabled)     \
            IPC_Log _args;     \
    PR_END_MACRO

#define IPC_LOG_ENABLED() ipcLogEnabled
    
#define LOG(args)     IPC_LOG(args)

#endif /* !VBOX */

#else // IPC_LOGGING

#define IPC_InitLog(prefix) PR_BEGIN_MACRO PR_END_MACRO
#define IPC_LogBinary(data, len) PR_BEGIN_MACRO PR_END_MACRO
#define IPC_LOG(_args) PR_BEGIN_MACRO PR_END_MACRO
#define IPC_LOG_ENABLED() (0)
#define LOG(args) PR_BEGIN_MACRO PR_END_MACRO

#endif // IPC_LOGGING

#endif // !ipcLog_h__
