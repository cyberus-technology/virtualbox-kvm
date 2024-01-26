/* vim:set ts=2 sw=2 et cindent: */
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
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
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

#include "ipcdclient.h"
#include "ipcConnection.h"
#include "ipcConfig.h"
#include "ipcMessageQ.h"
#include "ipcMessageUtils.h"
#include "ipcLog.h"
#include "ipcm.h"

#include "nsIFile.h"
#include "nsEventQueueUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCOMPtr.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"
#include "nsAutoLock.h"
#include "nsProxyRelease.h"
#include "nsCOMArray.h"

#include "prio.h"
#include "prproces.h"
#include "pratom.h"

#ifdef VBOX
# include <iprt/critsect.h>
# define VBOX_WITH_IPCCLIENT_RW_CS
#endif

/* ------------------------------------------------------------------------- */

#define IPC_REQUEST_TIMEOUT PR_SecondsToInterval(30)

/* ------------------------------------------------------------------------- */

class ipcTargetData
{
public:
  static NS_HIDDEN_(ipcTargetData*) Create();

  // threadsafe addref/release
  NS_HIDDEN_(nsrefcnt) AddRef()  { return PR_AtomicIncrement(&refcnt); }
  NS_HIDDEN_(nsrefcnt) Release() { PRInt32 r = PR_AtomicDecrement(&refcnt); if (r == 0) delete this; return r; }

  NS_HIDDEN_(void) SetObserver(ipcIMessageObserver *aObserver, PRBool aOnCurrentThread);

  // protects access to the members of this class
  PRMonitor *monitor;

  // this may be null
  nsCOMPtr<ipcIMessageObserver> observer;

  // the message observer is called via this event queue
  nsCOMPtr<nsIEventQueue> eventQ;

  // incoming messages are added to this list
  ipcMessageQ pendingQ;

  // non-zero if the observer has been disabled (this means that new messages
  // should not be dispatched to the observer until the observer is re-enabled
  // via IPC_EnableMessageObserver).
  PRInt32 observerDisabled;

private:

  ipcTargetData()
    : monitor(nsAutoMonitor::NewMonitor("ipcTargetData"))
    , observerDisabled(0)
    , refcnt(0)
    {}

  ~ipcTargetData()
  {
    if (monitor)
      nsAutoMonitor::DestroyMonitor(monitor);
  }

  PRInt32 refcnt;
};

ipcTargetData *
ipcTargetData::Create()
{
  ipcTargetData *td = new ipcTargetData;
  if (!td)
    return NULL;

  if (!td->monitor)
  {
    delete td;
    return NULL;
  }
  return td;
}

void
ipcTargetData::SetObserver(ipcIMessageObserver *aObserver, PRBool aOnCurrentThread)
{
  observer = aObserver;

  if (aOnCurrentThread)
    NS_GetCurrentEventQ(getter_AddRefs(eventQ));
  else
    eventQ = nsnull;
}

/* ------------------------------------------------------------------------- */

typedef nsRefPtrHashtable<nsIDHashKey, ipcTargetData> ipcTargetMap;

class ipcClientState
{
public:
  static NS_HIDDEN_(ipcClientState *) Create();

  ~ipcClientState()
  {
#ifndef VBOX_WITH_IPCCLIENT_RW_CS
    if (monitor)
      nsAutoMonitor::DestroyMonitor(monitor);
#else
    RTCritSectRwDelete(&critSect);
#endif
  }

#ifndef VBOX_WITH_IPCCLIENT_RW_CS
  //
  // the monitor protects the targetMap and the connected and shutdown flags.
  //
  // NOTE: we use a PRMonitor for this instead of a PRLock because we need
  //       the lock to be re-entrant.  since we don't ever need to wait on
  //       this monitor, it might be worth it to implement a re-entrant
  //       wrapper for PRLock.
  //
  PRMonitor    *monitor;
#else  /* VBOX_WITH_IPCCLIENT_RW_CS */
  RTCRITSECTRW  critSect;
#endif /* VBOX_WITH_IPCCLIENT_RW_CS */
  ipcTargetMap  targetMap;
  PRBool        connected;
  PRBool        shutdown;

  // our process's client id
  PRUint32      selfID;

  nsCOMArray<ipcIClientObserver> clientObservers;

private:

  ipcClientState()
#ifndef VBOX_WITH_IPCCLIENT_RW_CS
    : monitor(nsAutoMonitor::NewMonitor("ipcClientState"))
    , connected(PR_FALSE)
#else
    : connected(PR_FALSE)
#endif
    , shutdown(PR_FALSE)
    , selfID(0)
  {
#ifdef VBOX_WITH_IPCCLIENT_RW_CS
    /* Not employing the lock validator here to keep performance up in debug builds. */
    RTCritSectRwInitEx(&critSect, RTCRITSECT_FLAGS_NO_LOCK_VAL, NIL_RTLOCKVALCLASS, RTLOCKVAL_SUB_CLASS_NONE, NULL);
#endif
  }
};

ipcClientState *
ipcClientState::Create()
{
  ipcClientState *cs = new ipcClientState;
  if (!cs)
    return NULL;

#ifndef VBOX_WITH_IPCCLIENT_RW_CS
  if (!cs->monitor || !cs->targetMap.Init())
#else
  if (!RTCritSectRwIsInitialized(&cs->critSect) || !cs->targetMap.Init())
#endif
  {
    delete cs;
    return NULL;
  }

  return cs;
}

/* ------------------------------------------------------------------------- */

static ipcClientState *gClientState;

