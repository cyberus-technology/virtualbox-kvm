/** @file
 * IPRT - Logging.
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

#ifndef IPRT_INCLUDED_log_h
#define IPRT_INCLUDED_log_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/stdarg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_log    RTLog - Logging
 * @ingroup grp_rt
 * @{
 */

/**
 * IPRT Logging Groups.
 * (Remember to update RT_LOGGROUP_NAMES!)
 *
 * @remark It should be pretty obvious, but just to have
 *         mentioned it, the values are sorted alphabetically (using the
 *         english alphabet) except for _DEFAULT which is always first.
 *
 *         If anyone might be wondering what the alphabet looks like:
 *              a b c d e f g h i j k l m n o p q r s t u v w x y z
 */
typedef enum RTLOGGROUP
{
    /** Default logging group. */
    RTLOGGROUP_DEFAULT,
    RTLOGGROUP_CRYPTO,
    RTLOGGROUP_DBG,
    RTLOGGROUP_DBG_DWARF,
    RTLOGGROUP_DIR,
    RTLOGGROUP_FILE,
    RTLOGGROUP_FS,
    RTLOGGROUP_FTP,
    RTLOGGROUP_HTTP,
    RTLOGGROUP_IOQUEUE,
    RTLOGGROUP_LDR,
    RTLOGGROUP_LOCALIPC,
    RTLOGGROUP_PATH,
    RTLOGGROUP_PROCESS,
    RTLOGGROUP_REST,
    RTLOGGROUP_SYMLINK,
    RTLOGGROUP_THREAD,
    RTLOGGROUP_TIME,
    RTLOGGROUP_TIMER,
    RTLOGGROUP_VFS,
    RTLOGGROUP_ZIP = 31,
    RTLOGGROUP_FIRST_USER = 32
} RTLOGGROUP;

/** @def RT_LOGGROUP_NAMES
 * IPRT Logging group names.
 *
 * Must correspond 100% to RTLOGGROUP!
 * Don't forget commas!
 *
 * @remark It should be pretty obvious, but just to have
 *         mentioned it, the values are sorted alphabetically (using the
 *         english alphabet) except for _DEFAULT which is always first.
 *
 *         If anyone might be wondering what the alphabet looks like:
 *              a b c d e f g h i j k l m n o p q r s t u v w x y z
 *
 * The RT_XX log group names are placeholders for new modules being added,
 * to make sure that there always is a total of 32 log group entries.
 */
#define RT_LOGGROUP_NAMES \
    "DEFAULT", \
    "RT_CRYPTO", \
    "RT_DBG", \
    "RT_DBG_DWARF", \
    "RT_DIR", \
    "RT_FILE", \
    "RT_FS", \
    "RT_FTP", \
    "RT_HTTP", \
    "RT_IOQUEUE", \
    "RT_LDR", \
    "RT_LOCALIPC", \
    "RT_PATH", \
    "RT_PROCESS", \
    "RT_REST", \
    "RT_SYMLINK", \
    "RT_THREAD", \
    "RT_TIME", \
    "RT_TIMER", \
    "RT_VFS", \
    "RT_20", \
    "RT_21", \
    "RT_22", \
    "RT_23", \
    "RT_24", \
    "RT_25", \
    "RT_26", \
    "RT_27", \
    "RT_28", \
    "RT_29", \
    "RT_30", \
    "RT_ZIP"


/** @def LOG_GROUP
 * Active logging group.
 */
#ifndef LOG_GROUP
# define LOG_GROUP          RTLOGGROUP_DEFAULT
#endif

/** @def LOG_FN_FMT
 * You can use this to specify your desired way of printing __PRETTY_FUNCTION__
 * if you dislike the default one.
 */
#ifndef LOG_FN_FMT
# define LOG_FN_FMT "%Rfn"
#endif

#ifdef LOG_INSTANCE
# error "LOG_INSTANCE is no longer supported."
#endif
#ifdef LOG_REL_INSTANCE
# error "LOG_REL_INSTANCE is no longer supported."
#endif

/** Logger structure. */
typedef struct RTLOGGER RTLOGGER;
/** Pointer to logger structure. */
typedef RTLOGGER *PRTLOGGER;
/** Pointer to const logger structure. */
typedef const RTLOGGER *PCRTLOGGER;


/** Pointer to a log buffer descriptor. */
typedef struct RTLOGBUFFERDESC *PRTLOGBUFFERDESC;


/**
 * Logger phase.
 *
 * Used for signalling the log header/footer callback what to do.
 */
typedef enum RTLOGPHASE
{
    /** Begin of the logging. */
    RTLOGPHASE_BEGIN = 0,
    /** End of the logging. */
    RTLOGPHASE_END,
    /** Before rotating the log file. */
    RTLOGPHASE_PREROTATE,
    /** After rotating the log file. */
    RTLOGPHASE_POSTROTATE,
    /** 32-bit type blow up hack.  */
    RTLOGPHASE_32BIT_HACK = 0x7fffffff
} RTLOGPHASE;


#if 0 /* retired */
/**
 * Logger function.
 *
 * @param   pszFormat   Format string.
 * @param   ...         Optional arguments as specified in the format string.
 */
typedef DECLCALLBACKTYPE(void, FNRTLOGGER,(const char *pszFormat, ...)) RT_IPRT_FORMAT_ATTR(1, 2);
/** Pointer to logger function. */
typedef FNRTLOGGER *PFNRTLOGGER;
#endif

/**
 * Custom buffer flushing function.
 *
 * @retval  true if flushed and the buffer can be reused.
 * @retval  false for switching to the next buffer because an async flush of
 *          @a pBufDesc is still pending.  The implementation is responsible for
 *          only returning when the next buffer is ready for reuse, the generic
 *          logger code has no facility to make sure of this.
 *
 * @param   pLogger     Pointer to the logger instance which is to be flushed.
 * @param   pBufDesc    The descriptor of the buffer to be flushed.
 */
typedef DECLCALLBACKTYPE(bool, FNRTLOGFLUSH,(PRTLOGGER pLogger, PRTLOGBUFFERDESC pBufDesc));
/** Pointer to flush function. */
typedef FNRTLOGFLUSH *PFNRTLOGFLUSH;

/**
 * Header/footer message callback.
 *
 * @param   pLogger     Pointer to the logger instance.
 * @param   pszFormat   Format string.
 * @param   ...         Optional arguments specified in the format string.
 */
typedef DECLCALLBACKTYPE(void, FNRTLOGPHASEMSG,(PRTLOGGER pLogger, const char *pszFormat, ...)) RT_IPRT_FORMAT_ATTR(2, 3);
/** Pointer to header/footer message callback function. */
typedef FNRTLOGPHASEMSG *PFNRTLOGPHASEMSG;

/**
 * Log file header/footer callback.
 *
 * @param   pLogger         Pointer to the logger instance.
 * @param   enmLogPhase     Indicates at what time the callback is invoked.
 * @param   pfnLogPhaseMsg  Callback for writing the header/footer (RTLogPrintf
 *                          and others are out of bounds).
 */
typedef DECLCALLBACKTYPE(void, FNRTLOGPHASE,(PRTLOGGER pLogger, RTLOGPHASE enmLogPhase, PFNRTLOGPHASEMSG pfnLogPhaseMsg));
/** Pointer to log header/footer callback function. */
typedef FNRTLOGPHASE *PFNRTLOGPHASE;

/**
 * Custom log prefix callback.
 *
 *
 * @returns The number of chars written.
 *
 * @param   pLogger     Pointer to the logger instance.
 * @param   pchBuf      Output buffer pointer.
 *                      No need to terminate the output.
 * @param   cchBuf      The size of the output buffer.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACKTYPE(size_t, FNRTLOGPREFIX,(PRTLOGGER pLogger, char *pchBuf, size_t cchBuf, void *pvUser));
/** Pointer to prefix callback function. */
typedef FNRTLOGPREFIX *PFNRTLOGPREFIX;


/** Pointer to a constant log output interface. */
typedef const struct RTLOGOUTPUTIF *PCRTLOGOUTPUTIF;

/**
 * Logging output interface.
 */
typedef struct RTLOGOUTPUTIF
{
    /**
     * Opens a new log file with the given name.
     *
     * @returns IPRT status code.
     * @param   pIf             Pointer to this interface.
     * @param   pvUser          Opaque user data passed when setting the callbacks.
     * @param   pszFilename     The filename to open.
     * @param   fFlags          Open flags, combination of RTFILE_O_XXX.
     */
    DECLR3CALLBACKMEMBER(int, pfnOpen, (PCRTLOGOUTPUTIF pIf, void *pvUser, const char *pszFilename, uint32_t fFlags));

    /**
     * Closes the currently open file.
     *
     * @returns IPRT status code.
     * @param   pIf             Pointer to this interface.
     * @param   pvUser          Opaque user data passed when setting the callbacks.
     */
    DECLR3CALLBACKMEMBER(int, pfnClose, (PCRTLOGOUTPUTIF pIf, void *pvUser));

    /**
     * Deletes the given file.
     *
     * @returns IPRT status code.
     * @param   pIf             Pointer to this interface.
     * @param   pvUser          Opaque user data passed when setting the callbacks.
     * @param   pszFilename     The filename to delete.
     */
    DECLR3CALLBACKMEMBER(int, pfnDelete, (PCRTLOGOUTPUTIF pIf, void *pvUser, const char *pszFilename));

    /**
     * Renames the given file.
     *
     * @returns IPRT status code.
     * @param   pIf             Pointer to this interface.
     * @param   pvUser          Opaque user data passed when setting the callbacks.
     * @param   pszFilenameOld  The old filename to rename.
     * @param   pszFilenameNew  The new filename.
     * @param   fFlags          Flags for the operation, combination of RTFILEMOVE_FLAGS_XXX.
     */
    DECLR3CALLBACKMEMBER(int, pfnRename, (PCRTLOGOUTPUTIF pIf, void *pvUser, const char *pszFilenameOld,
                                          const char *pszFilenameNew, uint32_t fFlags));

    /**
     * Queries the size of the log file.
     *
     * @returns IPRT status code.
     * @param   pIf             Pointer to this interface.
     * @param   pvUser          Opaque user data passed when setting the callbacks.
     * @param   pcbFile         Where to store the file size in bytes on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnQuerySize, (PCRTLOGOUTPUTIF pIf, void *pvUser, uint64_t *pcbSize));

    /**
     * Writes data to the log file.
     *
     * @returns IPRT status code.
     * @param   pIf             Pointer to this interface.
     * @param   pvUser          Opaque user data passed when setting the callbacks.
     * @param   pvBuf           The data to write.
     * @param   cbWrite         Number of bytes to write.
     * @param   pcbWritten      Where to store the actual number of bytes written on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnWrite, (PCRTLOGOUTPUTIF pIf, void *pvUser, const void *pvBuf,
                                         size_t cbWrite, size_t *pcbWritten));

    /**
     * Flushes data to the underlying storage medium.
     *
     * @returns IPRT status code.
     * @param   pIf             Pointer to this interface.
     * @param   pvUser          Opaque user data passed when setting the callbacks.
     */
    DECLR3CALLBACKMEMBER(int, pfnFlush, (PCRTLOGOUTPUTIF pIf, void *pvUser));
} RTLOGOUTPUTIF;
/** Pointer to a logging output interface. */
typedef struct RTLOGOUTPUTIF *PRTLOGOUTPUTIF;


/**
 * Auxiliary buffer descriptor.
 *
 * This is what we share we ring-3 and use for flushing ring-0 EMT loggers when
 * we return to ring-3.
 */
typedef struct RTLOGBUFFERAUXDESC
{
    /** Flush indicator.
     * Ring-3 sets this if it flushed the buffer, ring-0 clears it again after
     * writing. */
    bool volatile           fFlushedIndicator;
    bool                    afPadding[3];
    /** Copy of RTLOGBUFFERDESC::offBuf. */
    uint32_t                offBuf;
} RTLOGBUFFERAUXDESC;
/** Pointer to auxiliary buffer descriptor. */
typedef RTLOGBUFFERAUXDESC *PRTLOGBUFFERAUXDESC;

/**
 * Log buffer desciptor.
 */
typedef struct RTLOGBUFFERDESC
{
    /** Magic value / eye catcher (RTLOGBUFFERDESC_MAGIC). */
    uint32_t                u32Magic;
    /** Padding. */
    uint32_t                uReserved;
    /** The buffer size. */
    uint32_t                cbBuf;
    /** The current buffer offset. */
    uint32_t                offBuf;
    /** Pointer to the buffer. */
    char                   *pchBuf;
    /** Pointer to auxiliary desciptor, NULL if not used. */
    PRTLOGBUFFERAUXDESC     pAux;
} RTLOGBUFFERDESC;

