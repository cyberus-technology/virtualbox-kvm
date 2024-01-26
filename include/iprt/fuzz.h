/** @file
 * IPRT - Fuzzing framework
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_fuzz_h
#define IPRT_INCLUDED_fuzz_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/process.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_fuzz RTFuzz - Data fuzzing framework
 * @ingroup grp_rt
 * @sa grp_rt_test
 * @{
 */

/** A fuzzer context handle. */
typedef struct RTFUZZCTXINT      *RTFUZZCTX;
/** Pointer to a fuzzer context handle. */
typedef RTFUZZCTX                *PRTFUZZCTX;
/** NIL fuzzer context handle. */
#define NIL_RTFUZZCTX            ((RTFUZZCTX)~(uintptr_t)0)
/** A fuzzer input handle. */
typedef struct RTFUZZINPUTINT    *RTFUZZINPUT;
/** Pointer to a fuzzer input handle. */
typedef RTFUZZINPUT              *PRTFUZZINPUT;
/** NIL fuzzer input handle. */
#define NIL_RTFUZZINPUT          ((RTFUZZINPUT)~(uintptr_t)0)


/** A fuzzer config handle. */
typedef struct RTFUZZCFGINT      *RTFUZZCFG;
/** Pointer to a fuzzer config handle. */
typedef RTFUZZCFG                *PRTFUZZCFG;
/** NIL fuzzer config handle. */
#define NIL_RTFUZZCFG            ((RTFUZZCFG)~(uintptr_t)0)


/** A fuzzer target recorder handler. */
typedef struct RTFUZZTGTRECINT   *RTFUZZTGTREC;
/** Pointer to a fuzzer target recorder handle. */
typedef RTFUZZTGTREC             *PRTFUZZTGTREC;
/** NIL fuzzer target recorder handle. */
#define NIL_RTFUZZTGTREC         ((RTFUZZTGTREC)~(uintptr_t)0)
/** A fuzzed target state handle. */
typedef struct RTFUZZTGTSTATEINT *RTFUZZTGTSTATE;
/** Pointer to a fuzzed target state handle. */
typedef RTFUZZTGTSTATE           *PRTFUZZTGTSTATE;
/** NIL fuzzed target state handle. */
#define NIL_RTFUZZTGTSTATE       ((RTFUZZTGTSTATE)~(uintptr_t)0)


/** Fuzzing observer handle. */
typedef struct RTFUZZOBSINT      *RTFUZZOBS;
/** Pointer to a fuzzing observer handle. */
typedef RTFUZZOBS                *PRTFUZZOBS;
/** NIL fuzzing observer handle. */
#define NIL_RTFUZZOBS             ((RTFUZZOBS)~(uintptr_t)0)


/**
 * Fuzzing context type.
 */
typedef enum RTFUZZCTXTYPE
{
    /** Invalid type. */
    RTFUZZCTXTYPE_INVALID = 0,
    /** Original input data is a single binary large object (BLOB), from a file or similar. */
    RTFUZZCTXTYPE_BLOB,
    /** Original input data is from a data stream like a network connection. */
    RTFUZZCTXTYPE_STREAM,
    /** 32bit hack. */
    RTFUZZCTXTYPE_32BIT_HACK = 0x7fffffff
} RTFUZZCTXTYPE;


/**
 * Fuzzing context statistics.
 */
typedef struct RTFUZZCTXSTATS
{
    /** Amount of memory currently allocated. */
    size_t                      cbMemory;
    /** Number of mutations accumulated in the corpus. */
    uint64_t                    cMutations;
} RTFUZZCTXSTATS;
/** Pointer to fuzzing context statistics. */
typedef RTFUZZCTXSTATS *PRTFUZZCTXSTATS;


/** @name RTFUZZCTX_F_XXX - Flags for RTFuzzCtxCfgSetBehavioralFlags
 * @{ */
/** Adds all generated inputs automatically to the input corpus for the owning context. */
#define RTFUZZCTX_F_BEHAVIORAL_ADD_INPUT_AUTOMATICALLY_TO_CORPUS    RT_BIT_32(0)
/** All valid behavioral modification flags. */
#define RTFUZZCTX_F_BEHAVIORAL_VALID                                (RTFUZZCTX_F_BEHAVIORAL_ADD_INPUT_AUTOMATICALLY_TO_CORPUS)
/** @} */


/** @name RTFUZZOBS_SANITIZER_F_XXX - Flags for RTFuzzObsSetTestBinarySanitizers().
 * @{ */