static PRBool
GetTarget(const nsID &aTarget, ipcTargetData **td)
{
#ifndef VBOX_WITH_IPCCLIENT_RW_CS
  nsAutoMonitor mon(gClientState->monitor);
  return gClientState->targetMap.Get(nsIDHashKey(&aTarget).GetKey(), td);
#else
  RTCritSectRwEnterShared(&gClientState->critSect);
  PRBool fRc = gClientState->targetMap.Get(nsIDHashKey(&aTarget).GetKey(), td);
  RTCritSectRwLeaveShared(&gClientState->critSect);
  return fRc;
#endif
}

static PRBool
PutTarget(const nsID &aTarget, ipcTargetData *td)
{
#ifndef VBOX_WITH_IPCCLIENT_RW_CS
  nsAutoMonitor mon(gClientState->monitor);
  return gClientState->targetMap.Put(nsIDHashKey(&aTarget).GetKey(), td);
#else
  RTCritSectRwEnterExcl(&gClientState->critSect);
  PRBool fRc = gClientState->targetMap.Put(nsIDHashKey(&aTarget).GetKey(), td);
  RTCritSectRwLeaveExcl(&gClientState->critSect);
  return fRc;
#endif
}

static void
DelTarget(const nsID &aTarget)
{
#ifndef VBOX_WITH_IPCCLIENT_RW_CS
  nsAutoMonitor mon(gClientState->monitor);
  gClientState->targetMap.Remove(nsIDHashKey(&aTarget).GetKey());
#else
  RTCritSectRwEnterExcl(&gClientState->critSect);
  gClientState->targetMap.Remove(nsIDHashKey(&aTarget).GetKey());
  RTCritSectRwLeaveExcl(&gClientState->critSect);
#endif
}

/* ------------------------------------------------------------------------- */

static nsresult
GetDaemonPath(nsCString &dpath)
{
  nsCOMPtr<nsIFile> file;

  nsresult rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                       getter_AddRefs(file));
  if (NS_SUCCEEDED(rv))
  {
    rv = file->AppendNative(NS_LITERAL_CSTRING(IPC_DAEMON_APP_NAME));
    if (NS_SUCCEEDED(rv))
      rv = file->GetNativePath(dpath);
  }

  return rv;
}

/* ------------------------------------------------------------------------- */

static void
ProcessPendingQ(const nsID &aTarget)
{
  ipcMessageQ tempQ;

  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    nsAutoMonitor mon(td->monitor);

    // if the observer for this target has been temporarily disabled, then
    // we must not processing any pending messages at this time.

    if (!td->observerDisabled)
      td->pendingQ.MoveTo(tempQ);
  }

  // process pending queue outside monitor
  while (!tempQ.IsEmpty())
  {
    ipcMessage *msg = tempQ.First();

    // it is possible that messages for other targets are in the queue
    // (currently, this can be only a IPCM_MSG_PSH_CLIENT_STATE message
    // initially addressed to IPCM_TARGET, see IPC_OnMessageAvailable())
    // --ignore them.
    if (td->observer && msg->Target().Equals(aTarget))
      td->observer->OnMessageAvailable(msg->mMetaData,
                                       msg->Target(),
                                       (const PRUint8 *) msg->Data(),
                                       msg->DataLen());
    else
    {
      // the IPCM target does not have an observer, and therefore any IPCM
      // messages that make it here will simply be dropped.
      NS_ASSERTION(aTarget.Equals(IPCM_TARGET) || msg->Target().Equals(IPCM_TARGET),
                   "unexpected target");
      LOG(("dropping IPCM message: type=%x\n", IPCM_GetType(msg)));
    }
    tempQ.DeleteFirst();
  }
}

/* ------------------------------------------------------------------------- */

// WaitTarget enables support for multiple threads blocking on the same
// message target.  the selector is called while inside the target's monitor.

typedef nsresult (* ipcMessageSelector)(
  void *arg,
  ipcTargetData *td,
  const ipcMessage *msg
);

// selects any
static nsresult
DefaultSelector(void *arg, ipcTargetData *td, const ipcMessage *msg)
{
  return NS_OK;
}

