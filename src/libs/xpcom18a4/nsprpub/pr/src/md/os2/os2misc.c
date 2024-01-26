/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape Portable Runtime (NSPR).
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * os2misc.c
 *
 */
#include <string.h>
#include "primpl.h"

extern int   _CRT_init(void);
extern void  _CRT_term(void);
extern void __ctordtorInit(int flag);
extern void __ctordtorTerm(int flag);

char *
_PR_MD_GET_ENV(const char *name)
{
    return getenv(name);
}

PRIntn
_PR_MD_PUT_ENV(const char *name)
{
    return putenv(name);
}


/* see assembleEnvBlock() below */
#define USE_DOSALLOCMEM


/*
 **************************************************************************
 **************************************************************************
 **
 **     Date and time routines
 **
 **************************************************************************
 **************************************************************************
 */

#include <sys/timeb.h>
/*
 *-----------------------------------------------------------------------
 *
 * PR_Now --
 *
 *     Returns the current time in microseconds since the epoch.
 *     The epoch is midnight January 1, 1970 GMT.
 *     The implementation is machine dependent.  This is the
 *     implementation for OS/2.
 *     Cf. time_t time(time_t *tp)
 *
 *-----------------------------------------------------------------------
 */

PR_IMPLEMENT(PRTime)
PR_Now(void)
{
    PRInt64 s, ms, ms2us, s2us;
    struct timeb b;

    ftime(&b);
    LL_I2L(ms2us, PR_USEC_PER_MSEC);
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(s, b.time);
    LL_I2L(ms, b.millitm);
    LL_MUL(ms, ms, ms2us);
    LL_MUL(s, s, s2us);
    LL_ADD(s, s, ms);
    return s;       
}


/*
 ***********************************************************************
 ***********************************************************************
 *
 * Process creation routines
 *
 ***********************************************************************
 ***********************************************************************
 */

/*
 * Assemble the command line by concatenating the argv array.
 * On success, this function returns 0 and the resulting command
 * line is returned in *cmdLine.  On failure, it returns -1.
 */
static int assembleCmdLine(char *const *argv, char **cmdLine)
{
    char *const *arg;
    int cmdLineSize;

    /*
     * Find out how large the command line buffer should be.
     */
    cmdLineSize = 1; /* final null */
    for (arg = argv+1; *arg; arg++) {
        cmdLineSize += strlen(*arg) + 1; /* space in between */
    }
    *cmdLine = PR_MALLOC(cmdLineSize);
    if (*cmdLine == NULL) {
        return -1;
    }

    (*cmdLine)[0] = '\0';

    for (arg = argv+1; *arg; arg++) {
        if (arg > argv +1) {
            strcat(*cmdLine, " ");
        }
        strcat(*cmdLine, *arg);
    } 
    return 0;
}

/*
 * Assemble the environment block by concatenating the envp array
 * (preserving the terminating null byte in each array element)
 * and adding a null byte at the end.
 *
 * Returns 0 on success.  The resulting environment block is returned
 * in *envBlock.  Note that if envp is NULL, a NULL pointer is returned
 * in *envBlock.  Returns -1 on failure.
 */
static int assembleEnvBlock(char **envp, char **envBlock)
{
    char *p;
    char *q;
    char **env;
    char *curEnv;
    char *cwdStart, *cwdEnd;
    int envBlockSize;

    PPIB ppib = NULL;
    PTIB ptib = NULL;

    if (envp == NULL) {
        *envBlock = NULL;
        return 0;
    }

    if(DosGetInfoBlocks(&ptib, &ppib) != NO_ERROR)
       return -1;

    curEnv = ppib->pib_pchenv;

    cwdStart = curEnv;
    while (*cwdStart) {
        if (cwdStart[0] == '=' && cwdStart[1] != '\0'
                && cwdStart[2] == ':' && cwdStart[3] == '=') {
            break;
        }
        cwdStart += strlen(cwdStart) + 1;
    }
    cwdEnd = cwdStart;
    if (*cwdEnd) {
        cwdEnd += strlen(cwdEnd) + 1;
        while (*cwdEnd) {
            if (cwdEnd[0] != '=' || cwdEnd[1] == '\0'
                    || cwdEnd[2] != ':' || cwdEnd[3] != '=') {
                break;
            }
            cwdEnd += strlen(cwdEnd) + 1;
        }
    }
    envBlockSize = cwdEnd - cwdStart;

    for (env = envp; *env; env++) {
        envBlockSize += strlen(*env) + 1;
    }
    envBlockSize++;

    /* It seems that the Environment parameter of DosStartSession() and/or
     * DosExecPgm() wants a memory block that is completely within the 64K
     * memory object; otherwise we will get the environment truncated on the
     * 64K boundary in the child process. PR_MALLOC() cannot guarantee this,
     * so use DosAllocMem directly. */
#ifdef USE_DOSALLOCMEM
    DosAllocMem((PPVOID) envBlock, envBlockSize, PAG_COMMIT | PAG_READ | PAG_WRITE);
    p = *envBlock;
#else
    p = *envBlock = PR_MALLOC(envBlockSize);
#endif
    if (p == NULL) {
        return -1;
    }

    q = cwdStart;
    while (q < cwdEnd) {
        *p++ = *q++;
    }

    for (env = envp; *env; env++) {
        q = *env;
        while (*q) {
            *p++ = *q++;
        }
        *p++ = '\0';
    }
    *p = '\0';
    return 0;
}