/** RTLOGBUFFERDESC::u32Magic value. (Avram Noam Chomsky) */
#define RTLOGBUFFERDESC_MAGIC   UINT32_C(0x19281207)

/**
 * The public logger instance part.
 *
 * The logger instance is mostly abstract and kept as RTLOGGERINTERNAL within
 * log.cpp.  This public part is at the start of RTLOGGERINTERNAL.
 */
struct RTLOGGER
{
    /** Magic number (RTLOGGER_MAGIC). */
    uint32_t                u32Magic;
    /** User value \#1, initialized to zero. */
    uint32_t                u32UserValue1;
    /** User value \#2, initialized to zero. */
    uint64_t                u64UserValue2;
    /** User value \#3, initialized to zero. */
    uint64_t                u64UserValue3;
#if 0
    /** Pointer to the logger function (used in non-C99 mode only).
     *
     * This is actually pointer to a wrapper/stub function which will push a pointer
     * to the instance pointer onto the stack before jumping to the real logger
     * function.  A very unfortunate hack to work around the missing variadic macro
     * support in older C++/C standards.  (The memory is allocated using
     * RTMemExecAlloc(), except for agnostic R0 code.) */
    PFNRTLOGGER             pfnLogger;
#else
    /** Unused. */
    uintptr_t               uUsedToBeNonC99Logger;
#endif
#if ARCH_BITS == 32
    /** Explicit padding. */
    uint32_t                uReserved1;
#endif
};

/** RTLOGGER::u32Magic value. (John Rogers Searle) */
#define RTLOGGER_MAGIC          UINT32_C(0x19320731)

/**
 * Logger flags.
 */
typedef enum RTLOGFLAGS
{
    /** The logger instance is disabled for normal output. */
    RTLOGFLAGS_DISABLED             = 0x00000001,
    /** The logger instance is using buffered output. */
    RTLOGFLAGS_BUFFERED             = 0x00000002,
    /** The logger instance expands LF to CR/LF. */
    RTLOGFLAGS_USECRLF              = 0x00000010,
    /** Append to the log destination where applicable. */
    RTLOGFLAGS_APPEND               = 0x00000020,
    /** Show relative timestamps with PREFIX_TSC and PREFIX_TS */
    RTLOGFLAGS_REL_TS               = 0x00000040,
    /** Show decimal timestamps with PREFIX_TSC and PREFIX_TS */
    RTLOGFLAGS_DECIMAL_TS           = 0x00000080,
    /** Open the file in write through mode. */
    RTLOGFLAGS_WRITE_THROUGH        = 0x00000100,
    /** Flush the file to disk when flushing the buffer. */
    RTLOGFLAGS_FLUSH                = 0x00000200,
    /** Restrict the number of log entries per group. */
    RTLOGFLAGS_RESTRICT_GROUPS      = 0x00000400,
    /** New lines should be prefixed with the write and read lock counts. */
    RTLOGFLAGS_PREFIX_LOCK_COUNTS   = 0x00008000,
    /** New lines should be prefixed with the CPU id (ApicID on intel/amd). */
    RTLOGFLAGS_PREFIX_CPUID         = 0x00010000,
    /** New lines should be prefixed with the native process id. */
    RTLOGFLAGS_PREFIX_PID           = 0x00020000,
    /** New lines should be prefixed with group flag number causing the output. */
    RTLOGFLAGS_PREFIX_FLAG_NO       = 0x00040000,
    /** New lines should be prefixed with group flag name causing the output. */
    RTLOGFLAGS_PREFIX_FLAG          = 0x00080000,
    /** New lines should be prefixed with group number. */
    RTLOGFLAGS_PREFIX_GROUP_NO      = 0x00100000,
    /** New lines should be prefixed with group name. */
    RTLOGFLAGS_PREFIX_GROUP         = 0x00200000,
    /** New lines should be prefixed with the native thread id. */
    RTLOGFLAGS_PREFIX_TID           = 0x00400000,
    /** New lines should be prefixed with thread name. */
    RTLOGFLAGS_PREFIX_THREAD        = 0x00800000,
    /** New lines should be prefixed with data from a custom callback. */
    RTLOGFLAGS_PREFIX_CUSTOM        = 0x01000000,
    /** New lines should be prefixed with formatted timestamp since program start. */
    RTLOGFLAGS_PREFIX_TIME_PROG     = 0x04000000,
    /** New lines should be prefixed with formatted timestamp (UCT). */
    RTLOGFLAGS_PREFIX_TIME          = 0x08000000,
    /** New lines should be prefixed with milliseconds since program start. */
    RTLOGFLAGS_PREFIX_MS_PROG       = 0x10000000,
    /** New lines should be prefixed with timestamp. */
    RTLOGFLAGS_PREFIX_TSC           = 0x20000000,
    /** New lines should be prefixed with timestamp. */
    RTLOGFLAGS_PREFIX_TS            = 0x40000000,
    /** The prefix mask. */
    RTLOGFLAGS_PREFIX_MASK          = 0x7dff8000
} RTLOGFLAGS;
/** Don't use locking. */
#define RTLOG_F_NO_LOCKING          RT_BIT_64(63)
/** Mask with all valid log flags (for validation). */
#define RTLOG_F_VALID_MASK          UINT64_C(0x800000007fff87f3)

/**
 * Logger per group flags.
 *
 * @remarks We only use the lower 16 bits here.  We'll be combining it with the
 *          group number in a few places.
 */
typedef enum RTLOGGRPFLAGS
{
    /** Enabled. */
    RTLOGGRPFLAGS_ENABLED      = 0x0001,
    /** Flow logging. */
    RTLOGGRPFLAGS_FLOW         = 0x0002,
    /** Warnings logging. */
    RTLOGGRPFLAGS_WARN         = 0x0004,
    /* 0x0008 for later. */
    /** Level 1 logging. */
    RTLOGGRPFLAGS_LEVEL_1      = 0x0010,
    /** Level 2 logging. */
    RTLOGGRPFLAGS_LEVEL_2      = 0x0020,
    /** Level 3 logging. */
    RTLOGGRPFLAGS_LEVEL_3      = 0x0040,
    /** Level 4 logging. */
    RTLOGGRPFLAGS_LEVEL_4      = 0x0080,
    /** Level 5 logging. */
    RTLOGGRPFLAGS_LEVEL_5      = 0x0100,
    /** Level 6 logging. */
    RTLOGGRPFLAGS_LEVEL_6      = 0x0200,
    /** Level 7 logging. */
    RTLOGGRPFLAGS_LEVEL_7      = 0x0400,
    /** Level 8 logging. */
    RTLOGGRPFLAGS_LEVEL_8      = 0x0800,
    /** Level 9 logging. */
    RTLOGGRPFLAGS_LEVEL_9      = 0x1000,
    /** Level 10 logging. */
    RTLOGGRPFLAGS_LEVEL_10     = 0x2000,
    /** Level 11 logging. */
    RTLOGGRPFLAGS_LEVEL_11     = 0x4000,
    /** Level 12 logging. */
    RTLOGGRPFLAGS_LEVEL_12     = 0x8000,

    /** Restrict the number of log entries. */
    RTLOGGRPFLAGS_RESTRICT     = 0x40000000,
    /** Blow up the type. */
    RTLOGGRPFLAGS_32BIT_HACK   = 0x7fffffff
} RTLOGGRPFLAGS;

/**
 * Logger destination types and flags.
 */
typedef enum RTLOGDEST
{
    /** Log to file. */
    RTLOGDEST_FILE          = 0x00000001,
    /** Log to stdout. */
    RTLOGDEST_STDOUT        = 0x00000002,
    /** Log to stderr. */
    RTLOGDEST_STDERR        = 0x00000004,
    /** Log to debugger (win32 only). */
    RTLOGDEST_DEBUGGER      = 0x00000008,
    /** Log to com port. */
    RTLOGDEST_COM           = 0x00000010,
    /** Log a memory ring buffer. */
    RTLOGDEST_RINGBUF       = 0x00000020,
    /** The parent VMM debug log. */
    RTLOGDEST_VMM           = 0x00000040,
    /** The parent VMM release log. */
    RTLOGDEST_VMM_REL       = 0x00000080,
    /** Open files with no deny (share read, write, delete) on Windows. */
    RTLOGDEST_F_NO_DENY     = 0x00010000,
    /** Delay opening the log file, logging to the buffer untill
     * RTLogClearFileDelayFlag is called. */
    RTLOGDEST_F_DELAY_FILE  = 0x00020000,
    /** Don't allow changes to the filename or mode of opening it. */
    RTLOGDEST_FIXED_FILE    = 0x01000000,
    /** Don't allow changing the directory. */
    RTLOGDEST_FIXED_DIR     = 0x02000000,
    /** Just a dummy flag to be used when no other flag applies. */
    RTLOGDEST_DUMMY         = 0x20000000,
    /** Log to a user defined output stream. */
    RTLOGDEST_USER          = 0x40000000
} RTLOGDEST;
/** Valid log destinations. */
#define RTLOG_DST_VALID_MASK    UINT32_C(0x630300ff)
/** Log destinations that can be changed via RTLogChangeDestinations. */
#define RTLOG_DST_CHANGE_MASK   UINT32_C(0x400000de)


#ifdef DOXYGEN_RUNNING
# define LOG_DISABLED
# define LOG_ENABLED
# define LOG_ENABLE_FLOW
#endif

/** @def LOG_DISABLED
 * Use this compile time define to disable all logging macros. It can
 * be overridden for each of the logging macros by the LOG_ENABLE*
 * compile time defines.
 */

/** @def LOG_ENABLED
 * Use this compile time define to enable logging when not in debug mode
 * or LOG_DISABLED is set.
 * This will enable Log() only.
 */

/** @def LOG_ENABLE_FLOW
 * Use this compile time define to enable flow logging when not in
 * debug mode or LOG_DISABLED is defined.
 * This will enable LogFlow() only.
 */

/*
 * Determine whether logging is enabled and forcefully normalize the indicators.
 */
#if (defined(DEBUG) || defined(LOG_ENABLED)) && !defined(LOG_DISABLED)
# undef  LOG_DISABLED
# undef  LOG_ENABLED
# define LOG_ENABLED
#else
# undef  LOG_ENABLED
# undef  LOG_DISABLED
# define LOG_DISABLED
#endif


/** @def LOG_USE_C99
 * Governs the use of variadic macros.
 */
#ifndef LOG_USE_C99
# define LOG_USE_C99
#endif


/** @name Macros for checking whether a log level is enabled.
 * @{ */
/** @def LogIsItEnabled
 * Checks whether the specified logging group is enabled or not.
 */
#ifdef LOG_ENABLED
# define LogIsItEnabled(a_fFlags, a_iGroup) ( RTLogDefaultInstanceEx(RT_MAKE_U32(a_fFlags, a_iGroup)) != NULL )
#else
# define LogIsItEnabled(a_fFlags, a_iGroup) (false)
#endif

/** @def LogIsEnabled
 * Checks whether level 1 logging is enabled.
 */
#define LogIsEnabled()      LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP)

/** @def LogIs2Enabled
 * Checks whether level 2 logging is enabled.
 */
#define LogIs2Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP)

/** @def LogIs3Enabled
 * Checks whether level 3 logging is enabled.
 */
#define LogIs3Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_3, LOG_GROUP)

/** @def LogIs4Enabled
 * Checks whether level 4 logging is enabled.
 */
#define LogIs4Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_4, LOG_GROUP)

/** @def LogIs5Enabled
 * Checks whether level 5 logging is enabled.
 */
#define LogIs5Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP)

/** @def LogIs6Enabled
 * Checks whether level 6 logging is enabled.
 */
#define LogIs6Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_6, LOG_GROUP)

/** @def LogIs7Enabled
 * Checks whether level 7 logging is enabled.
 */
#define LogIs7Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_7, LOG_GROUP)

/** @def LogIs8Enabled
 * Checks whether level 8 logging is enabled.
 */
#define LogIs8Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_8, LOG_GROUP)

/** @def LogIs9Enabled
 * Checks whether level 9 logging is enabled.
 */
#define LogIs9Enabled()     LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_9, LOG_GROUP)

/** @def LogIs10Enabled
 * Checks whether level 10 logging is enabled.
 */
#define LogIs10Enabled()    LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_10, LOG_GROUP)

/** @def LogIs11Enabled
 * Checks whether level 11 logging is enabled.
 */
#define LogIs11Enabled()    LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_11, LOG_GROUP)

/** @def LogIs12Enabled
 * Checks whether level 12 logging is enabled.
 */
