/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86OSpriv.h,v 1.5 2000/10/28 01:42:24 mvojkovi Exp $ */

#ifndef _XF86OSPRIV_H
#define _XF86OSPRIV_H

typedef pointer (*MapMemProcPtr)(int, unsigned long, unsigned long, int);
typedef void (*UnmapMemProcPtr)(int, pointer, unsigned long);
typedef pointer (*SetWCProcPtr)(int, unsigned long, unsigned long, Bool,
				MessageType);
typedef void (*ProtectMemProcPtr)(int, pointer, unsigned long, Bool); 
typedef void (*UndoWCProcPtr)(int, pointer);
typedef void (*ReadSideEffectsProcPtr)(int, pointer, unsigned long);

typedef struct {
	Bool			initialised;
	MapMemProcPtr		mapMem;
	UnmapMemProcPtr		unmapMem;
	ProtectMemProcPtr	protectMem;
	SetWCProcPtr		setWC;
	UndoWCProcPtr		undoWC;
	ReadSideEffectsProcPtr	readSideEffects;
	Bool			linearSupported;
} VidMemInfo, *VidMemInfoPtr;

void xf86OSInitVidMem(VidMemInfoPtr);

#endif /* _XF86OSPRIV_H */