/*
 * For qsort.  We sort (case-insensitive) the environment strings
 * before generating the environment block.
 */
static int compare(const void *arg1, const void *arg2)
{
    return stricmp(* (char**)arg1, * (char**)arg2);
}

/*
 * On OS/2, a process can be detached only when it is started -- you cannot
 * make it detached afterwards. This is why _PR_CreateOS2ProcessEx() is
 * necessary. This function is called directly from
 * PR_CreateProcessDetached().  */
PRProcess * _PR_CreateOS2ProcessEx(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr,
    PRBool detached)
{
    PRProcess *proc = NULL;
    char *cmdLine = NULL;
    char **newEnvp = NULL;
    char *envBlock = NULL;
   
    APIRET    rc;
    ULONG     ulAppType = 0;
    PID       pid = 0;
    char     *pEnvWPS = NULL;
    char     *pszComSpec;
    char      pszEXEName[CCHMAXPATH] = "";
    char      pszFormatString[CCHMAXPATH];
    char      pszObjectBuffer[CCHMAXPATH];
    char     *pszFormatResult = NULL;
    char     *pszArg0 = NULL;

    proc = PR_NEW(PRProcess);
    if (!proc) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }
   
    if (assembleCmdLine(argv, &cmdLine) == -1) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

    /* the 0th argument to the program (by convention, the program name, as
     * entered by user) */
    pszArg0 = argv[0];

    /*
     * If attr->fdInheritBuffer is not NULL, we need to insert
     * it into the envp array, so envp cannot be NULL.
     */
    if (envp == NULL && attr && attr->fdInheritBuffer) {
        envp = environ;
    }

    if (envp != NULL) {
        int idx;
        int numEnv;
        int newEnvpSize;

        numEnv = 0;
        while (envp[numEnv]) {
            numEnv++;
        }
        newEnvpSize = numEnv + 1;  /* terminating null pointer */
        if (attr && attr->fdInheritBuffer) {
            newEnvpSize++;
        }
        newEnvp = (char **) PR_MALLOC(newEnvpSize * sizeof(char *));
        for (idx = 0; idx < numEnv; idx++) {
            newEnvp[idx] = envp[idx];
        }
        if (attr && attr->fdInheritBuffer) {
            newEnvp[idx++] = attr->fdInheritBuffer;
        }
        newEnvp[idx] = NULL;
        qsort((void *) newEnvp, (size_t) (newEnvpSize - 1),
                sizeof(char *), compare);
    }
    if (assembleEnvBlock(newEnvp, &envBlock) == -1) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        goto errorExit;
    }

    if (attr) {
        if (attr->stdinFd || attr->stdoutFd || attr->stderrFd)
            PR_ASSERT(!"Stdin/stdout redirection is not implemented");
        if (attr->currentDirectory)
            PR_ASSERT(!"Setting current directory is not implemented");
    }

    rc = DosQueryAppType(path, &ulAppType);
    if (rc != NO_ERROR) {
       char *pszDot = strrchr(path, '.');
       if (pszDot) {
          /* If it is a CMD file, launch the users command processor */
          if (!stricmp(pszDot, ".cmd")) {
             rc = DosScanEnv("COMSPEC", &pszComSpec);
             if (!rc) {
                strcpy(pszFormatString, "/C %s %s");
                strcpy(pszEXEName, pszComSpec);
                pszArg0 = pszEXEName;
                ulAppType = FAPPTYP_WINDOWCOMPAT;
             }
          }
       }
    }
    if (ulAppType == 0) {
       PR_SetError(PR_UNKNOWN_ERROR, 0);
       goto errorExit;
    }

    /* We don't want to use DosExecPgm for detached processes because
     * they won't have stdin/stderr/stdout by default which will hang up
     * the child process if it tries to write/read from there. Instead,
     * we will detach console processes by starting them using the PM session
     * (yes, it requires PM, but the whole XPCOM does so too).
     */