static nsresult
WaitTarget(const nsID           &aTarget,
           PRIntervalTime        aTimeout,
           ipcMessage          **aMsg,
           ipcMessageSelector    aSelector = nsnull,
           void                 *aArg = nsnull)
{
  *aMsg = nsnull;

  if (!aSelector)
    aSelector = DefaultSelector;

  nsRefPtr<ipcTargetData> td;
  if (!GetTarget(aTarget, getter_AddRefs(td)))
    return NS_ERROR_INVALID_ARG; // bad aTarget

  PRBool isIPCMTarget = aTarget.Equals(IPCM_TARGET);

  PRIntervalTime timeStart = PR_IntervalNow();
  PRIntervalTime timeEnd;
  if (aTimeout == PR_INTERVAL_NO_TIMEOUT)
    timeEnd = aTimeout;
  else if (aTimeout == PR_INTERVAL_NO_WAIT)
    timeEnd = timeStart;
  else
  {
    timeEnd = timeStart + aTimeout;

    // if overflowed, then set to max value
    if (timeEnd < timeStart)
      timeEnd = PR_INTERVAL_NO_TIMEOUT;
  }

  ipcMessage *lastChecked = nsnull, *beforeLastChecked = nsnull;
  nsresult rv = NS_ERROR_ABORT;

  nsAutoMonitor mon(td->monitor);

  // only the ICPM target is allowed to wait for a message after shutdown
  // (but before disconnection).  this gives client observers called from
  // IPC_Shutdown a chance to use IPC_SendMessage to send necessary
  // "last minute" messages to other clients.

  while (gClientState->connected && (!gClientState->shutdown || isIPCMTarget))
  {
    NS_ASSERTION(!lastChecked, "oops");

    //
    // NOTE:
    //
    // we must start at the top of the pending queue, possibly revisiting
    // messages that our selector has already rejected.  this is necessary
    // because the queue may have been modified while we were waiting on
    // the monitor.  the impact of this on performance remains to be seen.
    //
    // one cheap solution is to keep a counter that is incremented each
    // time a message is removed from the pending queue.  that way we can
    // avoid revisiting all messages sometimes.
    //

    lastChecked = td->pendingQ.First();
    beforeLastChecked = nsnull;

    // loop over pending queue until we find a message that our selector likes.
    while (lastChecked)
    {
      //
      // it is possible that this call to WaitTarget() has been initiated by
      // some other selector, that might be currently processing the same
      // message (since the message remains in the queue until the selector
      // returns TRUE).  here we prevent this situation by using a special flag
      // to guarantee that every message is processed only once.
      //

      if (!lastChecked->TestFlag(IPC_MSG_FLAG_IN_PROCESS))
      {
        lastChecked->SetFlag(IPC_MSG_FLAG_IN_PROCESS);
        nsresult acceptedRV = (aSelector)(aArg, td, lastChecked);
        lastChecked->ClearFlag(IPC_MSG_FLAG_IN_PROCESS);

        if (acceptedRV != IPC_WAIT_NEXT_MESSAGE)
        {
          if (acceptedRV == NS_OK)
          {
            // remove from pending queue
            if (beforeLastChecked)
              td->pendingQ.RemoveAfter(beforeLastChecked);
            else
              td->pendingQ.RemoveFirst();

            lastChecked->mNext = nsnull;
            *aMsg = lastChecked;
            break;
          }
          else /* acceptedRV == IPC_DISCARD_MESSAGE */
          {
            ipcMessage *nextToCheck = lastChecked->mNext;

            // discard from pending queue
            if (beforeLastChecked)
              td->pendingQ.DeleteAfter(beforeLastChecked);
            else
              td->pendingQ.DeleteFirst();

            lastChecked = nextToCheck;

            continue;
          }
        }
      }

      beforeLastChecked = lastChecked;
      lastChecked = lastChecked->mNext;
    }

    if (*aMsg)
    {
      rv = NS_OK;
      break;
    }
#ifdef VBOX
    else
    {
      /* Special client liveness check if there is no message to process.
       * This is necessary as there might be several threads waiting for
       * a message from a single client, and only one gets the DOWN msg. */
      nsresult aliveRV = (aSelector)(aArg, td, NULL);
      if (aliveRV != IPC_WAIT_NEXT_MESSAGE)
      {
        *aMsg = NULL;
        break;
      }
    }
#endif /* VBOX */

    PRIntervalTime t = PR_IntervalNow();
    if (t > timeEnd) // check if timeout has expired
    {
      rv = IPC_ERROR_WOULD_BLOCK;
      break;
    }
    mon.Wait(timeEnd - t);

    LOG(("woke up from sleep [pendingQempty=%d connected=%d shutdown=%d isIPCMTarget=%d]\n",
          td->pendingQ.IsEmpty(), gClientState->connected,
          gClientState->shutdown, isIPCMTarget));
  }

  return rv;
}

/* ------------------------------------------------------------------------- */

static void
PostEvent(nsIEventTarget *eventTarget, PLEvent *ev)
{
  if (!ev)
    return;

  nsresult rv = eventTarget->PostEvent(ev);
  if (NS_FAILED(rv))
  {
    NS_WARNING("PostEvent failed");
    PL_DestroyEvent(ev);
  }
}

static void
PostEventToMainThread(PLEvent *ev)
{
  nsCOMPtr<nsIEventQueue> eventQ;
  NS_GetMainEventQ(getter_AddRefs(eventQ));
  if (!eventQ)
  {
    NS_WARNING("unable to get reference to main event queue");
    PL_DestroyEvent(ev);
    return;
  }
  PostEvent(eventQ, ev);
}

/* ------------------------------------------------------------------------- */

class ipcEvent_ClientState : public PLEvent
{
public:
  ipcEvent_ClientState(PRUint32 aClientID, PRUint32 aClientState)
    : mClientID(aClientID)
    , mClientState(aClientState)
  {
    PL_InitEvent(this, nsnull, HandleEvent, DestroyEvent);
  }

  PR_STATIC_CALLBACK(void *) HandleEvent(PLEvent *ev)
  {
    // maybe we've been shutdown!
    if (!gClientState)
      return nsnull;

    ipcEvent_ClientState *self = (ipcEvent_ClientState *) ev;

    for (PRInt32 i=0; i<gClientState->clientObservers.Count(); ++i)
      gClientState->clientObservers[i]->OnClientStateChange(self->mClientID,
                                                            self->mClientState);
    return nsnull;
  }

  PR_STATIC_CALLBACK(void) DestroyEvent(PLEvent *ev)
  {
    delete (ipcEvent_ClientState *) ev;
  }

private:
  PRUint32 mClientID;
  PRUint32 mClientState;
};

/* ------------------------------------------------------------------------- */

class ipcEvent_ProcessPendingQ : public PLEvent
{
public:
  ipcEvent_ProcessPendingQ(const nsID &aTarget)
    : mTarget(aTarget)
  {
    PL_InitEvent(this, nsnull, HandleEvent, DestroyEvent);
  }

