/** @file
 *
 * Test utility to fill a given image with random data up to a certain size (sequentially).
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <VBox/vd.h>
#include <iprt/errcore.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/dir.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/initterm.h>
#include <iprt/getopt.h>
#include <iprt/rand.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The error count. */
unsigned g_cErrors = 0;
/** Global RNG state. */
RTRAND   g_hRand;

#define TSTVDFILL_TEST_PATTERN_SIZE _1M

static DECLCALLBACK(void) tstVDError(void *pvUser, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va)
{
    RT_NOREF1(pvUser);
    g_cErrors++;
    RTPrintf("tstVDFill: Error %Rrc at %s:%u (%s): ", rc, RT_SRC_POS_ARGS);
    RTPrintfV(pszFormat, va);
    RTPrintf("\n");
}

static DECLCALLBACK(int) tstVDMessage(void *pvUser, const char *pszFormat, va_list va)
{
    RT_NOREF1(pvUser);
    RTPrintf("tstVDFill: ");
    RTPrintfV(pszFormat, va);
    return VINF_SUCCESS;
}

static int tstFill(const char *pszFilename, const char *pszFormat, bool fStreamOptimized, uint64_t cbDisk, uint64_t cbFill)
{
    int rc;
    PVDISK pVD = NULL;
    VDGEOMETRY       PCHS = { 0, 0, 0 };
    VDGEOMETRY       LCHS = { 0, 0, 0 };
    PVDINTERFACE     pVDIfs = NULL;
    VDINTERFACEERROR VDIfError;

    /** Buffer storing the random test pattern. */
    uint8_t *pbTestPattern = NULL;

    /* Create the virtual disk test data */
    pbTestPattern = (uint8_t *)RTMemAlloc(TSTVDFILL_TEST_PATTERN_SIZE);

    RTRandAdvBytes(g_hRand, pbTestPattern, TSTVDFILL_TEST_PATTERN_SIZE);

    RTPrintf("Disk size is %llu bytes\n", cbDisk);

    /* Create error interface. */
    /* Create error interface. */
    VDIfError.pfnError = tstVDError;
    VDIfError.pfnMessage = tstVDMessage;

    rc = VDInterfaceAdd(&VDIfError.Core, "tstVD_Error", VDINTERFACETYPE_ERROR,
                        NULL, sizeof(VDINTERFACEERROR), &pVDIfs);
    AssertRC(rc);

#define CHECK(str) \
    do \
    { \
        RTPrintf("%s rc=%Rrc\n", str, rc); \
        if (RT_FAILURE(rc)) \
        { \
            if (pbTestPattern) \
                RTMemFree(pbTestPattern); \
            VDDestroy(pVD); \
            g_cErrors++; \
            return rc; \
        } \
    } while (0)

    rc = VDCreate(pVDIfs, VDTYPE_HDD, &pVD);
    CHECK("VDCreate()");

    rc = VDCreateBase(pVD, pszFormat, pszFilename, cbDisk,
                      fStreamOptimized ? VD_VMDK_IMAGE_FLAGS_STREAM_OPTIMIZED : VD_IMAGE_FLAGS_NONE,
                      "Test image", &PCHS, &LCHS, NULL, VD_OPEN_FLAGS_NORMAL,
                      NULL, NULL);
    CHECK("VDCreateBase()");

    uint64_t uOff = 0;
    uint64_t cbGb = 0;
    while (   uOff < cbFill
           && RT_SUCCESS(rc))
    {
        size_t cbThisWrite = RT_MIN(TSTVDFILL_TEST_PATTERN_SIZE, cbFill - uOff);
        rc = VDWrite(pVD, uOff, pbTestPattern, cbThisWrite);
        if (RT_SUCCESS(rc))
        {
            uOff += cbThisWrite;
            cbGb += cbThisWrite;
            /* Print a message for every GB we wrote. */
            if (cbGb >= _1G)
            {
                RTStrmPrintf(g_pStdErr, "Wrote %llu bytes\n", uOff);
                cbGb = 0;
            }
        }
    }

    VDDestroy(pVD);
    if (pbTestPattern)
        RTMemFree(pbTestPattern);

#undef CHECK
    return rc;
}

/**
 * Shows help message.
 */
static void printUsage(void)
{
    RTPrintf("Usage:\n"
             "--disk-size <size in MB>    Size of the disk\n"
             "--fill-size <size in MB>    How much to fill\n"
             "--filename <filename>       Filename of the image\n"
             "--format <VDI|VMDK|...>     Format to use\n"
             "--streamoptimized           Use the stream optimized format\n"
             "--help                      Show this text\n");
}

static const RTGETOPTDEF g_aOptions[] =
{
    { "--disk-size",       's', RTGETOPT_REQ_UINT64 },
    { "--fill-size",       'f', RTGETOPT_REQ_UINT64 },
    { "--filename",        'p', RTGETOPT_REQ_STRING },
    { "--format",          't', RTGETOPT_REQ_STRING },
    { "--streamoptimized", 'r', RTGETOPT_REQ_NOTHING },
    { "--help",            'h', RTGETOPT_REQ_NOTHING }
};

int main(int argc, char *argv[])
{
    RTR3InitExe(argc, &argv, 0);
    int rc;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    char c;
    uint64_t cbDisk = 0;
    uint64_t cbFill = 0;
    const char *pszFilename = NULL;
    const char *pszFormat = NULL;
    bool fStreamOptimized = false;

    rc = VDInit();
    if (RT_FAILURE(rc))
        return RTEXITCODE_FAILURE;

    RTGetOptInit(&GetState, argc, argv, g_aOptions,
                 RT_ELEMENTS(g_aOptions), 1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);

    while (   RT_SUCCESS(rc)
           && (c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 's':
                cbDisk = ValueUnion.u64 * _1M;
                break;
            case 'f':
                cbFill = ValueUnion.u64 * _1M;
                break;
            case 'p':
                pszFilename = ValueUnion.psz;
                break;
            case 't':
                pszFormat = ValueUnion.psz;
                break;
            case 'r':
                fStreamOptimized = true;
                break;
            case 'h':
            default:
                printUsage();
                break;
        }
    }

    if (!cbDisk || !cbFill || !pszFilename || !pszFormat)
    {
        RTPrintf("tstVDFill: Arguments missing!\n");
        return 1;
    }

    rc = RTRandAdvCreateParkMiller(&g_hRand);
    if (RT_FAILURE(rc))
    {
        RTPrintf("tstVDFill: Creating RNG failed rc=%Rrc\n", rc);
        return 1;
    }

    RTRandAdvSeed(g_hRand, 0x12345678);

    rc = tstFill(pszFilename, pszFormat, fStreamOptimized, cbDisk, cbFill);
    if (RT_FAILURE(rc))
        RTPrintf("tstVDFill: Filling disk failed! rc=%Rrc\n", rc);

    rc = VDShutdown();
    if (RT_FAILURE(rc))
        RTPrintf("tstVDFill: unloading backends failed! rc=%Rrc\n", rc);

    return RTEXITCODE_SUCCESS;
}

