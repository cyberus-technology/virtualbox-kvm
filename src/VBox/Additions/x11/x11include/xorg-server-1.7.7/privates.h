/***********************************************************

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef PRIVATES_H
#define PRIVATES_H 1

#include "dix.h"
#include "resource.h"

/*****************************************************************
 * STUFF FOR PRIVATES
 *****************************************************************/

typedef int *DevPrivateKey;
struct _Private;
typedef struct _Private PrivateRec;

/*
 * Request pre-allocated private space for your driver/module.
 * Calling this is not necessary if only a pointer by itself is needed.
 */
extern _X_EXPORT int
dixRequestPrivate(const DevPrivateKey key, unsigned size);

/*
 * Allocates a new private and attaches it to an existing object.
 */
extern _X_EXPORT pointer *
dixAllocatePrivate(PrivateRec **privates, const DevPrivateKey key);

/*
 * Look up a private pointer.
 */
extern _X_EXPORT pointer
dixLookupPrivate(PrivateRec **privates, const DevPrivateKey key);

/*
 * Look up the address of a private pointer.
 */
extern _X_EXPORT pointer *
dixLookupPrivateAddr(PrivateRec **privates, const DevPrivateKey key);

/*
 * Set a private pointer.
 */
extern _X_EXPORT int
dixSetPrivate(PrivateRec **privates, const DevPrivateKey key, pointer val);

/*
 * Register callbacks to be called on private allocation/freeing.
 * The calldata argument to the callbacks is a PrivateCallbackPtr.
 */
typedef struct _PrivateCallback {
    DevPrivateKey key;	/* private registration key */
    pointer *value;	/* address of private pointer */
} PrivateCallbackRec;

extern _X_EXPORT int
dixRegisterPrivateInitFunc(const DevPrivateKey key, 
			   CallbackProcPtr callback, pointer userdata);

extern _X_EXPORT int
dixRegisterPrivateDeleteFunc(const DevPrivateKey key,
			     CallbackProcPtr callback, pointer userdata);

/*
 * Frees private data.
 */
extern _X_EXPORT void
dixFreePrivates(PrivateRec *privates);

/*
 * Resets the subsystem, called from the main loop.
 */
extern _X_EXPORT int
dixResetPrivates(void);

/*
 * These next two functions are necessary because the position of
 * the devPrivates field varies by structure and calling code might
 * only know the resource type, not the structure definition.
 */

/*
 * Looks up the offset where the devPrivates field is located.
 * Returns -1 if no offset has been registered for the resource type.
 */
extern _X_EXPORT int
dixLookupPrivateOffset(RESTYPE type);

/*
 * Specifies the offset where the devPrivates field is located.
 * A negative value indicates no devPrivates field is available.
 */
extern _X_EXPORT int
dixRegisterPrivateOffset(RESTYPE type, int offset);

/*
 * Convenience macro for adding an offset to an object pointer
 * when making a call to one of the devPrivates functions
 */
#define DEVPRIV_AT(ptr, offset) ((PrivateRec **)((char *)ptr + offset))

#endif /* PRIVATES_H */
