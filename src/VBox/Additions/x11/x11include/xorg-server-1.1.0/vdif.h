/* $XFree86: xc/programs/Xserver/hw/xfree86/ddc/vdif.h,v 1.4tsi Exp $ */

#ifndef _VDIF_H
#define _VDIF_H

#define VDIF_MONITOR_MONOCHROME 0
#define VDIF_MONITOR_COLOR 1
#define VDIF_VIDEO_TTL 0
#define VDIF_VIDEO_ANALOG 1
#define VDIF_VIDEO_ECL 2
#define VDIF_VIDEO_DECL 3
#define VDIF_VIDEO_OTHER 4
#define VDIF_SYNC_SEPARATE 0
#define VDIF_SYNC_C 1
#define VDIF_SYNC_CP 2
#define VDIF_SYNC_G 3
#define VDIF_SYNC_GP 4
#define VDIF_SYNC_OTHER 5
#define VDIF_SCAN_NONINTERLACED 0
#define VDIF_SCAN_INTERLACED 1
#define VDIF_SCAN_OTHER 2
#define VDIF_POLARITY_NEGATIVE 0
#define VDIF_POLARITY_POSITIVE 1

#include <X11/Xmd.h>

#undef  CARD32
#define CARD32 unsigned int	/* ... on all supported platforms */

typedef struct _VDIF { /* Monitor Description: */
    CARD8 VDIFId[4]; /* alway "VDIF" */
    CARD32 FileLength; /* lenght of the whole file */
    CARD32 Checksum; /* sum of all bytes in the file after*/
    /* this field */
    CARD16 VDIFVersion; /* structure version number */
    CARD16 VDIFRevision; /* structure revision number */
    CARD16 Date[3]; /* file date Year/Month/Day */
    CARD16 DateManufactured[3]; /* date Year/Month/Day */
    CARD32 FileRevision; /* file revision string */
    CARD32 Manufacturer; /* ASCII ID of the manufacturer */
    CARD32 ModelNumber; /* ASCII ID of the model */
    CARD32 MinVDIFIndex; /* ASCII ID of Minimum VDIF index */
    CARD32 Version; /* ASCII ID of the model version */
    CARD32 SerialNumber; /* ASCII ID of the serial number */
    CARD8 MonitorType; /* Monochrome or Color */
    CARD8 CRTSize; /* inches */
    CARD8 BorderRed; /* percent */
    CARD8 BorderGreen; /* percent */
    CARD8 BorderBlue; /* percent */
    CARD8 Reserved1; /* padding */
    CARD16 Reserved2; /* padding */
    CARD32 RedPhosphorDecay; /* microseconds */
    CARD32 GreenPhosphorDecay; /* microseconds */
    CARD32 BluePhosphorDecay; /* microseconds */
    CARD16 WhitePoint_x; /* WhitePoint in CIExyY (scale 1000) */
    CARD16 WhitePoint_y;
    CARD16 WhitePoint_Y;
    CARD16 RedChromaticity_x; /* Red chromaticity in x,y */
    CARD16 RedChromaticity_y;
    CARD16 GreenChromaticity_x; /* Green chromaticity in x,y */
    CARD16 GreenChromaticity_y;
    CARD16 BlueChromaticity_x; /* Blue chromaticity in x,y */
    CARD16 BlueChromaticity_y;
    CARD16 RedGamma; /* Gamme curve exponent (scale 1000) */
    CARD16 GreenGamma;
    CARD16 BlueGamma;
    CARD32 NumberOperationalLimits;
    CARD32 OffsetOperationalLimits;
    CARD32 NumberOptions; /* optinal sections (gamma table) */
    CARD32 OffsetOptions;
    CARD32 OffsetStringTable;
} xf86VdifRec, *xf86VdifPtr;

typedef enum { /* Tags for section identification */
    VDIF_OPERATIONAL_LIMITS_TAG = 1,
    VDIF_PREADJUSTED_TIMING_TAG,
    VDIF_GAMMA_TABLE_TAG
} VDIFScnTag;

typedef struct _VDIFScnHdr { /* Generic Section Header: */
    CARD32 ScnLength; /* lenght of section */
    CARD32 ScnTag; /* tag for section identification */
} VDIFScnHdrRec, *VDIFScnHdrPtr;

