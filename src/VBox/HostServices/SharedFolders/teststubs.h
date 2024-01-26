/** @file
 * VBox Shared Folders testcase stub redefinitions.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

/**
 * Macros for renaming iprt file operations to redirect them to testcase
 * stub functions (mocks).  The religiously correct way to do this would be
 * to make the service use a file operations structure with function pointers
 * but I'm not sure that would be universally appreciated. */

#ifndef VBOX_INCLUDED_SRC_SharedFolders_teststubs_h
#define VBOX_INCLUDED_SRC_SharedFolders_teststubs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/time.h>

#define RTDirClose           testRTDirClose
extern int testRTDirClose(RTDIR hDir);
#define RTDirCreate          testRTDirCreate
extern int testRTDirCreate(const char *pszPath, RTFMODE fMode, uint32_t fCreate);
#define RTDirOpen            testRTDirOpen
extern int testRTDirOpen(RTDIR *phDir, const char *pszPath);
#define RTDirOpenFiltered    testRTDirOpenFiltered
extern int testRTDirOpenFiltered(RTDIR *phDir, const char *pszPath, RTDIRFILTER enmFilter, uint32_t fFlags);
#define RTDirQueryInfo       testRTDirQueryInfo
extern int testRTDirQueryInfo(RTDIR hDir, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs);
#define RTDirRemove          testRTDirRemove
extern int testRTDirRemove(const char *pszPath);
#define RTDirReadEx          testRTDirReadEx
extern int testRTDirReadEx(RTDIR hDir, PRTDIRENTRYEX pDirEntry, size_t *pcbDirEntry, RTFSOBJATTRADD enmAdditionalAttribs, uint32_t fFlags);
#define RTDirSetMode         testRTDirSetMode
extern int testRTDirSetMode(RTDIR hDir, RTFMODE fMode);
#define RTDirSetTimes        testRTDirSetTimes
extern int testRTDirSetTimes(RTDIR hDir, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime, PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime);
#define RTFileClose          testRTFileClose
extern int testRTFileClose(RTFILE hFile);
#define RTFileDelete         testRTFileDelete
extern int testRTFileDelete(const char *pszFilename);
#define RTFileFlush          testRTFileFlush
extern int testRTFileFlush(RTFILE hFile);
#define RTFileLock           testRTFileLock
extern int testRTFileLock(RTFILE hFile, unsigned fLock, int64_t offLock, uint64_t cbLock);
#define RTFileOpenEx         testRTFileOpenEx
extern int testRTFileOpenEx(const char *pszFilename, uint64_t fOpen, PRTFILE phFile, PRTFILEACTION penmActionTaken);
#define RTFileQueryInfo      testRTFileQueryInfo
extern int testRTFileQueryInfo(RTFILE hFile, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs);
#define RTFileRead           testRTFileRead
extern int testRTFileRead(RTFILE hFile, void *pvBuf, size_t cbToRead, size_t *pcbRead);
#define RTFileReadAt         testRTFileReadAt
extern int testRTFileReadAt(RTFILE hFile, uint64_t offFile, void *pvBuf, size_t cbToRead, size_t *pcbRead);
#define RTFileSetMode        testRTFileSetMode
extern int testRTFileSetMode(RTFILE hFile, RTFMODE fMode);
#define RTFileSetSize        testRTFileSetSize
extern int testRTFileSetSize(RTFILE hFile, uint64_t cbSize);
#define RTFileSetTimes       testRTFileSetTimes
extern int testRTFileSetTimes(RTFILE hFile, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime, PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime);
#define RTFileSeek           testRTFileSeek
extern int testRTFileSeek(RTFILE hFile, int64_t offSeek, unsigned uMethod, uint64_t *poffActual);
#define RTFileUnlock         testRTFileUnlock
extern int testRTFileUnlock(RTFILE hFile, int64_t offLock, uint64_t cbLock);
#define RTFileWrite          testRTFileWrite
extern int testRTFileWrite(RTFILE hFile, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);
#define RTFileWriteAt        testRTFileWriteAt
extern int testRTFileWriteAt(RTFILE hFile, uint64_t offFile, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);
#define RTFsQueryProperties  testRTFsQueryProperties
extern int testRTFsQueryProperties(const char *pszFsPath, PRTFSPROPERTIES pProperties);
#define RTFsQuerySerial      testRTFsQuerySerial
extern int testRTFsQuerySerial(const char *pszFsPath, uint32_t *pu32Serial);
#define RTFsQuerySizes       testRTFsQuerySizes
extern int testRTFsQuerySizes(const char *pszFsPath, RTFOFF *pcbTotal, RTFOFF *pcbFree, uint32_t *pcbBlock, uint32_t *pcbSector);
#define RTPathQueryInfoEx    testRTPathQueryInfoEx
extern int testRTPathQueryInfoEx(const char *pszPath, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs, uint32_t fFlags);
#define RTSymlinkDelete      testRTSymlinkDelete
extern int testRTSymlinkDelete(const char *pszSymlink, uint32_t fDelete);
#define RTSymlinkRead        testRTSymlinkRead
extern int testRTSymlinkRead(const char *pszSymlink, char *pszTarget, size_t cbTarget, uint32_t fRead);

#endif /* !VBOX_INCLUDED_SRC_SharedFolders_teststubs_h */