#if 0
    if (detached) {
        /* we don't care about parent/child process type matching,
         * DosExecPgm() should complain if there is a mismatch. */

        size_t cbArg0 = strlen(pszArg0);
        char *pszArgs = NULL;

        if (pszEXEName[0]) {
            pszFormatResult = PR_MALLOC(cbArg0 + 1 +
                                        strlen(pszFormatString) +
                                        strlen(path) + strlen(cmdLine) + 1 + 1);
            if (!pszFormatResult) {
                PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
                goto errorExit;
            }
            pszArgs = pszFormatResult + cbArg0 + 1;
            sprintf(pszArgs, pszFormatString, path, cmdLine);
        } else {
            strcpy(pszEXEName, path);
            pszFormatResult = PR_MALLOC(cbArg0 + 1 +
                                        strlen(cmdLine) + 1 + 1);
            if (!pszFormatResult) {
                PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
                goto errorExit;
            }
            pszArgs = pszFormatResult + cbArg0 + 1;
            strcpy(pszArgs, cmdLine);
        }

        strcpy(pszFormatResult, pszArg0);
        /* add final NULL */
        pszArgs[strlen(pszArgs) + 1] = '\0';

        RESULTCODES res = {0};
        rc = DosExecPgm(pszObjectBuffer, CCHMAXPATH, EXEC_BACKGROUND,
                        pszFormatResult, envBlock, &res, pszEXEName);

        if (rc != NO_ERROR) {
            PR_SetError(PR_UNKNOWN_ERROR, rc);
            goto errorExit;
        }

        /* use 0 to indicate the detached process in the internal
         * process structure (I believe no process may have pid of 0) */
        proc->md.pid = 0 /* res.codeTerminate */;
    }
    else
#endif
    {
        STARTDATA startData = {0};

        if ((ulAppType & FAPPTYP_WINDOWAPI) == FAPPTYP_WINDOWAPI) {
            startData.SessionType = SSF_TYPE_PM;
        }
        else if (ulAppType & FAPPTYP_WINDOWCOMPAT) {
            startData.SessionType = detached ? SSF_TYPE_PM
                                             : SSF_TYPE_WINDOWABLEVIO;
        }
        else if (ulAppType & FAPPTYP_NOTWINDOWCOMPAT) {
            startData.SessionType = detached ? SSF_TYPE_PM
                                             : SSF_TYPE_DEFAULT;
        }
        else {
            startData.SessionType = SSF_TYPE_DEFAULT;
        }

        if (ulAppType & (FAPPTYP_WINDOWSPROT31 | FAPPTYP_WINDOWSPROT | FAPPTYP_WINDOWSREAL))
        {
            strcpy(pszEXEName, "WINOS2.COM");
            startData.SessionType = PROG_31_STDSEAMLESSVDM;
            strcpy(pszFormatString, "/3 %s %s");
        }

        startData.InheritOpt = SSF_INHERTOPT_PARENT;

        if (pszEXEName[0]) {
            pszFormatResult = PR_MALLOC(strlen(pszFormatString)+strlen(path)+strlen(cmdLine));
            sprintf(pszFormatResult, pszFormatString, path, cmdLine);
            startData.PgmInputs = pszFormatResult;
        } else {
            strcpy(pszEXEName, path);
            startData.PgmInputs = cmdLine;
        }
        startData.PgmName = pszEXEName;

        startData.Related = detached ? SSF_RELATED_INDEPENDENT : SSF_RELATED_CHILD;

        startData.Length = sizeof(startData);
        startData.ObjectBuffer = pszObjectBuffer;
        startData.ObjectBuffLen = CCHMAXPATH;
        startData.Environment = envBlock;

        rc = DosStartSession(&startData, &ulAppType, &pid);

        if ((rc != NO_ERROR) && (rc != ERROR_SMG_START_IN_BACKGROUND)) {
            PR_SetError(PR_UNKNOWN_ERROR, rc);
            goto errorExit;
        }

        /* if Related is SSF_RELATED_INDEPENDENT, we don't get pid of the started
         * process and use 0 to indicate the detached process in the internal
         * process structure (I believe no process may have pid of 0).
         */
        proc->md.pid = detached ? 0 : pid;
    }

    if (pszFormatResult) {
        PR_DELETE(pszFormatResult);
    }
    if (cmdLine) {
        PR_DELETE(cmdLine);
    }
    if (newEnvp) {
        PR_DELETE(newEnvp);
    }
    if (envBlock) {
#ifdef USE_DOSALLOCMEM
        DosFreeMem(envBlock);
#else
        PR_DELETE(envBlock);
#endif
    }
    return proc;