/** ASAN is compiled and enabled (observer needs to configure to abort on error to catch memory errors). */
#define RTFUZZOBS_SANITIZER_F_ASAN                 UINT32_C(0x00000001)
/** A converage sanitizer is compiled in which can be used to produce coverage reports aiding in the
 * fuzzing process. */
#define RTFUZZOBS_SANITIZER_F_SANCOV               UINT32_C(0x00000002)
/** @} */


/** @name RTFUZZTGT_REC_STATE_F_XXX - Flags for RTFuzzTgtRecorderCreate().
 * @{ */
/** The output from stdout is used to compare states. */
#define RTFUZZTGT_REC_STATE_F_STDOUT               RT_BIT_32(0)
/** The output from stderr is used to compare states. */
#define RTFUZZTGT_REC_STATE_F_STDERR               RT_BIT_32(1)
/** The process status is used to compare states. */
#define RTFUZZTGT_REC_STATE_F_PROCSTATUS           RT_BIT_32(2)
/** The coverage report is used to compare states. */
#define RTFUZZTGT_REC_STATE_F_SANCOV               RT_BIT_32(3)
/** Mask of all valid flags. */
#define RTFUZZTGT_REC_STATE_F_VALID                UINT32_C(0x0000000f)
/** @} */


/** @name RTFUZZCFG_IMPORT_F_XXX - Flags for RTFuzzCfgImport().
 * @{ */
/** Default flags. */
#define RTFUZZCFG_IMPORT_F_DEFAULT                 0
/** Adds only the inputs and doesn't set any glboal configuration flags of the fuzzing context. */
#define RTFUZZCFG_IMPORT_F_ONLY_INPUT              RT_BIT_32(0)
/** Mask of all valid flags. */
#define RTFUZZCFG_IMPORT_F_VALID                   UINT32_C(0x00000001)
/** @} */


/**
 * Fuzzing context state export callback.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            Handle of the fuzzing context.
 * @param   pvBuf               The data to write.
 * @param   cbWrite             Number of bytes to write.
 * @param   pvUser              Opaque user data passed in RTFuzzCtxStateExport().
 */
typedef DECLCALLBACKTYPE(int, FNRTFUZZCTXEXPORT,(RTFUZZCTX hFuzzCtx, const void *pvBuf, size_t cbWrite, void *pvUser));
/** Pointer to a fuzzing context state export callback. */
typedef FNRTFUZZCTXEXPORT *PFNRTFUZZCTXEXPORT;

/**
 * Fuzzing context state import callback.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            Handle of the fuzzing context.
 * @param   pvBuf               Where to store the read data.
 * @param   cbRead              Number of bytes to read.
 * @param   pcbRead             Where to store the amount of data written, optional.
 * @param   pvUser              Opaque user data passed in RTFuzzCtxCreateFromState().
 */
typedef DECLCALLBACKTYPE(int, FNRTFUZZCTXIMPORT,(RTFUZZCTX hFuzzCtx, void *pvBuf, size_t cbRead, size_t *pcbRead, void *pvUser));
/** Pointer to a fuzzing context state export callback. */
typedef FNRTFUZZCTXIMPORT *PFNRTFUZZCTXIMPORT;


/**
 * Creates a new fuzzing context.
 *
 * @returns IPRT status code.
 * @param   phFuzzCtx           Where to store the handle to the fuzzing context on success.
 * @param   enmType             Fuzzing context data type.
 */
RTDECL(int) RTFuzzCtxCreate(PRTFUZZCTX phFuzzCtx, RTFUZZCTXTYPE enmType);

/**
 * Creates a new fuzzing context from the given state.
 *
 * @returns IPRT status code.
 * @param   phFuzzCtx           Where to store the handle to the fuzzing context on success.
 * @param   pfnImport           State import callback.
 * @param   pvUser              Opaque user data to pass to the callback.
 */
RTDECL(int) RTFuzzCtxCreateFromState(PRTFUZZCTX phFuzzCtx, PFNRTFUZZCTXIMPORT pfnImport, void *pvUser);

/**
 * Creates a new fuzzing context loading the state from the given memory buffer.
 *
 * @returns IPRT status code.
 * @param   phFuzzCtx           Where to store the handle to the fuzzing context on success.
 * @param   pvState             Pointer to the memory containing the state.
 * @param   cbState             Size of the state buffer.
 */
RTDECL(int) RTFuzzCtxCreateFromStateMem(PRTFUZZCTX phFuzzCtx, const void *pvState, size_t cbState);

/**
 * Creates a new fuzzing context loading the state from the given file.
 *
 * @returns IPRT status code.
 * @param   phFuzzCtx           Where to store the handle to the fuzzing context on success.
 * @param   pszFilename         File to load the fuzzing context from.
 */
