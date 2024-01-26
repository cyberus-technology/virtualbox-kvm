
#ifndef VBOX_INCLUDED_SRC_VBoxUSB_os2_usbcalls_h
#define VBOX_INCLUDED_SRC_VBoxUSB_os2_usbcalls_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifdef __cplusplus
  extern "C" {
#endif

typedef ULONG USBHANDLE, *PUSBHANDLE;
typedef ULONG USBNOTIFY, *PUSBNOTIFY;
typedef ULONG ISOHANDLE, *PISOHANDLE;

#define USB_NOT_INIT                    7000
#define USB_ERROR_NO_MORE_NOTIFICATIONS 7001
#define USB_ERROR_OUTOF_RESOURCES       7002
#define USB_ERROR_INVALID_ENDPOINT      7003
#define USB_ERROR_LESSTRANSFERED        7004

/* IORB status codes */
#define USB_IORB_DONE                   0x0000
#define USB_IORB_FAILED                 0x8000

#define USB_ANY_PRODUCTVERSION          0xFFFF
#define USB_OPEN_FIRST_UNUSED           0

#ifdef USB_BIND_DYNAMIC
  typedef APIRET APIENTRY USBREGISTERDEVICENOTIFICATION( PUSBNOTIFY pNotifyID,
                                                 HEV hDeviceAdded,
                                                 HEV hDeviceRemoved,
                                                 USHORT usVendor,
                                                 USHORT usProduct,
                                                 USHORT usBCDVersion);
  typedef USBREGISTERDEVICENOTIFICATION *PUSBREGISTERDEVICENOTIFICATION;

  typedef APIRET APIENTRY USBDEREGISTERNOTIFICATION( USBNOTIFY NotifyID);

  typedef USBDEREGISTERNOTIFICATION *PUSBDEREGISTERNOTIFICATION;

  typedef APIRET APIENTRY USBOPEN( PUSBHANDLE pHandle,
                           USHORT usVendor,
                           USHORT usProduct,
                           USHORT usBCDDevice,
                           USHORT usEnumDevice);
  typedef USBOPEN *PUSBOPEN;
  typedef APIRET APIENTRY USBCLOSE( USBHANDLE Handle);
  typedef USBCLOSE *PUSBCLOSE;

  typedef APIRET APIENTRY USBCTRLMESSAGE( USBHANDLE Handle,
                                  UCHAR  ucRequestType,
                                  UCHAR  ucRequest,
                                  USHORT usValue,
                                  USHORT usIndex,
                                  USHORT usLength,
                                  PVOID  pvData,
                                  ULONG  ulTimeout);
  typedef USBCTRLMESSAGE *PUSBCTRLMESSAGE;
#else
  APIRET APIENTRY UsbQueryNumberDevices( ULONG *pulNumDev);

  APIRET APIENTRY UsbQueryDeviceReport( ULONG ulDevNumber,
                                        ULONG *ulBufLen,
                                        PVOID pvData);
  APIRET APIENTRY UsbRegisterChangeNotification( PUSBNOTIFY pNotifyID,
                                                 HEV hDeviceAdded,
                                                 HEV hDeviceRemoved);

  APIRET APIENTRY UsbRegisterDeviceNotification( PUSBNOTIFY pNotifyID,
                                                 HEV hDeviceAdded,
                                                 HEV hDeviceRemoved,
                                                 USHORT usVendor,
                                                 USHORT usProduct,
                                                 USHORT usBCDVersion);

  APIRET APIENTRY UsbDeregisterNotification( USBNOTIFY NotifyID);

  APIRET APIENTRY UsbOpen( PUSBHANDLE pHandle,
                           USHORT usVendor,
                           USHORT usProduct,
                           USHORT usBCDDevice,
                           USHORT usEnumDevice);

  APIRET APIENTRY UsbClose( USBHANDLE Handle);

  APIRET APIENTRY UsbCtrlMessage( USBHANDLE Handle,
                                  UCHAR  ucRequestType,
                                  UCHAR  ucRequest,
                                  USHORT usValue,
                                  USHORT usIndex,
                                  USHORT usLength,
                                  PVOID  pvData,
                                  ULONG  ulTimeout);

  APIRET APIENTRY UsbBulkRead( USBHANDLE Handle,
                               UCHAR  Endpoint,
                               UCHAR  Interface,
                               ULONG *ulNumBytes,
                               PVOID  pvData,
                               ULONG  ulTimeout);

  APIRET APIENTRY UsbBulkRead2( USBHANDLE Handle,
                                UCHAR  Endpoint,
                                UCHAR  Interface,
                                BOOL   fShortOk,
                                ULONG *ulNumBytes,
                                PVOID  pvData,
                                ULONG  ulTimeout);

  APIRET APIENTRY UsbBulkWrite( USBHANDLE Handle,
                                UCHAR  Endpoint,
                                UCHAR  Interface,
                                ULONG  ulNumBytes,
                                PVOID  pvData,
                                ULONG  ulTimeout);

  APIRET APIENTRY UsbBulkWrite2( USBHANDLE Handle,
                                 UCHAR  Endpoint,
                                 UCHAR  AltInterface,
                                 BOOL   fShortOk,
                                 ULONG  ulNumBytes,
                                 PVOID  pvData,
                                 ULONG  ulTimeout);

  APIRET APIENTRY UsbIrqStart( USBHANDLE Handle,
                               UCHAR  Endpoint,
                               UCHAR  Interface,
                               USHORT usNumBytes,
                               PVOID  pvData,
                               PHEV   pHevModified);
  APIRET APIENTRY UsbIrqStop(  USBHANDLE Handle,
                               HEV       HevModified);

  APIRET APIENTRY UsbIsoStart( USBHANDLE Handle,
                               UCHAR  Endpoint,
                               UCHAR  Interface,
                               ISOHANDLE *phIso);
  APIRET APIENTRY UsbIsoStop( ISOHANDLE hIso);

  APIRET APIENTRY UsbIsoDequeue( ISOHANDLE hIso,
                                 PVOID pBuffer,
                                 ULONG ulNumBytes);
  APIRET APIENTRY UsbIsoEnqueue( ISOHANDLE hIso,
                                 const UCHAR * pBuffer,
                                 ULONG ulNumBytes);
  APIRET APIENTRY UsbIsoPeekQueue( ISOHANDLE hIso,
                                   UCHAR * pByte,
                                   ULONG ulOffset);
  APIRET APIENTRY UsbIsoGetLength( ISOHANDLE hIso,
                                   ULONG *pulLength);

  APIRET APIENTRY UsbIrqRead( USBHANDLE Handle,
                              UCHAR  Endpoint,
                              UCHAR  Interface,
                              ULONG  *ulNumBytes,
                              PVOID  pvData,
                              ULONG  ulTimeout);

  APIRET APIENTRY UsbFixupDevice( USBHANDLE Handle,
                                  UCHAR ucConfiguration,
                                  UCHAR *pucConfigurationData,
                                  ULONG ulConfigurationLen );

  APIRET APIENTRY InitUsbCalls(void);
  APIRET APIENTRY TermUsbCalls(void);

  /* Standard USB Requests See 9.4. in USB 1.1. spec. */

  /* 09 01 2003 - KIEWITZ */
  #define UsbDeviceClearFeature(HANDLE, FEAT)             \
		UsbCtrlMessage(HANDLE, 0x00, 0x01, FEAT, 0, 0, NULL, 0)
  #define UsbDeviceSetFeature(HANDLE, FEAT)             \
		UsbCtrlMessage(HANDLE, 0x80, 0x03, FEAT, 0, 0, NULL, 0)
  #define UsbInterfaceClearFeature(HANDLE, IFACE, FEAT)   \
		UsbCtrlMessage(HANDLE, 0x01, 0x01, FEAT, IFACE, 0, NULL, 0)
  #define UsbInterfaceSetFeature(HANDLE, IFACE, FEAT)   \
		UsbCtrlMessage(HANDLE, 0x80, 0x03, FEAT, IFACE, 0, NULL, 0)
  #define UsbEndpointClearFeature(HANDLE, ENDPOINT, FEAT) \
		UsbCtrlMessage(HANDLE, 0x02, 0x01, FEAT, ENDPOINT, 0, NULL, 0)
  #define UsbEndpointSetFeature(HANDLE, ENDPOINT, FEAT) \
		UsbCtrlMessage(HANDLE, 0x80, 0x03, FEAT, ENDPOINT, 0, NULL, 0)
  #define FEATURE_DEVICE_REMOTE_WAKEUP 1
  #define FEATURE_ENDPOINT_HALT 0
  #define UsbEndpointClearHalt(HANDLE, ENDPOINT)	  \
		UsbEndpointClearFeature(HANDLE, ENDPOINT, FEATURE_ENDPOINT_HALT)

  #define UsbDeviceGetConfiguration(HANDLE, DATA)	  \
		UsbCtrlMessage(HANDLE, 0x80, 0x08, 0, 0, 1, DATA, 0)
  #define UsbDeviceSetConfiguration(HANDLE, CONFIG) 	  \
		UsbCtrlMessage(HANDLE, 0x00, 0x09, CONFIG, 0, 0, NULL, 0)

  #define UsbDeviceGetStatus(HANDLE, STATUS)              \
		UsbCtrlMessage(HANDLE, 0x80, 0x00, 0, 0, 2, STATUS, 0)
  #define UsbInterfaceGetStatus(HANDLE, IFACE, STATUS)    \
		UsbCtrlMessage(HANDLE, 0x80, 0x00, 0, IFACE, 2, STATUS, 0)
  #define UsbEndpointGetStatus(HANDLE, ENDPOINT, STATUS)  \
		UsbCtrlMessage(HANDLE, 0x80, 0x00, 0, ENDPOINT, 2, STATUS, 0)

  #define STATUS_ENDPOINT_HALT       0x0001
  #define STATUS_DEVICE_SELFPOWERD   0x0001
  #define STATUS_DEVICE_REMOTEWAKEUP 0x0002

  #define UsbDeviceSetAddress(HANDLE, ADDRESS)		  \
		UsbCtrlMessage(HANDLE, 0x80, 0x05, ADDRESS, 0, 0, NULL, 0)

  #define UsbDeviceGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)        \
		UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0100|INDEX), LID, LEN, DATA, 0)
  #define UsbDeviceSetDescriptor(HANDLE, INDEX, LID, LEN, DATA)        \
		UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0100|INDEX), LID, LEN, DATA, 0)
  #define UsbConfigurationGetDescriptor(HANDLE, INDEX, LID, LEN, DATA) \
		UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0200|INDEX), LID, LEN, DATA, 0)
  #define UsbConfigurationSetDescriptor(HANDLE, INDEX, LID, LEN, DATA) \
		UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0200|INDEX), LID, LEN, DATA, 0)
  #define UsbStringGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)        \
		UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0300|INDEX), LID, LEN, DATA, 0)
  #define UsbStringSetDescriptor(HANDLE, INDEX, LID, LEN, DATA) \
		UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0300|INDEX), LID, LEN, DATA, 0)
  #define UsbInterfaceGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)     \
		UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0400|INDEX), LID, LEN, DATA, 0)
  #define UsbInterfaceSetDescriptor(HANDLE, INDEX, LID, LEN, DATA)     \
		UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0400|INDEX), LID, LEN, DATA, 0)
  #define UsbEndpointGetDescriptor(HANDLE, INDEX, LID, LEN, DATA)      \
		UsbCtrlMessage(HANDLE, 0x80, 0x06, (0x0500|INDEX), LID, LEN, DATA, 0)
  #define UsbEndpointSetDescriptor(HANDLE, INDEX, LID, LEN, DATA)      \
		UsbCtrlMessage(HANDLE, 0x80, 0x07, (0x0500|INDEX), LID, LEN, DATA, 0)

  #define UsbInterfaceGetAltSetting(HANDLE, IFACE, SETTING)            \
		UsbCtrlMessage(HANDLE, 0x81, 0x0A, 0, IFACE, 1, SETTING, 0)
  #define UsbInterfaceSetAltSetting(HANDLE, IFACE, ALTSET) \
		UsbCtrlMessage(HANDLE, 0x01, 0x0B, ALTSET, IFACE, 0, NULL, 0)

  #define UsbEndpointSynchFrame(HANDLE, ENDPOINT, FRAMENUM) \
		UsbCtrlMessage(HANDLE, 0x82, 0x0B, 0, ENDPOINT, 2, FRAMENUM, 0)
#endif


#ifdef __cplusplus
}
#endif

#endif /* !VBOX_INCLUDED_SRC_VBoxUSB_os2_usbcalls_h */
