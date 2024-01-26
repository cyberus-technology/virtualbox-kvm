/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsIEnumerator.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "prprf.h"
#include "nsWeakReference.h"

static nsIObserverService *anObserverService = NULL;

#ifdef VBOX
static bool testResult( nsresult rv ) {
    if ( NS_SUCCEEDED( rv ) ) {
        printf("...ok\n");
        return true;
    }
    printf("...failed, rv=0x%x\n", (int)rv);
    return false;
}
#else
static void testResult( nsresult rv ) {
    if ( NS_SUCCEEDED( rv ) ) {
        printf("...ok\n");
    } else {
        printf("...failed, rv=0x%x\n", (int)rv);
    }
    return;
}
#endif

void printString(nsString &str) {
#ifdef VBOX  /* asan complains about mixing different allocators */
    char *cstr = ToNewCString(str);
    printf("%s", cstr);
    nsMemory::Free(cstr);
#else
    const char *cstr = ToNewCString(str);
    printf("%s", cstr);
    delete [] (char*)cstr;
#endif
}

class TestObserver : public nsIObserver, public nsSupportsWeakReference {
public:
    TestObserver( const nsAString &name )
        : mName( name ) {
    }
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsString mName;

private:
    ~TestObserver() {}
};

NS_IMPL_ISUPPORTS2( TestObserver, nsIObserver, nsISupportsWeakReference )

NS_IMETHODIMP
TestObserver::Observe( nsISupports     *aSubject,
                       const char *aTopic,
                       const PRUnichar *someData ) {
    nsCString topic( aTopic );
    nsString data( someData );
    	/*
    		The annoying double-cast below is to work around an annoying bug in
    		the compiler currently used on wensleydale.  This is a test.
    	*/
    printString(mName);
    printf(" has observed something: subject@%p", (void*)aSubject);
    printf(" name=");
    printString(NS_REINTERPRET_CAST(TestObserver*, NS_REINTERPRET_CAST(void*, aSubject))->mName);
    printf(" aTopic=%s", topic.get());
    printf(" someData=");
    printString(data);
    printf("\n");
    return NS_OK;
}

int main(int argc, char *argv[])
{
    nsCString topicA; topicA.Assign( "topic-A" );
    nsCString topicB; topicB.Assign( "topic-B" );
    nsresult rv;

    nsresult res = nsComponentManager::CreateInstance("@mozilla.org/observer-service;1",
                                                NULL,
                                                 NS_GET_IID(nsIObserverService),
                                                (void **) &anObserverService);
#ifdef VBOX
    bool fSuccess = res == NS_OK;
#endif
	
    if (res == NS_OK) {

        nsIObserver *aObserver = new TestObserver(NS_LITERAL_STRING("Observer-A"));
        aObserver->AddRef();
        nsIObserver *bObserver = new TestObserver(NS_LITERAL_STRING("Observer-B"));
        bObserver->AddRef();
            
        printf("Adding Observer-A as observer of topic-A...\n");
        rv = anObserverService->AddObserver(aObserver, topicA.get(), PR_FALSE);
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);
 
        printf("Adding Observer-B as observer of topic-A...\n");
        rv = anObserverService->AddObserver(bObserver, topicA.get(), PR_FALSE);
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);
 
        printf("Adding Observer-B as observer of topic-B...\n");
        rv = anObserverService->AddObserver(bObserver, topicB.get(), PR_FALSE);
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);

        printf("Testing Notify(observer-A, topic-A)...\n");
        rv = anObserverService->NotifyObservers( aObserver,
                                   topicA.get(),
                                   NS_LITERAL_STRING("Testing Notify(observer-A, topic-A)").get() );
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);

        printf("Testing Notify(observer-B, topic-B)...\n");
        rv = anObserverService->NotifyObservers( bObserver,
                                   topicB.get(),
                                   NS_LITERAL_STRING("Testing Notify(observer-B, topic-B)").get() );
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);
 
        printf("Testing EnumerateObserverList (for topic-A)...\n");
        nsCOMPtr<nsISimpleEnumerator> e;
        rv = anObserverService->EnumerateObservers(topicA.get(), getter_AddRefs(e));

#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);

        printf("Enumerating observers of topic-A...\n");
        if ( e ) {
          nsCOMPtr<nsIObserver> observer;
          PRBool loop = PR_TRUE;
          while( NS_SUCCEEDED(e->HasMoreElements(&loop)) && loop) 
          {
              e->GetNext(getter_AddRefs(observer));
              printf("Calling observe on enumerated observer ");
              printString(NS_REINTERPRET_CAST(TestObserver*,
                                              NS_REINTERPRET_CAST(void*, observer.get()))->mName);
              printf("...\n");
              rv = observer->Observe( observer, 
                                      topicA.get(), 
                                      NS_LITERAL_STRING("during enumeration").get() );
#ifdef VBOX
              fSuccess = fSuccess &&
#endif
              testResult(rv);
          }
        }
        printf("...done enumerating observers of topic-A\n");

        printf("Removing Observer-A...\n");
        rv = anObserverService->RemoveObserver(aObserver, topicA.get());
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);


        printf("Removing Observer-B (topic-A)...\n");
        rv = anObserverService->RemoveObserver(bObserver, topicB.get());
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);
        printf("Removing Observer-B (topic-B)...\n");
        rv = anObserverService->RemoveObserver(bObserver, topicA.get());
#ifdef VBOX
        fSuccess = fSuccess &&
#endif
        testResult(rv);

#ifdef VBOX
        /* Cleanup: */
        nsrefcnt refs = bObserver->Release();
        fSuccess = fSuccess && refs == 0;
        if (refs != 0)
            printf("bObserver->Release() -> %d, expected 0\n", (int)refs);

        refs = aObserver->Release();
        fSuccess = fSuccess && refs == 0;
        if (refs != 0)
            printf("aObserver->Release() -> %d, expected 0\n", (int)refs);
#endif
    }
#ifdef VBOX
    return fSuccess ? 0 : 1;
#else
    return NS_OK;
#endif
}
