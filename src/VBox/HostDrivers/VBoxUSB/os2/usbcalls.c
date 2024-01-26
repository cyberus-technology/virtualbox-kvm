#define INCL_DOSERRORS
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSMODULEMGR
#include <os2.h>

#if !defined(__GNUC__) || defined(STATIC_USBCALLS)
#include <string.h>
#else
#define memcpy __builtin_memcpy
#endif
#include <stdlib.h>
#include <stdio.h>

#define LOG_GROUP LOG_GROUP_DRV_USBPROXY
#include <VBox/log.h>

#ifdef __GNUC__
# define APIEXPORT  __declspec(dllexport)
#else
# define APIEXPORT
#endif

#ifndef ERROR_USER_DEFINED_BASE
/*#define ERROR_USER_DEFINED_BASE         0xFF00 */

#define ERROR_I24_WRITE_PROTECT         0
#define ERROR_I24_BAD_UNIT              1
#define ERROR_I24_NOT_READY             2
#define ERROR_I24_BAD_COMMAND           3
#define ERROR_I24_CRC                   4
#define ERROR_I24_BAD_LENGTH            5
#define ERROR_I24_SEEK                  6
#define ERROR_I24_NOT_DOS_DISK          7
#define ERROR_I24_SECTOR_NOT_FOUND      8
#define ERROR_I24_OUT_OF_PAPER          9
#define ERROR_I24_WRITE_FAULT           10
#define ERROR_I24_READ_FAULT            11
#define ERROR_I24_GEN_FAILURE           12
#define ERROR_I24_DISK_CHANGE           13
#define ERROR_I24_WRONG_DISK            15
#define ERROR_I24_UNCERTAIN_MEDIA       16
#define ERROR_I24_CHAR_CALL_INTERRUPTED 17
#define ERROR_I24_NO_MONITOR_SUPPORT    18
#define ERROR_I24_INVALID_PARAMETER     19
#define ERROR_I24_DEVICE_IN_USE         20
#define ERROR_I24_QUIET_INIT_FAIL       21
#endif

#include "usbcalls.h"

#define  IOCAT_USBRES            0x000000A0  /* USB Resource device control */
#define  IOCTLF_NUMDEVICE        0x00000031  /* Get Number of plugged in Devices */
#define  IOCTLF_GETINFO          0x00000032  /* Get Info About a device */
#define  IOCTLF_AQUIREDEVICE     0x00000033
#define  IOCTLF_RELEASEDEVICE    0x00000034
#define  IOCTLF_GETSTRING        0x00000035
#define  IOCTLF_SENDCONTROLURB   0x00000036
#define  IOCTLF_SENDBULKURB      0x00000037  /* Send */
#define  IOCTLF_START_IRQ_PROC   0x00000038  /* Start IRQ polling in a buffer */
#define  IOCTLF_GETDEVINFO       0x00000039  /* Get information about device */
#define  IOCTLF_STOP_IRQ_PROC    0x0000003A  /* Stop IRQ Polling */
#define  IOCTLF_START_ISO_PROC   0x0000003B  /* Start ISO buffering in a Ringbuffer */
#define  IOCTLF_STOP_ISO_PROC    0x0000003C  /* Stop ISO buffering */
#define  IOCTLF_CANCEL_IORB      0x0000003D  /* Abort an IORB; */
#define  IOCTLF_SELECT_BULKPIPE  0x0000003E  /* Select which Bulk endpoints can be used via Read/Write */
#define  IOCTLF_SENDIRQURB       0x0000003F  /* Start IRQ polling in a buffer */
#define  IOCTLF_FIXUPDEVUCE      0x00000040  /* Fixup USB device configuration data */
#define  IOCTLF_REG_STATUSSEM    0x00000041  /* Register Semaphore for general Statuschange */
#define  IOCTLF_DEREG_STATUSSEM  0x00000042  /* Deregister Semaphore */
#define  IOCTLF_REG_DEVICESEM    0x00000043  /* Register Semaphore for a vendor&deviceID */
#define  IOCTLF_DEREG_DEVICESEM  0x00000044  /* Deregister Semaphore */


#define NOTIFY_FREE   0
#define NOTIFY_CHANGE 1
#define NOTIFY_DEVICE 2
#define MAX_NOTIFICATIONS 256

#pragma pack(1)

typedef struct
{
  HEV    hDeviceAdded;
  HEV    hDeviceRemoved;
  USHORT usFlags;
  USHORT usVendor;
  USHORT usProduct;
  USHORT usBCDDevice;
} NOTIFYENTRY, *PNOTIFYENTRY;

#define DEV_SEM_ADD       0x00000001
#define DEV_SEM_REMOVE    0x00000002
#define DEV_SEM_MASK      0x00000003
#define DEV_SEM_VENDORID  0x00000004
#define DEV_SEM_PRODUCTID 0x00000008
#define DEV_SEM_BCDDEVICE 0x00000010

typedef struct{
  ULONG  ulSize;
  ULONG  ulCaps;
  ULONG  ulSemDeviceAdd;
  ULONG  ulSemDeviceRemove;
} STATUSEVENTSET, * PSTATUSEVENTSET;


typedef struct{
  ULONG  ulSize;
  ULONG  ulCaps;
  ULONG  ulSemDeviceAdd;
  ULONG  ulSemDeviceRemove;
  USHORT usVendorID;
  USHORT usProductID;
  USHORT usBCDDevice;
  USHORT usStatus;
} DEVEVENTSET, * PDEVEVENTSET;

typedef struct
{
  USHORT usVendorID;
  USHORT usProductID;
  USHORT usBCDDevice;
  USHORT usDeviceNumber; /* Get the usDeviceNumber device in the system fi. if 2 acquire the 2nd device
                            0 means first not acquired device. */
} AQUIREDEV, *PAQUIREDEV;

