/** @file
 * IPRT - Compression.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_zip_h
#define IPRT_INCLUDED_zip_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_zip        RTZip - Compression
 * @ingroup grp_rt
 * @{
 */



/**
 * Callback function for consuming compressed data during compression.
 *
 * @returns iprt status code.
 * @param   pvUser      User argument.
 * @param   pvBuf       Compressed data.
 * @param   cbBuf       Size of the compressed data.
 */
typedef DECLCALLBACKTYPE(int, FNRTZIPOUT,(void *pvUser, const void *pvBuf, size_t cbBuf));
/** Pointer to FNRTZIPOUT() function. */
typedef FNRTZIPOUT *PFNRTZIPOUT;

/**
 * Callback function for supplying compressed data during decompression.
 *
 * @returns iprt status code.
 * @param   pvUser      User argument.
 * @param   pvBuf       Where to store the compressed data.
 * @param   cbBuf       Size of the buffer.
 * @param   pcbBuf      Number of bytes actually stored in the buffer.
 */
typedef DECLCALLBACKTYPE(int, FNRTZIPIN,(void *pvUser, void *pvBuf, size_t cbBuf, size_t *pcbBuf));
/** Pointer to FNRTZIPIN() function. */
typedef FNRTZIPIN *PFNRTZIPIN;

/**
 * Compression type.
 * (Be careful with these they are stored in files!)
 */
typedef enum RTZIPTYPE
{
    /** Invalid. */
    RTZIPTYPE_INVALID = 0,
    /** Choose best fitting one. */
    RTZIPTYPE_AUTO,
    /** Store the data. */
    RTZIPTYPE_STORE,
    /** Zlib compression the data. */
    RTZIPTYPE_ZLIB,
    /** BZlib compress. */
    RTZIPTYPE_BZLIB,
    /** libLZF compress. */
    RTZIPTYPE_LZF,
    /** Lempel-Ziv-Jeff-Bonwick compression. */
    RTZIPTYPE_LZJB,
    /** Lempel-Ziv-Oberhumer compression. */
    RTZIPTYPE_LZO,
    /* Zlib compression the data without zlib header. */
    RTZIPTYPE_ZLIB_NO_HEADER,
    /** End of valid the valid compression types.  */
    RTZIPTYPE_END
} RTZIPTYPE;

/**
 * Compression level.
 */
typedef enum RTZIPLEVEL
{
    /** Store, don't compress. */
    RTZIPLEVEL_STORE = 0,
    /** Fast compression. */
    RTZIPLEVEL_FAST,
    /** Default compression. */
    RTZIPLEVEL_DEFAULT,
    /** Maximal compression. */
    RTZIPLEVEL_MAX
} RTZIPLEVEL;


/**
 * Create a stream compressor instance.
 *
 * @returns iprt status code.
 * @param   ppZip       Where to store the instance handle.
 * @param   pvUser      User argument which will be passed on to pfnOut and pfnIn.
 * @param   pfnOut      Callback for consuming output of compression.
 * @param   enmType     Type of compressor to create.
 * @param   enmLevel    Compression level.
 */
RTDECL(int)     RTZipCompCreate(PRTZIPCOMP *ppZip, void *pvUser, PFNRTZIPOUT pfnOut, RTZIPTYPE enmType, RTZIPLEVEL enmLevel);

/**
 * Compresses a chunk of memory.
 *
 * @returns iprt status code.
 * @param   pZip        The compressor instance.
 * @param   pvBuf       Pointer to buffer containing the bits to compress.
 * @param   cbBuf       Number of bytes to compress.
 */
RTDECL(int)     RTZipCompress(PRTZIPCOMP pZip, const void *pvBuf, size_t cbBuf);

/**
 * Finishes the compression.
 * This will flush all data and terminate the compression data stream.
 *
 * @returns iprt status code.
 * @param   pZip        The stream compressor instance.
 */
