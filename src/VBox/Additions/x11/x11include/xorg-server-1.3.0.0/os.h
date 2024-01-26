/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/


#ifndef OS_H
#define OS_H

#include "misc.h"
#define ALLOCATE_LOCAL_FALLBACK(_size) Xalloc((unsigned long)(_size))
#define DEALLOCATE_LOCAL_FALLBACK(_ptr) Xfree((pointer)(_ptr))
#include <X11/Xalloca.h>
#include <stdarg.h>

#define NullFID ((FID) 0)

#define SCREEN_SAVER_ON   0
#define SCREEN_SAVER_OFF  1
#define SCREEN_SAVER_FORCER 2
#define SCREEN_SAVER_CYCLE  3

#ifndef MAX_REQUEST_SIZE
#define MAX_REQUEST_SIZE 65535
#endif
#ifndef MAX_BIG_REQUEST_SIZE
#define MAX_BIG_REQUEST_SIZE 4194303
#endif

typedef pointer	FID;
typedef struct _FontPathRec *FontPathPtr;
typedef struct _NewClientRec *NewClientPtr;

#ifndef xalloc
#define xnfalloc(size) XNFalloc((unsigned long)(size))
#define xnfcalloc(_num, _size) XNFcalloc((unsigned long)(_num)*(unsigned long)(_size))
#define xnfrealloc(ptr, size) XNFrealloc((pointer)(ptr), (unsigned long)(size))

#define xalloc(size) Xalloc((unsigned long)(size))
#define xcalloc(_num, _size) Xcalloc((unsigned long)(_num)*(unsigned long)(_size))
#define xrealloc(ptr, size) Xrealloc((pointer)(ptr), (unsigned long)(size))
#define xfree(ptr) Xfree((pointer)(ptr))
#define xstrdup(s) Xstrdup(s)
#define xnfstrdup(s) XNFstrdup(s)
#endif

#include <stdio.h>
#include <stdarg.h>

/* have to put $(SIGNAL_DEFINES) in DEFINES in Imakefile to get this right */
#ifdef SIGNALRETURNSINT
#define SIGVAL int
#else
#define SIGVAL void
#endif

extern Bool OsDelayInitColors;
extern void (*OsVendorVErrorFProc)(const char *, va_list args);

extern int WaitForSomething(
    int* /*pClientsReady*/
);

extern int ReadRequestFromClient(ClientPtr /*client*/);

extern Bool InsertFakeRequest(
    ClientPtr /*client*/, 
    char* /*data*/, 
    int /*count*/);

extern void ResetCurrentRequest(ClientPtr /*client*/);

extern void FlushAllOutput(void);

extern void FlushIfCriticalOutputPending(void);

extern void SetCriticalOutputPending(void);

extern int WriteToClient(ClientPtr /*who*/, int /*count*/, char* /*buf*/);

extern void ResetOsBuffers(void);

extern void InitConnectionLimits(void);

extern void CreateWellKnownSockets(void);

extern void ResetWellKnownSockets(void);

extern void CloseWellKnownConnections(void);

extern XID AuthorizationIDOfClient(ClientPtr /*client*/);

extern char *ClientAuthorized(
    ClientPtr /*client*/,
    unsigned int /*proto_n*/,
    char* /*auth_proto*/,
    unsigned int /*string_n*/,
    char* /*auth_string*/);

extern Bool EstablishNewConnections(
    ClientPtr /*clientUnused*/,
    pointer /*closure*/);

extern void CheckConnections(void);

extern void CloseDownConnection(ClientPtr /*client*/);

extern void AddGeneralSocket(int /*fd*/);

extern void RemoveGeneralSocket(int /*fd*/);

extern void AddEnabledDevice(int /*fd*/);

extern void RemoveEnabledDevice(int /*fd*/);

extern void OnlyListenToOneClient(ClientPtr /*client*/);

extern void ListenToAllClients(void);

extern void IgnoreClient(ClientPtr /*client*/);

extern void AttendClient(ClientPtr /*client*/);

extern void MakeClientGrabImpervious(ClientPtr /*client*/);

extern void MakeClientGrabPervious(ClientPtr /*client*/);

extern void AvailableClientInput(ClientPtr /* client */);

extern CARD32 GetTimeInMillis(void);

extern void AdjustWaitForDelay(
    pointer /*waitTime*/,
    unsigned long /*newdelay*/);

typedef	struct _OsTimerRec *OsTimerPtr;

typedef CARD32 (*OsTimerCallback)(
    OsTimerPtr /* timer */,
    CARD32 /* time */,
    pointer /* arg */);

extern void TimerInit(void);

extern Bool TimerForce(OsTimerPtr /* timer */);

#define TimerAbsolute (1<<0)
#define TimerForceOld (1<<1)

extern OsTimerPtr TimerSet(
    OsTimerPtr /* timer */,
    int /* flags */,
    CARD32 /* millis */,
    OsTimerCallback /* func */,
    pointer /* arg */);

extern void TimerCheck(void);
extern void TimerCancel(OsTimerPtr /* pTimer */);
extern void TimerFree(OsTimerPtr /* pTimer */);