  PR_STATIC_CALLBACK(void *) HandleEvent(PLEvent *ev)
  {
    ProcessPendingQ(((ipcEvent_ProcessPendingQ *) ev)->mTarget);
    return nsnull;
  }

  PR_STATIC_CALLBACK(void) DestroyEvent(PLEvent *ev)
  {
    delete (ipcEvent_ProcessPendingQ *) ev;
  }

private:
  const nsID mTarget;
};

static void
CallProcessPendingQ(const nsID &target, ipcTargetData *td)
{
  // we assume that we are inside td's monitor

  PLEvent *ev = new ipcEvent_ProcessPendingQ(target);
  if (!ev)
    return;

  nsresult rv;

  if (td->eventQ)
    rv = td->eventQ->PostEvent(ev);
  else
    rv = IPC_DoCallback((ipcCallbackFunc) PL_HandleEvent, ev);

  if (NS_FAILED(rv))
    PL_DestroyEvent(ev);
}

/* ------------------------------------------------------------------------- */

static void
DisableMessageObserver(const nsID &aTarget)
{
  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    nsAutoMonitor mon(td->monitor);
    ++td->observerDisabled;
  }
}

static void
EnableMessageObserver(const nsID &aTarget)
{
  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    nsAutoMonitor mon(td->monitor);
    if (td->observerDisabled > 0 && --td->observerDisabled == 0)
      if (!td->pendingQ.IsEmpty())
        CallProcessPendingQ(aTarget, td);
  }
}

/* ------------------------------------------------------------------------- */

// converts IPCM_ERROR_* status codes to NS_ERROR_* status codes
static nsresult nsresult_from_ipcm_result(PRInt32 status)
{
  nsresult rv = NS_ERROR_FAILURE;

  switch (status)
  {
    case IPCM_ERROR_GENERIC:        rv = NS_ERROR_FAILURE; break;
    case IPCM_ERROR_INVALID_ARG:    rv = NS_ERROR_INVALID_ARG; break;
    case IPCM_ERROR_NO_CLIENT:      rv = NS_ERROR_CALL_FAILED; break;
    // TODO: select better mapping for the below codes
    case IPCM_ERROR_NO_SUCH_DATA:
    case IPCM_ERROR_ALREADY_EXISTS: rv = NS_ERROR_FAILURE; break;
    default:                        NS_ASSERTION(PR_FALSE, "No conversion");
  }

  return rv;
}

/* ------------------------------------------------------------------------- */

// selects the next IPCM message with matching request index
static nsresult
WaitIPCMResponseSelector(void *arg, ipcTargetData *td, const ipcMessage *msg)
{
#ifdef VBOX
  if (!msg)
    return IPC_WAIT_NEXT_MESSAGE;
#endif /* VBOX */
  PRUint32 requestIndex = *(PRUint32 *) arg;
  return IPCM_GetRequestIndex(msg) == requestIndex ? NS_OK : IPC_WAIT_NEXT_MESSAGE;
}

// wait for an IPCM response message.  if responseMsg is null, then it is
// assumed that the caller does not care to get a reference to the
// response itself.  if the response is an IPCM_MSG_ACK_RESULT, then the
// status code is mapped to a nsresult and returned by this function.
static nsresult
WaitIPCMResponse(PRUint32 requestIndex, ipcMessage **responseMsg = nsnull)
{
  ipcMessage *msg;

  nsresult rv = WaitTarget(IPCM_TARGET, IPC_REQUEST_TIMEOUT, &msg,
                           WaitIPCMResponseSelector, &requestIndex);
  if (NS_FAILED(rv))
    return rv;

  if (IPCM_GetType(msg) == IPCM_MSG_ACK_RESULT)
  {
    ipcMessageCast<ipcmMessageResult> result(msg);
    if (result->Status() < 0)
      rv = nsresult_from_ipcm_result(result->Status());
    else
      rv = NS_OK;
  }

  if (responseMsg)
    *responseMsg = msg;
  else
    delete msg;

  return rv;
}

// make an IPCM request and wait for a response.
static nsresult
MakeIPCMRequest(ipcMessage *msg, ipcMessage **responseMsg = nsnull)
{
  if (!msg)
    return NS_ERROR_OUT_OF_MEMORY;

  PRUint32 requestIndex = IPCM_GetRequestIndex(msg);

  // suppress 'ProcessPendingQ' for IPCM messages until we receive the
  // response to this IPCM request.  if we did not do this then there
  // would be a race condition leading to the possible removal of our
  // response from the pendingQ between sending the request and waiting
  // for the response.
  DisableMessageObserver(IPCM_TARGET);

  nsresult rv = IPC_SendMsg(msg);
  if (NS_SUCCEEDED(rv))
    rv = WaitIPCMResponse(requestIndex, responseMsg);

  EnableMessageObserver(IPCM_TARGET);
  return rv;
}

/* ------------------------------------------------------------------------- */

static void
RemoveTarget(const nsID &aTarget, PRBool aNotifyDaemon)
{
  DelTarget(aTarget);

  if (aNotifyDaemon)
  {
    nsresult rv = MakeIPCMRequest(new ipcmMessageClientDelTarget(aTarget));
    if (NS_FAILED(rv))
      LOG(("failed to delete target: rv=%x\n", rv));
  }
}

