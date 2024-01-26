/** @file
 * IPRT / No-CRT - sys/stat.h
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

#ifndef IPRT_INCLUDED_nocrt_sys_stat_h
#define IPRT_INCLUDED_nocrt_sys_stat_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/nocrt/time.h>    /* Establish timespec and timeval before iprt/fs.h includes iprt/time.h. */
#include <iprt/fs.h>
#include <iprt/nocrt/sys/types.h>
#include <iprt/nocrt/limits.h>

#ifdef IPRT_NO_CRT_FOR_3RD_PARTY

struct RT_NOCRT(stat)
{
    RTINODE     st_ino;
    RTDEV       st_dev;
    RTDEV       st_rdev;
    RTFMODE     st_mode;
    uint32_t    st_link;
    RTUID       st_uid;
    RTGID       st_gid;
    RTFOFF      st_size;
    RTFOFF      st_blocks;
    uint32_t    st_blksize; /**< Not related to st_blocks! */
    time_t      st_birthtime;
    time_t      st_ctime;
    time_t      st_mtime;
    time_t      st_atime;
};

# define _S_IFIFO               RTFS_TYPE_FIFO
# define _S_IFCHR               RTFS_TYPE_DEV_CHAR
# define _S_IFDIR               RTFS_TYPE_DIRECTORY
# define _S_IFBLK               RTFS_TYPE_DEV_BLOCK
# define _S_IFREG               RTFS_TYPE_FILE
# define _S_IFLNK               RTFS_TYPE_SYMLINK
# define _S_IFSOCK              RTFS_TYPE_SOCKET
# define _S_IFWHT               RTFS_TYPE_WHITEOUT
# define _S_IFMT                RTFS_TYPE_MASK

# define S_IFIFO                _S_IFIFO
# define S_IFCHR                _S_IFCHR
# define S_IFDIR                _S_IFDIR
# define S_IFBLK                _S_IFBLK
# define S_IFREG                _S_IFREG
# define S_IFLNK                _S_IFLNK
# define S_IFSOCK               _S_IFSOCK
# define S_IFWHT                _S_IFWHT
# define S_IFMT                 _S_IFMT

# define S_ISFIFO(a_fMode)      RTFS_IS_FIFO(a_fMode)
# define S_ISCHR(a_fMode)       RTFS_IS_DEV_CHAR(a_fMode)
# define S_ISDIR(a_fMode)       RTFS_IS_DIRECTORY(a_fMode)
# define S_ISBLK(a_fMode)       RTFS_IS_DEV_BLOCK(a_fMode)
# define S_ISREG(a_fMode)       RTFS_IS_FILE(a_fMode)
# define S_ISLNK(a_fMode)       RTFS_IS_SYMLINK(a_fMode)
# define S_ISSOCK(a_fMode)      RTFS_IS_SOCKET(a_fMode)
# define S_ISWHT(a_fMode)       RTFS_IS_WHITEOUT(a_fMode)


RT_C_DECLS_BEGIN

int     RT_NOCRT(chmod)(const char *pszPath, RTFMODE fMode);
int     RT_NOCRT(fchmod)(int fd, RTFMODE fMode);
int     RT_NOCRT(fstat)(int fd, struct RT_NOCRT(stat) *pStat);
int     RT_NOCRT(lstat)(const  char *pszPath, struct RT_NOCRT(stat) *pStat);
int     RT_NOCRT(stat)(const  char *pszPath, struct RT_NOCRT(stat) *pStat);
RTFMODE RT_NOCRT(umask)(RTFMODE fMode);
int     RT_NOCRT(mkdir)(const char *, RTFMODE fMode);

int     RT_NOCRT(_chmod)(const char *pszPath, RTFMODE fMode);
int     RT_NOCRT(_fchmod)(int fd, RTFMODE fMode);
int     RT_NOCRT(_fstat)(int fd, struct RT_NOCRT(stat) *pStat);
int     RT_NOCRT(_lstat)(const  char *pszPath, struct RT_NOCRT(stat) *pStat);
int     RT_NOCRT(_stat)(const  char *pszPath, struct RT_NOCRT(stat) *pStat);
RTFMODE RT_NOCRT(_umask)(RTFMODE fMode);
int     RT_NOCRT(_mkdir)(const char *, RTFMODE fMode);

# if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
#  define chmod      RT_NOCRT(chmod)
#  define fchmod     RT_NOCRT(fchmod)
#  define fstat      RT_NOCRT(fstat)
#  define lstat      RT_NOCRT(lstat)
#  define stat       RT_NOCRT(stat)
#  define umask      RT_NOCRT(umask)
#  define mkdir      RT_NOCRT(mkdir)

#  define _chmod     RT_NOCRT(chmod)
#  define _fchmod    RT_NOCRT(fchmod)
#  define _fstat     RT_NOCRT(fstat)
#  define _lstat     RT_NOCRT(lstat)
#  define _stat      RT_NOCRT(stat)
#  define _umask     RT_NOCRT(umask)
#  define _mkdir     RT_NOCRT(mkdir)
# endif

RT_C_DECLS_END

#endif /* IPRT_NO_CRT_FOR_3RD_PARTY */

#endif /* !IPRT_INCLUDED_nocrt_sys_stat_h */