extern void SetScreenSaverTimer(void);
extern void FreeScreenSaverTimer(void);

extern SIGVAL AutoResetServer(int /*sig*/);

extern SIGVAL GiveUp(int /*sig*/);

extern void UseMsg(void);

extern void InitGlobals(void);

extern void ProcessCommandLine(int /*argc*/, char* /*argv*/[]);

extern int set_font_authorizations(
    char ** /* authorizations */, 
    int * /*authlen */, 
    pointer /* client */);

#ifndef _HAVE_XALLOC_DECLS
#define _HAVE_XALLOC_DECLS
extern pointer Xalloc(unsigned long /*amount*/);
extern pointer Xcalloc(unsigned long /*amount*/);
extern pointer Xrealloc(pointer /*ptr*/, unsigned long /*amount*/);
extern void Xfree(pointer /*ptr*/);
#endif

extern pointer XNFalloc(unsigned long /*amount*/);
extern pointer XNFcalloc(unsigned long /*amount*/);
extern pointer XNFrealloc(pointer /*ptr*/, unsigned long /*amount*/);

extern void OsInitAllocator(void);

extern char *Xstrdup(const char *s);
extern char *XNFstrdup(const char *s);
extern char *Xprintf(const char *fmt, ...);
extern char *Xvprintf(const char *fmt, va_list va);
extern char *XNFprintf(const char *fmt, ...);
extern char *XNFvprintf(const char *fmt, va_list va);

typedef SIGVAL (*OsSigHandlerPtr)(int /* sig */);

extern OsSigHandlerPtr OsSignal(int /* sig */, OsSigHandlerPtr /* handler */);

extern int auditTrailLevel;

#ifdef SERVER_LOCK
extern void LockServer(void);
extern void UnlockServer(void);
#endif

extern int OsLookupColor(
    int	/*screen*/,
    char * /*name*/,
    unsigned /*len*/,
    unsigned short * /*pred*/,
    unsigned short * /*pgreen*/,
    unsigned short * /*pblue*/);

extern void OsInit(void);

extern void OsCleanup(Bool);

extern void OsVendorFatalError(void);

extern void OsVendorInit(void);

extern int OsInitColors(void);

void OsBlockSignals (void);

void OsReleaseSignals (void);

#if !defined(WIN32) && !defined(__UNIXOS2__)
extern int System(char *);
extern pointer Popen(char *, char *);
extern int Pclose(pointer);
extern pointer Fopen(char *, char *);
extern int Fclose(pointer);
#else
#define System(a) system(a)
#define Popen(a,b) popen(a,b)
#define Pclose(a) pclose(a)
#define Fopen(a,b) fopen(a,b)
#define Fclose(a) fclose(a)
#endif

extern void CheckUserParameters(int argc, char **argv, char **envp);
extern void CheckUserAuthorization(void);

extern int AddHost(
    ClientPtr	/*client*/,
    int         /*family*/,
    unsigned    /*length*/,
    pointer     /*pAddr*/);

extern Bool ForEachHostInFamily (
    int	    /*family*/,
    Bool    (* /*func*/ )(
            unsigned char * /* addr */,
            short           /* len */,
            pointer         /* closure */),
    pointer /*closure*/);

extern int RemoveHost(
    ClientPtr	/*client*/,
    int         /*family*/,
    unsigned    /*length*/,
    pointer     /*pAddr*/);

extern int GetHosts(
    pointer * /*data*/,
    int	    * /*pnHosts*/,
    int	    * /*pLen*/,
    BOOL    * /*pEnabled*/);

typedef struct sockaddr * sockaddrPtr;

extern int InvalidHost(sockaddrPtr /*saddr*/, int /*len*/, ClientPtr client);

extern int LocalClient(ClientPtr /* client */);

extern int LocalClientCred(ClientPtr, int *, int *);

extern int ChangeAccessControl(ClientPtr /*client*/, int /*fEnabled*/);

extern int GetAccessControl(void);


extern void AddLocalHosts(void);

extern void ResetHosts(char *display);

extern void EnableLocalHost(void);

extern void DisableLocalHost(void);

extern void AccessUsingXdmcp(void);

extern void DefineSelf(int /*fd*/);

extern void AugmentSelf(pointer /*from*/, int /*len*/);

extern void InitAuthorization(char * /*filename*/);

/* extern int LoadAuthorization(void); */

extern void RegisterAuthorizations(void);

extern XID AuthorizationToID (
	unsigned short	name_length,
	char		*name,
	unsigned short	data_length,
	char		*data);

extern int AuthorizationFromID (
	XID 		id,
	unsigned short	*name_lenp,
	char		**namep,
	unsigned short	*data_lenp,
	char		**datap);

extern XID CheckAuthorization(
    unsigned int /*namelength*/,
    char * /*name*/,
    unsigned int /*datalength*/,
    char * /*data*/,
    ClientPtr /*client*/,
    char ** /*reason*/
);

extern void ResetAuthorization(void);

extern int RemoveAuthorization (
    unsigned short	name_length,
    char		*name,
    unsigned short	data_length,
    char		*data);