RTDECL(int) RTFuzzCtxCreateFromStateFile(PRTFUZZCTX phFuzzCtx, const char *pszFilename);

/**
 * Retains a reference to the given fuzzing context.
 *
 * @returns New reference count on success.
 * @param   hFuzzCtx            Handle of the fuzzing context.
 */
RTDECL(uint32_t) RTFuzzCtxRetain(RTFUZZCTX hFuzzCtx);

/**
 * Releases a reference from the given fuzzing context, destroying it when reaching 0.
 *
 * @returns New reference count on success, 0 if the fuzzing context got destroyed.
 * @param   hFuzzCtx            Handle of the fuzzing context.
 */
RTDECL(uint32_t) RTFuzzCtxRelease(RTFUZZCTX hFuzzCtx);

/**
 * Queries statistics about the given fuzzing context.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            Handle of the fuzzing context.
 * @param   pStats              Where to store the stats on success.
 */
RTDECL(int) RTFuzzCtxQueryStats(RTFUZZCTX hFuzzCtx, PRTFUZZCTXSTATS pStats);

/**
 * Exports the given fuzzing context state.
 *
 * @returns IPRT statuse code
 * @param   hFuzzCtx            The fuzzing context to export.
 * @param   pfnExport           Export callback.
 * @param   pvUser              Opaque user data to pass to the callback.
 */
RTDECL(int) RTFuzzCtxStateExport(RTFUZZCTX hFuzzCtx, PFNRTFUZZCTXEXPORT pfnExport, void *pvUser);

/**
 * Exports the given fuzzing context state to memory allocating the buffer.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context to export.
 * @param   ppvState            Where to store the pointer to the memory containing state on success.
 *                              Free with RTMemFree().
 * @param   pcbState            Where to store the size of the state in bytes.
 */
RTDECL(int) RTFuzzCtxStateExportToMem(RTFUZZCTX hFuzzCtx, void **ppvState, size_t *pcbState);

/**
 * Exports the given fuzzing context state to the given file.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context to export.
 * @param   pszFilename         The file to save the state to.
 */
RTDECL(int) RTFuzzCtxStateExportToFile(RTFUZZCTX hFuzzCtx, const char *pszFilename);

/**
 * Adds a new seed to the input corpus of the given fuzzing context.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   pvInput             The pointer to the input buffer.
 * @param   cbInput             Size of the input buffer.
 */
RTDECL(int) RTFuzzCtxCorpusInputAdd(RTFUZZCTX hFuzzCtx, const void *pvInput, size_t cbInput);

/**
 * Adds a new seed to the input corpus of the given fuzzing context - extended version.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   pvInput             The pointer to the input buffer.
 * @param   cbInput             Size of the input buffer.
 * @param   offMutStart         Start offset at which a mutation can happen.
 * @param   cbMutRange          Size of the range in bytes where a mutation can happen,
 *                              use UINT64_MAX to allow mutations till the end of the input.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddEx(RTFUZZCTX hFuzzCtx, const void *pvInput, size_t cbInput,
                                      uint64_t offMutStart, uint64_t cbMutRange);

/**
 * Adds a new seed to the input corpus of the given fuzzing context from the given file.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   pszFilename         The filename to load the seed from.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddFromFile(RTFUZZCTX hFuzzCtx, const char *pszFilename);

/**
 * Adds a new seed to the input corpus of the given fuzzing context from the given file - extended version.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   pszFilename         The filename to load the seed from.
 * @param   offMutStart         Start offset at which a mutation can happen.
 * @param   cbMutRange          Size of the range in bytes where a mutation can happen,
 *                              use UINT64_MAX to allow mutations till the end of the input.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddFromFileEx(RTFUZZCTX hFuzzCtx, const char *pszFilename,
                                              uint64_t offMutStart, uint64_t cbMutRange);

/**
 * Adds a new seed to the input corpus of the given fuzzing context from the given VFS file.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   hVfsFile            The VFS file handle to load the seed from.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddFromVfsFile(RTFUZZCTX hFuzzCtx, RTVFSFILE hVfsFile);

/**
 * Adds a new seed to the input corpus of the given fuzzing context from the given VFS file - extended version.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   hVfsFile            The VFS file handle to load the seed from.
 * @param   offMutStart         Start offset at which a mutation can happen.
 * @param   cbMutRange          Size of the range in bytes where a mutation can happen,
 *                              use UINT64_MAX to allow mutations till the end of the input.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddFromVfsFileEx(RTFUZZCTX hFuzzCtx, RTVFSFILE hVfsFile,
                                                 uint64_t offMutStart, uint64_t cbMutRange);

/**
 * Adds a new seed to the input corpus of the given fuzzing context from the given VFS I/O stream.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   hVfsIos             The VFS I/O stream handle to load the seed from.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddFromVfsIoStrm(RTFUZZCTX hFuzzCtx, RTVFSIOSTREAM hVfsIos);

/**
 * Adds a new seed to the input corpus of the given fuzzing context from the given VFS I/O stream - extended version.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   hVfsIos             The VFS I/O stream handle to load the seed from.
 * @param   offMutStart         Start offset at which a mutation can happen.
 * @param   cbMutRange          Size of the range in bytes where a mutation can happen,
 *                              use UINT64_MAX to allow mutations till the end of the input.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddFromVfsIoStrmEx(RTFUZZCTX hFuzzCtx, RTVFSIOSTREAM hVfsIos,
                                                   uint64_t offMutStart, uint64_t cbMutRange);

/**
 * Adds new seeds to the input corpus of the given fuzzing context from the given directory.
 *
 * Will only process regular files, i.e. ignores directories, symbolic links, devices, fifos
 * and such.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   pszDirPath          The directory to load seeds from.
 */