typedef struct
{
  UCHAR  bRequestType;
  UCHAR  bRequest;
  USHORT wValue;
  USHORT wIndex;
  USHORT wLength;
  ULONG  ulTimeout; /* in milliseconds */
} SETUPPACKET, *PSETUPPACKET;

typedef struct
{
  ULONG  ulHandle;
  UCHAR  bRequestType;
  UCHAR  bRequest;
  USHORT wValue;
  USHORT wIndex;
  USHORT wLength;
  ULONG  ulTimeout; /* in milliseconds */
  USHORT usStatus;
} USBCALLS_CTRL_REQ, *PUSBCALLS_CTRL_REQ;

typedef struct
{
  ULONG  ulDevHandle;
  UCHAR  ucEndpoint;
  UCHAR  ucAltInterface;
  USHORT usStatus;
  ULONG  ulEvent;
  ULONG  ulID;
} USBCALLS_ISO_START,  *NPUSBCALLS_ISO_START,  FAR *PUSBCALLS_ISO_START,
  USBCALLS_IRQ_START,  *NPUSBCALLS_IRQ_START,  FAR *PUSBCALLS_IRQ_START,
  USBCALLS_CANCEL_REQ, *NPUSBCALLS_CANCEL_REQ, FAR *PUSBCALLS_CANCEL_REQ;

#define ISO_DIRMASK 0x80
typedef struct
{
  ULONG  hSemAccess;        /* Synchronise access to the Pos values */
  ULONG  hDevice;
  USHORT usPosWrite;
  USHORT usPosRead;
  USHORT usBufSize;
  UCHAR  ucEndpoint;
  UCHAR  ucAltInterface;
  UCHAR  ucBuffer[16*1023];
} ISORINGBUFFER, *PISORINGBUFFER;

typedef USBCALLS_ISO_START USBCALLS_ISO_STOP, * NPUSBCALLS_ISO_STOP, FAR *PUSBCALLS_ISO_STOP;
typedef USBCALLS_ISO_START USBCALLS_IRQ_STOP, * NPUSBCALLS_IRQ_STOP, FAR *PUSBCALLS_IRQ_STOP;

#define USB_TRANSFER_FULL_SIZE 0x01

typedef struct
{
  ULONG  ulDevHandle;
  UCHAR  ucEndpoint;
  UCHAR  ucAltInterface;
  USHORT usStatus;
  ULONG  ulEvent;
/*   ULONG  ulID; - yeah, right */
  ULONG  ulTimeout;
  USHORT usDataProcessed;
  USHORT usDataRemain;
  USHORT usFlags;
} USBCALLS_BULK_REQ, *PUSBCALLS_BULK_REQ;

typedef struct
{
  ULONG  ulDevHandle;
  UCHAR  ucEndpoint;
  UCHAR  ucAltInterface;
  USHORT usStatus;
  ULONG  ulEvent;
  ULONG  ulID;
  ULONG  ulTimeout;
  USHORT usDataLen;
} LIBUSB_IRQ_REQ, *NPLIBUSB_IRQ_REQ, FAR *PLIBUSB_IRQ_REQ;

typedef struct
{
  ULONG  ulDevHandle;
  UCHAR  ucConfiguration;
  UCHAR  ucAltInterface;
  USHORT usStatus;
} LIBUSB_FIXUP, *NPLIBUSB_FIXUP, FAR *PLIBUSB_FIXUP;

#pragma pack()

/******************************************************************************/

HFILE g_hUSBDrv;
ULONG g_cInit;
ULONG g_ulFreeNotifys;
HMTX  g_hSemNotifytable;
NOTIFYENTRY g_Notifications[MAX_NOTIFICATIONS];

HMTX  g_hSemRingBuffers;
PISORINGBUFFER g_pIsoRingBuffers;
ULONG g_ulNumIsoRingBuffers;

APIEXPORT APIRET APIENTRY
InitUsbCalls(void)
{
  int i;
  ULONG ulAction;
  APIRET rc;

  if (++g_cInit > 1)
      return NO_ERROR;

  rc = DosOpen( (PCSZ)"USBRESM$",
                &g_hUSBDrv,
                &ulAction,
                0,
                FILE_NORMAL,
                OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_ACCESS_READWRITE |
                OPEN_FLAGS_NOINHERIT |
                OPEN_SHARE_DENYNONE,
                0 );
  if(rc)
  {
    g_hUSBDrv = 0;
    g_cInit   = 0;

  }
  else
  {
    /* @@ToDO Add EnvVar or INI for dynamically setting the number */
    g_ulNumIsoRingBuffers = 8;
    for(i=0;i<MAX_NOTIFICATIONS;i++)
    {
      g_Notifications[i].usFlags        = NOTIFY_FREE;
      g_Notifications[i].hDeviceAdded   = 0;
      g_Notifications[i].hDeviceRemoved = 0;
      g_Notifications[i].usVendor       = 0;
      g_Notifications[i].usProduct      = 0;
      g_Notifications[i].usBCDDevice    = 0;
    }
    rc = DosAllocMem( (PPVOID)&g_pIsoRingBuffers,
                      g_ulNumIsoRingBuffers * sizeof(ISORINGBUFFER),
                      PAG_WRITE | PAG_COMMIT | OBJ_TILE);
    if(!rc)
    {
      PISORINGBUFFER pIter = g_pIsoRingBuffers;
      for(i=0;i< g_ulNumIsoRingBuffers;i++,pIter++)
      {
        pIter->hDevice        = 0;
        pIter->hSemAccess     = 0;      /* Synchronise access to the Pos values */
        pIter->usPosWrite     = 0;
        pIter->usPosRead      = 0;
        pIter->usBufSize      = 16*1023;
        pIter->ucEndpoint     = 0;
        pIter->ucAltInterface = 0;
        /*pIter->ucBuffer */
      }
      rc=DosCreateMutexSem(NULL,&g_hSemRingBuffers,DC_SEM_SHARED,FALSE);
      if(!rc)
      {
        rc=DosCreateMutexSem(NULL,&g_hSemNotifytable,DC_SEM_SHARED,FALSE);
        if(rc)
        {
          DosCloseMutexSem(g_hSemRingBuffers);
          DosFreeMem(g_pIsoRingBuffers);
        }
      }
      else
      {
        DosFreeMem(g_pIsoRingBuffers);
      }
    }

    if(rc)
    {
      DosClose(g_hUSBDrv);
      g_hUSBDrv = 0;
      g_cInit   = 0;
    }
  }
  return g_cInit ? NO_ERROR : rc ? rc : ERROR_GEN_FAILURE;
}

