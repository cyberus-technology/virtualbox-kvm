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
 *   Dmitry A. Kuminov <dmik@innotek.de>
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

// DConnect service is multithreaded by default...
#if !defined(DCONNECT_SINGLETHREADED) && !defined(DCONNECT_MULTITHREADED)
#define DCONNECT_MULTITHREADED
# ifdef VBOX
//#  define DCONNECT_WITH_IPRT_REQ_POOL
# endif
#endif

#include "ipcIDConnectService.h"
#include "ipcdclient.h"

#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsHashSets.h"
#include "nsVoidArray.h"
#include "nsAutoLock.h"
#include "xptcall.h"
#include "xptinfo.h"

#if defined(DCONNECT_MULTITHREADED)
# if defined(DCONNECT_WITH_IPRT_REQ_POOL)

#  include <iprt/req.h>

# else /* !DCONNECT_WITH_IPRT_REQ_POOL*/

#include "ipcList.h"

struct DConnectOp;

struct DConnectRequest : public ipcListNode<DConnectRequest>
{
  DConnectRequest (PRUint32 aPeer, const DConnectOp *aOp, PRUint32 aOpLen)
    : peer(aPeer)
    , opLen(aOpLen)
  {
    op = (const DConnectOp *) malloc(aOpLen);
    memcpy ((void *) op, aOp, aOpLen);
  }
  ~DConnectRequest() { free((void *) op); }

  const PRUint32 peer;
  const DConnectOp *op;
  const PRUint32 opLen;
};

# endif // !DCONNECT_WITH_IPRT_REQ_POOL
#endif // DCONNECT_MULTITHREADED

class nsIException;
class ipcMessageReader;
class ipcMessageWriter;

// a key class used to identify DConnectInstance objects stored in a hash table
// by a composite of peer ID, XPCOM object pointer and IID this pointer represents

class DConnectInstanceKey : public PLDHashEntryHdr
{
public:
  struct Key
  {
    Key(PRUint32 aPeer, const nsISupports *aObj, const nsID *aIID)
      : mPeer (aPeer), mObj (aObj), mIID (aIID) {}
    const PRUint32 mPeer;
    const nsISupports *mObj;
    const nsIID *mIID;
  };

  typedef const Key &KeyType;
  typedef const Key *KeyTypePointer;

  DConnectInstanceKey(const Key *aKey) : mKey (*aKey) {}
  DConnectInstanceKey(const DConnectInstanceKey &toCopy) : mKey (toCopy.mKey) {}
  ~DConnectInstanceKey() {}

  KeyType GetKey() const { return mKey; }
  KeyTypePointer GetKeyPointer() const { return &mKey; }

  PRBool KeyEquals(KeyTypePointer aKey) const {
    return mKey.mPeer == aKey->mPeer &&
           mKey.mObj == aKey->mObj &&
           mKey.mIID->Equals(*aKey->mIID);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) {
    return aKey->mPeer ^
           (NS_PTR_TO_INT32(aKey->mObj) >> 2) ^
           nsIDHashKey::HashKey(aKey->mIID);
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const Key mKey;
};

class DConnectInstance;
typedef nsDataHashtable<DConnectInstanceKey, DConnectInstance *> DConnectInstanceMap;

// extend nsVoidHashSet for compatibility with some nsTHashtable methods
class DConnectInstanceSet : public nsVoidHashSet
{
public:
  PRBool Init(PRUint32 initSize = PL_DHASH_MIN_SIZE) {
    return nsVoidHashSet::Init(initSize) == NS_OK;
  }
  void Clear() {
    PL_DHashTableEnumerate(&mHashTable, PL_DHashStubEnumRemove, nsnull);
  }
};

typedef PRUint64 DConAddr;

// a key class used to identify DConnectStub objects stored in a hash table
// by a composite of peer ID and DConAddr

class DConnectStubKey : public PLDHashEntryHdr
{
public:
  struct Key
  {
    Key(PRUint32 aPeer, const DConAddr aInstance)
      : mPeer (aPeer), mInstance (aInstance) {}
    const PRUint32 mPeer;
    const DConAddr mInstance;
  };