static nsresult
DefineTarget(const nsID           &aTarget,
             ipcIMessageObserver  *aObserver,
             PRBool                aOnCurrentThread,
             PRBool                aNotifyDaemon,
             ipcTargetData       **aResult)
{
  nsresult rv;

  nsRefPtr<ipcTargetData> td( ipcTargetData::Create() );
  if (!td)
    return NS_ERROR_OUT_OF_MEMORY;
  td->SetObserver(aObserver, aOnCurrentThread);

  if (!PutTarget(aTarget, td))
    return NS_ERROR_OUT_OF_MEMORY;

  if (aNotifyDaemon)
  {
    rv = MakeIPCMRequest(new ipcmMessageClientAddTarget(aTarget));
    if (NS_FAILED(rv))
    {
      LOG(("failed to add target: rv=%x\n", rv));
      RemoveTarget(aTarget, PR_FALSE);
      return rv;
    }
  }

  if (aResult)
    NS_ADDREF(*aResult = td);
  return NS_OK;
}

/* ------------------------------------------------------------------------- */

static nsresult
TryConnect()
{
  nsCAutoString dpath;
  nsresult rv = GetDaemonPath(dpath);
  if (NS_FAILED(rv))
    return rv;

  rv = IPC_Connect(dpath.get());
  if (NS_FAILED(rv))
    return rv;

  gClientState->connected = PR_TRUE;

  rv = DefineTarget(IPCM_TARGET, nsnull, PR_FALSE, PR_FALSE, nsnull);
  if (NS_FAILED(rv))
    return rv;

  ipcMessage *msg = NULL;

  // send CLIENT_HELLO and wait for CLIENT_ID response...
  rv = MakeIPCMRequest(new ipcmMessageClientHello(), &msg);
  if (NS_FAILED(rv))
  {
#ifdef VBOX  /* MakeIPCMRequest may return a failure (e.g. NS_ERROR_CALL_FAILED) and a response msg. */
    if (msg)
      delete msg;
#endif
    return rv;
  }

  if (IPCM_GetType(msg) == IPCM_MSG_ACK_CLIENT_ID)
    gClientState->selfID = ipcMessageCast<ipcmMessageClientID>(msg)->ClientID();
  else
  {
    LOG(("unexpected response from CLIENT_HELLO message: type=%x!\n",
        IPCM_GetType(msg)));
    rv = NS_ERROR_UNEXPECTED;
  }

  delete msg;
  return rv;
}

nsresult
IPC_Init()
{
  NS_ENSURE_TRUE(!gClientState, NS_ERROR_ALREADY_INITIALIZED);

  IPC_InitLog(">>>");

  gClientState = ipcClientState::Create();
  if (!gClientState)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = TryConnect();
  if (NS_FAILED(rv))
    IPC_Shutdown();

  return rv;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumerateTargetMapAndNotify(const nsID    &aKey,
                            ipcTargetData *aData,
                            void          *aClosure);

nsresult
IPC_Shutdown()
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  LOG(("IPC_Shutdown: connected=%d\n",gClientState->connected));

  if (gClientState->connected)
  {
    {
      // first, set the shutdown flag and unblock any calls to WaitTarget.
      // all targets but IPCM will not be able to use WaitTarget any more.

#ifndef VBOX_WITH_IPCCLIENT_RW_CS
      nsAutoMonitor mon(gClientState->monitor);
#else
      RTCritSectRwEnterExcl(&gClientState->critSect);
#endif

      gClientState->shutdown = PR_TRUE;
      gClientState->targetMap.EnumerateRead(EnumerateTargetMapAndNotify, nsnull);

#ifdef VBOX_WITH_IPCCLIENT_RW_CS
      RTCritSectRwLeaveExcl(&gClientState->critSect);
#endif
    }

    // inform all client observers that we're being shutdown to let interested
    // parties gracefully uninitialize themselves.  the IPCM target is still
    // fully operational at this point, so they can use IPC_SendMessage
    // (this is essential for the DConnect extension, for example, to do the
    // proper uninitialization).

    ipcEvent_ClientState *ev = new ipcEvent_ClientState(IPC_SENDER_ANY,
                                                        IPCM_CLIENT_STATE_DOWN);
    ipcEvent_ClientState::HandleEvent (ev);
    ipcEvent_ClientState::DestroyEvent (ev);

    IPC_Disconnect();
  }

  //
  // make gClientState nsnull before deletion to cause all public IPC_*
  // calls (possibly made during ipcClientState destruction) to return
  // NS_ERROR_NOT_INITIALIZED.
  //
  // NOTE: isn't just checking for gClientState->connected in every appropriate
  // IPC_* method a better solution?
  //
  ipcClientState *aClientState = gClientState;
  gClientState = nsnull;
  delete aClientState;

  return NS_OK;
}

/* ------------------------------------------------------------------------- */

nsresult
IPC_DefineTarget(const nsID          &aTarget,
                 ipcIMessageObserver *aObserver,
                 PRBool               aOnCurrentThread)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  // do not permit the re-definition of the IPCM protocol's target.
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  nsresult rv;

  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    // clear out observer before removing target since we want to ensure that
    // the observer is released on the main thread.
    {
      nsAutoMonitor mon(td->monitor);
      td->SetObserver(aObserver, aOnCurrentThread);
    }

    // remove target outside of td's monitor to avoid holding the monitor
    // while entering the client state's monitor.
    if (!aObserver)
      RemoveTarget(aTarget, PR_TRUE);

    rv = NS_OK;
  }
  else
  {
    if (aObserver)
      rv = DefineTarget(aTarget, aObserver, aOnCurrentThread, PR_TRUE, nsnull);
    else
      rv = NS_ERROR_INVALID_ARG; // unknown target
  }

  return rv;
}

nsresult
IPC_DisableMessageObserver(const nsID &aTarget)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  // do not permit modifications to the IPCM protocol's target.
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  DisableMessageObserver(aTarget);
  return NS_OK;
}