APIEXPORT APIRET APIENTRY
TermUsbCalls(void)
{
    if (!g_cInit)
      return ERROR_GEN_FAILURE;
    if(!--g_cInit)
    {
      int i;
      for(i=0;i<MAX_NOTIFICATIONS;i++)
        if( g_Notifications[i].usFlags != NOTIFY_FREE);
          UsbDeregisterNotification((USBNOTIFY)(&g_Notifications[i]));

      DosClose(g_hUSBDrv);
      g_hUSBDrv = NULLHANDLE;

      if (g_pIsoRingBuffers)
        DosFreeMem(g_pIsoRingBuffers);

      DosCloseMutexSem(g_hSemRingBuffers);
      g_hSemRingBuffers = NULLHANDLE;
      DosCloseMutexSem(g_hSemNotifytable);
      g_hSemNotifytable = NULLHANDLE;
    }
    return NO_ERROR;
}


#ifdef VBOX /* complete wast of time */
# define IsBadReadPointer(pBase, ulSize)    (FALSE)
# define IsBadWritePointer(pBase, ulSize)   (FALSE)
#else
static BOOL IsBadReadPointer(PVOID pBase, ULONG ulSize)
{
  APIRET rc;
  ULONG ulFlags;
  rc = DosQueryMem(pBase, &ulSize, &ulFlags);

  return rc!=0?TRUE:(ulFlags&PAG_READ)&&(ulFlags&PAG_COMMIT)?FALSE:TRUE;
}

static BOOL IsBadWritePointer(PVOID pBase, ULONG ulSize)
{
  APIRET rc;
  ULONG ulFlags;
  rc = DosQueryMem(pBase, &ulSize, &ulFlags);

  return rc!=0?TRUE:((ulFlags&PAG_WRITE)==PAG_WRITE&&(ulFlags&PAG_COMMIT)==PAG_COMMIT)?FALSE:TRUE;
}
#endif

