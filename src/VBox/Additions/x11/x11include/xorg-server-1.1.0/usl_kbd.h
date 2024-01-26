/* $XdotOrg: xserver/xorg/hw/xfree86/os-support/usl/usl_kbd.h,v 1.2 2005/11/08 06:33:30 jkj Exp $ */
#ifndef SCO_KBD_HDR
#define SCO_KBD_HDR

typedef struct {
  int orig_kbm;
  struct termio kbdtty;
  keymap_t keymap, noledmap;
  int xq;
} USLKbdPrivRec, *USLKbdPrivPtr;

extern void KbdGetMapping(InputInfoPtr pInfo, KeySymsPtr pKeySyms,
  CARD8 *pModMap);
#endif /* SCO_KBD_HDR */
