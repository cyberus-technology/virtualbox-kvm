/* $XFree86$ */

/* #include "XKBfile.h" */

extern int ProcXkbUseExtension(ClientPtr client);
extern int ProcXkbSelectEvents(ClientPtr client);
extern int ProcXkbBell(ClientPtr client);
extern int ProcXkbGetState(ClientPtr client);
extern int ProcXkbLatchLockState(ClientPtr client);
extern int ProcXkbGetControls(ClientPtr client);
extern int ProcXkbSetControls(ClientPtr client);
extern int ProcXkbGetMap(ClientPtr client);
extern int ProcXkbSetMap(ClientPtr client);
extern int ProcXkbGetCompatMap(ClientPtr client);
extern int ProcXkbSetCompatMap(ClientPtr client);
extern int ProcXkbGetIndicatorState(ClientPtr client);
extern int ProcXkbGetIndicatorMap(ClientPtr client);
extern int ProcXkbSetIndicatorMap(ClientPtr client);
extern int ProcXkbGetNamedIndicator(ClientPtr client);
extern int ProcXkbSetNamedIndicator(ClientPtr client);
extern int ProcXkbGetNames(ClientPtr client);
extern int ProcXkbSetNames(ClientPtr client);
extern int ProcXkbGetGeometry(ClientPtr client);
extern int ProcXkbSetGeometry(ClientPtr client);
extern int ProcXkbPerClientFlags(ClientPtr client);
extern int ProcXkbListComponents(ClientPtr client);
extern int ProcXkbGetKbdByName(ClientPtr client);
extern int ProcXkbGetDeviceInfo(ClientPtr client);
extern int ProcXkbSetDeviceInfo(ClientPtr client);
extern int ProcXkbSetDebuggingFlags(ClientPtr client);

extern int XkbSetRepeatRate(DeviceIntPtr dev, int timeout, int interval, int major, int minor);
extern int XkbGetRepeatRate(DeviceIntPtr dev, int *timeout, int *interval);

extern Status XkbComputeGetIndicatorMapReplySize(
    XkbIndicatorPtr             indicators,
    xkbGetIndicatorMapReply     *rep);
extern int XkbSendIndicatorMap(
    ClientPtr                   client,
    XkbIndicatorPtr             indicators,
    xkbGetIndicatorMapReply     *rep);

extern void XkbComputeCompatState(XkbSrvInfoPtr xkbi);
extern void XkbSetPhysicalLockingKey(DeviceIntPtr dev, unsigned key);

extern Bool XkbFilterEvents(ClientPtr pClient, int nEvents, xEvent *xE);

extern Bool XkbApplyLEDChangeToKeyboard(
    XkbSrvInfoPtr           xkbi,
    XkbIndicatorMapPtr      map,
    Bool                    on,
    XkbChangesPtr           change);

extern Bool XkbWriteRulesProp(ClientPtr client, pointer closure);

extern XkbAction XkbGetButtonAction(DeviceIntPtr kbd, DeviceIntPtr dev, int button);

/* extern Status XkbMergeFile(XkbDescPtr xkb, XkbFileInfo finfo); */

extern Bool XkbDDXCompileNamedKeymap(
    XkbDescPtr              xkb,
    XkbComponentNamesPtr    names,
    char *                  nameRtrn,
    int                     nameRtrnLen);

extern Bool XkbDDXCompileKeymapByNames(
    XkbDescPtr              xkb,
    XkbComponentNamesPtr    names,
    unsigned                want,
    unsigned                need,
    char *                  nameRtrn,
    int                     nameRtrnLen);