RTDECL(int) RTFuzzCtxCorpusInputAddFromDirPath(RTFUZZCTX hFuzzCtx, const char *pszDirPath);

/**
 * Restricts the maximum input size to generate by the fuzzing context.
 *
 * @returns IPRT status code
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   cbMax               Maximum input size in bytes.
 */
RTDECL(int) RTFuzzCtxCfgSetInputSeedMaximum(RTFUZZCTX hFuzzCtx, size_t cbMax);

/**
 * Returns the maximum input size of the given fuzzing context.
 *
 * @returns Maximum input size generated in bytes.
 * @param   hFuzzCtx            The fuzzing context handle.
 */
RTDECL(size_t) RTFuzzCtxCfgGetInputSeedMaximum(RTFUZZCTX hFuzzCtx);

/**
 * Sets flags controlling the behavior of the fuzzing context.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   fFlags              Flags controlling the fuzzing context, RTFUZZCTX_F_XXX.
 */
RTDECL(int) RTFuzzCtxCfgSetBehavioralFlags(RTFUZZCTX hFuzzCtx, uint32_t fFlags);

/**
 * Returns the current set behavioral flags for the given fuzzing context.
 *
 * @returns Behavioral flags of the given fuzzing context.
 * @param   hFuzzCtx            The fuzzing context handle.
 */
RTDECL(uint32_t) RTFuzzCfgGetBehavioralFlags(RTFUZZCTX hFuzzCtx);

/**
 * Sets the temporary directory used by the fuzzing context.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   pszPathTmp          The directory for the temporary state.
 */
RTDECL(int) RTFuzzCtxCfgSetTmpDirectory(RTFUZZCTX hFuzzCtx, const char *pszPathTmp);

/**
 * Returns the current temporary directory.
 *
 * @returns Current temporary directory.
 * @param   hFuzzCtx            The fuzzing context handle.
 */
RTDECL(const char *) RTFuzzCtxCfgGetTmpDirectory(RTFUZZCTX hFuzzCtx);

/**
 * Sets the range in which a particular input can get mutated.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   offStart            Start offset at which a mutation can happen.
 * @param   cbRange             Size of the range in bytes where a mutation can happen,
 *                              use UINT64_MAX to allow mutations till the end of the input.
 */
RTDECL(int) RTFuzzCtxCfgSetMutationRange(RTFUZZCTX hFuzzCtx, uint64_t offStart, uint64_t cbRange);

/**
 * Reseeds the PRNG of the given fuzzing context.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   uSeed               The new seed.
 */
RTDECL(int) RTFuzzCtxReseed(RTFUZZCTX hFuzzCtx, uint64_t uSeed);

/**
 * Generates a new input from the given fuzzing context and returns it.
 *
 * @returns IPRT status code.
 * @param   hFuzzCtx            The fuzzing context handle.
 * @param   phFuzzInput         Where to store the handle to the fuzzed input on success.
 */
RTDECL(int) RTFuzzCtxInputGenerate(RTFUZZCTX hFuzzCtx, PRTFUZZINPUT phFuzzInput);


/**
 * Retains a reference to the given fuzzing input handle.
 *
 * @returns New reference count on success.
 * @param   hFuzzInput          The fuzzing input handle.
 */
RTDECL(uint32_t) RTFuzzInputRetain(RTFUZZINPUT hFuzzInput);

