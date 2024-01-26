/* $XFree86$ */
#ifndef SCO_KBD_HDR
#define SCO_KBD_HDR

typedef struct {
  int use_tcs;
  int use_kd;
  int no_nmap;
  int no_emap;
  int orig_getsc;
  int orig_kbm;
  struct termios kbdtty;
  keymap_t keymap, noledmap;
  uchar_t *sc_mapbuf;
  uchar_t *sc_mapbuf2;
} ScoKbdPrivRec, *ScoKbdPrivPtr;

extern void KbdGetMapping(InputInfoPtr pInfo, KeySymsPtr pKeySyms,
  CARD8 *pModMap);
#endif /* SCO_KBD_HDR */
