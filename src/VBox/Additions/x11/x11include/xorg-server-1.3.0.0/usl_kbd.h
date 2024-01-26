/* $XdotOrg$ */
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