APIEXPORT APIRET APIENTRY
UsbQueryNumberDevices(ULONG *pulNumDev)
{
  APIRET rc;
  ULONG ulLength;
  if(!g_cInit)
    return USB_NOT_INIT;

  if( IsBadWritePointer(pulNumDev,sizeof(ULONG)) )
    return ERROR_INVALID_PARAMETER;
  ulLength=sizeof(ULONG);
  *pulNumDev = 0;
  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_NUMDEVICE,
                    NULL, 0, NULL,
                    pulNumDev, ulLength, &ulLength);
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbQueryDeviceReport(ULONG ulDevNumber, ULONG *pulBufLen, PVOID pData)
{
  APIRET rc;
  ULONG ulParmLen;

  if(!g_cInit)
    return USB_NOT_INIT;

  if( IsBadWritePointer(pulBufLen, sizeof(ULONG)) )
    return ERROR_INVALID_PARAMETER;

  if( pData!=NULL && IsBadWritePointer(pData,*pulBufLen) )
    return ERROR_INVALID_PARAMETER;
  if(pData==NULL)
   *pulBufLen = 0;
  ulParmLen = sizeof(ulDevNumber);
  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_GETINFO,
                     (PVOID)&ulDevNumber, ulParmLen, &ulParmLen,
                    pData, *pulBufLen, pulBufLen);
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbRegisterChangeNotification( PUSBNOTIFY pNotifyID,
                               HEV hDeviceAdded,
                               HEV hDeviceRemoved)
{
  APIRET rc;
  int i;
  STATUSEVENTSET EventSet;
  ULONG ulSize;

  if(!g_cInit)
    return USB_NOT_INIT;

  if( IsBadWritePointer(pNotifyID, sizeof(ULONG)) ||
      (hDeviceAdded==0 && hDeviceRemoved==0) )
    return ERROR_INVALID_PARAMETER;

  ulSize = sizeof(EventSet);
  EventSet.ulSize = ulSize;
  EventSet.ulCaps = 0;

  if(hDeviceAdded!=0)
  {
    ULONG ulCnt;
    rc = DosQueryEventSem(hDeviceAdded,&ulCnt);
    if(rc)
      return rc;
    EventSet.ulCaps         |= DEV_SEM_ADD;
    EventSet.ulSemDeviceAdd = hDeviceAdded;
  }

  if(hDeviceRemoved!=0)
  {
    ULONG ulCnt;
    rc = DosQueryEventSem(hDeviceRemoved,&ulCnt);
    if(rc)
      return rc;
    EventSet.ulCaps            |= DEV_SEM_REMOVE;
    EventSet.ulSemDeviceRemove = hDeviceRemoved;
  }

  rc = DosRequestMutexSem(g_hSemNotifytable,SEM_INDEFINITE_WAIT);
  if(rc)
    return rc;

  for(i=0;i<MAX_NOTIFICATIONS;i++)
  {
    if( g_Notifications[i].usFlags == NOTIFY_FREE)
    {
      g_Notifications[i].usFlags = NOTIFY_CHANGE;
      g_Notifications[i].hDeviceAdded   = hDeviceAdded;
      g_Notifications[i].hDeviceRemoved = hDeviceRemoved;
      g_Notifications[i].usVendor       = 0;
      g_Notifications[i].usProduct      = 0;
      g_Notifications[i].usBCDDevice    = 0;
      break;
    }
  }
  DosReleaseMutexSem(g_hSemNotifytable);
  if(i==MAX_NOTIFICATIONS)
    return USB_ERROR_NO_MORE_NOTIFICATIONS;

  /* @@ToDo come up with a better way to generate IDs */
  *pNotifyID = (USBNOTIFY) (&g_Notifications[i]);
  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_REG_STATUSSEM,
                    NULL, 0, NULL,
                    &EventSet,ulSize, &ulSize);
  if(rc)
  {
    g_Notifications[i].usFlags = NOTIFY_FREE;
    *pNotifyID = 0;
  }
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbRegisterDeviceNotification( PUSBNOTIFY pNotifyID,
                               HEV hDeviceAdded,
                               HEV hDeviceRemoved,
                               USHORT usVendor,
                               USHORT usProduct,
                               USHORT usBCDVersion)
{
  DEVEVENTSET EventSet;
  ULONG ulCnt,ulSize;
  int i;
  APIRET rc;

  if(!g_cInit)
    return USB_NOT_INIT;

  if( IsBadWritePointer(pNotifyID, sizeof(ULONG)) ||
      hDeviceAdded==0 || hDeviceRemoved==0 ||
      usVendor  == 0  || usVendor  == 0xFFFF ||
      usProduct == 0  || usProduct == 0xFFFF )
    return ERROR_INVALID_PARAMETER;


  rc = DosQueryEventSem(hDeviceAdded,&ulCnt);
  if(rc)
    return rc;
  rc = DosQueryEventSem(hDeviceRemoved,&ulCnt);
  if(rc)
    return rc;

  ulSize = sizeof(EventSet);
  EventSet.ulSize            = ulSize;
  EventSet.ulCaps            = DEV_SEM_ADD | DEV_SEM_REMOVE |
                               DEV_SEM_VENDORID | DEV_SEM_PRODUCTID |
                               DEV_SEM_BCDDEVICE ;
  EventSet.ulSemDeviceAdd    = hDeviceAdded;
  EventSet.ulSemDeviceRemove = hDeviceRemoved;
  EventSet.usVendorID        = usVendor;
  EventSet.usProductID       = usProduct;
  EventSet.usBCDDevice       = usBCDVersion;
  EventSet.usStatus          = 0;

  rc = DosRequestMutexSem(g_hSemNotifytable,SEM_INDEFINITE_WAIT);

  if(rc)
    return rc;

  for(i=0;i<MAX_NOTIFICATIONS;i++)
  {
    if( g_Notifications[i].usFlags == NOTIFY_FREE)
    {
      g_Notifications[i].usFlags = NOTIFY_DEVICE;
      g_Notifications[i].hDeviceAdded   = hDeviceAdded;
      g_Notifications[i].hDeviceRemoved = hDeviceRemoved;
      g_Notifications[i].usVendor       = usVendor;
      g_Notifications[i].usProduct      = usProduct;
      g_Notifications[i].usBCDDevice    = usBCDVersion;
      break;
    }
  }
  DosReleaseMutexSem(g_hSemNotifytable);
  if(i==MAX_NOTIFICATIONS)
    return USB_ERROR_NO_MORE_NOTIFICATIONS;

  /* @@ToDo come up with a better way to generate IDs */
  *pNotifyID = (USBNOTIFY) (&g_Notifications[i]);
  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_REG_DEVICESEM,
                    NULL, 0, NULL,
                    &EventSet,ulSize, &ulSize);
  if(rc)
  {
    if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_INVALID_PARAMETER) )
      rc= ERROR_INVALID_PARAMETER;
    if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_GEN_FAILURE) )
      rc= EventSet.usStatus;

    g_Notifications[i].usFlags = NOTIFY_FREE;
    *pNotifyID = 0;
  }
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbDeregisterNotification( USBNOTIFY NotifyID )
{
  DEVEVENTSET EventSet;
  USBNOTIFY   MinID,MaxID;
  ULONG       Index, ulFunction, ulSize;
  APIRET      rc;

  if(!g_cInit)
    return USB_NOT_INIT;

  MinID = (USBNOTIFY) (&g_Notifications[0]);
  MaxID = (USBNOTIFY) (&g_Notifications[MAX_NOTIFICATIONS-1]);

  if(NotifyID<MinID || NotifyID>MaxID)
    return ERROR_INVALID_PARAMETER;

  Index = NotifyID - MinID;

  if(Index % sizeof(NOTIFYENTRY))
    return ERROR_INVALID_PARAMETER;

  Index /= sizeof(NOTIFYENTRY);

  rc = DosRequestMutexSem(g_hSemNotifytable,SEM_INDEFINITE_WAIT);

  switch(g_Notifications[Index].usFlags)
  {
    case NOTIFY_FREE:
      DosReleaseMutexSem(g_hSemNotifytable);
      return ERROR_INVALID_PARAMETER;
    case NOTIFY_CHANGE:
      ulFunction  = IOCTLF_DEREG_STATUSSEM;
      ulSize = sizeof(STATUSEVENTSET);
      EventSet.ulSize            = ulSize;
      EventSet.ulCaps            = DEV_SEM_ADD | DEV_SEM_REMOVE;
      EventSet.ulSemDeviceAdd    = g_Notifications[Index].hDeviceAdded;
      EventSet.ulSemDeviceRemove = g_Notifications[Index].hDeviceRemoved;
      break;
    case NOTIFY_DEVICE:
      ulFunction = IOCTLF_DEREG_DEVICESEM;
      ulSize = sizeof(DEVEVENTSET);
      EventSet.ulSize            = ulSize;
      EventSet.ulCaps            = DEV_SEM_ADD | DEV_SEM_REMOVE |
                                   DEV_SEM_VENDORID | DEV_SEM_PRODUCTID |
                                   DEV_SEM_BCDDEVICE ;
      EventSet.ulSemDeviceAdd    = g_Notifications[Index].hDeviceAdded;
      EventSet.ulSemDeviceRemove = g_Notifications[Index].hDeviceRemoved;
      EventSet.usVendorID        = g_Notifications[Index].usVendor;
      EventSet.usProductID       = g_Notifications[Index].usProduct;
      EventSet.usBCDDevice       = g_Notifications[Index].usBCDDevice;
      EventSet.usStatus          = 0;
      break;
    default:
      DosReleaseMutexSem(g_hSemNotifytable);
      return ERROR_GEN_FAILURE;
  }

  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, ulFunction,
                    NULL, 0, NULL,
                    &EventSet,ulSize, &ulSize);
  if(0==rc)
  {
    g_Notifications[Index].usFlags        = NOTIFY_FREE;
    g_Notifications[Index].hDeviceAdded   = 0;
    g_Notifications[Index].hDeviceRemoved = 0;
    g_Notifications[Index].usVendor       = 0;
    g_Notifications[Index].usProduct      = 0;
    g_Notifications[Index].usBCDDevice    = 0;
  } else
  {
    if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_INVALID_PARAMETER) )
      rc= ERROR_INVALID_PARAMETER;
    if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_GEN_FAILURE) )
      rc= EventSet.usStatus;
  }
  DosReleaseMutexSem(g_hSemNotifytable);

  return rc;
}

