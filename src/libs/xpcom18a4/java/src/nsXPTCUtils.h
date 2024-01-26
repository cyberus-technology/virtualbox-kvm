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
 * The Original Code is Mozilla XPCOM.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation <http://www.mozilla.org/>.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Benjamin Smedberg <benjamin@smedbergs.us> - New code
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

#ifndef nsXPTCUtils_h__
#define nsXPTCUtils_h__

#include "xptcall.h"

#ifdef VBOX

#define NS_DECLARE_STATIC_IID_ACCESSOR(the_iid)                         \
  template <class Dummy>                                                \
  struct COMTypeInfo                                                    \
  {                                                                     \
    static const nsIID kIID NS_HIDDEN;                                  \
  };                                                                    \
  static const nsIID& GetIID() {return COMTypeInfo<int>::kIID;}


#define NS_DEFINE_STATIC_IID_ACCESSOR2(the_interface, the_iid)           \
  template <class Dummy>                                                \
  const nsIID the_interface::COMTypeInfo<Dummy>::kIID NS_HIDDEN = the_iid;


class nsIXPTCProxy : public nsISupports
{
public:
    NS_IMETHOD CallMethod(PRUint16 aMethodIndex,
                          const XPTMethodDescriptor *aInfo,
                          nsXPTCMiniVariant *aParams) = 0;
};

/**
 * This is a typedef to avoid confusion between the canonical
 * nsISupports* that provides object identity and an interface pointer
 * for inheriting interfaces that aren't known at compile-time.
 */
typedef nsISupports nsISomeInterface;

/**
 * Get a proxy object to implement the specified interface.
 *
 * @param aIID    The IID of the interface to implement.
 * @param aOuter  An object to receive method calls from the proxy object.
 *                The stub forwards QueryInterface/AddRef/Release to the
 *                outer object. The proxy object does not hold a reference to
 *                the outer object; it is the caller's responsibility to
 *                ensure that this pointer remains valid until the stub has
 *                been destroyed.
 * @param aStub   Out parameter for the new proxy object. The object is
 *                not addrefed. The object never destroys itself. It must be
 *                explicitly destroyed by calling
 *                NS_DestroyXPTCallStub when it is no longer needed.
 */
XPTC_PUBLIC_API(nsresult)
NS_GetXPTCallStub(REFNSIID aIID, nsIXPTCProxy* aOuter,
                  nsISomeInterface* *aStub);

/**
 * Destroys an XPTCall stub previously created with NS_GetXPTCallStub.
 */
XPTC_PUBLIC_API(void)
NS_DestroyXPTCallStub(nsISomeInterface* aStub);


#endif


/**
 * A helper class that initializes an xptcall helper at construction
 * and releases it at destruction.
 */
class nsAutoXPTCStub : protected nsIXPTCProxy
{
public:
  nsISomeInterface* mXPTCStub;

protected:
  nsAutoXPTCStub() : mXPTCStub(nsnull) { }

  nsresult
  InitStub(const nsIID& aIID)
  {
    return NS_GetXPTCallStub(aIID, this, &mXPTCStub);
  }

  ~nsAutoXPTCStub()
  {
    if (mXPTCStub)
      NS_DestroyXPTCallStub(mXPTCStub);
  }
};

#endif // nsXPTCUtils_h__
