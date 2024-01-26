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
 * The Original Code is Mozilla IPC.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@netscape.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(VBOX) && !defined(XP_OS2)
# include <sys/resource.h>
# include <errno.h>
#endif

#ifdef VBOX
# include <iprt/initterm.h>
#endif

#include "prio.h"
#include "prerror.h"
#include "prthread.h"
#include "prinrval.h"
#include "plstr.h"
#include "prprf.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcMessage.h"
#include "ipcClient.h"
#include "ipcModuleReg.h"
#include "ipcdPrivate.h"
#include "ipcd.h"

#if 0
static void
IPC_Sleep(int seconds)
{
    while (seconds > 0) {
        LOG(("\rsleeping for %d seconds...", seconds));
        PR_Sleep(PR_SecondsToInterval(1));
        --seconds;
    }
    LOG(("\ndone sleeping\n"));
}
#endif

//-----------------------------------------------------------------------------
// ipc directory and locking...
//-----------------------------------------------------------------------------

//
// advisory file locking is used to ensure that only one IPC daemon is active
// and bound to the local domain socket at a time.
//
// XXX this code does not work on OS/2.
//
#if !defined(XP_OS2)
#define IPC_USE_FILE_LOCK
#endif

#ifdef IPC_USE_FILE_LOCK

enum Status
{
    EOk = 0,
    ELockFileOpen = -1,
    ELockFileLock = -2,
    ELockFileOwner = -3,
};

static int ipcLockFD = 0;

static Status AcquireDaemonLock(const char *baseDir)
{
    const char lockName[] = "lock";

    int dirLen = strlen(baseDir);
    int len = dirLen            // baseDir
            + 1                 // "/"
            + sizeof(lockName); // "lock"

    char *lockFile = (char *) malloc(len);
    memcpy(lockFile, baseDir, dirLen);
    lockFile[dirLen] = '/';
    memcpy(lockFile + dirLen + 1, lockName, sizeof(lockName));

#ifdef VBOX
    //
    // Security checks for the directory
    //
    struct stat st;
    if (stat(baseDir, &st) == -1)
    {
        printf("Cannot stat '%s'.\n", baseDir);
        return ELockFileOwner;
    }

    if (st.st_uid != getuid() && st.st_uid != geteuid())
    {
        printf("Wrong owner (%d) of '%s'", st.st_uid, baseDir);
        if (   !stat("/tmp", &st)
            && (st.st_mode & 07777) != 01777)
            printf(" -- check /tmp permissions (%o should be 1777)\n",
                    st.st_mode & 07777);
        printf(".\n");
        return ELockFileOwner;
    }

    if (st.st_mode != (S_IRUSR | S_IWUSR | S_IXUSR | S_IFDIR))
    {
        printf("Wrong mode (%o) of '%s'", st.st_mode, baseDir);
        if (   !stat("/tmp", &st)
            && (st.st_mode & 07777) != 01777)
            printf(" -- check /tmp permissions (%o should be 1777)\n",
                    st.st_mode & 07777);
        printf(".\n");
        return ELockFileOwner;
    }
#endif

    //
    // open lock file.  it remains open until we shutdown.
    //
    ipcLockFD = open(lockFile, O_WRONLY|O_CREAT, S_IWUSR|S_IRUSR);

#ifndef VBOX
    free(lockFile);
#endif

    if (ipcLockFD == -1)
        return ELockFileOpen;

#ifdef VBOX
    //
    // Security checks for the lock file
    //
    if (fstat(ipcLockFD, &st) == -1)
    {
        printf("Cannot stat '%s'.\n", lockFile);
        free(lockFile);
        return ELockFileOwner;
    }

    if (st.st_uid != getuid() && st.st_uid != geteuid())
    {
        printf("Wrong owner (%d) of '%s'.\n", st.st_uid, lockFile);
        free(lockFile);
        return ELockFileOwner;
    }

    if (st.st_mode != (S_IRUSR | S_IWUSR | S_IFREG))
    {
        printf("Wrong mode (%o) of '%s'.\n", st.st_mode, lockFile);
        free(lockFile);
        return ELockFileOwner;
    }

    free(lockFile);
#endif

    //
    // we use fcntl for locking.  assumption: filesystem should be local.
    // this API is nice because the lock will be automatically released
    // when the process dies.  it will also be released when the file
    // descriptor is closed.
    //
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;
    if (fcntl(ipcLockFD, F_SETLK, &lock) == -1)
        return ELockFileLock;

    //
    // truncate lock file once we have exclusive access to it.
    //
    ftruncate(ipcLockFD, 0);

    //
    // write our PID into the lock file (this just seems like a good idea...
    // no real purpose otherwise).
    //
    char buf[32];
    int nb = PR_snprintf(buf, sizeof(buf), "%u\n", (unsigned long) getpid());
    write(ipcLockFD, buf, nb);

    return EOk;
}