extern int AddAuthorization(
    unsigned int	/*name_length*/,
    char *		/*name*/,
    unsigned int	/*data_length*/,
    char *		/*data*/);

extern XID GenerateAuthorization(
    unsigned int   /* name_length */,
    char	*  /* name */,
    unsigned int   /* data_length */,
    char	*  /* data */,
    unsigned int * /* data_length_return */,
    char	** /* data_return */);

#ifdef COMMANDLINE_CHALLENGED_OPERATING_SYSTEMS
extern void ExpandCommandLine(int * /*pargc*/, char *** /*pargv*/);
#endif

extern void ddxInitGlobals(void);

extern int ddxProcessArgument(int /*argc*/, char * /*argv*/ [], int /*i*/);

extern void ddxUseMsg(void);

/*
 *  idiom processing stuff
 */

extern xReqPtr PeekNextRequest(xReqPtr req, ClientPtr client, Bool readmore);

extern void SkipRequests(xReqPtr req, ClientPtr client, int numskipped);

/* int ReqLen(xReq *req, ClientPtr client)
 * Given a pointer to a *complete* request, return its length in bytes.
 * Note that if the request is a big request (as defined in the Big
 * Requests extension), the macro lies by returning 4 less than the
 * length that it actually occupies in the request buffer.  This is so you
 * can blindly compare the length with the various sz_<request> constants
 * in Xproto.h without having to know/care about big requests.
 */
#define ReqLen(_pxReq, _client) \
 ((_pxReq->length ? \
     (_client->swapped ? lswaps(_pxReq->length) : _pxReq->length) \
  : ((_client->swapped ? \
	lswapl(((CARD32*)_pxReq)[1]) : ((CARD32*)_pxReq)[1])-1) \
  ) << 2)

/* otherReqTypePtr CastxReq(xReq *req, otherReqTypePtr)
 * Cast the given request to one of type otherReqTypePtr to access
 * fields beyond the length field.
 */
#define CastxReq(_pxReq, otherReqTypePtr) \
    (_pxReq->length ? (otherReqTypePtr)_pxReq \
		    : (otherReqTypePtr)(((CARD32*)_pxReq)+1))

/* stuff for SkippedRequestsCallback */
extern CallbackListPtr SkippedRequestsCallback;
typedef struct {
    xReqPtr req;
    ClientPtr client;
    int numskipped;
} SkippedRequestInfoRec;

/* stuff for ReplyCallback */
extern CallbackListPtr ReplyCallback;
typedef struct {
    ClientPtr client;
    pointer replyData;
    unsigned long dataLenBytes;
    unsigned long bytesRemaining;
    Bool startOfReply;
} ReplyInfoRec;

/* stuff for FlushCallback */
extern CallbackListPtr FlushCallback;

extern void AbortDDX(void);
extern void ddxGiveUp(void);
extern int TimeSinceLastInputEvent(void);

/* Logging. */
typedef enum _LogParameter {
    XLOG_FLUSH,
    XLOG_SYNC,
    XLOG_VERBOSITY,
    XLOG_FILE_VERBOSITY
} LogParameter;

/* Flags for log messages. */
typedef enum {
    X_PROBED,			/* Value was probed */
    X_CONFIG,			/* Value was given in the config file */
    X_DEFAULT,			/* Value is a default */
    X_CMDLINE,			/* Value was given on the command line */
    X_NOTICE,			/* Notice */
    X_ERROR,			/* Error message */
    X_WARNING,			/* Warning message */
    X_INFO,			/* Informational message */
    X_NONE,			/* No prefix */
    X_NOT_IMPLEMENTED,		/* Not implemented */
    X_UNKNOWN = -1		/* unknown -- this must always be last */
} MessageType;

/* XXX Need to check which GCC versions have the format(printf) attribute. */
#if defined(__GNUC__) && \
    ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ > 4)))
#define _printf_attribute(a,b) __attribute((format(__printf__,a,b)))
#else
#define _printf_attribute(a,b) /**/
#endif

extern const char *LogInit(const char *fname, const char *backup);
extern void LogClose(void);
extern Bool LogSetParameter(LogParameter param, int value);
extern void LogVWrite(int verb, const char *f, va_list args);
extern void LogWrite(int verb, const char *f, ...) _printf_attribute(2,3);
extern void LogVMessageVerb(MessageType type, int verb, const char *format,
			    va_list args);
extern void LogMessageVerb(MessageType type, int verb, const char *format,
			   ...) _printf_attribute(3,4);
extern void LogMessage(MessageType type, const char *format, ...)
			_printf_attribute(2,3);
extern void FreeAuditTimer(void);
extern void AuditF(const char *f, ...) _printf_attribute(1,2);
extern void VAuditF(const char *f, va_list args);
extern void FatalError(const char *f, ...) _printf_attribute(1,2)
#if defined(__GNUC__) && \
    ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ > 4)))
__attribute((noreturn))
#endif
;

extern void VErrorF(const char *f, va_list args);
extern void ErrorF(const char *f, ...) _printf_attribute(1,2);
extern void Error(char *str);
extern void LogPrintMarkers(void);

#endif /* OS_H */