RTDECL(int)     RTZipCompFinish(PRTZIPCOMP pZip);

/**
 * Destroys the stream compressor instance.
 *
 * @returns iprt status code.
 * @param   pZip        The compressor instance.
 */
RTDECL(int)     RTZipCompDestroy(PRTZIPCOMP pZip);


/**
 * Create a stream decompressor instance.
 *
 * @returns iprt status code.
 * @param   ppZip       Where to store the instance handle.
 * @param   pvUser      User argument which will be passed on to pfnOut and pfnIn.
 * @param   pfnIn       Callback for producing input for decompression.
 */
RTDECL(int)     RTZipDecompCreate(PRTZIPDECOMP *ppZip, void *pvUser, PFNRTZIPIN pfnIn);

/**
 * Decompresses a chunk of memory.
 *
 * @returns iprt status code.
 * @param   pZip        The stream decompressor instance.
 * @param   pvBuf       Where to store the decompressed data.
 * @param   cbBuf       Number of bytes to produce. If pcbWritten is set
 *                      any number of bytes up to cbBuf might be returned.
 * @param   pcbWritten  Number of bytes actually written to the buffer. If NULL
 *                      cbBuf number of bytes must be written.
 */
RTDECL(int)     RTZipDecompress(PRTZIPDECOMP pZip, void *pvBuf, size_t cbBuf, size_t *pcbWritten);

/**
 * Destroys the stream decompressor instance.
 *
 * @returns iprt status code.
 * @param   pZip        The decompressor instance.
 */
RTDECL(int)     RTZipDecompDestroy(PRTZIPDECOMP pZip);


/**
 * Compress a chunk of memory into a block.
 *
 * @returns IPRT status code.
 *
 * @param   enmType         The compression type.
 * @param   enmLevel        The compression level.
 * @param   fFlags          Flags reserved for future extensions, MBZ.
 * @param   pvSrc           Pointer to the input block.
 * @param   cbSrc           Size of the input block.
 * @param   pvDst           Pointer to the output buffer.
 * @param   cbDst           The size of the output buffer.
 * @param   pcbDstActual    Where to return the compressed size.
 */
RTDECL(int)     RTZipBlockCompress(RTZIPTYPE enmType, RTZIPLEVEL enmLevel, uint32_t fFlags,
                                   void const *pvSrc, size_t cbSrc,
                                   void *pvDst, size_t cbDst, size_t *pcbDstActual) RT_NO_THROW_PROTO;


/**
 * Decompress a block.
 *
 * @returns IPRT status code.
 *
 * @param   enmType         The compression type.
 * @param   fFlags          Flags reserved for future extensions, MBZ.
 * @param   pvSrc           Pointer to the input block.
 * @param   cbSrc           Size of the input block.
 * @param   pcbSrcActual    Where to return the compressed size.
 * @param   pvDst           Pointer to the output buffer.
 * @param   cbDst           The size of the output buffer.
 * @param   pcbDstActual    Where to return the decompressed size.
 */
RTDECL(int)     RTZipBlockDecompress(RTZIPTYPE enmType, uint32_t fFlags,
                                     void const *pvSrc, size_t cbSrc, size_t *pcbSrcActual,
                                     void *pvDst, size_t cbDst, size_t *pcbDstActual) RT_NO_THROW_PROTO;


/**
 * Opens a gzip decompression I/O stream.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsIosIn           The compressed input stream (must be readable).
 *                              The reference is not consumed, instead another
 *                              one is retained.
 * @param   fFlags              Flags, MBZ.
 * @param   phVfsIosGunzip      Where to return the handle to the gunzipped I/O
 *                              stream (read).
 */
RTDECL(int) RTZipGzipDecompressIoStream(RTVFSIOSTREAM hVfsIosIn, uint32_t fFlags, PRTVFSIOSTREAM phVfsIosGunzip);

/** @name RTZipGzipDecompressIoStream flags.
 * @{ */