  typedef const Key &KeyType;
  typedef const Key *KeyTypePointer;

  DConnectStubKey(const Key *aKey) : mKey (*aKey) {}
  DConnectStubKey(const DConnectStubKey &toCopy) : mKey (toCopy.mKey) {}
  ~DConnectStubKey() {}

  KeyType GetKey() const { return mKey; }
  KeyTypePointer GetKeyPointer() const { return &mKey; }

  PRBool KeyEquals(KeyTypePointer aKey) const {
    return mKey.mPeer == aKey->mPeer &&
           mKey.mInstance == aKey->mInstance;
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) {
    return aKey->mPeer ^
           (NS_PTR_TO_INT32(aKey->mInstance) >> 2);
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  const Key mKey;
};

// used elsewhere like nsAtomTable to safely represent the integral value
// of an address.
typedef PRUint64 PtrBits;

// bit flag that defines if a PtrBits value represents a remote object
#define PTRBITS_REMOTE_BIT 0x1

class DConnectStub;
typedef nsDataHashtable<DConnectStubKey, DConnectStub *> DConnectStubMap;

class ipcDConnectService : public ipcIDConnectService
                         , public ipcIMessageObserver
                         , public ipcIClientObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IPCIDCONNECTSERVICE
  NS_DECL_IPCIMESSAGEOBSERVER
  NS_DECL_IPCICLIENTOBSERVER

  ipcDConnectService();

  NS_HIDDEN_(nsresult) Init();
  NS_HIDDEN_(void) Shutdown();

  NS_HIDDEN_(nsresult) GetInterfaceInfo(const nsID &iid, nsIInterfaceInfo **);
  NS_HIDDEN_(nsresult) GetIIDForMethodParam(nsIInterfaceInfo *iinfo,
                                            const nsXPTMethodInfo *methodInfo,
                                            const nsXPTParamInfo &paramInfo,
                                            const nsXPTType &type,
                                            PRUint16 methodIndex,
                                            nsXPTCMiniVariant *dispatchParams,
                                            PRBool isXPTCVariantArray,
                                            nsID &result);

  NS_HIDDEN_(nsresult) SerializeInterfaceParam(ipcMessageWriter &writer,
                                               PRUint32 peer, const nsID &iid,
                                               nsISupports *obj,
                                               nsVoidArray &wrappers);
  NS_HIDDEN_(nsresult) DeserializeInterfaceParamBits(PtrBits bits, PRUint32 peer,
                                                     const nsID &iid,
                                                     nsISupports *&obj);

  NS_HIDDEN_(nsresult) SerializeException(ipcMessageWriter &writer,
                                          PRUint32 peer, nsIException *xcpt,
                                          nsVoidArray &wrappers);
  NS_HIDDEN_(nsresult) DeserializeException(ipcMessageReader &reader,
                                            PRUint32 peer, nsIException **xcpt);

  NS_HIDDEN_(void)     ReleaseWrappers(nsVoidArray &wrappers,  PRUint32 peer);

  NS_HIDDEN_(nsresult) CreateStub(const nsID &, PRUint32, DConAddr, DConnectStub **);
#if 0
  NS_HIDDEN_(PRBool)   FindStubAndAddRef(PRUint32, const DConAddr, DConnectStub **);
#endif
  // public only for DConnectStub::~DConnectStub()
  NS_HIDDEN_(void)     DeleteStub(DConnectStub *);

  // public only for DConnectInstance::Release()
  NS_HIDDEN_(void)     DeleteInstance(DConnectInstance *, PRBool locked = PR_FALSE);
  // public only for DConnectStub::CallMethod()
  NS_HIDDEN_(PRBool)   CheckInstanceAndAddRef(DConnectInstance *, PRUint32);

  PRLock *StubLock() { return mStubLock; }
  PRLock *StubQILock() { return mStubQILock; }