typedef struct _VDIFLimits { /* Operational Limits: */
    VDIFScnHdrRec Header; /* common section info */
    CARD16 MaxHorPixel; /* pixels */
    CARD16 MaxVerPixel; /* lines */
    CARD16 MaxHorActiveLength; /* millimeters */
    CARD16 MaxVerActiveHeight; /* millimeters */
    CARD8 VideoType; /* TTL / Analog / ECL / DECL */
    CARD8 SyncType; /* TTL / Analog / ECL / DECL */
    CARD8 SyncConfiguration; /* separate / composite / other */
    CARD8 Reserved1; /* padding */
    CARD16 Reserved2; /* padding */
    CARD16 TerminationResistance; /* */
    CARD16 WhiteLevel; /* millivolts */
    CARD16 BlackLevel; /* millivolts */
    CARD16 BlankLevel; /* millivolts */
    CARD16 SyncLevel; /* millivolts */
    CARD32 MaxPixelClock; /* kiloHertz */
    CARD32 MinHorFrequency; /* Hertz */
    CARD32 MaxHorFrequency; /* Hertz */
    CARD32 MinVerFrequency; /* milliHertz */
    CARD32 MaxVerFrequency; /* milliHertz */
    CARD16 MinHorRetrace; /* nanoseconds */
    CARD16 MinVerRetrace; /* microseconds */
    CARD32 NumberPreadjustedTimings;
    CARD32 OffsetNextLimits;
} xf86VdifLimitsRec, *xf86VdifLimitsPtr;

typedef struct _VDIFTiming { /* Preadjusted Timing: */
    VDIFScnHdrRec Header; /* common section info */
    CARD32 PreadjustedTimingName; /* SVGA/SVPMI mode number */
    CARD16 HorPixel; /* pixels */
    CARD16 VerPixel; /* lines */
    CARD16 HorAddrLength; /* millimeters */
    CARD16 VerAddrHeight; /* millimeters */
    CARD8 PixelWidthRatio; /* gives H:V */
    CARD8 PixelHeightRatio;
    CARD8 Reserved1; /* padding */
    CARD8 ScanType; /* noninterlaced / interlaced / other*/
    CARD8 HorSyncPolarity; /* negative / positive */
    CARD8 VerSyncPolarity; /* negative / positive */
    CARD16 CharacterWidth; /* pixels */
    CARD32 PixelClock; /* kiloHertz */
    CARD32 HorFrequency; /* Hertz */
    CARD32 VerFrequency; /* milliHertz */
    CARD32 HorTotalTime; /* nanoseconds */
    CARD32 VerTotalTime; /* microseconds */
    CARD16 HorAddrTime; /* nanoseconds */
    CARD16 HorBlankStart; /* nanoseconds */
    CARD16 HorBlankTime; /* nanoseconds */
    CARD16 HorSyncStart; /* nanoseconds */
    CARD16 HorSyncTime; /* nanoseconds */
    CARD16 VerAddrTime; /* microseconds */
    CARD16 VerBlankStart; /* microseconds */
    CARD16 VerBlankTime; /* microseconds */
    CARD16 VerSyncStart; /* microseconds */
    CARD16 VerSyncTime; /* microseconds */
} xf86VdifTimingRec, *xf86VdifTimingPtr; 

typedef struct _VDIFGamma { /* Gamma Table: */
    VDIFScnHdrRec Header; /* common section info */
    CARD16 GammaTableEntries; /* count of grays or RGB 3-tuples */
    CARD16 Unused1;
} xf86VdifGammaRec, *xf86VdifGammaPtr;

/* access macros */
#define VDIF_OPERATIONAL_LIMITS(vdif) \
((xf86VdifLimitsPtr)((char*)(vdif) + (vdif)->OffsetOperationalLimits))
#define VDIF_NEXT_OPERATIONAL_LIMITS(limits) limits = \
     ((xf86VdifLimitsPtr)((char*)(limits) + (limits)->OffsetNextLimits))
#define VDIF_PREADJUSTED_TIMING(limits) \
((xf86VdifTimingPtr)((char*)(limits) + (limits)->Header.ScnLength))
#define VDIF_NEXT_PREADJUSTED_TIMING(timing) timing = \
     ((xf86VdifTimingPtr)((char*)(timing) + (timing)->Header.ScnLength))
#define VDIF_OPTIONS(vdif) \
     ((VDIFScnHdrPtr)((char*)(vdif) + (vdif)->OffsetOptions))
#define VDIF_NEXT_OPTIONS(options) options = \
     ((xf86VdifGammaPtr)((char*)(options) + (options)->Header.ScnLength))
#define VDIF_STRING(vdif, string) \
     ((char*)((char*)vdif + vdif->OffsetStringTable + (string)))

typedef struct  _vdif {
    xf86VdifPtr vdif;
    xf86VdifLimitsPtr *limits;
    xf86VdifTimingPtr *timings;
    xf86VdifGammaPtr *gamma;
    char * strings;
} xf86vdif, *xf86vdifPtr;

#undef CARD32

#endif