static Status InitDaemonDir(const char *socketPath)
{
    LOG(("InitDaemonDir [sock=%s]\n", socketPath));

    char *baseDir = PL_strdup(socketPath);

    //
    // make sure IPC directory exists (XXX this should be recursive)
    //
    char *p = strrchr(baseDir, '/');
    if (p)
        p[0] = '\0';
    mkdir(baseDir, 0700);

    //
    // if we can't acquire the daemon lock, then another daemon
    // must be active, so bail.
    //
    Status status = AcquireDaemonLock(baseDir);

    PL_strfree(baseDir);

    if (status == EOk) {
        // delete an existing socket to prevent bind from failing.
        unlink(socketPath);
    }
    return status;
}

static void ShutdownDaemonDir()
{
    LOG(("ShutdownDaemonDir\n"));

    // deleting directory and files underneath it allows another process
    // to think it has exclusive access.  better to just leave the hidden
    // directory in /tmp and let the OS clean it up via the usual tmpdir
    // cleanup cron job.

    // this removes the advisory lock, allowing other processes to acquire it.
    if (ipcLockFD) {
        close(ipcLockFD);
        ipcLockFD = 0;
    }
}

#endif // IPC_USE_FILE_LOCK

//-----------------------------------------------------------------------------
// poll list
//-----------------------------------------------------------------------------

//
// declared in ipcdPrivate.h
//
ipcClient *ipcClients = NULL;
int        ipcClientCount = 0;

//
// the first element of this array is always zero; this is done so that the
// k'th element of ipcClientArray corresponds to the k'th element of
// ipcPollList.
//
static ipcClient ipcClientArray[IPC_MAX_CLIENTS + 1];

//
// element 0 contains the "server socket"
//
static PRPollDesc ipcPollList[IPC_MAX_CLIENTS + 1];

//-----------------------------------------------------------------------------

static int AddClient(PRFileDesc *fd)
{
    if (ipcClientCount == IPC_MAX_CLIENTS) {
        LOG(("reached maximum client limit\n"));
        return -1;
    }

    int pollCount = ipcClientCount + 1;

    ipcClientArray[pollCount].Init();

    ipcPollList[pollCount].fd = fd;
    ipcPollList[pollCount].in_flags = PR_POLL_READ;
    ipcPollList[pollCount].out_flags = 0;

    ++ipcClientCount;
    return 0;
}

static int RemoveClient(int clientIndex)
{
    PRPollDesc *pd = &ipcPollList[clientIndex];

    PR_Close(pd->fd);

    ipcClientArray[clientIndex].Finalize();

    //
    // keep the clients and poll_fds contiguous; move the last one into
    // the spot held by the one that is going away.
    //
    int toIndex = clientIndex;
    int fromIndex = ipcClientCount;
    if (fromIndex != toIndex) {
        memcpy(&ipcClientArray[toIndex], &ipcClientArray[fromIndex], sizeof(ipcClient));
        memcpy(&ipcPollList[toIndex], &ipcPollList[fromIndex], sizeof(PRPollDesc));
    }

    //
    // zero out the old entries.
    //
    memset(&ipcClientArray[fromIndex], 0, sizeof(ipcClient));
    memset(&ipcPollList[fromIndex], 0, sizeof(PRPollDesc));

    --ipcClientCount;
    return 0;
}