/** Allow the smaller ZLIB header as well as the regular GZIP header. */
#define RTZIPGZIPDECOMP_F_ALLOW_ZLIB_HDR    RT_BIT(0)
/** @} */


/**
 * Opens a gzip decompression I/O stream.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsIosDst          The compressed output stream (must be writable).
 *                              The reference is not consumed, instead another
 *                              one is retained.
 * @param   fFlags              Flags, MBZ.
 * @param   uLevel              The gzip compression level, 1 thru 9.
 * @param   phVfsIosGzip        Where to return the gzip input I/O stream handle
 *                              (you write to this).
 */
RTDECL(int) RTZipGzipCompressIoStream(RTVFSIOSTREAM hVfsIosDst, uint32_t fFlags, uint8_t uLevel, PRTVFSIOSTREAM phVfsIosGzip);

/**
 * A mini GZIP program.
 *
 * @returns Program exit code.
 *
 * @param   cArgs               The number of arguments.
 * @param   papszArgs           The argument vector.  (Note that this may be
 *                              reordered, so the memory must be writable.)
 */
RTDECL(RTEXITCODE) RTZipGzipCmd(unsigned cArgs, char **papszArgs);

/**
 * Opens a TAR filesystem stream.
 *
 * This is used to extract, list or check a TAR archive.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsIosIn           The input stream.  The reference is not
 *                              consumed, instead another one is retained.
 * @param   fFlags              Flags, MBZ.
 * @param   phVfsFss            Where to return the handle to the TAR
 *                              filesystem stream.
 */
RTDECL(int) RTZipTarFsStreamFromIoStream(RTVFSIOSTREAM hVfsIosIn, uint32_t fFlags, PRTVFSFSSTREAM phVfsFss);

/** TAR format type. */
typedef enum RTZIPTARFORMAT
{
    /** Customary invalid zero value. */
    RTZIPTARFORMAT_INVALID = 0,
    /** Default format (GNU). */
    RTZIPTARFORMAT_DEFAULT,
    /** The GNU format. */
    RTZIPTARFORMAT_GNU,
    /** USTAR format from POSIX.1-1988. */
    RTZIPTARFORMAT_USTAR,
    /** PAX format from POSIX.1-2001. */
    RTZIPTARFORMAT_PAX,
    /** End of valid formats. */
    RTZIPTARFORMAT_END,
    /** Make sure the type is at least 32 bits wide. */
    RTZIPTARFORMAT_32BIT_HACK = 0x7fffffff
} RTZIPTARFORMAT;

/**
 * Opens a TAR filesystem stream for the purpose of create a new TAR archive.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsIosOut          The output stream, i.e. where the tar stuff is
 *                              written.  The reference is not consumed, instead
 *                              another one is retained.
 * @param   enmFormat           The desired output format.
 * @param   fFlags              RTZIPTAR_C_XXX, except RTZIPTAR_C_UPDATE.
 * @param   phVfsFss            Where to return the handle to the TAR
 *                              filesystem stream.
 */
RTDECL(int) RTZipTarFsStreamToIoStream(RTVFSIOSTREAM hVfsIosOut, RTZIPTARFORMAT enmFormat,
                                       uint32_t fFlags, PRTVFSFSSTREAM phVfsFss);

/** @name RTZIPTAR_C_XXX - TAR creation flags (RTZipTarFsStreamToIoStream).
 * @{ */
/** Check for sparse files.
 * @note Only supported when adding file objects.  The files will be read
 *       twice. */
#define RTZIPTAR_C_SPARSE           RT_BIT_32(0)
/** Set if opening for updating. */
#define RTZIPTAR_C_UPDATE           RT_BIT_32(1)
/** Valid bits. */
#define RTZIPTAR_C_VALID_MASK       UINT32_C(0x00000003)
/** @} */