/**
 * Releases a reference from the given fuzzing input handle, destroying it when reaching 0.
 *
 * @returns New reference count on success, 0 if the fuzzing input got destroyed.
 * @param   hFuzzInput          The fuzzing input handle.
 */
RTDECL(uint32_t) RTFuzzInputRelease(RTFUZZINPUT hFuzzInput);

/**
 * Queries the data pointer and size of the given fuzzed input blob.
 *
 * @returns IPRT status code
 * @param   hFuzzInput          The fuzzing input handle.
 * @param   ppv                 Where to store the pointer to the input data on success.
 * @param   pcb                 Where to store the size of the input data on success.
 */
RTDECL(int) RTFuzzInputQueryBlobData(RTFUZZINPUT hFuzzInput, void **ppv, size_t *pcb);

/**
 * Processes the given data stream for a streamed fuzzing context.
 *
 * @returns IPRT status code.
 * @param   hFuzzInput          The fuzzing input handle.
 * @param   pvBuf               The data buffer.
 * @param   cbBuf               Size of the buffer.
 */
RTDECL(int) RTFuzzInputMutateStreamData(RTFUZZINPUT hFuzzInput, void *pvBuf, size_t cbBuf);

/**
 * Queries the string of the MD5 digest for the given fuzzed input.
 *
 * @returns IPRT status code.
 * @retval  VERR_BUFFER_OVERFLOW if the size of the string buffer is not sufficient.
 * @param   hFuzzInput          The fuzzing input handle.
 * @param   pszDigest           Where to store the digest string and a closing terminator.
 * @param   cchDigest           Size of the string buffer in characters (including the zero terminator).
 */
RTDECL(int) RTFuzzInputQueryDigestString(RTFUZZINPUT hFuzzInput, char *pszDigest, size_t cchDigest);

/**
 * Writes the given fuzzing input to the given file.
 *
 * @returns IPRT status code.
 * @param   hFuzzInput          The fuzzing input handle.
 * @param   pszFilename         The filename to store the input to.
 */
RTDECL(int) RTFuzzInputWriteToFile(RTFUZZINPUT hFuzzInput, const char *pszFilename);

/**
 * Adds the given fuzzed input to the input corpus of the owning context.
 *
 * @returns IPRT status code.
 * @retval  VERR_ALREADY_EXISTS if the input exists already.
 * @param   hFuzzInput          The fuzzing input handle.
 */
RTDECL(int) RTFuzzInputAddToCtxCorpus(RTFUZZINPUT hFuzzInput);

/**
 * Removes the given fuzzed input from the input corpus of the owning context.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if the input is not part of the corpus.
 * @param   hFuzzInput          The fuzzing input handle.
 */
RTDECL(int) RTFuzzInputRemoveFromCtxCorpus(RTFUZZINPUT hFuzzInput);


/**
 * Creates a fuzzing config from the given VFS file handle.
 *
 * @returns IPRT status code.
 * @param   phFuzzCfg           Where to store the handle to the fuzzing config on success.
 * @param   hVfsFile            The VFS file to use (retained).
 * @param   pErrInfo            Where to store extended error info. Optional.
 */
RTDECL(int) RTFuzzCfgCreateFromVfsFile(PRTFUZZCFG phFuzzCfg, RTVFSFILE hVfsFile, PRTERRINFO pErrInfo);

/**
 * Creates a fuzzing config from the given file path.
 *
 * @returns IPRT status code.
 * @param   phFuzzCfg           Where to store the handle to the fuzzing config on success.
 * @param   pszFilename         Filename to load the config from.
 * @param   pErrInfo            Where to store extended error info. Optional.
 */
RTDECL(int) RTFuzzCfgCreateFromFile(PRTFUZZCFG phFuzzCfg, const char *pszFilename, PRTERRINFO pErrInfo);

/**
 * Retains a reference to the given fuzzing config.
 *
 * @returns New reference count on success.
 * @param   hFuzzCfg            Handle of the fuzzing config.
 */
RTDECL(uint32_t) RTFuzzCfgRetain(RTFUZZCFG hFuzzCfg);

/**
 * Releases a reference from the given fuzzing config, destroying it when reaching 0.
 *
 * @returns New reference count on success, 0 if the fuzzing config got destroyed.
 * @param   hFuzzCfg            Handle of the fuzzing config.
 */
RTDECL(uint32_t) RTFuzzCfgRelease(RTFUZZCFG hFuzzCfg);