nsresult
IPC_EnableMessageObserver(const nsID &aTarget)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  // do not permit modifications to the IPCM protocol's target.
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  EnableMessageObserver(aTarget);
  return NS_OK;
}

nsresult
IPC_SendMessage(PRUint32       aReceiverID,
                const nsID    &aTarget,
                const PRUint8 *aData,
                PRUint32       aDataLen)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  // do not permit sending IPCM messages
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  nsresult rv;
  if (aReceiverID == 0)
  {
    ipcMessage *msg = new ipcMessage(aTarget, (const char *) aData, aDataLen);
    if (!msg)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = IPC_SendMsg(msg);
  }
  else
    rv = MakeIPCMRequest(new ipcmMessageForward(IPCM_MSG_REQ_FORWARD,
                                                aReceiverID,
                                                aTarget,
                                                (const char *) aData,
                                                aDataLen));

  return rv;
}

struct WaitMessageSelectorData
{
  PRUint32             senderID;
  ipcIMessageObserver *observer;
  PRBool               senderDead;
};

static nsresult WaitMessageSelector(void *arg, ipcTargetData *td, const ipcMessage *msg)
{
  WaitMessageSelectorData *data = (WaitMessageSelectorData *) arg;
#ifdef VBOX
  if (!msg)
  {
    /* Special NULL message which asks to check whether the client is
     * still alive. Called when there is nothing suitable in the queue. */
    ipcIMessageObserver *obs = data->observer;
    if (!obs)
      obs = td->observer;
    NS_ASSERTION(obs, "must at least have a default observer");

    nsresult rv = obs->OnMessageAvailable(IPC_SENDER_ANY, nsID(), 0, 0);
    if (rv != IPC_WAIT_NEXT_MESSAGE)
      data->senderDead = PR_TRUE;

    return rv;
  }
#endif /* VBOX */

  // process the specially forwarded client state message to see if the
  // sender we're waiting a message from has died.

  if (msg->Target().Equals(IPCM_TARGET))
  {
    switch (IPCM_GetType(msg))
    {
      case IPCM_MSG_PSH_CLIENT_STATE:
      {
        ipcMessageCast<ipcmMessageClientState> status(msg);
        if ((data->senderID == IPC_SENDER_ANY ||
             status->ClientID() == data->senderID) &&
            status->ClientState() == IPCM_CLIENT_STATE_DOWN)
        {
          LOG(("sender (%d) we're waiting a message from (%d) has died\n",
               status->ClientID(), data->senderID));

          if (data->senderID != IPC_SENDER_ANY)
          {
            // we're waiting on a particular client, so IPC_WaitMessage must
            // definitely fail with the NS_ERROR_xxx result.

            data->senderDead = PR_TRUE;
            return IPC_DISCARD_MESSAGE; // consume the message
          }
          else
          {
            // otherwise inform the observer about the client death using a special
            // null message with an empty target id, and fail IPC_WaitMessage call
            // with NS_ERROR_xxx only if the observer accepts this message.

            ipcIMessageObserver *obs = data->observer;
            if (!obs)
              obs = td->observer;
            NS_ASSERTION(obs, "must at least have a default observer");

            nsresult rv = obs->OnMessageAvailable(status->ClientID(), nsID(), 0, 0);
            if (rv != IPC_WAIT_NEXT_MESSAGE)
              data->senderDead = PR_TRUE;

            return IPC_DISCARD_MESSAGE; // consume the message
          }
        }
#ifdef VBOX
        else if ((data->senderID == IPC_SENDER_ANY ||
                  status->ClientID() == data->senderID) &&
                 status->ClientState() == IPCM_CLIENT_STATE_UP)
        {
          LOG(("sender (%d) we're waiting a message from (%d) has come up\n",
               status->ClientID(), data->senderID));
          if (data->senderID == IPC_SENDER_ANY)
          {
            // inform the observer about the client appearance using a special
            // null message with an empty target id, but a length of 1.

            ipcIMessageObserver *obs = data->observer;
            if (!obs)
              obs = td->observer;
            NS_ASSERTION(obs, "must at least have a default observer");

            nsresult rv = obs->OnMessageAvailable(status->ClientID(), nsID(), 0, 1);
            /* VBoxSVC/VBoxXPCOMIPCD auto-start can cause that a client up
             * message arrives while we're already waiting for a response
             * from this client. Don't declare the connection as dead in
             * this case. A client ID wraparound can't falsely trigger
             * this, since the waiting thread would have hit the liveness
             * check in the mean time. We MUST consume the message, otherwise
             * IPCM messages pile up as long as there is a pending call, which
             * can lead to severe processing overhead. */
            return IPC_DISCARD_MESSAGE; // consume the message
          }
        }
#endif /* VBOX */
        break;
      }
      default:
        NS_NOTREACHED("unexpected message");
    }
    return IPC_WAIT_NEXT_MESSAGE; // continue iterating
  }

  nsresult rv = IPC_WAIT_NEXT_MESSAGE;

  if (data->senderID == IPC_SENDER_ANY ||
      msg->mMetaData == data->senderID)
  {
    ipcIMessageObserver *obs = data->observer;
    if (!obs)
      obs = td->observer;
    NS_ASSERTION(obs, "must at least have a default observer");

    rv = obs->OnMessageAvailable(msg->mMetaData,
                                 msg->Target(),
                                 (const PRUint8 *) msg->Data(),
                                 msg->DataLen());
  }

  // stop iterating if we got a match that the observer accepted.
  return rv != IPC_WAIT_NEXT_MESSAGE ? NS_OK : IPC_WAIT_NEXT_MESSAGE;
}