//-----------------------------------------------------------------------------

static void PollLoop(PRFileDesc *listenFD)
{
    // the first element of ipcClientArray is unused.
    memset(ipcClientArray, 0, sizeof(ipcClientArray));
    ipcClients = ipcClientArray + 1;
    ipcClientCount = 0;

    ipcPollList[0].fd = listenFD;
    ipcPollList[0].in_flags = PR_POLL_EXCEPT | PR_POLL_READ;

    while (1) {
        PRInt32 rv;
        PRIntn i;

        int pollCount = ipcClientCount + 1;

        ipcPollList[0].out_flags = 0;

        //
        // poll
        //
        // timeout after 5 minutes.  if no connections after timeout, then
        // exit.  this timeout ensures that we don't stay resident when no
        // clients are interested in connecting after spawning the daemon.
        //
        // XXX add #define for timeout value
        //
        LOG(("calling PR_Poll [pollCount=%d]\n", pollCount));
        rv = PR_Poll(ipcPollList, pollCount, PR_SecondsToInterval(60 * 5));
        if (rv == -1) {
            LOG(("PR_Poll failed [%d]\n", PR_GetError()));
            return;
        }

        if (rv > 0) {
            //
            // process clients that are ready
            //
            for (i = 1; i < pollCount; ++i) {
                if (ipcPollList[i].out_flags != 0) {
                    ipcPollList[i].in_flags =
                        ipcClientArray[i].Process(ipcPollList[i].fd,
                                                  ipcPollList[i].out_flags);
                    ipcPollList[i].out_flags = 0;
                }
            }

            //
            // cleanup any dead clients (indicated by a zero in_flags)
            //
            for (i = pollCount - 1; i >= 1; --i) {
                if (ipcPollList[i].in_flags == 0)
                    RemoveClient(i);
            }

            //
            // check for new connection
            //
            if (ipcPollList[0].out_flags & PR_POLL_READ) {
                LOG(("got new connection\n"));

                PRNetAddr clientAddr;
                memset(&clientAddr, 0, sizeof(clientAddr));
                PRFileDesc *clientFD;

                // @todo : We need to handle errors from accept() especially something like
                //          EMFILE, which happens when we run out of file descriptors.
                //          and puts XPCOMIPCD in a poll/accept endless loop!
                clientFD = PR_Accept(listenFD, &clientAddr, PR_INTERVAL_NO_WAIT);
                if (clientFD == NULL) {
                    // ignore this error... perhaps the client disconnected.
                    LOG(("PR_Accept failed [%d]\n", PR_GetError()));
                }
                else {
                    // make socket non-blocking
                    PRSocketOptionData opt;
                    opt.option = PR_SockOpt_Nonblocking;
                    opt.value.non_blocking = PR_TRUE;
                    PR_SetSocketOption(clientFD, &opt);

                    if (AddClient(clientFD) != 0)
                        PR_Close(clientFD);
                }
            }
        }

        //
        // shutdown if no clients
        //
        if (ipcClientCount == 0) {
            LOG(("shutting down\n"));
            break;
        }
    }
}

//-----------------------------------------------------------------------------

PRStatus
IPC_PlatformSendMsg(ipcClient  *client, ipcMessage *msg)
{
    LOG(("IPC_PlatformSendMsg\n"));

    //
    // must copy message onto send queue.
    //
    client->EnqueueOutboundMsg(msg);

    //
    // since our Process method may have already been called, we must ensure
    // that the PR_POLL_WRITE flag is set.
    //
    int clientIndex = client - ipcClientArray;
    ipcPollList[clientIndex].in_flags |= PR_POLL_WRITE;

    return PR_SUCCESS;
}

//-----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    PRFileDesc *listenFD = NULL;
    PRNetAddr addr;

#ifdef VBOX
    /* Set up the runtime without loading the support driver. */
    RTR3InitExe(argc, &argv, 0);
