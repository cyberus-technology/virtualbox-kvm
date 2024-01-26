
#ifndef _XAA_WRAPPER_H
# define _XAA_WRAPPER_H

typedef void (*SyncFunc)(ScreenPtr);

extern _X_EXPORT Bool xaaSetupWrapper(ScreenPtr pScreen,
		     XAAInfoRecPtr infoPtr, int depth, SyncFunc *func);

#endif