APIEXPORT APIRET APIENTRY
UsbOpen( PUSBHANDLE pHandle,
         USHORT usVendor,
         USHORT usProduct,
         USHORT usBCDDevice,
         USHORT usEnumDevice)
{
  ULONG     ulCat, ulFunc;
  ULONG     ulParmLen, ulDataLen;
  AQUIREDEV Aquire;
  APIRET    rc;

  if(!g_cInit)
    return USB_NOT_INIT;
  if(IsBadWritePointer(pHandle,sizeof(USBHANDLE)) )
    return ERROR_INVALID_PARAMETER;

  Aquire.usVendorID     = usVendor;
  Aquire.usProductID    = usProduct;
  Aquire.usBCDDevice    = usBCDDevice;
  Aquire.usDeviceNumber = usEnumDevice;
  ulCat  = 0xA0;
  ulFunc = 0x33;
  ulParmLen = sizeof(Aquire);
  ulDataLen = sizeof(USBHANDLE);
  rc = DosDevIOCtl( g_hUSBDrv,
                    ulCat,ulFunc, /*IOCAT_USBRES, IOCTLF_AQUIREDEVICE, */
                    &Aquire, ulParmLen, &ulParmLen,
                    pHandle, ulDataLen, &ulDataLen);

  /* @@ ToDO maybe gether some info about device here (endpoints etc for safety checks) */
  return rc;

}

APIEXPORT APIRET APIENTRY
UsbClose( USBHANDLE Handle)
{
  APIRET rc;
  ULONG ulDataLen,ulParmLen;
  if(!g_cInit)
    return USB_NOT_INIT;

  ulParmLen = sizeof(USBHANDLE);
  ulDataLen = 0;

  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_RELEASEDEVICE,
                    (PVOID)&Handle, ulParmLen, &ulParmLen,
                    NULL, ulDataLen, &ulDataLen);
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbCtrlMessage( USBHANDLE Handle,
                UCHAR  ucRequestType,
                UCHAR  ucRequest,
                USHORT usValue,
                USHORT usIndex,
                USHORT usLength,
                PVOID pData,
                ULONG  ulTimeout)
{
  APIRET            rc;
  USBCALLS_CTRL_REQ CtrlRequest;
  ULONG             ulParmLen, ulDataLen;

  if(!g_cInit)
    return USB_NOT_INIT;

  ulParmLen = sizeof(USBCALLS_CTRL_REQ);
  CtrlRequest.ulHandle     = Handle;
  CtrlRequest.bRequestType = ucRequestType;
  CtrlRequest.bRequest     = ucRequest;
  CtrlRequest.wValue       = usValue;
  CtrlRequest.wIndex       = usIndex;
  CtrlRequest.wLength      = usLength;
  CtrlRequest.ulTimeout    = ulTimeout;
  ulDataLen = usLength;

  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_SENDCONTROLURB,
                    (PVOID)&CtrlRequest, ulParmLen, &ulParmLen,
                    ulDataLen>0?(PVOID)pData:NULL,
                    ulDataLen,
                    ulDataLen>0?&ulDataLen:NULL);
  if( rc != NO_ERROR )
  {
     if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_INVALID_PARAMETER) )
       rc= ERROR_INVALID_PARAMETER;
     if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_GEN_FAILURE) )
       rc= CtrlRequest.usStatus;
  }
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbBulkRead( USBHANDLE Handle,
             UCHAR  Endpoint,
             UCHAR  AltInterface,
             ULONG  *ulNumBytes,
             PVOID  pvData,
             ULONG  ulTimeout)
{
    return UsbBulkRead2(Handle, Endpoint, AltInterface, TRUE /* fShortOk */, ulNumBytes, pvData, ulTimeout);
}