/**
 * Opens a TAR filesystem stream for the purpose of create a new TAR archive or
 * updating an existing one.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsFile            The TAR file handle, i.e. where the tar stuff is
 *                              written and optionally read/update.  The
 *                              reference is not consumed, instead another one
 *                              is retained.
 * @param   enmFormat           The desired output format.
 * @param   fFlags              RTZIPTAR_C_XXX.
 * @param   phVfsFss            Where to return the handle to the TAR
 *                              filesystem stream.
 */
RTDECL(int) RTZipTarFsStreamForFile(RTVFSFILE hVfsFile, RTZIPTARFORMAT enmFormat, uint32_t fFlags, PRTVFSFSSTREAM phVfsFss);

/**
 * Set the owner to store the archive entries with.
 *
 * @returns IPRT status code.
 * @param   hVfsFss             The handle to a TAR creator.
 * @param   uid                 The UID value to set.  Passing NIL_RTUID makes
 *                              it use the value found in RTFSOBJINFO.
 * @param   pszOwner            The owner name to store.  Passing NULL makes it
 *                              use the value found in RTFSOBJINFO.
 */
RTDECL(int) RTZipTarFsStreamSetOwner(RTVFSFSSTREAM hVfsFss, RTUID uid, const char *pszOwner);

/**
 * Set the group to store the archive entries with.
 *
 * @returns IPRT status code.
 * @param   hVfsFss             The handle to a TAR creator.
 * @param   gid                 The GID value to set.  Passing NIL_RTUID makes
 *                              it use the value found in RTFSOBJINFO.
 * @param   pszGroup            The group name to store.  Passing NULL makes it
 *                              use the value found in RTFSOBJINFO.
 */
RTDECL(int) RTZipTarFsStreamSetGroup(RTVFSFSSTREAM hVfsFss, RTGID gid, const char *pszGroup);

/**
 * Set path prefix to store the archive entries with.
 *
 * @returns IPRT status code.
 * @param   hVfsFss             The handle to a TAR creator.
 * @param   pszPrefix           The path prefix to join the names with.  Pass
 *                              NULL for no prefix.
 */
RTDECL(int) RTZipTarFsStreamSetPrefix(RTVFSFSSTREAM hVfsFss, const char *pszPrefix);

/**
 * Set the AND and OR masks to apply to file (non-dir) modes in the archive.
 *
 * @returns IPRT status code.
 * @param   hVfsFss             The handle to a TAR creator.
 * @param   fAndMode            The bits to keep
 * @param   fOrMode             The bits to set.
 */
RTDECL(int) RTZipTarFsStreamSetFileMode(RTVFSFSSTREAM hVfsFss, RTFMODE fAndMode, RTFMODE fOrMode);

/**
 * Set the AND and OR masks to apply to directory modes in the archive.
 *
 * @returns IPRT status code.
 * @param   hVfsFss             The handle to a TAR creator.
 * @param   fAndMode            The bits to keep
 * @param   fOrMode             The bits to set.
 */
RTDECL(int) RTZipTarFsStreamSetDirMode(RTVFSFSSTREAM hVfsFss, RTFMODE fAndMode, RTFMODE fOrMode);

/**
 * Set the modification time to store the archive entires with.
 *
 * @returns IPRT status code.
 * @param   hVfsFss             The handle to a TAR creator.
 * @param   pModificationTime   The modification time to use.  Pass NULL to use
 *                              the value found in RTFSOBJINFO.
 */
RTDECL(int) RTZipTarFsStreamSetMTime(RTVFSFSSTREAM hVfsFss, PCRTTIMESPEC pModificationTime);