  static nsRefPtr <ipcDConnectService> GetInstance() {
    return nsRefPtr <ipcDConnectService> (mInstance);
  }

private:

  NS_HIDDEN ~ipcDConnectService();

  NS_HIDDEN_(nsresult) StoreInstance(DConnectInstance *);
  NS_HIDDEN_(PRBool)   FindInstanceAndAddRef(PRUint32,
                                             const nsISupports *,
                                             const nsIID *,
                                             DConnectInstance **);

  NS_HIDDEN_(nsresult) StoreStub(DConnectStub *);

  NS_HIDDEN_(void) OnIncomingRequest(PRUint32 peer, const struct DConnectOp *op, PRUint32 opLen);

  NS_HIDDEN_(void) OnSetup(PRUint32 peer, const struct DConnectSetup *, PRUint32 opLen);
  NS_HIDDEN_(void) OnRelease(PRUint32 peer, const struct DConnectRelease *);
  NS_HIDDEN_(void) OnInvoke(PRUint32 peer, const struct DConnectInvoke *, PRUint32 opLen);

#if defined(DCONNECT_MULTITHREADED)
# if defined(DCONNECT_WITH_IPRT_REQ_POOL)
  static DECLCALLBACK(void) ProcessMessageOnWorkerThread(ipcDConnectService *aThis, PRUint32 aSenderID, void *aData, PRUint32 aDataLen);
# else
  NS_HIDDEN_(nsresult) CreateWorker();
# endif
#endif

private:
  nsCOMPtr<nsIInterfaceInfoManager> mIIM;

  // lock to protect access to instance sets and the disconnected flag
  PRLock *mLock;

  // table of local object instances allocated on behalf of a peer
  // (keys are interface pointers of real objects these instances represent)
  DConnectInstanceMap mInstances;
  // hashset containing the same instances as above
  // (used for quick parameter validity checks)
  DConnectInstanceSet mInstanceSet;

  // lock to protect access to mStubs and DConnectStub::mRefCntLevels
  // (also guards every DConnectStub::Release call to provide atomicity)
  PRLock *mStubLock;

  // table of remote object stubs allocated to communicate with peer's instances
  DConnectStubMap mStubs;

  // this is true after IPC_Shutdown() has been called
  PRBool mDisconnected;

// member is never initialized or used, no point in wasting memory or making
// someone believe it contains anything relevant
#ifndef VBOX
  // our IPC client ID
  PRUint32 mSelfID;
#endif

  // global lock to protect access to protect DConnectStub::QueryInterface()
  // (we cannot use mStubLock because it isn't supposed to be held long,
  // like in case of an IPC call and such)
  PRLock *mStubQILock;

#if defined(DCONNECT_MULTITHREADED)
# if defined(DCONNECT_WITH_IPRT_REQ_POOL)

  /** Request pool. */
  RTREQPOOL mhReqPool;

# else

  friend class DConnectWorker;

  // pool of worker threads to serve incoming requests
  nsVoidArray mWorkers;
  // queue of pending requests
  ipcList<DConnectRequest> mPendingQ;
  // monitor to protect mPendingQ
  PRMonitor *mPendingMon;
  // number of waiting workers
  PRUint32 mWaitingWorkers;
  // monitor used to wait on changes in mWaitingWorkers.
  PRMonitor *mWaitingWorkersMon;
# endif
#endif

  // global ipcDConnectService instance for internal usage
  static ipcDConnectService *mInstance;
};

#define IPC_DCONNECTSERVICE_CLASSNAME \
  "ipcDConnectService"
#define IPC_DCONNECTSERVICE_CONTRACTID \
  "@mozilla.org/ipc/dconnect-service;1"
#define IPC_DCONNECTSERVICE_CID                    \
{ /* 63a5d9dc-4828-425a-bd50-bd10a4b26f2c */       \
  0x63a5d9dc,                                      \
  0x4828,                                          \
  0x425a,                                          \
  {0xbd, 0x50, 0xbd, 0x10, 0xa4, 0xb2, 0x6f, 0x2c} \
}