APIEXPORT APIRET APIENTRY
UsbBulkRead2( USBHANDLE Handle,
              UCHAR  Endpoint,
              UCHAR  AltInterface,
              BOOL   fShortOk,
              ULONG  *ulNumBytes,
              PVOID  pvData,
              ULONG  ulTimeout)
{
  APIRET            rc;
  ULONG             ulParmLen, ulDataLen, ulToProcess, ulTotalProcessed;
  USBCALLS_BULK_REQ BulkRequest;

  if(!g_cInit)
    return USB_NOT_INIT;

  if(*ulNumBytes==0)
    return 0;

  /* just require this */
  if ((ULONG)pvData & 0xfff)
    return ERROR_INVALID_ADDRESS;
  if ((ULONG)pvData >= 0x20000000)
    return ERROR_INVALID_ADDRESS;

  ulToProcess                = *ulNumBytes;
  ulTotalProcessed           = 0;

  do
  {
    /* Process up to 64k, making sure we're working on segments. */
    ulDataLen = 0x10000 - ((ULONG)pvData & 0xffff);
    if (ulDataLen > ulToProcess)
        ulDataLen = ulToProcess;

    ulParmLen = sizeof(USBCALLS_BULK_REQ);

    memset(&BulkRequest, 0, sizeof(BulkRequest));
    BulkRequest.ulDevHandle    = Handle;
    BulkRequest.ucEndpoint     = Endpoint;
    BulkRequest.ucAltInterface = AltInterface;
    BulkRequest.usStatus       = 0;
    BulkRequest.ulEvent        = 0;
    //BulkRequest.ulID           = (ULONG)pvData;
    BulkRequest.ulTimeout      = ulTimeout;
    BulkRequest.usDataProcessed = 0;
    BulkRequest.usDataRemain   = ulDataLen;
    BulkRequest.usFlags        = fShortOk && ulDataLen == ulToProcess ? 0 : USB_TRANSFER_FULL_SIZE;

    rc = DosDevIOCtl( g_hUSBDrv,
                      IOCAT_USBRES, IOCTLF_SENDBULKURB,
                      (PVOID)&BulkRequest, ulParmLen, &ulParmLen,
                      pvData, ulDataLen, &ulDataLen);
    Log(("BulkRead: usStatus=%d rc=%ld usDataProcessed=%d usDataRemain=%d ulDataLen=%ld\n",
         BulkRequest.usStatus, rc, BulkRequest.usDataProcessed, BulkRequest.usDataRemain, ulDataLen));

    if (rc != NO_ERROR)
    {
      if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_INVALID_PARAMETER) )
        rc= ERROR_INVALID_PARAMETER;
      if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_GEN_FAILURE) )
        rc= BulkRequest.usStatus;
      break;
    }

    /* Adjust count and source pointer */
    ulToProcess      -= ulDataLen;
    pvData           = (PBYTE)pvData + ulDataLen;
    ulTotalProcessed += BulkRequest.usDataProcessed;

    if (BulkRequest.usDataProcessed != ulDataLen)
    {
      /* Transferred less than we wanted? so something is wrong,
         or device doesn't wish to send more, exit loop */
      rc = USB_ERROR_LESSTRANSFERED;
      break;
    }
  } while( ulToProcess>0 );

  *ulNumBytes = ulTotalProcessed;

  return rc;
}

APIEXPORT APIRET APIENTRY
UsbBulkWrite( USBHANDLE Handle,
              UCHAR  Endpoint,
              UCHAR  AltInterface,
              ULONG  ulNumBytes,
              PVOID  pvData,
              ULONG  ulTimeout)
{
    return UsbBulkWrite2(Handle, Endpoint, AltInterface, FALSE /* fShortOk */, ulNumBytes, pvData, ulTimeout);
}

APIEXPORT APIRET APIENTRY
UsbBulkWrite2( USBHANDLE Handle,
               UCHAR  Endpoint,
               UCHAR  AltInterface,
               BOOL   fShortOk,
               ULONG  ulNumBytes,
               PVOID  pvData,
               ULONG  ulTimeout)
{
  APIRET            rc;
  ULONG             ulParmLen, ulDataLen;
  USBCALLS_BULK_REQ BulkRequest;

  if(!g_cInit)
    return USB_NOT_INIT;

  /* just require this */
  if ((ULONG)pvData & 0xfff)
    return ERROR_INVALID_ADDRESS;
  if ((ULONG)pvData >= 0x20000000)
    return ERROR_INVALID_ADDRESS;

  do
  {
    /* Process up to 64k, making sure we're working on segments. */
    ulDataLen = 0x10000 - ((ULONG)pvData & 0xffff);
    if (ulDataLen > ulNumBytes)
        ulDataLen = ulNumBytes;

    ulParmLen = sizeof(USBCALLS_BULK_REQ);

    memset(&BulkRequest, 0, sizeof(BulkRequest));
    BulkRequest.ulDevHandle    = Handle;
    BulkRequest.ucEndpoint     = Endpoint;
    BulkRequest.ucAltInterface = AltInterface;
    BulkRequest.usStatus       = 0;
    BulkRequest.ulEvent        = 0;
    //BulkRequest.ulID           = (ULONG)pvData;
    BulkRequest.ulTimeout      = ulTimeout;
    BulkRequest.usDataProcessed = 0;
    BulkRequest.usDataRemain   = ulDataLen;
    BulkRequest.usFlags        = fShortOk && ulDataLen == ulNumBytes ? 0 : USB_TRANSFER_FULL_SIZE;

    rc = DosDevIOCtl( g_hUSBDrv,
                      IOCAT_USBRES, IOCTLF_SENDBULKURB,
                      &BulkRequest, ulParmLen, &ulParmLen,
                      pvData, ulDataLen, &ulDataLen );
    Log(("BulkWrite: usStatus=%d rc=%ld usDataProcessed=%d usDataRemain=%d ulDataLen=%ld\n",
         BulkRequest.usStatus, rc, BulkRequest.usDataProcessed, BulkRequest.usDataRemain, ulDataLen));
    if (rc != NO_ERROR)
    {
      if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_INVALID_PARAMETER) )
        rc= ERROR_INVALID_PARAMETER;
      if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_GEN_FAILURE) )
        rc= BulkRequest.usStatus;
      break;
    }
    /* Adjust count and source pointer */
    ulNumBytes -= ulDataLen;
    pvData      = (PBYTE)pvData + ulDataLen;
  } while( ulNumBytes > 0 );

  return rc;
}