/**
 * Imports the given fuzzing config into a previously created fuzzing context.
 *
 * @returns IPRT status code.
 * @param   hFuzzCfg            Handle of the fuzzing config.
 * @param   hFuzzCtx            Handle of the fuzzing context.
 * @param   fFlags              Flags controlling what to import exactly, combination of RTFUZZCFG_IMPORT_F_XXX.
 */
RTDECL(int) RTFuzzCfgImport(RTFUZZCFG hFuzzCfg, RTFUZZCTX hFuzzCtx, uint32_t fFlags);

/**
 * Queries the custom config for the controller of the fuzzing process.
 *
 * @returns IPRT status code.
 * @param   hFuzzCfg            Handle of the fuzzing config.
 * @param   phVfsFile           Where to store the handle of the VFS file containing the custom config.
 */
RTDECL(int) RTFuzzCfgQueryCustomCfg(RTFUZZCFG hFuzzCfg, PRTVFSFILE phVfsFile);


/**
 * Creates a new fuzzed target recorder.
 *
 * @returns IPRT status code.
 * @param   phFuzzTgtRec        Where to store the handle to the fuzzed target recorder on success.
 * @param   fRecFlags           What to take into account when checking for equal states.
 *                              Combination of RTFUZZTGT_REC_STATE_F_*
 */
RTDECL(int) RTFuzzTgtRecorderCreate(PRTFUZZTGTREC phFuzzTgtRec, uint32_t fRecFlags);

/**
 * Retains a reference to the given fuzzed target recorder handle.
 *
 * @returns New reference count on success.
 * @param   hFuzzTgtRec         The fuzzed target recorder handle.
 */
RTDECL(uint32_t) RTFuzzTgtRecorderRetain(RTFUZZTGTREC hFuzzTgtRec);

/**
 * Releases a reference from the given fuzzed target recorder handle, destroying it when reaching 0.
 *
 * @returns New reference count on success, 0 if the fuzzed target recorder got destroyed.
 * @param   hFuzzTgtRec         The fuzzed target recorder handle.
 */
RTDECL(uint32_t) RTFuzzTgtRecorderRelease(RTFUZZTGTREC hFuzzTgtRec);

/**
 * Creates a new empty fuzzed target state.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtRec         The fuzzed target recorder handle.
 * @param   phFuzzTgtState      Where to store the handle to the fuzzed target state on success.
 */
RTDECL(int) RTFuzzTgtRecorderCreateNewState(RTFUZZTGTREC hFuzzTgtRec, PRTFUZZTGTSTATE phFuzzTgtState);

/**
 * Retains a reference to the given fuzzed target state handle.
 *
 * @returns New reference count on success.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 */
RTDECL(uint32_t) RTFuzzTgtStateRetain(RTFUZZTGTSTATE hFuzzTgtState);

/**
 * Releases a reference from the given fuzzed target state handle, destroying it when reaching 0.
 *
 * @returns New reference count on success, 0 if the fuzzed target recorder got destroyed.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 */
RTDECL(uint32_t) RTFuzzTgtStateRelease(RTFUZZTGTSTATE hFuzzTgtState);

/**
 * Resets the given fuzzed target state to an empty state (keeping allocated memory).
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 *
 * @note Useful when the state is not added to the recorded set to avoid allocating memory.
 */
RTDECL(int) RTFuzzTgtStateReset(RTFUZZTGTSTATE hFuzzTgtState);

/**
 * Finalizes the given fuzzed target state, making it readonly.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 */
RTDECL(int) RTFuzzTgtStateFinalize(RTFUZZTGTSTATE hFuzzTgtState);

/**
 * Adds the given state to the set for the owning target recorder.
 *
 * @returns IPRT status code.
 * @retval  VERR_ALREADY_EXISTS if the state is already existing in the recorder set.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 *
 * @note This also finalizes the target state if not already done.
 */
RTDECL(int) RTFuzzTgtStateAddToRecorder(RTFUZZTGTSTATE hFuzzTgtState);

/**
 * Appends the given stdout output to the given target state.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 * @param   pvStdOut            Pointer to the stdout data buffer.
 * @param   cbStdOut            Size of the stdout data buffer in bytes.
 */
RTDECL(int) RTFuzzTgtStateAppendStdoutFromBuf(RTFUZZTGTSTATE hFuzzTgtState, const void *pvStdOut, size_t cbStdOut);

/**
 * Appends the given stderr output to the given target state.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 * @param   pvStdErr            Pointer to the stderr data buffer.
 * @param   cbStdErr            Size of the stderr data buffer in bytes.
 */
RTDECL(int) RTFuzzTgtStateAppendStderrFromBuf(RTFUZZTGTSTATE hFuzzTgtState, const void *pvStdErr, size_t cbStdErr);

