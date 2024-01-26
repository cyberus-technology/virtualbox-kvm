/*
 * Minimal implementation of PanoramiX/Xinerama
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/pseudoramiX.h,v 1.3 2004/07/02 01:30:33 torrey Exp $ */

extern int noPseudoramiXExtension;

void PseudoramiXAddScreen(int x, int y, int w, int h);
void PseudoramiXExtensionInit(int argc, char *argv[]);
void PseudoramiXResetScreens(void);