#endif

    //
    // ignore SIGINT so <ctrl-c> from terminal only kills the client
    // which spawned this daemon.
    //
    signal(SIGINT, SIG_IGN);
    // XXX block others?  check cartman

    // ensure strict file permissions
    umask(0077);

    IPC_InitLog("###");

    LOG(("daemon started...\n"));

    //XXX uncomment these lines to test slow starting daemon
    //IPC_Sleep(2);

    // set socket address
    addr.local.family = PR_AF_LOCAL;
    if (argc < 2)
        IPC_GetDefaultSocketPath(addr.local.path, sizeof(addr.local.path));
    else
        PL_strncpyz(addr.local.path, argv[1], sizeof(addr.local.path));

#ifdef IPC_USE_FILE_LOCK
    Status status = InitDaemonDir(addr.local.path);
    if (status != EOk) {
        if (status == ELockFileLock) {
            LOG(("Another daemon is already running, exiting.\n"));
            // send a signal to the blocked parent to indicate success
            IPC_NotifyParent();
            return 0;
        }
        else {
            LOG(("InitDaemonDir failed (status=%d)\n", status));
            // don't notify the parent to cause it to fail in PR_Read() after
            // we terminate
#ifdef VBOX
            if (status != ELockFileOwner)
                printf("Cannot create a lock file for '%s'.\n"
                        "Check permissions.\n", addr.local.path);
#endif
            return 0;
        }
    }
#endif

    listenFD = PR_OpenTCPSocket(PR_AF_LOCAL);
    if (!listenFD) {
        LOG(("PR_OpenTCPSocket failed [%d]\n", PR_GetError()));
    }
    else if (PR_Bind(listenFD, &addr) != PR_SUCCESS) {
        LOG(("PR_Bind failed [%d]\n", PR_GetError()));
    }
    else {
        IPC_InitModuleReg(argv[0]);

#ifdef VBOX
        // Use large backlog, as otherwise local sockets can reject connection
        // attempts. Usually harmless, but causes an unnecessary start attempt
        // of IPCD (which will terminate straight away), and the next attempt
        // usually succeeds. But better avoid unnecessary activities.
        if (PR_Listen(listenFD, 128) != PR_SUCCESS) {
#else /* !VBOX */
        if (PR_Listen(listenFD, 5) != PR_SUCCESS) {
#endif /* !VBOX */
            LOG(("PR_Listen failed [%d]\n", PR_GetError()));
        }
        else {
#ifndef VBOX
            // redirect all standard file descriptors to /dev/null for
            // proper daemonizing
            PR_Close(PR_STDIN);
            PR_Open("/dev/null", O_RDONLY, 0);
            PR_Close(PR_STDOUT);
            PR_Open("/dev/null", O_WRONLY, 0);
            PR_Close(PR_STDERR);
            PR_Open("/dev/null", O_WRONLY, 0);
#endif

            IPC_NotifyParent();

#if defined(VBOX) && !defined(XP_OS2)
            // Increase the file table size to 10240 or as high as possible.
            struct rlimit lim;
            if (getrlimit(RLIMIT_NOFILE, &lim) == 0)
            {
                if (    lim.rlim_cur < 10240
                    &&  lim.rlim_cur < lim.rlim_max)
                {
                    lim.rlim_cur = lim.rlim_max <= 10240 ? lim.rlim_max : 10240;
                    if (setrlimit(RLIMIT_NOFILE, &lim) == -1)
                        printf("WARNING: failed to increase file descriptor limit. (%d)\n", errno);
                }
            }
            else
                printf("WARNING: failed to obtain per-process file-descriptor limit (%d).\n", errno);
#endif

            PollLoop(listenFD);
        }

        IPC_ShutdownModuleReg();
    }

    //IPC_Sleep(5);

#ifdef IPC_USE_FILE_LOCK
    // it is critical that we release the lock before closing the socket,
    // otherwise, a client might launch another daemon that would be unable
    // to acquire the lock and would then leave the client without a daemon.

    ShutdownDaemonDir();
#endif

    if (listenFD) {
        LOG(("closing socket\n"));
        PR_Close(listenFD);
    }

    return 0;
}