/**
 * Appends the given stdout output to the given target state, reading from the given pipe.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 * @param   hPipe               The stdout pipe to read the data from.
 */
RTDECL(int) RTFuzzTgtStateAppendStdoutFromPipe(RTFUZZTGTSTATE hFuzzTgtState, RTPIPE hPipe);

/**
 * Appends the given stderr output to the given target state, reading from the given pipe.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 * @param   hPipe               The stdout pipe to read the data from.
 */
RTDECL(int) RTFuzzTgtStateAppendStderrFromPipe(RTFUZZTGTSTATE hFuzzTgtState, RTPIPE hPipe);

/**
 * Adds the SanCov coverage information from the given file to the given target state.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 * @param   pszFilename         Filename of the coverage report.
 */
RTDECL(int) RTFuzzTgtStateAddSanCovReportFromFile(RTFUZZTGTSTATE hFuzzTgtState, const char *pszFilename);

/**
 * Adds the given process status to the target state.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 * @param   pProcSts            The process status to add.
 */
RTDECL(int) RTFuzzTgtStateAddProcSts(RTFUZZTGTSTATE hFuzzTgtState, PCRTPROCSTATUS pProcSts);

/**
 * Dumps the given target state to the given directory.
 *
 * @returns IPRT status code.
 * @param   hFuzzTgtState       The fuzzed target state handle.
 * @param   pszDirPath          The directory to dump to.
 */
RTDECL(int) RTFuzzTgtStateDumpToDir(RTFUZZTGTSTATE hFuzzTgtState, const char *pszDirPath);


/**
 * Fuzzed binary input channel.
 */
typedef enum RTFUZZOBSINPUTCHAN
{
    /** Invalid. */
    RTFUZZOBSINPUTCHAN_INVALID = 0,
    /** File input. */
    RTFUZZOBSINPUTCHAN_FILE,
    /** Input over stdin. */
    RTFUZZOBSINPUTCHAN_STDIN,
    /** The binary is a fuzzing aware client using the
     * specified protocol over stdin/stdout. */
    RTFUZZOBSINPUTCHAN_FUZZING_AWARE_CLIENT,
    /** TCP server. */
    RTFUZZOBSINPUTCHAN_TCP_SERVER,
    /** TCP client. */
    RTFUZZOBSINPUTCHAN_TCP_CLIENT,
    /** UDP server. */
    RTFUZZOBSINPUTCHAN_UDP_SERVER,
    /** UDP client. */
    RTFUZZOBSINPUTCHAN_UDP_CLIENT,
    /** 32bit hack. */
    RTFUZZOBSINPUTCHAN_32BIT_HACK = 0x7fffffff
} RTFUZZOBSINPUTCHAN;

/**
 * Fuzzing observer statistics.
 */
typedef struct RTFUZZOBSSTATS
{
    /** Number of fuzzed inputs per second. */
    uint32_t                    cFuzzedInputsPerSec;
    /** Number of overall fuzzed inputs. */
    uint32_t                    cFuzzedInputs;
    /** Number of observed hangs. */
    uint32_t                    cFuzzedInputsHang;
    /** Number of observed crashes. */
    uint32_t                    cFuzzedInputsCrash;
} RTFUZZOBSSTATS;
/** Pointer to a fuzzing observer statistics record. */
typedef RTFUZZOBSSTATS *PRTFUZZOBSSTATS;

/**
 * Creates a new fuzzing observer.
 *
 * @returns IPRT status code.
 * @param   phFuzzObs           Where to store the fuzzing observer handle on success.
 * @param   enmType             Fuzzing context data type.
 * @param   fTgtRecFlags        Flags to pass to the target state recorder, see RTFuzzTgtRecorderCreate().
 */
RTDECL(int) RTFuzzObsCreate(PRTFUZZOBS phFuzzObs, RTFUZZCTXTYPE enmType, uint32_t fTgtRecFlags);

/**
 * Destroys a previously created fuzzing observer.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 */
RTDECL(int) RTFuzzObsDestroy(RTFUZZOBS hFuzzObs);

/**
 * Queries the internal fuzzing context of the given observer.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   phFuzzCtx           Where to store the handle to the fuzzing context on success.
 *
 * @note The fuzzing context handle should be released with RTFuzzCtxRelease() when not used anymore.
 */
RTDECL(int) RTFuzzObsQueryCtx(RTFUZZOBS hFuzzObs, PRTFUZZCTX phFuzzCtx);

/**
 * Queries the current statistics for the given fuzzing observer.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   pStats              Where to store the statistics to.
 */