errorExit:
    if (pszFormatResult) {
        PR_DELETE(pszFormatResult);
    }
    if (cmdLine) {
        PR_DELETE(cmdLine);
    }
    if (newEnvp) {
        PR_DELETE(newEnvp);
    }
    if (envBlock) {
#ifdef USE_DOSALLOCMEM
        DosFreeMem(envBlock);
#else
        PR_DELETE(envBlock);
#endif
    }
    if (proc) {
        PR_DELETE(proc);
    }
    return NULL;
}  /* _PR_CreateOS2ProcessEx */

PRProcess * _PR_CreateOS2Process(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr)
{
    return _PR_CreateOS2ProcessEx(path, argv, envp, attr, PR_FALSE);
}

PRStatus _PR_DetachOS2Process(PRProcess *process)
{
    /* On OS/2, a process is either created as a child or not. 
     * You can't 'detach' it later on.
     */
    if (process->md.pid == 0) {
        /* this is a detached process, just free memory */
        PR_DELETE(process);
        return PR_SUCCESS;
    }
    /* For a normal child process, we can't complete the request. Note that
     * terminating the parent process w/o calling PR_WaitProcess() on the
     * child will terminate the child as well (since it is not detached).
     */
    PR_SetError(PR_OPERATION_NOT_SUPPORTED_ERROR, 0);
    return PR_FAILURE;
}

/*
 * XXX: This will currently only work on a child process.
 */
PRStatus _PR_WaitOS2Process(PRProcess *process,
    PRInt32 *exitCode)
{
    ULONG ulRetVal;
    RESULTCODES results;
    PID pidEnded = 0;

    ulRetVal = DosWaitChild(DCWA_PROCESS, DCWW_WAIT, 
                            &results,
                            &pidEnded, process->md.pid);

    if (ulRetVal != NO_ERROR) {
       printf("\nDosWaitChild rc = %lu\n", ulRetVal);
        PR_SetError(PR_UNKNOWN_ERROR, ulRetVal);
        return PR_FAILURE;
    }
    PR_DELETE(process);
    return PR_SUCCESS;
}

PRStatus _PR_KillOS2Process(PRProcess *process)
{
    ULONG ulRetVal;
    if ((ulRetVal = DosKillProcess(DKP_PROCESS, process->md.pid)) == NO_ERROR) {
	return PR_SUCCESS;
    }
    PR_SetError(PR_UNKNOWN_ERROR, ulRetVal);
    return PR_FAILURE;
}

PRStatus _MD_OS2GetHostName(char *name, PRUint32 namelen)
{
    PRIntn rv;

    rv = gethostname(name, (PRInt32) namelen);
    if (0 == rv) {
        return PR_SUCCESS;
    }
	_PR_MD_MAP_GETHOSTNAME_ERROR(sock_errno());
    return PR_FAILURE;
}

void
_PR_MD_WAKEUP_CPUS( void )
{
    return;
}    


/*
 **********************************************************************
 *
 * Memory-mapped files are not supported on OS/2 (or Win16).
 *
 **********************************************************************
 */

PRStatus _MD_CreateFileMap(PRFileMap *fmap, PRInt64 size)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PRInt32 _MD_GetMemMapAlignment(void)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return -1;
}

void * _MD_MemMap(
    PRFileMap *fmap,
    PROffset64 offset,
    PRUint32 len)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

PRStatus _MD_MemUnmap(void *addr, PRUint32 len)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PRStatus _MD_CloseFileMap(PRFileMap *fmap)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

/*
 *  Automatically set apptype switch for interactive and other
 *  tests that create an invisible plevent window.
 */
unsigned long _System _DLL_InitTerm( unsigned long mod_handle, unsigned long flag)
{
   unsigned long rc = 0; /* failure */

   if( !flag)
   {
      /* init */
      if( _CRT_init() == 0)
      {
         PPIB pPib;
         PTIB pTib;

         /* probably superfluous, but can't hurt */
         __ctordtorInit(0);

         DosGetInfoBlocks( &pTib, &pPib);
         pPib->pib_ultype = 3; /* PM */

         rc = 1;
      }
   }
   else
   {
      __ctordtorTerm(0);
      _CRT_term();
      rc = 1;
   }

   return rc;
}

