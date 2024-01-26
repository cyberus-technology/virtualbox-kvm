#ifndef _FB_XX_H_
# define  _FB_XX_H_

typedef void (*xxSyncFunc)(ScreenPtr);
extern Bool xxSetup(ScreenPtr pScreen, int myDepth,
		    int baseDepth, char *addr, xxSyncFunc sync);
extern void xxPrintVisuals(void);


#endif /* _FB_XX_H_ */