#define LogIs12Enabled()    LogIsItEnabled(RTLOGGRPFLAGS_LEVEL_12, LOG_GROUP)

/** @def LogIsFlowEnabled
 * Checks whether execution flow logging is enabled.
 */
#define LogIsFlowEnabled()  LogIsItEnabled(RTLOGGRPFLAGS_FLOW, LOG_GROUP)

/** @def LogIsWarnEnabled
 * Checks whether execution flow logging is enabled.
 */
#define LogIsWarnEnabled()  LogIsItEnabled(RTLOGGRPFLAGS_WARN, LOG_GROUP)
/** @} */


/** @def LogIt
 * Write to specific logger if group enabled.
 */
#ifdef LOG_ENABLED
# if defined(LOG_USE_C99)
#  define _LogRemoveParentheseis(...)                   __VA_ARGS__
#  define _LogIt(a_fFlags, a_iGroup, ...) \
   do \
   { \
        PRTLOGGER LogIt_pLogger = RTLogDefaultInstanceEx(RT_MAKE_U32(a_fFlags, a_iGroup)); \
        if (RT_LIKELY(!LogIt_pLogger)) \
        {   /* likely */ } \
        else \
            RTLogLoggerEx(LogIt_pLogger, a_fFlags, a_iGroup, __VA_ARGS__); \
   } while (0)
#  define LogIt(a_fFlags, a_iGroup, fmtargs)            _LogIt(a_fFlags, a_iGroup, _LogRemoveParentheseis fmtargs)
#  define _LogItAlways(a_fFlags, a_iGroup, ...)          RTLogLoggerEx(NULL, a_fFlags, UINT32_MAX, __VA_ARGS__)
#  define LogItAlways(a_fFlags, a_iGroup, fmtargs)      _LogItAlways(a_fFlags, a_iGroup, _LogRemoveParentheseis fmtargs)
        /** @todo invent a flag or something for skipping the group check so we can pass iGroup. LogItAlways. */
# else
#  define LogIt(a_fFlags, a_iGroup, fmtargs) \
    do \
    { \
        PRTLOGGER LogIt_pLogger = RTLogDefaultInstanceEx(RT_MAKE_U32(a_fFlags, a_iGroup)); \
        if (RT_LIKELY(!LogIt_pLogger)) \
        {   /* likely */ } \
        else \
        { \
            LogIt_pLogger->pfnLogger fmtargs; \
        } \
    } while (0)
#  define LogItAlways(a_fFlags, a_iGroup, fmtargs) \
    do \
    { \
        PRTLOGGER LogIt_pLogger = RTLogDefaultInstanceEx(RT_MAKE_U32(0, UINT16_MAX)); \
        if (LogIt_pLogger) \
            LogIt_pLogger->pfnLogger fmtargs; \
    } while (0)
# endif
#else
# define LogIt(a_fFlags, a_iGroup, fmtargs)             do { } while (0)
# define LogItAlways(a_fFlags, a_iGroup, fmtargs)       do { } while (0)
# if defined(LOG_USE_C99)
#  define _LogRemoveParentheseis(...)                   __VA_ARGS__
#  define _LogIt(a_fFlags, a_iGroup, ...)               do { } while (0)
#  define _LogItAlways(a_fFlags, a_iGroup, ...)         do { } while (0)
# endif
#endif


/** @name Basic logging macros
 * @{ */
/** @def Log
 * Level 1 logging that works regardless of the group settings.
 */
#define LogAlways(a)    LogItAlways(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, a)

/** @def Log
 * Level 1 logging.
 */
#define Log(a)          LogIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, a)

/** @def Log2
 * Level 2 logging.
 */
#define Log2(a)         LogIt(RTLOGGRPFLAGS_LEVEL_2,  LOG_GROUP, a)

/** @def Log3
 * Level 3 logging.
 */
#define Log3(a)         LogIt(RTLOGGRPFLAGS_LEVEL_3,  LOG_GROUP, a)

/** @def Log4
 * Level 4 logging.
 */
#define Log4(a)         LogIt(RTLOGGRPFLAGS_LEVEL_4,  LOG_GROUP, a)

/** @def Log5
 * Level 5 logging.
 */
#define Log5(a)         LogIt(RTLOGGRPFLAGS_LEVEL_5,  LOG_GROUP, a)

/** @def Log6
 * Level 6 logging.
 */
#define Log6(a)         LogIt(RTLOGGRPFLAGS_LEVEL_6,  LOG_GROUP, a)

/** @def Log7
 * Level 7 logging.
 */
#define Log7(a)         LogIt(RTLOGGRPFLAGS_LEVEL_7,  LOG_GROUP, a)

/** @def Log8
 * Level 8 logging.
 */
#define Log8(a)         LogIt(RTLOGGRPFLAGS_LEVEL_8,  LOG_GROUP, a)

/** @def Log9
 * Level 9 logging.
 */
#define Log9(a)         LogIt(RTLOGGRPFLAGS_LEVEL_9,  LOG_GROUP, a)

/** @def Log10
 * Level 10 logging.
 */
#define Log10(a)        LogIt(RTLOGGRPFLAGS_LEVEL_10, LOG_GROUP, a)

/** @def Log11
 * Level 11 logging.
 */
#define Log11(a)        LogIt(RTLOGGRPFLAGS_LEVEL_11, LOG_GROUP, a)

/** @def Log12
 * Level 12 logging.
 */
#define Log12(a)        LogIt(RTLOGGRPFLAGS_LEVEL_12,  LOG_GROUP, a)

/** @def LogFlow
 * Logging of execution flow.
 */
#define LogFlow(a)      LogIt(RTLOGGRPFLAGS_FLOW,      LOG_GROUP, a)

/** @def LogWarn
 * Logging of warnings.
 */
#define LogWarn(a)      LogIt(RTLOGGRPFLAGS_WARN,      LOG_GROUP, a)
/** @} */


/** @name Logging macros prefixing the current function name.
 * @{ */
/** @def LogFunc
 * Level 1 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogFunc(a)   _LogIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogFunc(a)   do { Log((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log(a); } while (0)
#endif

/** @def Log2Func
 * Level 2 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log2Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log2Func(a)  do { Log2((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log2(a); } while (0)
#endif

/** @def Log3Func
 * Level 3 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log3Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_3, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log3Func(a)  do { Log3((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log3(a); } while (0)
#endif

/** @def Log4Func
 * Level 4 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log4Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_4, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log4Func(a)  do { Log4((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log4(a); } while (0)
#endif

/** @def Log5Func
 * Level 5 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log5Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log5Func(a)  do { Log5((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log5(a); } while (0)
#endif

/** @def Log6Func
 * Level 6 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log6Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_6, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log6Func(a)  do { Log6((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log6(a); } while (0)
#endif

/** @def Log7Func
 * Level 7 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log7Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_7, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log7Func(a)  do { Log7((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log7(a); } while (0)
#endif

/** @def Log8Func
 * Level 8 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log8Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_8, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log8Func(a)  do { Log8((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log8(a); } while (0)
#endif

/** @def Log9Func
 * Level 9 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log9Func(a)  _LogIt(RTLOGGRPFLAGS_LEVEL_9, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log9Func(a)  do { Log9((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log9(a); } while (0)
#endif

/** @def Log10Func
 * Level 10 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log10Func(a) _LogIt(RTLOGGRPFLAGS_LEVEL_10, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log10Func(a) do { Log10((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log10(a); } while (0)
#endif

/** @def Log11Func
 * Level 11 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log11Func(a) _LogIt(RTLOGGRPFLAGS_LEVEL_11, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log11Func(a) do { Log11((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log11(a); } while (0)
#endif

/** @def Log12Func
 * Level 12 logging inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by a
 * semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log12Func(a) _LogIt(RTLOGGRPFLAGS_LEVEL_12, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log12Func(a) do { Log12((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log12(a); } while (0)
#endif

/** @def LogFlowFunc
 * Macro to log the execution flow inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by
 * a semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogFlowFunc(a) \
    _LogIt(RTLOGGRPFLAGS_FLOW, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogFlowFunc(a) \
    do { LogFlow((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogFlow(a); } while (0)
#endif

/** @def LogWarnFunc
 * Macro to log a warning inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by
 * a semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogWarnFunc(a) \
    _LogIt(RTLOGGRPFLAGS_WARN, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogWarnFunc(a) \
    do { LogFlow((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogFlow(a); } while (0)
#endif
/** @} */


/** @name Logging macros prefixing the this pointer value and method name.
 * @{ */

/** @def LogThisFunc
 * Level 1 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogThisFunc(a) do { Log(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log(a); } while (0)
#endif

/** @def Log2ThisFunc
 * Level 2 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log2ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log2ThisFunc(a) do { Log2(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log2(a); } while (0)
#endif

/** @def Log3ThisFunc
 * Level 3 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log3ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_3, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log3ThisFunc(a) do { Log3(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log3(a); } while (0)
#endif

/** @def Log4ThisFunc
 * Level 4 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log4ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_4, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log4ThisFunc(a) do { Log4(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log4(a); } while (0)
#endif

/** @def Log5ThisFunc
 * Level 5 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log5ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log5ThisFunc(a) do { Log5(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log5(a); } while (0)
#endif

/** @def Log6ThisFunc
 * Level 6 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log6ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_6, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log6ThisFunc(a) do { Log6(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log6(a); } while (0)
#endif

/** @def Log7ThisFunc
 * Level 7 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log7ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_7, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log7ThisFunc(a) do { Log7(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log7(a); } while (0)
#endif

/** @def Log8ThisFunc
 * Level 8 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log8ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_8, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log8ThisFunc(a) do { Log8(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log8(a); } while (0)
#endif

/** @def Log9ThisFunc
 * Level 9 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log9ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_9, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log9ThisFunc(a) do { Log9(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log9(a); } while (0)
#endif

/** @def Log10ThisFunc
 * Level 10 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log10ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_10, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log10ThisFunc(a) do { Log10(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log10(a); } while (0)
#endif

/** @def Log11ThisFunc
 * Level 11 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log11ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_11, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log11ThisFunc(a) do { Log11(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log11(a); } while (0)
#endif

/** @def Log12ThisFunc
 * Level 12 logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log12ThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_12, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log12ThisFunc(a) do { Log12(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log12(a); } while (0)
#endif

/** @def LogFlowThisFunc
 * Flow level logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogFlowThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_FLOW, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogFlowThisFunc(a) do { LogFlow(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogFlow(a); } while (0)
#endif

/** @def LogWarnThisFunc
 * Warning level logging inside a C++ non-static method, with object pointer and
 * method name prefixed to the given message.
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogWarnThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_WARN, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogWarnThisFunc(a) do { LogWarn(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogWarn(a); } while (0)
#endif
/** @} */


/** @name Misc Logging Macros
 * @{ */

/** @def Log1Warning
 * The same as Log(), but prepents a <tt>"WARNING! "</tt> string to the message.
 *
 * @param   a   Custom log message in format <tt>("string\n" [, args])</tt>.
 */
#if defined(LOG_USE_C99)
# define Log1Warning(a)     _LogIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, "WARNING! %M", _LogRemoveParentheseis a )
#else
# define Log1Warning(a)     do { Log(("WARNING! ")); Log(a); } while (0)
#endif

/** @def Log1WarningFunc
 * The same as LogWarning(), but prepents the log message with the function name.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log1WarningFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, LOG_FN_FMT ": WARNING! %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log1WarningFunc(a) \
    do { Log((LOG_FN_FMT ": WARNING! ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log(a); } while (0)
#endif

/** @def Log1WarningThisFunc
 * The same as LogWarningFunc() but for class functions (methods): the resulting
 * log line is additionally prepended with a hex value of |this| pointer.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define Log1WarningThisFunc(a) \
    _LogIt(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, "{%p} " LOG_FN_FMT ": WARNING! %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define Log1WarningThisFunc(a) \
    do { Log(("{%p} " LOG_FN_FMT ": WARNING! ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); Log(a); } while (0)
#endif


/** Shortcut to |LogFlowFunc ("ENTER\n")|, marks the beginnig of the function. */
#define LogFlowFuncEnter()      LogFlowFunc(("ENTER\n"))

/** Shortcut to |LogFlowFunc ("LEAVE\n")|, marks the end of the function. */
#define LogFlowFuncLeave()      LogFlowFunc(("LEAVE\n"))

/** Shortcut to |LogFlowFunc ("LEAVE: %Rrc\n")|, marks the end of the function. */
#define LogFlowFuncLeaveRC(rc)  LogFlowFunc(("LEAVE: %Rrc\n", (rc)))

/** Shortcut to |LogFlowThisFunc ("ENTER\n")|, marks the beginnig of the function. */
#define LogFlowThisFuncEnter()  LogFlowThisFunc(("ENTER\n"))