APIRET APIENTRY
UsbIrqStart( USBHANDLE Handle,
             UCHAR  Endpoint,
             UCHAR  AltInterface,
             USHORT ulNumBytes,
             PVOID  pData,
             PHEV   pHevModified)
{
  APIRET rc;
  ULONG ulParmLen, ulDataLen;
  USBCALLS_IRQ_START IrqStart;
  HEV hEvent;

  if(!g_cInit)
    return USB_NOT_INIT;

  if(0==ulNumBytes || IsBadWritePointer(pData, ulNumBytes))
    return ERROR_INVALID_PARAMETER;

  rc = DosCreateEventSem( NULL,
                          &hEvent,
                          DC_SEM_SHARED,
                          FALSE);
  if(rc)
    return rc;

  IrqStart.ulDevHandle    = Handle;
  IrqStart.ucEndpoint     = Endpoint;
  IrqStart.ucAltInterface = AltInterface;
  IrqStart.usStatus       = 0;
  IrqStart.ulEvent        = hEvent;
  ulParmLen = sizeof(IrqStart);
  ulDataLen = ulNumBytes;

  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_START_IRQ_PROC,
                    (PVOID)&IrqStart, ulParmLen, &ulParmLen,
                    pData, ulDataLen,&ulDataLen);
  if(rc)
    DosCloseEventSem(hEvent);
  else
    *pHevModified = hEvent;
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbIrqStop( USBHANDLE Handle,
            HEV       HevModified)
{
  APIRET rc;
  ULONG ulParmLen, ulDataLen;

  if(!g_cInit)
    return USB_NOT_INIT;

  ulParmLen = sizeof(Handle);
  ulDataLen = sizeof(HevModified);
  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_STOP_IRQ_PROC,
                    (PVOID)&Handle, ulParmLen, &ulParmLen,
                    &HevModified, ulDataLen, &ulDataLen);
  if(!rc)
    DosCloseEventSem(HevModified);

  return rc;
}

APIEXPORT APIRET APIENTRY
UsbIsoStart( USBHANDLE Handle,
             UCHAR  Endpoint,
             UCHAR  AltInterface,
             ISOHANDLE *phIso)
{
  APIRET rc;
  PISORINGBUFFER pIter = g_pIsoRingBuffers;
  USBCALLS_ISO_START IsoStart;
  ULONG ulParmLen, ulDataLen;
  int   i;

  if(!g_cInit)
    return USB_NOT_INIT;

  rc = DosRequestMutexSem(g_hSemRingBuffers,SEM_INDEFINITE_WAIT);
  if(rc)
    return rc;

  for(i=0;i< g_ulNumIsoRingBuffers;i++,pIter++)
  {
    if (pIter->hDevice==0)
    {
      pIter->hDevice = Handle;
      break;
    }
  }
  DosReleaseMutexSem(g_hSemRingBuffers);

  if(i==g_ulNumIsoRingBuffers)
    return USB_ERROR_OUTOF_RESOURCES;

  IsoStart.ulDevHandle    = Handle;
  IsoStart.ucEndpoint     = Endpoint;
  IsoStart.ucAltInterface = AltInterface;
  ulParmLen = sizeof(IsoStart);
  ulDataLen = sizeof(ISORINGBUFFER);

  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_STOP_IRQ_PROC,
                    (PVOID)&IsoStart, ulParmLen, &ulParmLen,
                    pIter, ulDataLen, &ulDataLen);
  if(rc)
  {
    pIter->hDevice = 0;
    *phIso = 0;
  }
  else
  {
    pIter->ucEndpoint     = Endpoint;
    pIter->ucAltInterface = AltInterface;
  }
  return rc;
}

static APIRET IsInvalidIsoHandle(const ISOHANDLE hIso)
{
  PISORINGBUFFER pIter;
  ULONG i;
  pIter = g_pIsoRingBuffers;

  for(i=0;i<g_ulNumIsoRingBuffers;i++,pIter++)
  {
    if(pIter==(PISORINGBUFFER)hIso && pIter->hDevice)
      return 0;
  }
  return ERROR_INVALID_PARAMETER;
}