nsresult
IPC_WaitMessage(PRUint32             aSenderID,
                const nsID          &aTarget,
                ipcIMessageObserver *aObserver,
                ipcIMessageObserver *aConsumer,
                PRIntervalTime       aTimeout)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  // do not permit waiting for IPCM messages
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  // use aObserver as the message selector
  WaitMessageSelectorData data = { aSenderID, aObserver, PR_FALSE };

  ipcMessage *msg;
  nsresult rv = WaitTarget(aTarget, aTimeout, &msg, WaitMessageSelector, &data);
  if (NS_FAILED(rv))
    return rv;

  // if the selector has accepted some message, then we pass it to aConsumer
  // for safe processing.  The IPC susbsystem is quite stable here (i.e. we're
  // not inside any of the monitors, and the message has been already removed
  // from the pending queue).
  if (aObserver && aConsumer)
  {
    aConsumer->OnMessageAvailable(msg->mMetaData,
                                  msg->Target(),
                                  (const PRUint8 *) msg->Data(),
                                  msg->DataLen());
  }

  delete msg;

  // if the requested sender has died while waiting, return an error
  if (data.senderDead)
    return NS_ERROR_ABORT; // XXX better error code?

  return NS_OK;
}

/* ------------------------------------------------------------------------- */

nsresult
IPC_GetID(PRUint32 *aClientID)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  *aClientID = gClientState->selfID;
  return NS_OK;
}

nsresult
IPC_AddName(const char *aName)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  return MakeIPCMRequest(new ipcmMessageClientAddName(aName));
}

nsresult
IPC_RemoveName(const char *aName)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  return MakeIPCMRequest(new ipcmMessageClientDelName(aName));
}

/* ------------------------------------------------------------------------- */

nsresult
IPC_AddClientObserver(ipcIClientObserver *aObserver)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  return gClientState->clientObservers.AppendObject(aObserver)
      ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
IPC_RemoveClientObserver(ipcIClientObserver *aObserver)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  for (PRInt32 i = 0; i < gClientState->clientObservers.Count(); ++i)
  {
    if (gClientState->clientObservers[i] == aObserver)
      gClientState->clientObservers.RemoveObjectAt(i);
  }

  return NS_OK;
}

/* ------------------------------------------------------------------------- */

// this function could be called on any thread
nsresult
IPC_ResolveClientName(const char *aName, PRUint32 *aClientID)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  ipcMessage *msg = NULL;

  nsresult rv = MakeIPCMRequest(new ipcmMessageQueryClientByName(aName), &msg);
  if (NS_FAILED(rv))
  {
#ifdef VBOX  /* MakeIPCMRequest may return a failure (e.g. NS_ERROR_CALL_FAILED) and a response msg. */
    if (msg)
      delete msg;
#endif
    return rv;
  }

  if (IPCM_GetType(msg) == IPCM_MSG_ACK_CLIENT_ID)
    *aClientID = ipcMessageCast<ipcmMessageClientID>(msg)->ClientID();
  else
  {
    LOG(("unexpected IPCM response: type=%x\n", IPCM_GetType(msg)));
    rv = NS_ERROR_UNEXPECTED;
  }

  delete msg;
  return rv;
}

/* ------------------------------------------------------------------------- */

nsresult
IPC_ClientExists(PRUint32 aClientID, PRBool *aResult)
{
  // this is a bit of a hack.  we forward a PING to the specified client.
  // the assumption is that the forwarding will only succeed if the client
  // exists, so we wait for the RESULT message corresponding to the FORWARD
  // request.  if that gives a successful status, then we know that the
  // client exists.

  ipcmMessagePing ping;

  return MakeIPCMRequest(new ipcmMessageForward(IPCM_MSG_REQ_FORWARD,
                                                aClientID,
                                                IPCM_TARGET,
                                                ping.Data(),
                                                ping.DataLen()));
}

/* ------------------------------------------------------------------------- */

nsresult
IPC_SpawnDaemon(const char *path)
{
  PRFileDesc *readable = nsnull, *writable = nsnull;
  PRProcessAttr *attr = nsnull;
  nsresult rv = NS_ERROR_FAILURE;
  PRFileDesc *devNull;
  char *const argv[] = { (char *const) path, nsnull };
  char c;

  // setup an anonymous pipe that we can use to determine when the daemon
  // process has started up.  the daemon will write a char to the pipe, and
  // when we read it, we'll know to proceed with trying to connect to the
  // daemon.

  if (PR_CreatePipe(&readable, &writable) != PR_SUCCESS)
    goto end;
  PR_SetFDInheritable(writable, PR_TRUE);

  attr = PR_NewProcessAttr();
  if (!attr)
    goto end;

  if (PR_ProcessAttrSetInheritableFD(attr, writable, IPC_STARTUP_PIPE_NAME) != PR_SUCCESS)
  goto end;

  devNull = PR_Open("/dev/null", PR_RDWR, 0);
  if (!devNull)
    goto end;

  PR_ProcessAttrSetStdioRedirect(attr, PR_StandardInput, devNull);
  PR_ProcessAttrSetStdioRedirect(attr, PR_StandardOutput, devNull);
  PR_ProcessAttrSetStdioRedirect(attr, PR_StandardError, devNull);

  if (PR_CreateProcessDetached(path, argv, nsnull, attr) != PR_SUCCESS)
    goto end;

  // Close /dev/null
  PR_Close(devNull);
  // close the child end of the pipe in order to get notification on unexpected
  // child termination instead of being infinitely blocked in PR_Read().
  PR_Close(writable);
  writable = nsnull;

  if ((PR_Read(readable, &c, 1) != 1) || (c != IPC_STARTUP_PIPE_MAGIC))
    goto end;

  rv = NS_OK;
end:
  if (readable)
    PR_Close(readable);
  if (writable)
    PR_Close(writable);
  if (attr)
    PR_DestroyProcessAttr(attr);
  return rv;
}

