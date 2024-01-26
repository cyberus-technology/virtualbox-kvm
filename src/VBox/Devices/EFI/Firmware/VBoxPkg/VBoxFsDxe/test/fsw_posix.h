/**
 * \file fsw_posix.h
 * POSIX user space host environment header.
 */

/*-
 * Copyright (c) 2006 Christoph Pfisterer
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSW_POSIX_H_
#define _FSW_POSIX_H_

#include "fsw_core.h"

#include <fcntl.h>
#include <sys/types.h>


/**
 * POSIX Host: Private per-volume structure.
 */

struct fsw_posix_volume {
    struct fsw_volume           *vol;           //!< FSW volume structure

    int                         fd;             //!< System file descriptor for data access

};

/**
 * POSIX Host: Private structure for an open file.
 */

struct fsw_posix_file {
    struct fsw_posix_volume     *pvol;          //!< POSIX host volume structure

    struct fsw_shandle          shand;          //!< FSW handle for this file

};

/**
 * POSIX Host: Private structure for an open directory.
 */

struct fsw_posix_dir {
    struct fsw_posix_volume     *pvol;          //!< POSIX host volume structure

    struct fsw_shandle          shand;          //!< FSW handle for this file

};


#define NAME_MAX    4096

#define DT_UNKNOWN  'u'
#define DT_REG      'r'
#define DT_DIR      'd'
#define DT_LNK      'l'


/**
 * POSIX Host: Private structure for an open directory.
 */

struct fsw_posix_dirent {
    char                d_attr;             /* file's attribute */
    unsigned            d_type;
    unsigned short int  d_time;             /* file's time */
    unsigned short int  d_date;             /* file's date */
    long                d_size;             /* file's size */
    char                d_name[NAME_MAX+1]; /* file's name */
    unsigned            d_fileno;           /* file number/inode */
};
typedef struct fsw_posix_dirent DIR;

/* functions */

struct fsw_posix_volume * fsw_posix_mount(const char *path, struct fsw_fstype_table *fstype_table);
int fsw_posix_unmount(struct fsw_posix_volume *pvol);

struct fsw_posix_file * fsw_posix_open(struct fsw_posix_volume *pvol, const char *path, int flags, mode_t mode);
ssize_t fsw_posix_read(struct fsw_posix_file *file, void *buf, size_t nbytes);
off_t fsw_posix_lseek(struct fsw_posix_file *file, off_t offset, int whence);
int fsw_posix_close(struct fsw_posix_file *file);

struct fsw_posix_dir * fsw_posix_opendir(struct fsw_posix_volume *pvol, const char *path);
struct fsw_posix_dirent * fsw_posix_readdir(struct fsw_posix_dir *dir);
void fsw_posix_rewinddir(struct fsw_posix_dir *dir);
int fsw_posix_closedir(struct fsw_posix_dir *dir);


#endif