APIEXPORT APIRET APIENTRY
UsbIsoStop( ISOHANDLE hIso)
{

  APIRET rc = NO_ERROR;
  if(!g_cInit)
    return USB_NOT_INIT;

/*  rc = DosDevIOCtl( g_hUSBDrv, */
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbIsoDequeue( ISOHANDLE hIso,
               PVOID pBuffer,
               ULONG ulNumBytes)
{
  APIRET rc;
  PISORINGBUFFER pRB = (PISORINGBUFFER)hIso;

  rc = IsInvalidIsoHandle(hIso);
  if(rc)
    return rc;
  if(!(pRB->ucEndpoint & ISO_DIRMASK))
    return ERROR_INVALID_PARAMETER;

  return rc;
}

APIEXPORT APIRET APIENTRY
UsbIsoPeekQueue( ISOHANDLE hIso,
                 UCHAR * pByte,
                 ULONG ulOffset)
{
  APIRET rc;
  PISORINGBUFFER pRB = (PISORINGBUFFER)hIso;

  rc = IsInvalidIsoHandle(hIso);
  if(rc)
    return rc;
  if(!(pRB->ucEndpoint & ISO_DIRMASK))
    return ERROR_INVALID_PARAMETER;
  return rc;
}

APIEXPORT APIRET APIENTRY
UsbIsoEnqueue( ISOHANDLE hIso,
               const UCHAR * pBuffer,
               ULONG ulNumBytes)
{
  APIRET rc;
  PISORINGBUFFER pRB = (PISORINGBUFFER)hIso;

  rc = IsInvalidIsoHandle(hIso);
  if(rc)
    return rc;
  if(pRB->ucEndpoint & ISO_DIRMASK)
    return ERROR_INVALID_PARAMETER;

  return rc;
}

APIEXPORT APIRET APIENTRY
UsbIsoGetLength( ISOHANDLE hIso,
                 ULONG *pulLength)
{
  APIRET rc;
  PISORINGBUFFER pRB = (PISORINGBUFFER) hIso;
  USHORT ri,wi;

  rc = IsInvalidIsoHandle(hIso);
  if(rc)
    return rc;
  wi = pRB->usPosWrite;
  ri = pRB->usPosRead;

  if (ri == wi)
    *pulLength = 0;
  else if (ri < wi)
    *pulLength =  wi - ri;
  else
    *pulLength = wi + (pRB->usBufSize - ri);

  return 0;
}

APIEXPORT APIRET APIENTRY
UsbIrqRead( USBHANDLE Handle,
            UCHAR  Endpoint,
            UCHAR  AltInterface,
            ULONG  *ulNumBytes,
            PVOID  pData,
            ULONG  ulTimeout)
{
  APIRET            rc;
  ULONG             ulParmLen, ulDataLen;
  LIBUSB_IRQ_REQ    IrqRequest;

  if(!g_cInit)
    return USB_NOT_INIT;

  /* 10 01 2003 - KIEWITZ -> Still @@ToDo Add Endpoint check based on descriptors
     We currently only allow Endpoint-addresses 80h->8Fh here */
  if ((Endpoint<0x80) || (Endpoint>0x8F))
     return USB_ERROR_INVALID_ENDPOINT;

  if(*ulNumBytes==0)
     return 0;

  IrqRequest.ulDevHandle    = Handle;
  IrqRequest.ucEndpoint     = Endpoint;
  IrqRequest.ucAltInterface = AltInterface;
  IrqRequest.usStatus       = 0;
  IrqRequest.ulEvent        = 0;
  IrqRequest.ulTimeout      = ulTimeout;
  ulParmLen                 = sizeof(LIBUSB_IRQ_REQ);
  ulDataLen                 = *ulNumBytes;

  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_SENDIRQURB,
                    (PVOID)&IrqRequest, ulParmLen, &ulParmLen,
                    pData, ulDataLen, &ulDataLen);

  if( rc == NO_ERROR )
  {
    *ulNumBytes = IrqRequest.usDataLen;
  } else
  {
     if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_INVALID_PARAMETER) )
       rc= ERROR_INVALID_PARAMETER;
     if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_GEN_FAILURE) )
       rc= IrqRequest.usStatus;
  }
  return rc;
}


APIEXPORT APIRET APIENTRY
UsbFixupDevice( USBHANDLE Handle,
                UCHAR ucConfiguration,
                UCHAR *pucConfigurationData,
                ULONG ulConfigurationLen )
{
  LIBUSB_FIXUP request;
  ULONG        ulParmLen;
  APIRET rc;

  request.ulDevHandle= Handle;
  request.ucConfiguration= ucConfiguration;
  request.usStatus= 0;
  ulParmLen= sizeof(LIBUSB_FIXUP);
  rc = DosDevIOCtl( g_hUSBDrv,
                    IOCAT_USBRES, IOCTLF_FIXUPDEVUCE,
                    (PVOID)&request, ulParmLen, &ulParmLen,
                    pucConfigurationData, ulConfigurationLen, &ulConfigurationLen);
  if( rc != NO_ERROR )
  {
    if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_INVALID_PARAMETER) )
      rc= ERROR_INVALID_PARAMETER;
    if( rc == (ERROR_USER_DEFINED_BASE|ERROR_I24_GEN_FAILURE) )
      rc= request.usStatus;
  }
  return rc;
}

#ifndef STATIC_USBCALLS
 /*+-------------------------------------------------------------------+*/
 /*| _CRT_init is the C run-time environment initialization function.  |*/
 /*|It will return 0 to indicate success and -1 to indicate failure.   |*/
 /*+-------------------------------------------------------------------+*/

/* int _CRT_init (void); */

 /*+-------------------------------------------------------------------+*/
 /*| _CRT_term is the C run-time environment termination function.     |*/
 /*+-------------------------------------------------------------------+*/

/* void _CRT_term (unsigned long);*/

 /*+-------------------------------------------------------------------+*/
 /*| _DLL_InitTerm is the function that gets called by the operating   |*/
 /*| system loader when it loads and frees this DLL for each process   |*/
 /*| that accesses this DLL.  However, it only gets called the first   |*/
 /*| time the DLL is loaded and the last time it is freed for a        |*/
 /*| particular process.  The system linkage convention must be used   |*/
 /*| because the operating system loader is calling this function.     |*/
 /*+-------------------------------------------------------------------+*/

#ifdef STATIC_LINK
int   _CRT_init (void);
void  _CRT_term(0UL);
#endif

unsigned long _System  _DLL_InitTerm (unsigned long modhandle, unsigned long flag)
{

  /* If flag is zero then the DLL is being loaded so initialization  */
  /* should be performed.  If flag is 1 then the DLL is being freed  */
  /* so termination should be performed.                             */

  switch (flag)
  {
    case 0:
    /* The C run-time environment initialization function must   */
    /* be called before any calls to C run-time functions that   */
    /* are not inlined.                                          */

#ifdef STATIC_LINK
      if (_CRT_init () == -1)
         return 0UL;
#endif
      InitUsbCalls();
      break;

    case 1:
      TermUsbCalls();
#ifdef STATIC_LINK
      _CRT_term(0UL);
#endif
      break;

    default:
      return 0UL;

  }

  /* A nonzero value must be returned to indicate success. */
  return 1UL;
}
#endif /* !STATIC_USBCALLS */