/**
 * Truncates a TAR creator stream in update mode.
 *
 * Use RTVfsFsStrmNext to examine the TAR stream and locate the cut-off point.
 *
 * After performing this call, the stream will be in write mode and
 * RTVfsFsStrmNext will stop working (VERR_WRONG_ORDER).   The RTVfsFsStrmAdd()
 * and RTVfsFsStrmPushFile() can be used to add new object to the TAR file,
 * starting at the trunction point.  RTVfsFsStrmEnd() is used to finish the TAR
 * file (this performs the actual file trunction).
 *
 * @returns IPRT status code.
 * @param   hVfsFss             The handle to a TAR creator in update mode.
 * @param   hVfsObj             Object returned by RTVfsFsStrmNext that the
 *                              trunction is relative to.  This doesn't have to
 *                              be the current stream object, it can be an
 *                              earlier one too.
 * @param   fAfter              If set, @a hVfsObj will remain in the update TAR
 *                              file.  If clear, @a hVfsObj will not be
 *                              included.
 */
RTDECL(int) RTZipTarFsStreamTruncate(RTVFSFSSTREAM hVfsFss, RTVFSOBJ hVfsObj, bool fAfter);

/**
 * A mini TAR program.
 *
 * @returns Program exit code.
 *
 * @param   cArgs               The number of arguments.
 * @param   papszArgs           The argument vector.  (Note that this may be
 *                              reordered, so the memory must be writable.)
 */
RTDECL(RTEXITCODE) RTZipTarCmd(unsigned cArgs, char **papszArgs);

/**
 * Opens a ZIP filesystem stream.
 *
 * This is used to extract, list or check a ZIP archive.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsIosIn           The compressed input stream.  The reference is
 *                              not consumed, instead another one is retained.
 * @param   fFlags              Flags, MBZ.
 * @param   phVfsFss            Where to return the handle to the TAR
 *                              filesystem stream.
 */
RTDECL(int) RTZipPkzipFsStreamFromIoStream(RTVFSIOSTREAM hVfsIosIn, uint32_t fFlags, PRTVFSFSSTREAM phVfsFss);

/**
 * A mini UNZIP program.
 *
 * @returns Program exit code.
 * @
 * @param   cArgs               The number of arguments.
 * @param   papszArgs           The argument vector.  (Note that this may be
 *                              reordered, so the memory must be writable.)
 */
RTDECL(RTEXITCODE) RTZipUnzipCmd(unsigned cArgs, char **papszArgs);

/**
 * Helper for decompressing files of a ZIP file located in memory.
 *
 * @returns IPRT status code.
 *
 * @param   ppvDst              Where to store the pointer to the allocated
 *                              buffer. To be freed with RTMemFree().
 * @param   pcbDst              Where to store the pointer to the size of the
 *                              allocated buffer.
 * @param   pvSrc               Pointer to the buffer containing the .zip file.
 * @param   cbSrc               Size of the buffer containing the .zip file.
 * @param   pszObject           Name of the object to extract.
 */
RTDECL(int) RTZipPkzipMemDecompress(void **ppvDst, size_t *pcbDst, const void *pvSrc, size_t cbSrc, const char *pszObject);

/**
 * Opens a XAR filesystem stream.
 *
 * This is used to extract, list or check a XAR archive.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsIosIn           The compressed input stream.  The reference is
 *                              not consumed, instead another one is retained.
 * @param   fFlags              Flags, MBZ.
 * @param   phVfsFss            Where to return the handle to the XAR filesystem
 *                              stream.
 */
RTDECL(int) RTZipXarFsStreamFromIoStream(RTVFSIOSTREAM hVfsIosIn, uint32_t fFlags, PRTVFSFSSTREAM phVfsFss);

/**
 * Opens a CPIO filesystem stream.
 *
 * This is used to extract, list or check a CPIO archive.
 *
 * @returns IPRT status code.
 *
 * @param   hVfsIosIn           The input stream.  The reference is not
 *                              consumed, instead another one is retained.
 * @param   fFlags              Flags, MBZ.
 * @param   phVfsFss            Where to return the handle to the CPIO
 *                              filesystem stream.
 */
RTDECL(int) RTZipCpioFsStreamFromIoStream(RTVFSIOSTREAM hVfsIosIn, uint32_t fFlags, PRTVFSFSSTREAM phVfsFss);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_zip_h */