/** Shortcut to |LogFlowThisFunc ("LEAVE\n")|, marks the end of the function. */
#define LogFlowThisFuncLeave()  LogFlowThisFunc(("LEAVE\n"))


/** @def LogObjRefCnt
 * Helper macro to print the current reference count of the given COM object
 * to the log file.
 *
 * @param pObj  Pointer to the object in question (must be a pointer to an
 *              IUnknown subclass or simply define COM-style AddRef() and
 *              Release() methods)
 */
#define LogObjRefCnt(pObj) \
    do { \
        if (LogIsFlowEnabled()) \
        { \
            int cRefsForLog = (pObj)->AddRef(); \
            LogFlow((#pObj "{%p}.refCnt=%d\n", (pObj), cRefsForLog - 1)); \
            (pObj)->Release(); \
        } \
    } while (0)
/** @} */



/** @name Passing Function Call Position When Logging.
 *
 * This is a little bit ugly as we have to omit the comma before the
 * position parameters so that we don't inccur any overhead in non-logging
 * builds (!defined(LOG_ENABLED).
 *
 * @{  */
/** Source position for passing to a function call. */
#ifdef LOG_ENABLED
# define RTLOG_COMMA_SRC_POS        , __FILE__, __LINE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__
#else
# define RTLOG_COMMA_SRC_POS        RT_NOTHING
#endif
/** Source position declaration. */
#ifdef LOG_ENABLED
# define RTLOG_COMMA_SRC_POS_DECL   , const char *pszFile, unsigned iLine, const char *pszFunction
#else
# define RTLOG_COMMA_SRC_POS_DECL   RT_NOTHING
#endif
/** Source position arguments. */
#ifdef LOG_ENABLED
# define RTLOG_COMMA_SRC_POS_ARGS   , pszFile, iLine, pszFunction
#else
# define RTLOG_COMMA_SRC_POS_ARGS   RT_NOTHING
#endif
/** Applies NOREF() to the source position arguments. */
#ifdef LOG_ENABLED
# define RTLOG_SRC_POS_NOREF()      do { NOREF(pszFile); NOREF(iLine); NOREF(pszFunction); } while (0)
#else
# define RTLOG_SRC_POS_NOREF()      do { } while (0)
#endif
/** @}  */



/** @defgroup grp_rt_log_rel    Release Logging
 * @{
 */

#ifdef DOXYGEN_RUNNING
# define RTLOG_REL_DISABLED
# define RTLOG_REL_ENABLED
#endif

/** @def RTLOG_REL_DISABLED
 * Use this compile time define to disable all release logging
 * macros.
 */

/** @def RTLOG_REL_ENABLED
 * Use this compile time define to override RTLOG_REL_DISABLE.
 */

/*
 * Determine whether release logging is enabled and forcefully normalize the indicators.
 */
#if !defined(RTLOG_REL_DISABLED) || defined(RTLOG_REL_ENABLED)
# undef  RTLOG_REL_DISABLED
# undef  RTLOG_REL_ENABLED
# define RTLOG_REL_ENABLED
#else
# undef  RTLOG_REL_ENABLED
# undef  RTLOG_REL_DISABLED
# define RTLOG_REL_DISABLED
#endif

/** @name Macros for checking whether a release log level is enabled.
 * @{ */
/** @def LogRelIsItEnabled
 * Checks whether the specified release logging group is enabled or not.
 */
#define LogRelIsItEnabled(a_fFlags, a_iGroup) ( RTLogRelGetDefaultInstanceExWeak(RT_MAKE_U32(a_fFlags, a_iGroup)) != NULL )

/** @def LogRelIsEnabled
 * Checks whether level 1 release logging is enabled.
 */
#define LogRelIsEnabled()      LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP)

/** @def LogRelIs2Enabled
 * Checks whether level 2 release logging is enabled.
 */
#define LogRelIs2Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_2, LOG_GROUP)

/** @def LogRelIs3Enabled
 * Checks whether level 3 release logging is enabled.
 */
#define LogRelIs3Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_3, LOG_GROUP)

/** @def LogRelIs4Enabled
 * Checks whether level 4 release logging is enabled.
 */
#define LogRelIs4Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_4, LOG_GROUP)

/** @def LogRelIs5Enabled
 * Checks whether level 5 release logging is enabled.
 */
#define LogRelIs5Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_5, LOG_GROUP)

/** @def LogRelIs6Enabled
 * Checks whether level 6 release logging is enabled.
 */
#define LogRelIs6Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_6, LOG_GROUP)

/** @def LogRelIs7Enabled
 * Checks whether level 7 release logging is enabled.
 */
#define LogRelIs7Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_7, LOG_GROUP)

/** @def LogRelIs8Enabled
 * Checks whether level 8 release logging is enabled.
 */
#define LogRelIs8Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_8, LOG_GROUP)

/** @def LogRelIs2Enabled
 * Checks whether level 9 release logging is enabled.
 */
#define LogRelIs9Enabled()     LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_9, LOG_GROUP)

/** @def LogRelIs10Enabled
 * Checks whether level 10 release logging is enabled.
 */
#define LogRelIs10Enabled()    LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_10, LOG_GROUP)

/** @def LogRelIs11Enabled
 * Checks whether level 10 release logging is enabled.
 */
#define LogRelIs11Enabled()    LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_11, LOG_GROUP)

/** @def LogRelIs12Enabled
 * Checks whether level 12 release logging is enabled.
 */
#define LogRelIs12Enabled()    LogRelIsItEnabled(RTLOGGRPFLAGS_LEVEL_12, LOG_GROUP)

/** @def LogRelIsFlowEnabled
 * Checks whether execution flow release logging is enabled.
 */
#define LogRelIsFlowEnabled()  LogRelIsItEnabled(RTLOGGRPFLAGS_FLOW, LOG_GROUP)

/** @def LogRelIsWarnEnabled
 * Checks whether warning level release logging is enabled.
 */
#define LogRelIsWarnEnabled()  LogRelIsItEnabled(RTLOGGRPFLAGS_FLOW, LOG_GROUP)
/** @} */


/** @def LogRelIt
 * Write to specific logger if group enabled.
 */
/** @def LogRelItLikely
 * Write to specific logger if group enabled, assuming it likely it is enabled.
 */
/** @def LogRelMaxIt
 * Write to specific logger if group enabled and at less than a_cMax messages
 * have hit the log.  Uses a static variable to count.
 */
#ifdef RTLOG_REL_ENABLED
# if defined(LOG_USE_C99)
#  define _LogRelRemoveParentheseis(...)                    __VA_ARGS__
#  define _LogRelIt(a_fFlags, a_iGroup, ...) \
    do \
    { \
        PRTLOGGER LogRelIt_pLogger = RTLogRelGetDefaultInstanceExWeak(RT_MAKE_U32(a_fFlags, a_iGroup)); \
        if (RT_LIKELY(!LogRelIt_pLogger)) \
        { /* likely */ } \
        else \
            RTLogLoggerExWeak(LogRelIt_pLogger, a_fFlags, a_iGroup, __VA_ARGS__); \
        _LogIt(a_fFlags, a_iGroup, __VA_ARGS__); \
    } while (0)
#  define LogRelIt(a_fFlags, a_iGroup, fmtargs) \
    _LogRelIt(a_fFlags, a_iGroup, _LogRelRemoveParentheseis fmtargs)
#  define _LogRelItLikely(a_fFlags, a_iGroup, ...) \
    do \
    { \
        PRTLOGGER LogRelIt_pLogger = RTLogRelGetDefaultInstanceExWeak(RT_MAKE_U32(a_fFlags, a_iGroup)); \
        if (LogRelIt_pLogger) \
            RTLogLoggerExWeak(LogRelIt_pLogger, a_fFlags, a_iGroup, __VA_ARGS__); \
        _LogIt(a_fFlags, a_iGroup, __VA_ARGS__); \
    } while (0)
#  define LogRelItLikely(a_fFlags, a_iGroup, fmtargs) \
    _LogRelItLikely(a_fFlags, a_iGroup, _LogRelRemoveParentheseis fmtargs)
#  define _LogRelMaxIt(a_cMax, a_fFlags, a_iGroup, ...) \
    do \
    { \
        PRTLOGGER LogRelIt_pLogger = RTLogRelGetDefaultInstanceExWeak(RT_MAKE_U32(a_fFlags, a_iGroup)); \
        if (LogRelIt_pLogger) \
        { \
            static uint32_t s_LogRelMaxIt_cLogged = 0; \
            if (s_LogRelMaxIt_cLogged < (a_cMax)) \
            { \
                s_LogRelMaxIt_cLogged++; \
                RTLogLoggerExWeak(LogRelIt_pLogger, a_fFlags, a_iGroup, __VA_ARGS__); \
            } \
        } \
        _LogIt(a_fFlags, a_iGroup, __VA_ARGS__); \
    } while (0)
#  define LogRelMaxIt(a_cMax, a_fFlags, a_iGroup, fmtargs) \
    _LogRelMaxIt(a_cMax, a_fFlags, a_iGroup, _LogRelRemoveParentheseis fmtargs)
# else
#  define LogRelItLikely(a_fFlags, a_iGroup, fmtargs) \
   do \
   { \
       PRTLOGGER LogRelIt_pLogger = RTLogRelGetDefaultInstanceExWeak(RT_MAKE_U32(a_fFlags, a_iGroup)); \
       if (LogRelIt_pLogger) \
       { \
           LogRelIt_pLogger->pfnLogger fmtargs; \
       } \
       LogIt(a_fFlags, a_iGroup, fmtargs); \
  } while (0)
#  define LogRelIt(a_fFlags, a_iGroup, fmtargs) \
   do \
   { \
       PRTLOGGER LogRelIt_pLogger = RTLogRelGetDefaultInstanceExWeak(RT_MAKE_U32(a_fFlags, a_iGroup)); \
       if (RT_LIKELY(!LogRelIt_pLogger)) \
       { /* likely */ } \
       else \
       { \
           LogRelIt_pLogger->pfnLogger fmtargs; \
       } \
       LogIt(a_fFlags, a_iGroup, fmtargs); \
  } while (0)
#  define LogRelMaxIt(a_cMax, a_fFlags, a_iGroup, fmtargs) \
   do \
   { \
       PRTLOGGER LogRelIt_pLogger = RTLogRelGetDefaultInstanceExWeak(RT_MAKE_U32(a_fFlags, a_iGroup)); \
       if (LogRelIt_pLogger) \
       { \
           static uint32_t s_LogRelMaxIt_cLogged = 0; \
           if (s_LogRelMaxIt_cLogged < (a_cMax)) \
           { \
               s_LogRelMaxIt_cLogged++; \
               LogRelIt_pLogger->pfnLogger fmtargs; \
           } \
       } \
       LogIt(a_fFlags, a_iGroup, fmtargs); \
  } while (0)
# endif
#else   /* !RTLOG_REL_ENABLED */
# define LogRelIt(a_fFlags, a_iGroup, fmtargs)              do { } while (0)
# define LogRelItLikely(a_fFlags, a_iGroup, fmtargs)        do { } while (0)
# define LogRelMaxIt(a_cMax, a_fFlags, a_iGroup, fmtargs)   do { } while (0)
# if defined(LOG_USE_C99)
#  define _LogRelRemoveParentheseis(...)                    __VA_ARGS__
#  define _LogRelIt(a_fFlags, a_iGroup, ...)                do { } while (0)
#  define _LogRelItLikely(a_fFlags, a_iGroup, ...)          do { } while (0)
#  define _LogRelMaxIt(a_cMax, a_fFlags, a_iGroup, ...)     do { } while (0)
# endif
#endif  /* !RTLOG_REL_ENABLED */


/** @name Basic release logging macros
 * @{ */
/** @def LogRel
 * Level 1 release logging.
 */
#define LogRel(a)           LogRelItLikely(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, a)

/** @def LogRel2
 * Level 2 release logging.
 */
#define LogRel2(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_2,  LOG_GROUP, a)

/** @def LogRel3
 * Level 3 release logging.
 */
#define LogRel3(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_3,  LOG_GROUP, a)

/** @def LogRel4
 * Level 4 release logging.
 */
#define LogRel4(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_4,  LOG_GROUP, a)

/** @def LogRel5
 * Level 5 release logging.
 */
#define LogRel5(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_5,  LOG_GROUP, a)

/** @def LogRel6
 * Level 6 release logging.
 */
#define LogRel6(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_6,  LOG_GROUP, a)

/** @def LogRel7
 * Level 7 release logging.
 */
#define LogRel7(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_7,  LOG_GROUP, a)

/** @def LogRel8
 * Level 8 release logging.
 */
#define LogRel8(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_8,  LOG_GROUP, a)

/** @def LogRel9
 * Level 9 release logging.
 */
#define LogRel9(a)          LogRelIt(RTLOGGRPFLAGS_LEVEL_9,  LOG_GROUP, a)

/** @def LogRel10
 * Level 10 release logging.
 */
#define LogRel10(a)         LogRelIt(RTLOGGRPFLAGS_LEVEL_10, LOG_GROUP, a)

/** @def LogRel11
 * Level 11 release logging.
 */
#define LogRel11(a)         LogRelIt(RTLOGGRPFLAGS_LEVEL_11, LOG_GROUP, a)

/** @def LogRel12
 * Level 12 release logging.
 */
#define LogRel12(a)         LogRelIt(RTLOGGRPFLAGS_LEVEL_12, LOG_GROUP, a)

/** @def LogRelFlow
 * Logging of execution flow.
 */
#define LogRelFlow(a)       LogRelIt(RTLOGGRPFLAGS_FLOW,     LOG_GROUP, a)

/** @def LogRelWarn
 * Warning level release logging.
 */
#define LogRelWarn(a)       LogRelIt(RTLOGGRPFLAGS_WARN,     LOG_GROUP, a)
/** @} */



/** @name Basic release logging macros with local max
 * @{ */
/** @def LogRelMax
 * Level 1 release logging with a max number of log entries.
 */
#define LogRelMax(a_cMax, a)        LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, a)

/** @def LogRelMax2
 * Level 2 release logging with a max number of log entries.
 */
#define LogRelMax2(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_2,  LOG_GROUP, a)

/** @def LogRelMax3
 * Level 3 release logging with a max number of log entries.
 */
#define LogRelMax3(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_3,  LOG_GROUP, a)

/** @def LogRelMax4
 * Level 4 release logging with a max number of log entries.
 */
#define LogRelMax4(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_4,  LOG_GROUP, a)

/** @def LogRelMax5
 * Level 5 release logging with a max number of log entries.
 */
#define LogRelMax5(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_5,  LOG_GROUP, a)

/** @def LogRelMax6
 * Level 6 release logging with a max number of log entries.
 */
#define LogRelMax6(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_6,  LOG_GROUP, a)

/** @def LogRelMax7
 * Level 7 release logging with a max number of log entries.
 */
#define LogRelMax7(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_7,  LOG_GROUP, a)

/** @def LogRelMax8
 * Level 8 release logging with a max number of log entries.
 */
#define LogRelMax8(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_8,  LOG_GROUP, a)

/** @def LogRelMax9
 * Level 9 release logging with a max number of log entries.
 */
#define LogRelMax9(a_cMax, a)       LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_9,  LOG_GROUP, a)

/** @def LogRelMax10
 * Level 10 release logging with a max number of log entries.
 */
#define LogRelMax10(a_cMax, a)      LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_10, LOG_GROUP, a)

/** @def LogRelMax11
 * Level 11 release logging with a max number of log entries.
 */
#define LogRelMax11(a_cMax, a)      LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_11, LOG_GROUP, a)

/** @def LogRelMax12
 * Level 12 release logging with a max number of log entries.
 */
#define LogRelMax12(a_cMax, a)      LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_12, LOG_GROUP, a)

/** @def LogRelMaxFlow
 * Logging of execution flow with a max number of log entries.
 */
#define LogRelMaxFlow(a_cMax, a)    LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_FLOW,     LOG_GROUP, a)
/** @} */


/** @name Release logging macros prefixing the current function name.
 * @{ */

/** @def LogRelFunc
 * Release logging.  Prepends the given log message with the function name
 * followed by a semicolon and space.
 */
#ifdef LOG_USE_C99
# define LogRelFunc(a) \
    _LogRelItLikely(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogRelFunc(a)      do { LogRel((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogRel(a); } while (0)
#endif

/** @def LogRelFlowFunc
 * Release logging.  Macro to log the execution flow inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by
 * a semicolon and space.
 *
 * @param   a   Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogRelFlowFunc(a)  _LogRelIt(RTLOGGRPFLAGS_FLOW, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogRelFlowFunc(a)  do { LogRelFlow((LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogRelFlow(a); } while (0)
#endif

/** @def LogRelMaxFunc
 * Release logging.  Prepends the given log message with the function name
 * followed by a semicolon and space.
 */
#ifdef LOG_USE_C99
# define LogRelMaxFunc(a_cMax, a) \
    _LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogRelMaxFunc(a_cMax, a) \
    do { LogRelMax(a_cMax, (LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogRelMax(a_cMax, a); } while (0)
#endif

/** @def LogRelMaxFlowFunc
 * Release logging.  Macro to log the execution flow inside C/C++ functions.
 *
 * Prepends the given log message with the function name followed by
 * a semicolon and space.
 *
 * @param   a_cMax  Max number of times this should hit the log.
 * @param   a       Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogRelMaxFlowFunc(a_cMax, a) \
    _LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_FLOW, LOG_GROUP, LOG_FN_FMT ": %M", RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogRelMaxFlowFunc(a_cMax, a) \
    do { LogRelMaxFlow(a_cMax, (LOG_FN_FMT ": ", RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogRelFlow(a_cMax, a); } while (0)
#endif

/** @} */


/** @name Release Logging macros prefixing the this pointer value and method name.
 * @{ */

/** @def LogRelThisFunc
 * The same as LogRelFunc but for class functions (methods): the resulting log
 * line is additionally prepended with a hex value of |this| pointer.
 */
#ifdef LOG_USE_C99
# define LogRelThisFunc(a) \
    _LogRelItLikely(RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogRelThisFunc(a) \
    do { LogRel(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogRel(a); } while (0)
#endif

/** @def LogRelMaxThisFunc
 * The same as LogRelFunc but for class functions (methods): the resulting log
 * line is additionally prepended with a hex value of |this| pointer.
 * @param   a_cMax  Max number of times this should hit the log.
 * @param   a       Log message in format <tt>("string\n" [, args])</tt>.
 */
#ifdef LOG_USE_C99
# define LogRelMaxThisFunc(a_cMax, a) \
    _LogRelMaxIt(a_cMax, RTLOGGRPFLAGS_LEVEL_1, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogRelMaxThisFunc(a_cMax, a) \
    do { LogRelMax(a_cMax, ("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogRelMax(a_cMax, a); } while (0)
#endif

/** @def LogRelFlowThisFunc
 * The same as LogRelFlowFunc but for class functions (methods): the resulting
 * log line is additionally prepended with a hex value of |this| pointer.
 */
#ifdef LOG_USE_C99
# define LogRelFlowThisFunc(a) \
    _LogRelIt(RTLOGGRPFLAGS_FLOW, LOG_GROUP, "{%p} " LOG_FN_FMT ": %M", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__, _LogRemoveParentheseis a )
#else
# define LogRelFlowThisFunc(a) do { LogRelFlow(("{%p} " LOG_FN_FMT ": ", this, RT_GCC_EXTENSION __PRETTY_FUNCTION__)); LogRelFlow(a); } while (0)
#endif


/** Shortcut to |LogRelFlowFunc ("ENTER\n")|, marks the beginnig of the function. */
#define LogRelFlowFuncEnter()      LogRelFlowFunc(("ENTER\n"))

/** Shortcut to |LogRelFlowFunc ("LEAVE\n")|, marks the end of the function. */
#define LogRelFlowFuncLeave()      LogRelFlowFunc(("LEAVE\n"))

/** Shortcut to |LogRelFlowFunc ("LEAVE: %Rrc\n")|, marks the end of the function. */
#define LogRelFlowFuncLeaveRC(rc)  LogRelFlowFunc(("LEAVE: %Rrc\n", (rc)))

/** Shortcut to |LogRelFlowThisFunc ("ENTER\n")|, marks the beginnig of the function. */
#define LogRelFlowThisFuncEnter()  LogRelFlowThisFunc(("ENTER\n"))

/** Shortcut to |LogRelFlowThisFunc ("LEAVE\n")|, marks the end of the function. */
#define LogRelFlowThisFuncLeave()  LogRelFlowThisFunc(("LEAVE\n"))

/** @} */


/**
 * Sets the default release logger instance.
 *
 * @returns The old default instance.
 * @param   pLogger     The new default release logger instance.
 */
RTDECL(PRTLOGGER) RTLogRelSetDefaultInstance(PRTLOGGER pLogger);

/**
 * Gets the default release logger instance.
 *
 * @returns Pointer to default release logger instance if availble, otherwise NULL.
 */
RTDECL(PRTLOGGER) RTLogRelGetDefaultInstance(void);

/** @copydoc RTLogRelGetDefaultInstance   */
typedef DECLCALLBACKTYPE(PRTLOGGER, FNLOGRELGETDEFAULTINSTANCE,(void));
/** Pointer to RTLogRelGetDefaultInstance. */
typedef FNLOGRELGETDEFAULTINSTANCE *PFNLOGRELGETDEFAULTINSTANCE;

/** "Weak symbol" emulation for RTLogRelGetDefaultInstance.
 * @note This is first set when RTLogRelSetDefaultInstance is called. */
extern RTDATADECL(PFNLOGRELGETDEFAULTINSTANCE) g_pfnRTLogRelGetDefaultInstance;

/** "Weak symbol" wrapper for RTLogRelGetDefaultInstance. */
DECL_FORCE_INLINE(PRTLOGGER) RTLogRelGetDefaultInstanceWeak(void)
{
#if defined(IN_RING3) && (defined(IN_RT_STATIC) || defined(IPRT_NO_CRT))
    if (g_pfnRTLogRelGetDefaultInstance)
        return g_pfnRTLogRelGetDefaultInstance();
    return NULL;
#else
    return RTLogRelGetDefaultInstance();
#endif
}

/**
 * Gets the default release logger instance.
 *
 * @returns Pointer to default release logger instance if availble, otherwise NULL.
 * @param   fFlagsAndGroup  The flags in the lower 16 bits, the group number in
 *                          the high 16 bits.
 */
RTDECL(PRTLOGGER) RTLogRelGetDefaultInstanceEx(uint32_t fFlagsAndGroup);

/** @copydoc RTLogRelGetDefaultInstanceEx   */
typedef DECLCALLBACKTYPE(PRTLOGGER, FNLOGRELGETDEFAULTINSTANCEEX,(uint32_t fFlagsAndGroup));
/** Pointer to RTLogRelGetDefaultInstanceEx. */
typedef FNLOGRELGETDEFAULTINSTANCEEX *PFNLOGRELGETDEFAULTINSTANCEEX;

/** "Weak symbol" emulation for RTLogRelGetDefaultInstanceEx.
 * @note This is first set when RTLogRelSetDefaultInstance is called. */
extern RTDATADECL(PFNLOGRELGETDEFAULTINSTANCEEX) g_pfnRTLogRelGetDefaultInstanceEx;

/** "Weak symbol" wrapper for RTLogRelGetDefaultInstanceEx. */
DECL_FORCE_INLINE(PRTLOGGER) RTLogRelGetDefaultInstanceExWeak(uint32_t fFlagsAndGroup)
{
#if defined(IN_RING3) && (defined(IN_RT_STATIC) || defined(IPRT_NO_CRT))
    if (g_pfnRTLogRelGetDefaultInstanceEx)
        return g_pfnRTLogRelGetDefaultInstanceEx(fFlagsAndGroup);
    return NULL;
#else
    return RTLogRelGetDefaultInstanceEx(fFlagsAndGroup);
#endif
}


/**
 * Write to a logger instance, defaulting to the release one.
 *
 * This function will check whether the instance, group and flags makes up a
 * logging kind which is currently enabled before writing anything to the log.
 *
 * @param   pLogger     Pointer to logger instance.
 * @param   fFlags      The logging flags.
 * @param   iGroup      The group.
 *                      The value ~0U is reserved for compatibility with RTLogLogger[V] and is
 *                      only for internal usage!
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 * @remark  This is a worker function for LogRelIt.
 */
RTDECL(void) RTLogRelLogger(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup,
                            const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(4, 5);

/**
 * Write to a logger instance, defaulting to the release one.
 *
 * This function will check whether the instance, group and flags makes up a
 * logging kind which is currently enabled before writing anything to the log.
 *
 * @param   pLogger     Pointer to logger instance. If NULL the default release instance is attempted.
 * @param   fFlags      The logging flags.
 * @param   iGroup      The group.
 *                      The value ~0U is reserved for compatibility with RTLogLogger[V] and is
 *                      only for internal usage!
 * @param   pszFormat   Format string.
 * @param   args        Format arguments.
 */
RTDECL(void) RTLogRelLoggerV(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup,
                             const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(4, 0);

/**
 * printf like function for writing to the default release log.
 *
 * @param   pszFormat   Printf like format string.
 * @param   ...         Optional arguments as specified in pszFormat.
 *
 * @remark The API doesn't support formatting of floating point numbers at the moment.
 */
RTDECL(void) RTLogRelPrintf(const char *pszFormat, ...)  RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * vprintf like function for writing to the default release log.
 *
 * @param   pszFormat   Printf like format string.
 * @param   args        Optional arguments as specified in pszFormat.
 *
 * @remark The API doesn't support formatting of floating point numbers at the moment.
 */
RTDECL(void) RTLogRelPrintfV(const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Changes the buffering setting of the default release logger.
 *
 * This can be used for optimizing longish logging sequences.
 *
 * @returns The old state.
 * @param   fBuffered       The new state.
 */
RTDECL(bool) RTLogRelSetBuffering(bool fBuffered);

/** @} */



/** @name COM port logging
 * @{
 */

#ifdef DOXYGEN_RUNNING
# define LOG_TO_COM
# define LOG_NO_COM
#endif

/** @def LOG_TO_COM
 * Redirects the normal logging macros to the serial versions.
 */

/** @def LOG_NO_COM
 * Disables all LogCom* macros.
 */

/** @def LogCom
 * Generic logging to serial port.
 */
#if defined(LOG_ENABLED) && !defined(LOG_NO_COM)
# define LogCom(a) RTLogComPrintf a
#else
# define LogCom(a) do { } while (0)
#endif

/** @def LogComFlow
 * Logging to serial port of execution flow.
 */
#if defined(LOG_ENABLED) && defined(LOG_ENABLE_FLOW) && !defined(LOG_NO_COM)
# define LogComFlow(a) RTLogComPrintf a
#else
# define LogComFlow(a) do { } while (0)
#endif

#ifdef LOG_TO_COM
# undef Log
# define Log(a)             LogCom(a)
# undef LogFlow
# define LogFlow(a)         LogComFlow(a)
#endif

/** @} */


/** @name Backdoor Logging
 * @{
 */

#ifdef DOXYGEN_RUNNING
# define LOG_TO_BACKDOOR
# define LOG_NO_BACKDOOR
#endif

/** @def LOG_TO_BACKDOOR
 * Redirects the normal logging macros to the backdoor versions.
 */

/** @def LOG_NO_BACKDOOR
 * Disables all LogBackdoor* macros.
 */

/** @def LogBackdoor
 * Generic logging to the VBox backdoor via port I/O.
 */
#if defined(LOG_ENABLED) && !defined(LOG_NO_BACKDOOR)
# define LogBackdoor(a)     RTLogBackdoorPrintf a
#else
# define LogBackdoor(a)     do { } while (0)
#endif

/** @def LogBackdoorFlow
 * Logging of execution flow messages to the backdoor I/O port.
 */
#if defined(LOG_ENABLED) && !defined(LOG_NO_BACKDOOR)
# define LogBackdoorFlow(a) RTLogBackdoorPrintf a
#else
# define LogBackdoorFlow(a) do { } while (0)
#endif

/** @def LogRelBackdoor
 * Release logging to the VBox backdoor via port I/O.
 */
#if !defined(LOG_NO_BACKDOOR)
# define LogRelBackdoor(a)  RTLogBackdoorPrintf a
#else
# define LogRelBackdoor(a)  do { } while (0)
#endif

#ifdef LOG_TO_BACKDOOR
# undef Log
# define Log(a)         LogBackdoor(a)
# undef LogFlow
# define LogFlow(a)     LogBackdoorFlow(a)
# undef LogRel
# define LogRel(a)      LogRelBackdoor(a)
# if defined(LOG_USE_C99)
#  undef _LogIt
#  define _LogIt(a_fFlags, a_iGroup, ...)  LogBackdoor((__VA_ARGS__))
# endif
#endif

/** @} */



/**
 * Gets the default logger instance, creating it if necessary.
 *
 * @returns Pointer to default logger instance if availble, otherwise NULL.
 */
RTDECL(PRTLOGGER)   RTLogDefaultInstance(void);

/**
 * Gets the logger instance if enabled, creating it if necessary.
 *
 * @returns Pointer to default logger instance, if group has the specified
 *          flags enabled.  Otherwise NULL is returned.
 * @param   fFlagsAndGroup  The flags in the lower 16 bits, the group number in
 *                          the high 16 bits.
 */
RTDECL(PRTLOGGER)   RTLogDefaultInstanceEx(uint32_t fFlagsAndGroup);

/**
 * Gets the default logger instance (does not create one).
 *
 * @returns Pointer to default logger instance if availble, otherwise NULL.
 */
RTDECL(PRTLOGGER)   RTLogGetDefaultInstance(void);

/** @copydoc RTLogGetDefaultInstance   */
typedef DECLCALLBACKTYPE(PRTLOGGER, FNLOGGETDEFAULTINSTANCE,(void));
/** Pointer to RTLogGetDefaultInstance. */
typedef FNLOGGETDEFAULTINSTANCE *PFNLOGGETDEFAULTINSTANCE;

/** "Weak symbol" emulation for RTLogGetDefaultInstance.
 * @note This is first set when RTLogSetDefaultInstance is called. */
extern RTDATADECL(PFNLOGGETDEFAULTINSTANCE) g_pfnRTLogGetDefaultInstance;

/** "Weak symbol" wrapper for RTLogGetDefaultInstance. */
DECL_FORCE_INLINE(PRTLOGGER) RTLogGetDefaultInstanceWeak(void)
{
#if defined(IN_RING3) && (defined(IN_RT_STATIC) || defined(IPRT_NO_CRT))
    if (g_pfnRTLogGetDefaultInstance)
        return g_pfnRTLogGetDefaultInstance();
    return NULL;
#else
    return RTLogGetDefaultInstance();
#endif
}

/**
 * Gets the default logger instance if enabled (does not create one).
 *
 * @returns Pointer to default logger instance, if group has the specified
 *          flags enabled.  Otherwise NULL is returned.
 * @param   fFlagsAndGroup  The flags in the lower 16 bits, the group number in
 *                          the high 16 bits.
 */
RTDECL(PRTLOGGER)   RTLogGetDefaultInstanceEx(uint32_t fFlagsAndGroup);

/** @copydoc RTLogGetDefaultInstanceEx   */
typedef DECLCALLBACKTYPE(PRTLOGGER, FNLOGGETDEFAULTINSTANCEEX,(uint32_t fFlagsAndGroup));
/** Pointer to RTLogGetDefaultInstanceEx. */
typedef FNLOGGETDEFAULTINSTANCEEX *PFNLOGGETDEFAULTINSTANCEEX;

/** "Weak symbol" emulation for RTLogGetDefaultInstanceEx.
 * @note This is first set when RTLogSetDefaultInstance is called. */
extern RTDATADECL(PFNLOGGETDEFAULTINSTANCEEX) g_pfnRTLogGetDefaultInstanceEx;

/** "Weak symbol" wrapper for RTLogGetDefaultInstanceEx. */
DECL_FORCE_INLINE(PRTLOGGER) RTLogGetDefaultInstanceExWeak(uint32_t fFlagsAndGroup)
{
#if defined(IN_RING3) && (defined(IN_RT_STATIC) || defined(IPRT_NO_CRT))
    if (g_pfnRTLogGetDefaultInstanceEx)
        return g_pfnRTLogGetDefaultInstanceEx(fFlagsAndGroup);
    return NULL;
#else
    return RTLogGetDefaultInstanceEx(fFlagsAndGroup);
#endif
}

/**
 * Sets the default logger instance.
 *
 * @returns The old default instance.
 * @param   pLogger     The new default logger instance.
 */
RTDECL(PRTLOGGER)   RTLogSetDefaultInstance(PRTLOGGER pLogger);

#ifdef IN_RING0
/**
 * Changes the default logger instance for the current thread.
 *
 * @returns IPRT status code.
 * @param   pLogger     The logger instance. Pass NULL for deregistration.
 * @param   uKey        Associated key for cleanup purposes. If pLogger is NULL,
 *                      all instances with this key will be deregistered. So in
 *                      order to only deregister the instance associated with the
 *                      current thread use 0.
 */
RTR0DECL(int)       RTLogSetDefaultInstanceThread(PRTLOGGER pLogger, uintptr_t uKey);
#endif /* IN_RING0 */

/**
 * Creates the default logger instance for IPRT users.
 *
 * Any user of the logging features will need to implement
 * this or use the generic dummy.
 *
 * @returns Pointer to the logger instance.
 */
RTDECL(PRTLOGGER)   RTLogDefaultInit(void);

/**
 * This is the 2nd half of what RTLogGetDefaultInstanceEx() and
 * RTLogRelGetDefaultInstanceEx() does.
 *
 * @returns If the group has the specified flags enabled @a pLogger will be
 *          returned returned.  Otherwise NULL is returned.
 * @param   pLogger         The logger.  NULL is NULL.
 * @param   fFlagsAndGroup  The flags in the lower 16 bits, the group number in
 *                          the high 16 bits.
 */
RTDECL(PRTLOGGER)   RTLogCheckGroupFlags(PRTLOGGER pLogger, uint32_t fFlagsAndGroup);

/**
 * Create a logger instance.
 *
 * @returns iprt status code.
 *
 * @param   ppLogger            Where to store the logger instance.
 * @param   fFlags              Logger instance flags, a combination of the
 *                              RTLOGFLAGS_* values.
 * @param   pszGroupSettings    The initial group settings.
 * @param   pszEnvVarBase       Base name for the environment variables for
 *                              this instance.
 * @param   cGroups             Number of groups in the array.
 * @param   papszGroups         Pointer to array of groups.  This must stick
 *                              around for the life of the logger instance.
 * @param   fDestFlags          The destination flags.  RTLOGDEST_FILE is ORed
 *                              if pszFilenameFmt specified.
 * @param   pszFilenameFmt      Log filename format string.  Standard
 *                              RTStrFormat().
 * @param   ...                 Format arguments.
 */
RTDECL(int) RTLogCreate(PRTLOGGER *ppLogger, uint64_t fFlags, const char *pszGroupSettings,
                        const char *pszEnvVarBase, unsigned cGroups, const char * const * papszGroups,
                        uint32_t fDestFlags, const char *pszFilenameFmt, ...) RT_IPRT_FORMAT_ATTR_MAYBE_NULL(8, 9);

/**
 * Create a logger instance.
 *
 * @returns iprt status code.
 *
 * @param   ppLogger            Where to store the logger instance.
 * @param   pszEnvVarBase       Base name for the environment variables for
 *                              this instance (ring-3 only).
 * @param   fFlags              Logger instance flags, a combination of the
 *                              RTLOGFLAGS_* values.
 * @param   pszGroupSettings    The initial group settings.
 * @param   cGroups             Number of groups in the array.
 * @param   papszGroups         Pointer to array of groups.  This must stick
 *                              around for the life of the logger instance.
 * @param   cMaxEntriesPerGroup The max number of entries per group.  UINT32_MAX
 *                              or zero for unlimited.
 * @param   cBufDescs           Number of buffer descriptors that @a paBufDescs
 *                              points to. Zero for defaults.
 * @param   paBufDescs          Buffer descriptors, optional.
 * @param   fDestFlags          The destination flags.  RTLOGDEST_FILE is ORed
 *                              if pszFilenameFmt specified.
 * @param   pfnPhase            Callback function for starting logging and for
 *                              ending or starting a new file for log history
 *                              rotation.  NULL is OK.
 * @param   cHistory            Number of old log files to keep when performing
 *                              log history rotation.  0 means no history.
 * @param   cbHistoryFileMax    Maximum size of log file when performing
 *                              history rotation. 0 means no size limit.
 * @param   cSecsHistoryTimeSlot Maximum time interval per log file when
 *                              performing history rotation, in seconds.
 *                              0 means time limit.
 * @param   pOutputIf           The optional file output interface, can be NULL which will
 *                              make use of the default one.
 * @param   pvOutputIfUser      The opaque user data to pass to the callbacks in the output interface.
 * @param   pErrInfo            Where to return extended error information.
 *                              Optional.
 * @param   pszFilenameFmt      Log filename format string. Standard RTStrFormat().
 * @param   ...                 Format arguments.
 */
RTDECL(int) RTLogCreateEx(PRTLOGGER *ppLogger, const char *pszEnvVarBase, uint64_t fFlags, const char *pszGroupSettings,
                          unsigned cGroups, const char * const *papszGroups, uint32_t cMaxEntriesPerGroup,
                          uint32_t cBufDescs, PRTLOGBUFFERDESC paBufDescs, uint32_t fDestFlags,
                          PFNRTLOGPHASE pfnPhase, uint32_t cHistory, uint64_t cbHistoryFileMax, uint32_t cSecsHistoryTimeSlot,
                          PCRTLOGOUTPUTIF pOutputIf, void *pvOutputIfUser,
                          PRTERRINFO pErrInfo, const char *pszFilenameFmt, ...) RT_IPRT_FORMAT_ATTR_MAYBE_NULL(18, 19);

/**
 * Create a logger instance.
 *
 * @returns iprt status code.
 *
 * @param   ppLogger            Where to store the logger instance.
 * @param   pszEnvVarBase       Base name for the environment variables for
 *                              this instance (ring-3 only).
 * @param   fFlags              Logger instance flags, a combination of the
 *                              RTLOGFLAGS_* values.
 * @param   pszGroupSettings    The initial group settings.
 * @param   cGroups             Number of groups in the array.
 * @param   papszGroups         Pointer to array of groups.  This must stick
 *                              around for the life of the logger instance.
 * @param   cMaxEntriesPerGroup The max number of entries per group.  UINT32_MAX
 *                              or zero for unlimited.
 * @param   cBufDescs           Number of buffer descriptors that @a paBufDescs
 *                              points to. Zero for defaults.
 * @param   paBufDescs          Buffer descriptors, optional.
 * @param   fDestFlags          The destination flags.  RTLOGDEST_FILE is ORed
 *                              if pszFilenameFmt specified.
 * @param   pfnPhase            Callback function for starting logging and for
 *                              ending or starting a new file for log history
 *                              rotation.
 * @param   cHistory            Number of old log files to keep when performing
 *                              log history rotation.  0 means no history.
 * @param   cbHistoryFileMax    Maximum size of log file when performing
 *                              history rotation.  0 means no size limit.
 * @param   cSecsHistoryTimeSlot  Maximum time interval per log file when
 *                              performing history rotation, in seconds.
 *                              0 means no time limit.
 * @param   pOutputIf           The optional file output interface, can be NULL which will
 *                              make use of the default one.
 * @param   pvOutputIfUser      The opaque user data to pass to the callbacks in the output interface.
 * @param   pErrInfo            Where to return extended error information.
 *                              Optional.
 * @param   pszFilenameFmt      Log filename format string.  Standard
 *                              RTStrFormat().
 * @param   va                  Format arguments.
 */
RTDECL(int) RTLogCreateExV(PRTLOGGER *ppLogger, const char *pszEnvVarBase, uint64_t fFlags, const char *pszGroupSettings,
                           uint32_t cGroups, const char * const *papszGroups, uint32_t cMaxEntriesPerGroup,
                           uint32_t cBufDescs, PRTLOGBUFFERDESC paBufDescs, uint32_t fDestFlags,
                           PFNRTLOGPHASE pfnPhase, uint32_t cHistory, uint64_t cbHistoryFileMax, uint32_t cSecsHistoryTimeSlot,
                           PCRTLOGOUTPUTIF pOutputIf, void *pvOutputIfUser,
                           PRTERRINFO pErrInfo, const char *pszFilenameFmt, va_list va) RT_IPRT_FORMAT_ATTR_MAYBE_NULL(18, 0);

/**
 * Destroys a logger instance.
 *
 * The instance is flushed and all output destinations closed (where applicable).
 *
 * @returns iprt status code.
 * @param   pLogger     The logger instance which close destroyed. NULL is fine.
 */
RTDECL(int) RTLogDestroy(PRTLOGGER pLogger);

/**
 * Sets the custom prefix callback.
 *
 * @returns IPRT status code.
 * @param   pLogger     The logger instance.
 * @param   pfnCallback The callback.
 * @param   pvUser      The user argument for the callback.
 *  */
RTDECL(int) RTLogSetCustomPrefixCallback(PRTLOGGER pLogger, PFNRTLOGPREFIX pfnCallback, void *pvUser);

/**
 * Sets the custom flush callback.
 *
 * This can be handy for special loggers like the per-EMT ones in ring-0,
 * but also for implementing a log viewer in the debugger GUI.
 *
 * @returns IPRT status code.
 * @retval  VWRN_ALREADY_EXISTS if it was set to a different flusher.
 * @param   pLogger         The logger instance.
 * @param   pfnFlush        The flush callback.
 */
RTDECL(int) RTLogSetFlushCallback(PRTLOGGER pLogger, PFNRTLOGFLUSH pfnFlush);

/**
 * Sets the thread name for a thread specific ring-0 logger.
 *
 * @returns IPRT status code.
 * @param   pLogger     The logger. NULL is not allowed.
 * @param   pszNameFmt  The format string for the thread name.
 * @param   ...         Format arguments.
 */
RTR0DECL(int) RTLogSetR0ThreadNameF(PRTLOGGER pLogger, const char *pszNameFmt, ...) RT_IPRT_FORMAT_ATTR(2, 3);

/**
 * Sets the thread name for a thread specific ring-0 logger.
 *
 * @returns IPRT status code.
 * @param   pLogger     The logger. NULL is not allowed.
 * @param   pszNameFmt  The format string for the thread name.
 * @param   va          Format arguments.
 */
RTR0DECL(int) RTLogSetR0ThreadNameV(PRTLOGGER pLogger, const char *pszNameFmt, va_list va) RT_IPRT_FORMAT_ATTR(2, 0);

/**
 * Sets the program start time for a thread specific ring-0 logger.
 *
 * @returns IPRT status code.
 * @param   pLogger     The logger. NULL is not allowed.
 * @param   nsStart     The RTTimeNanoTS() value at program start.
 */
RTR0DECL(int) RTLogSetR0ProgramStart(PRTLOGGER pLogger, uint64_t nsStart);

/**
 * Get the current log group settings as a string.
 *
 * @returns VINF_SUCCESS or VERR_BUFFER_OVERFLOW.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   pszBuf      The output buffer.
 * @param   cchBuf      The size of the output buffer. Must be greater than
 *                      zero.
 */
RTDECL(int) RTLogQueryGroupSettings(PRTLOGGER pLogger, char *pszBuf, size_t cchBuf);

/**
 * Updates the group settings for the logger instance using the specified
 * specification string.
 *
 * @returns iprt status code.
 *          Failures can safely be ignored.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   pszValue    Value to parse.
 */
RTDECL(int) RTLogGroupSettings(PRTLOGGER pLogger, const char *pszValue);

/**
 * Sets the max number of entries per group.
 *
 * @returns Old restriction.
 *
 * @param   pLogger             The logger instance (NULL is an alias for the
 *                              default logger).
 * @param   cMaxEntriesPerGroup The max number of entries per group.
 *
 * @remarks Lowering the limit of an active logger may quietly mute groups.
 *          Raising it may reactive already muted groups.
 */
RTDECL(uint32_t) RTLogSetGroupLimit(PRTLOGGER pLogger, uint32_t cMaxEntriesPerGroup);

/**
 * Gets the current flag settings for the given logger.
 *
 * @returns Logger flags, UINT64_MAX if no logger.
 * @param   pLogger     Logger instance (NULL for default logger).
 */
RTDECL(uint64_t) RTLogGetFlags(PRTLOGGER pLogger);

/**
 * Modifies the flag settings for the given logger.
 *
 * @returns IPRT status code.  Returns VINF_LOG_NO_LOGGER if no default logger
 *          and @a pLogger is NULL.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   fSet        Mask of flags to set (OR).
 * @param   fClear      Mask of flags to clear (NAND).  This is allowed to
 *                      include invalid flags - e.g. UINT64_MAX is okay.
 */
RTDECL(int) RTLogChangeFlags(PRTLOGGER pLogger, uint64_t fSet, uint64_t fClear);

/**
 * Updates the flags for the logger instance using the specified
 * specification string.
 *
 * @returns iprt status code.
 *          Failures can safely be ignored.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   pszValue    Value to parse.
 */
RTDECL(int) RTLogFlags(PRTLOGGER pLogger, const char *pszValue);

/**
 * Changes the buffering setting of the specified logger.
 *
 * This can be used for optimizing longish logging sequences.
 *
 * @returns The old state.
 * @param   pLogger     The logger instance (NULL is an alias for the default
 *                      logger).
 * @param   fBuffered   The new state.
 */
RTDECL(bool) RTLogSetBuffering(PRTLOGGER pLogger, bool fBuffered);

/**
 * Get the current log flags as a string.
 *
 * @returns VINF_SUCCESS or VERR_BUFFER_OVERFLOW.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   pszBuf      The output buffer.
 * @param   cchBuf      The size of the output buffer. Must be greater than
 *                      zero.
 */
RTDECL(int) RTLogQueryFlags(PRTLOGGER pLogger, char *pszBuf, size_t cchBuf);

/**
 * Gets the current destinations flags for the given logger.
 *
 * @returns Logger destination flags, UINT32_MAX if no logger.
 * @param   pLogger     Logger instance (NULL for default logger).
 */
RTDECL(uint32_t) RTLogGetDestinations(PRTLOGGER pLogger);

/**
 * Modifies the log destinations settings for the given logger.
 *
 * This is only suitable for simple destination settings that doesn't take
 * additional arguments, like RTLOGDEST_FILE.
 *
 * @returns IPRT status code.  Returns VINF_LOG_NO_LOGGER if no default logger
 *          and @a pLogger is NULL.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   fSet        Mask of destinations to set (OR).
 * @param   fClear      Mask of destinations to clear (NAND).
 */
RTDECL(int) RTLogChangeDestinations(PRTLOGGER pLogger, uint32_t fSet, uint32_t fClear);

/**
 * Updates the logger destination using the specified string.
 *
 * @returns VINF_SUCCESS or VERR_BUFFER_OVERFLOW.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   pszValue    The value to parse.
 */
RTDECL(int) RTLogDestinations(PRTLOGGER pLogger, char const *pszValue);

/**
 * Clear the file delay flag if set, opening the destination and flushing.
 *
 * @returns IPRT status code.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   pErrInfo    Where to return extended error info.  Optional.
 */
RTDECL(int) RTLogClearFileDelayFlag(PRTLOGGER pLogger, PRTERRINFO pErrInfo);

/**
 * Get the current log destinations as a string.
 *
 * @returns VINF_SUCCESS or VERR_BUFFER_OVERFLOW.
 * @param   pLogger     Logger instance (NULL for default logger).
 * @param   pszBuf      The output buffer.
 * @param   cchBuf      The size of the output buffer. Must be greater than 0.
 */
RTDECL(int) RTLogQueryDestinations(PRTLOGGER pLogger, char *pszBuf, size_t cchBuf);

/**
 * Performs a bulk update of logger flags and group flags.
 *
 * This is for instanced used for copying settings from ring-3 to ring-0
 * loggers.
 *
 * @returns IPRT status code.
 * @param   pLogger             The logger instance (NULL for default logger).
 * @param   fFlags              The new logger flags.
 * @param   uGroupCrc32         The CRC32 of the group name strings.
 * @param   cGroups             Number of groups.
 * @param   pafGroups           Array of group flags.
 * @sa      RTLogQueryBulk
 */
RTDECL(int) RTLogBulkUpdate(PRTLOGGER pLogger, uint64_t fFlags, uint32_t uGroupCrc32, uint32_t cGroups, uint32_t const *pafGroups);

/**
 * Queries data for a bulk update of logger flags and group flags.
 *
 * This is for instanced used for copying settings from ring-3 to ring-0
 * loggers.
 *
 * @returns IPRT status code.
 * @retval  VERR_BUFFER_OVERFLOW if pafGroups is too small, @a pcGroups will be
 *          set to the actual number of groups.
 * @param   pLogger             The logger instance (NULL for default logger).
 * @param   pfFlags             Where to return the logger flags.
 * @param   puGroupCrc32        Where to return the CRC32 of the group names.
 * @param   pcGroups            Input: Size of the @a pafGroups allocation.
 *                              Output: Actual number of groups returned.
 * @param   pafGroups           Where to return the flags for each group.
 * @sa      RTLogBulkUpdate
 */
RTDECL(int) RTLogQueryBulk(PRTLOGGER pLogger, uint64_t *pfFlags, uint32_t *puGroupCrc32, uint32_t *pcGroups, uint32_t *pafGroups);

/**
 * Write/copy bulk log data from another logger.
 *
 * This is used for transferring stuff from the ring-0 loggers and into the
 * ring-3 one.  The text goes in as-is w/o any processing (i.e. prefixing or
 * newline fun).
 *
 * @returns IRPT status code.
 * @param   pLogger             The logger instance (NULL for default logger).
 * @param   pszBefore           Text to log before the bulk text.  Optional.
 * @param   pch                 Pointer to the block of bulk log text to write.
 * @param   cch                 Size of the block of bulk log text to write.
 * @param   pszAfter            Text to log after the bulk text.  Optional.
 */
RTDECL(int) RTLogBulkWrite(PRTLOGGER pLogger, const char *pszBefore, const char *pch, size_t cch, const char *pszAfter);

/**
 * Write/copy bulk log data from a nested VM logger.
 *
 * This is used for
 *
 * @returns IRPT status code.
 * @param   pLogger             The logger instance (NULL for default logger).
 * @param   pch                 Pointer to the block of bulk log text to write.
 * @param   cch                 Size of the block of bulk log text to write.
 * @param   pszInfix            String to put after the line prefixes and the
 *                              line content.
 */
RTDECL(int) RTLogBulkNestedWrite(PRTLOGGER pLogger, const char *pch, size_t cch, const char *pszInfix);

/**
 * Flushes the specified logger.
 *
 * @returns IRPT status code.
 * @param   pLogger     The logger instance to flush.
 *                      If NULL the default instance is used. The default instance
 *                      will not be initialized by this call.
 */
RTDECL(int) RTLogFlush(PRTLOGGER pLogger);

/**
 * Write to a logger instance.
 *
 * @param   pLogger     Pointer to logger instance.
 * @param   pvCallerRet Ignored.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
RTDECL(void) RTLogLogger(PRTLOGGER pLogger, void *pvCallerRet, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);

/**
 * Write to a logger instance, weak version.
 *
 * @param   pLogger     Pointer to logger instance.
 * @param   pvCallerRet Ignored.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
#if defined(IN_RING3) && (defined(IN_RT_STATIC) || defined(IPRT_NO_CRT))
RTDECL(void) RTLogLoggerWeak(PRTLOGGER pLogger, void *pvCallerRet, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);
#else /* Cannot use a DECL_FORCE_INLINE because older GCC versions doesn't support inlining va_start. */
# undef  RTLogLoggerWeak /* in case of mangling */
# define RTLogLoggerWeak RTLogLogger
#endif

/**
 * Write to a logger instance.
 *
 * @param   pLogger     Pointer to logger instance.
 * @param   pszFormat   Format string.
 * @param   args        Format arguments.
 */
RTDECL(void) RTLogLoggerV(PRTLOGGER pLogger, const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(2, 0);

/**
 * Write to a logger instance.
 *
 * This function will check whether the instance, group and flags makes up a
 * logging kind which is currently enabled before writing anything to the log.
 *
 * @param   pLogger     Pointer to logger instance. If NULL the default logger instance will be attempted.
 * @param   fFlags      The logging flags.
 * @param   iGroup      The group.
 *                      The value ~0U is reserved for compatibility with RTLogLogger[V] and is
 *                      only for internal usage!
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 * @remark  This is a worker function of LogIt.
 */
RTDECL(void) RTLogLoggerEx(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup,
                           const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(4, 5);

/**
 * Write to a logger instance, weak version.
 *
 * This function will check whether the instance, group and flags makes up a
 * logging kind which is currently enabled before writing anything to the log.
 *
 * @param   pLogger     Pointer to logger instance. If NULL the default logger instance will be attempted.
 * @param   fFlags      The logging flags.
 * @param   iGroup      The group.
 *                      The value ~0U is reserved for compatibility with RTLogLogger[V] and is
 *                      only for internal usage!
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 * @remark  This is a worker function of LogIt.
 */
#if defined(IN_RING3) && (defined(IN_RT_STATIC) || defined(IPRT_NO_CRT))
RTDECL(void) RTLogLoggerExWeak(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup,
                               const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(4, 5);
#else /* Cannot use a DECL_FORCE_INLINE because older GCC versions doesn't support inlining va_start. */
# undef  RTLogLoggerExWeak /* in case of mangling */
# define RTLogLoggerExWeak RTLogLoggerEx
#endif

/**
 * Write to a logger instance.
 *
 * This function will check whether the instance, group and flags makes up a
 * logging kind which is currently enabled before writing anything to the log.
 *
 * @returns VINF_SUCCESS, VINF_LOG_NO_LOGGER, VINF_LOG_DISABLED, or IPRT error
 *          status.
 * @param   pLogger     Pointer to logger instance. If NULL the default logger instance will be attempted.
 * @param   fFlags      The logging flags.
 * @param   iGroup      The group.
 *                      The value ~0U is reserved for compatibility with RTLogLogger[V] and is
 *                      only for internal usage!
 * @param   pszFormat   Format string.
 * @param   args        Format arguments.
 */
RTDECL(int) RTLogLoggerExV(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup,
                           const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(4, 0);

/** @copydoc RTLogLoggerExV */
typedef DECLCALLBACKTYPE(int, FNRTLOGLOGGEREXV,(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup,
                                                const char *pszFormat, va_list args)) RT_IPRT_FORMAT_ATTR(4, 0);
/** Pointer to RTLogLoggerExV. */
typedef FNRTLOGLOGGEREXV *PFNRTLOGLOGGEREXV;
/** "Weak symbol" emulation for RTLogLoggerExV.
 * @note This is first set when RTLogCreateEx or RTLogCreate is called. */
extern RTDATADECL(PFNRTLOGLOGGEREXV)  g_pfnRTLogLoggerExV;

/** "Weak symbol" wrapper for RTLogLoggerExV. */
DECL_FORCE_INLINE(int) RTLogLoggerExVWeak(PRTLOGGER pLogger, unsigned fFlags, unsigned iGroup,
                                          const char *pszFormat, va_list args) /* RT_IPRT_FORMAT_ATTR(4, 0) */
{
#if defined(IN_RING3) && (defined(IN_RT_STATIC) || defined(IPRT_NO_CRT))
    if (g_pfnRTLogLoggerExV)
        return g_pfnRTLogLoggerExV(pLogger, fFlags, iGroup, pszFormat, args);
    return 22301; /* VINF_LOG_DISABLED, don't want err.h dependency here. */
#else
    return RTLogLoggerExV(pLogger, fFlags, iGroup, pszFormat, args);
#endif
}

/**
 * printf like function for writing to the default log.
 *
 * @param   pszFormat   Printf like format string.
 * @param   ...         Optional arguments as specified in pszFormat.
 *
 * @remark The API doesn't support formatting of floating point numbers at the moment.
 */
RTDECL(void) RTLogPrintf(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * vprintf like function for writing to the default log.
 *
 * @param   pszFormat   Printf like format string.
 * @param   va          Optional arguments as specified in pszFormat.
 *
 * @remark The API doesn't support formatting of floating point numbers at the moment.
 */
RTDECL(void) RTLogPrintfV(const char *pszFormat, va_list va)  RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Dumper vprintf-like function outputting to a logger.
 *
 * @param   pvUser      Pointer to the logger instance to use, NULL for default
 *                      instance.
 * @param   pszFormat   Format string.
 * @param   va          Format arguments.
 */
RTDECL(void) RTLogDumpPrintfV(void *pvUser, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(2, 0);

/**
 * Used for logging assertions, debug and release log as appropriate.
 *
 * Implies flushing.
 *
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
typedef DECLCALLBACKTYPE(void, FNRTLOGASSERTION,(const char *pszFormat, ...)) RT_IPRT_FORMAT_ATTR(1, 2);
/** Pointer to an assertion logger, ellipsis variant. */
typedef FNRTLOGASSERTION *PFNRTLOGASSERTION;

/**
 * Used for logging assertions, debug and release log as appropriate.
 *
 * Implies flushing.
 *
 * @param   pszFormat   Format string.
 * @param   va          Format arguments.
 */
typedef DECLCALLBACKTYPE(void, FNRTLOGASSERTIONV,(const char *pszFormat, va_list va)) RT_IPRT_FORMAT_ATTR(1, 0);
/** Pointer to an assertion logger, va_list variant. */
typedef FNRTLOGASSERTIONV *PFNRTLOGASSERTIONV;

/** @copydoc FNRTLOGASSERTION */
RTDECL(void) RTLogAssert(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);
/** @copydoc FNRTLOGASSERTIONV */
RTDECL(void) RTLogAssertV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);

/** "Weak symbol" emulation for RTLogAssert. */
extern RTDATADECL(PFNRTLOGASSERTION)  g_pfnRTLogAssert;
/** "Weak symbol" emulation for RTLogAssertV. */
extern RTDATADECL(PFNRTLOGASSERTIONV) g_pfnRTLogAssertV;


#ifndef DECLARED_FNRTSTROUTPUT          /* duplicated in iprt/string.h & iprt/errcore.h */
#define DECLARED_FNRTSTROUTPUT
/**
 * Output callback.
 *
 * @returns number of bytes written.
 * @param   pvArg       User argument.
 * @param   pachChars   Pointer to an array of utf-8 characters.
 * @param   cbChars     Number of bytes in the character array pointed to by pachChars.
 */
typedef DECLCALLBACKTYPE(size_t, FNRTSTROUTPUT,(void *pvArg, const char *pachChars, size_t cbChars));
/** Pointer to callback function. */
typedef FNRTSTROUTPUT *PFNRTSTROUTPUT;
#endif

/**
 * Partial vsprintf worker implementation.
 *
 * @returns number of bytes formatted.
 * @param   pfnOutput   Output worker.
 *                      Called in two ways. Normally with a string an it's length.
 *                      For termination, it's called with NULL for string, 0 for length.
 * @param   pvArg       Argument to output worker.
 * @param   pszFormat   Format string.
 * @param   args        Argument list.
 */
RTDECL(size_t) RTLogFormatV(PFNRTSTROUTPUT pfnOutput, void *pvArg, const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(3, 0);

/**
 * Write log buffer to COM port.
 *
 * @param   pach        Pointer to the buffer to write.
 * @param   cb          Number of bytes to write.
 */
RTDECL(void) RTLogWriteCom(const char *pach, size_t cb);

/**
 * Prints a formatted string to the serial port used for logging.
 *
 * @returns Number of bytes written.
 * @param   pszFormat   Format string.
 * @param   ...         Optional arguments specified in the format string.
 */
RTDECL(size_t) RTLogComPrintf(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Prints a formatted string to the serial port used for logging.
 *
 * @returns Number of bytes written.
 * @param   pszFormat   Format string.
 * @param   args        Optional arguments specified in the format string.
 */
RTDECL(size_t)  RTLogComPrintfV(const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Write log buffer to a debugger (RTLOGDEST_DEBUGGER).
 *
 * @param   pach        What to write.
 * @param   cb          How much to write.
 * @remark  When linking statically, this function can be replaced by defining your own.
 */
RTDECL(void) RTLogWriteDebugger(const char *pach, size_t cb);

/**
 * Write log buffer to a user defined output stream (RTLOGDEST_USER).
 *
 * @param   pach        What to write.
 * @param   cb          How much to write.
 * @remark  When linking statically, this function can be replaced by defining your own.
 */
RTDECL(void) RTLogWriteUser(const char *pach, size_t cb);

/**
 * Write log buffer to a parent VMM (hypervisor).
 *
 * @param   pach        What to write.
 * @param   cb          How much to write.
 * @param   fRelease    Set if targeting the release log, clear if debug log.
 *
 * @note    Currently only available on AMD64 and x86.
 */
RTDECL(void) RTLogWriteVmm(const char *pach, size_t cb, bool fRelease);

/**
 * Write log buffer to stdout (RTLOGDEST_STDOUT).
 *
 * @param   pach        What to write.
 * @param   cb          How much to write.
 * @remark  When linking statically, this function can be replaced by defining your own.
 */
RTDECL(void) RTLogWriteStdOut(const char *pach, size_t cb);

/**
 * Write log buffer to stdout (RTLOGDEST_STDERR).
 *
 * @param   pach        What to write.
 * @param   cb          How much to write.
 * @remark  When linking statically, this function can be replaced by defining your own.
 */
RTDECL(void) RTLogWriteStdErr(const char *pach, size_t cb);

#ifdef VBOX

/**
 * Prints a formatted string to the backdoor port.
 *
 * @returns Number of bytes written.
 * @param   pszFormat   Format string.
 * @param   ...         Optional arguments specified in the format string.
 */
RTDECL(size_t) RTLogBackdoorPrintf(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Prints a formatted string to the backdoor port.
 *
 * @returns Number of bytes written.
 * @param   pszFormat   Format string.
 * @param   args        Optional arguments specified in the format string.
 */
RTDECL(size_t)  RTLogBackdoorPrintfV(const char *pszFormat, va_list args) RT_IPRT_FORMAT_ATTR(1, 0);

#endif /* VBOX */

RT_C_DECLS_END

/** @} */

#endif /* !IPRT_INCLUDED_log_h */