/* ------------------------------------------------------------------------- */

PR_STATIC_CALLBACK(PLDHashOperator)
EnumerateTargetMapAndNotify(const nsID    &aKey,
                            ipcTargetData *aData,
                            void          *aClosure)
{
  nsAutoMonitor mon(aData->monitor);

  // wake up anyone waiting on this target.
  mon.NotifyAll();

  return PL_DHASH_NEXT;
}

// called on a background thread
void
IPC_OnConnectionEnd(nsresult error)
{
  // now, go through the target map, and tickle each monitor.  that should
  // unblock any calls to WaitTarget.

#ifndef VBOX_WITH_IPCCLIENT_RW_CS
  nsAutoMonitor mon(gClientState->monitor);
#else
  RTCritSectRwEnterExcl(&gClientState->critSect);
#endif

  gClientState->connected = PR_FALSE;
  gClientState->targetMap.EnumerateRead(EnumerateTargetMapAndNotify, nsnull);

#ifdef VBOX_WITH_IPCCLIENT_RW_CS
  RTCritSectRwLeaveExcl(&gClientState->critSect);
#endif
}

/* ------------------------------------------------------------------------- */

static void
PlaceOnPendingQ(const nsID &target, ipcTargetData *td, ipcMessage *msg)
{
  nsAutoMonitor mon(td->monitor);

  // we only want to dispatch a 'ProcessPendingQ' event if we have not
  // already done so.
  PRBool dispatchEvent = td->pendingQ.IsEmpty();

  // put this message on our pending queue
  td->pendingQ.Append(msg);

#ifdef IPC_LOGGING
  if (IPC_LOG_ENABLED())
  {
    char *targetStr = target.ToString();
    LOG(("placed message on pending queue for target %s and notifying all...\n", targetStr));
    nsMemory::Free(targetStr);
  }
#endif

  // wake up anyone waiting on this queue
  mon.NotifyAll();

  // proxy call to target's message procedure
  if (dispatchEvent)
    CallProcessPendingQ(target, td);
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumerateTargetMapAndPlaceMsg(const nsID    &aKey,
                              ipcTargetData *aData,
                              void          *userArg)
{
  if (!aKey.Equals(IPCM_TARGET))
  {
    // place a message clone to a target's event queue
    ipcMessage *msg = (ipcMessage *) userArg;
    PlaceOnPendingQ(aKey, aData, msg->Clone());
  }

  return PL_DHASH_NEXT;
}

/* ------------------------------------------------------------------------- */

#ifdef IPC_LOGGING
#include "prprf.h"
#include <ctype.h>
#endif

// called on a background thread
void
IPC_OnMessageAvailable(ipcMessage *msg)
{
#ifdef IPC_LOGGING
  if (IPC_LOG_ENABLED())
  {
    char *targetStr = msg->Target().ToString();
    LOG(("got message for target: %s\n", targetStr));
    nsMemory::Free(targetStr);

//     IPC_LogBinary((const PRUint8 *) msg->Data(), msg->DataLen());
  }
#endif

  if (msg->Target().Equals(IPCM_TARGET))
  {
    switch (IPCM_GetType(msg))
    {
      // if this is a forwarded message, then post the inner message instead.
      case IPCM_MSG_PSH_FORWARD:
      {
        ipcMessageCast<ipcmMessageForward> fwd(msg);
        ipcMessage *innerMsg = new ipcMessage(fwd->InnerTarget(),
                                              fwd->InnerData(),
                                              fwd->InnerDataLen());
        // store the sender's client id in the meta-data field of the message.
        innerMsg->mMetaData = fwd->ClientID();

        delete msg;

        // recurse so we can handle forwarded IPCM messages
        IPC_OnMessageAvailable(innerMsg);
        return;
      }
      case IPCM_MSG_PSH_CLIENT_STATE:
      {
        ipcMessageCast<ipcmMessageClientState> status(msg);
        PostEventToMainThread(new ipcEvent_ClientState(status->ClientID(),
                                                       status->ClientState()));

        // go through the target map, and place this message to every target's
        // pending event queue.  that unblocks all WaitTarget calls (on all
        // targets) giving them an opportuninty to finish wait cycle because of
        // the peer client death, when appropriate.
#ifndef VBOX_WITH_IPCCLIENT_RW_CS
        nsAutoMonitor mon(gClientState->monitor);
#else
        RTCritSectRwEnterShared(&gClientState->critSect);
#endif

        gClientState->targetMap.EnumerateRead(EnumerateTargetMapAndPlaceMsg, msg);

#ifdef VBOX_WITH_IPCCLIENT_RW_CS
        RTCritSectRwLeaveShared(&gClientState->critSect);
#endif
        delete msg;

        return;
      }
    }
  }

  nsRefPtr<ipcTargetData> td;
  if (GetTarget(msg->Target(), getter_AddRefs(td)))
  {
    // make copy of target since |msg| may end up pointing to free'd memory
    // once we notify the monitor inside PlaceOnPendingQ().
    const nsID target = msg->Target();

    PlaceOnPendingQ(target, td, msg);
  }
  else
  {
    NS_WARNING("message target is undefined");
#ifdef VBOX
    delete msg;
#endif
  }
}