RTDECL(int) RTFuzzObsQueryStats(RTFUZZOBS hFuzzObs, PRTFUZZOBSSTATS pStats);

/**
 * Sets the temp directory for the given fuzzing observer.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   pszTmp              The temp directory path.
 */
RTDECL(int) RTFuzzObsSetTmpDirectory(RTFUZZOBS hFuzzObs, const char *pszTmp);

/**
 * Sets the directory to store results to.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   pszResults          The path to store the results.
 */
RTDECL(int) RTFuzzObsSetResultDirectory(RTFUZZOBS hFuzzObs, const char *pszResults);

/**
 * Sets the binary to run for each fuzzed input.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   pszBinary           The binary path.
 * @param   enmInputChan        The input channel to use.
 */
RTDECL(int) RTFuzzObsSetTestBinary(RTFUZZOBS hFuzzObs, const char *pszBinary, RTFUZZOBSINPUTCHAN enmInputChan);

/**
 * Sets additional arguments to run the binary with.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   papszArgs           Pointer to the array of arguments.
 * @param   cArgs               Number of arguments.
 */
RTDECL(int) RTFuzzObsSetTestBinaryArgs(RTFUZZOBS hFuzzObs, const char * const *papszArgs, unsigned cArgs);

/**
 * Sets an environment block to run the binary in.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   hEnv                The environment block to set for the test binary.
 *                              Use RTENV_DEFAULT for the default process environment or
 *                              NULL for an empty environment.
 *
 * @note Upon successful return of this function the observer has taken ownership over the
 *       environment block and can alter it in unexpected ways. It also destroys the environment
 *       block when the observer gets destroyed. So don't touch the environment block after
 *       calling this function.
 */
RTDECL(int) RTFuzzObsSetTestBinaryEnv(RTFUZZOBS hFuzzObs, RTENV hEnv);

/**
 * Makes the observer aware of any configured sanitizers for the test binary.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   fSanitizers         Bitmask of compiled and enabled sanitiziers in the
 *                              target binary.
 */
RTDECL(int) RTFuzzObsSetTestBinarySanitizers(RTFUZZOBS hFuzzObs, uint32_t fSanitizers);

/**
 * Sets maximum timeout until a process is considered hung and killed.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   msTimeoutMax        The maximum number of milliseconds to wait until the process
 *                              is considered hung.
 */
RTDECL(int) RTFuzzObsSetTestBinaryTimeout(RTFUZZOBS hFuzzObs, RTMSINTERVAL msTimeoutMax);

/**
 * Starts fuzzing the set binary.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 * @param   cProcs              Number of processes to run simulteanously,
 *                              0 will create as many processes as there are CPUs available.
 */
RTDECL(int) RTFuzzObsExecStart(RTFUZZOBS hFuzzObs, uint32_t cProcs);

/**
 * Stops the fuzzing process.
 *
 * @returns IPRT status code.
 * @param   hFuzzObs            The fuzzing observer handle.
 */
RTDECL(int) RTFuzzObsExecStop(RTFUZZOBS hFuzzObs);


/**
 * A fuzzing master program.
 *
 * @returns Program exit code.
 *
 * @param   cArgs               The number of arguments.
 * @param   papszArgs           The argument vector.  (Note that this may be
 *                              reordered, so the memory must be writable.)
 */
RTR3DECL(RTEXITCODE) RTFuzzCmdMaster(unsigned cArgs, char **papszArgs);


/**
 * Client input consumption callback.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS the fuzzed code accepted the input.
 * @retval  VERR_* the client rejected the input while parsing it.
 * @param   pvBuf               The buffer containing the input data.
 * @param   cbBuf               Size of the buffer in bytes.
 * @param   pvUser              Opaque user data.
 */
typedef DECLCALLBACKTYPE(int, FNFUZZCLIENTCONSUME,(const void *pvBuf, size_t cbBuf, void *pvUser));
/** Pointer to a client consumption callback. */
typedef FNFUZZCLIENTCONSUME *PFNFUZZCLIENTCONSUME;

/**
 * A fuzzing client program for more efficient fuzzing.
 *
 * @returns Program exit code.
 *
 * @param   cArgs               The number of arguments.
 * @param   papszArgs           The argument vector.  (Note that this may be
 *                              reordered, so the memory must be writable.)
 * @param   pfnConsume          Input data consumption callback.
 * @param   pvUser              Opaque user data to pass to the callback.
 */
RTR3DECL(RTEXITCODE) RTFuzzCmdFuzzingClient(unsigned cArgs, char **papszArgs, PFNFUZZCLIENTCONSUME pfnConsume, void *pvUser);
/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_fuzz_h */

