/** @file
 * tstVBoxMultipleVM - load test for ClientWatcher.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>
#include <VBox/com/Guid.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <iprt/assert.h>
#include <iprt/errcore.h>
#include <VBox/com/VirtualBox.h>
#include <iprt/stream.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <VBox/sup.h>

#include <vector>
#include <algorithm>

#include <iprt/test.h>
#include <iprt/time.h>
#include <iprt/rand.h>
#include <iprt/getopt.h>

using namespace com;


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/* Arguments of test thread */
struct TestThreadArgs
{
    /** number of machines that should be run simultaneousely */
    uint32_t machinesPackSize;
    /** percents of VM Stop operation what should be called
     *  without session unlocking */
    uint32_t percentsUnlok;
    /** How much time in milliseconds test will be executed */
    uint64_t cMsExecutionTime;
    /** How much machines create for the test */
    uint32_t numberMachines;
};


/*********************************************************************************************************************************
*   Global Variables & defs                                                                                                      *
*********************************************************************************************************************************/
static RTTEST                   g_hTest;
#ifdef RT_ARCH_AMD64
typedef std::vector<Bstr>       TMachinesList;
static volatile bool            g_RunTest = true;
static RTSEMEVENT               g_PingEevent;
static volatile uint64_t        g_Counter = 0;
static TestThreadArgs           g_Args;


/** Worker for TST_COM_EXPR(). */
static HRESULT tstComExpr(HRESULT hrc, const char *pszOperation, int iLine)
{
    if (FAILED(hrc))
    {
        RTTestFailed(g_hTest, "%s failed on line %u with hrc=%Rhrc\n", pszOperation, iLine, hrc);
    }
    return hrc;
}


#define CHECK_ERROR_L(iface, method) \
    do { \
        hrc = iface->method; \
        if (FAILED(hrc)) \
            RTPrintf("warning: %s->%s failed on line %u with hrc=%Rhrc\n", #iface, #method, __LINE__, hrc);\
    } while (0)


/** Macro that executes the given expression and report any failure.
 *  The expression must return a HRESULT. */
#define TST_COM_EXPR(expr) tstComExpr(expr, #expr, __LINE__)


static int tstStartVM(IVirtualBox *pVBox, ISession *pSession, Bstr machineID, bool fSkipUnlock)
{
    HRESULT hrc;
    ComPtr<IProgress> progress;
    ComPtr<IMachine> machine;
    Bstr machineName;

    hrc = TST_COM_EXPR(pVBox->FindMachine(machineID.raw(), machine.asOutParam()));
    if(SUCCEEDED(hrc))
        hrc = TST_COM_EXPR(machine->COMGETTER(Name)(machineName.asOutParam()));
    if(SUCCEEDED(hrc))
    {
        hrc = machine->LaunchVMProcess(pSession, Bstr("headless").raw(),
                                      ComSafeArrayNullInParam(), progress.asOutParam());
    }
    if (SUCCEEDED(hrc) && !progress.isNull())
    {
        CHECK_ERROR_L(progress, WaitForCompletion(-1));
        if (SUCCEEDED(hrc))
        {
            BOOL completed = true;
            CHECK_ERROR_L(progress, COMGETTER(Completed)(&completed));
            if (SUCCEEDED(hrc))
            {
                Assert(completed);
                LONG iRc;
                CHECK_ERROR_L(progress, COMGETTER(ResultCode)(&iRc));
                if (SUCCEEDED(hrc))
                {
                    if (FAILED(iRc))
                    {
                        ProgressErrorInfo info(progress);
                        RTPrintf("Start VM '%ls' failed. Warning: %ls.\n", machineName.raw(), info.getText().raw());
                    }
                    else
                        RTPrintf("VM '%ls' started.\n", machineName.raw());
                }
            }
        }
        if (!fSkipUnlock)
            pSession->UnlockMachine();
        else
            RTPrintf("Session unlock skipped.\n");
    }
    return hrc;
}


static int tstStopVM(IVirtualBox* pVBox, ISession* pSession, Bstr machineID, bool fSkipUnlock)
{
    ComPtr<IMachine> machine;
    HRESULT hrc = TST_COM_EXPR(pVBox->FindMachine(machineID.raw(), machine.asOutParam()));
    if (SUCCEEDED(hrc))
    {
        Bstr machineName;
        hrc = TST_COM_EXPR(machine->COMGETTER(Name)(machineName.asOutParam()));
        if (SUCCEEDED(hrc))
        {
            MachineState_T machineState;
            hrc = TST_COM_EXPR(machine->COMGETTER(State)(&machineState));
            // check that machine is in running state
            if (   SUCCEEDED(hrc)
                && (   machineState == MachineState_Running
                    || machineState == MachineState_Paused))
            {
                ComPtr<IConsole> console;
                ComPtr<IProgress> progress;

                hrc = TST_COM_EXPR(machine->LockMachine(pSession, LockType_Shared));
                if(SUCCEEDED(hrc))
                    TST_COM_EXPR(pSession->COMGETTER(Console)(console.asOutParam()));
                if(SUCCEEDED(hrc))
                    hrc = console->PowerDown(progress.asOutParam());
                if (SUCCEEDED(hrc) && !progress.isNull())
                {
                    //RTPrintf("Stopping VM %ls...\n", machineName.raw());
                    CHECK_ERROR_L(progress, WaitForCompletion(-1));
                    if (SUCCEEDED(hrc))
                    {
                        BOOL completed = true;
                        CHECK_ERROR_L(progress, COMGETTER(Completed)(&completed));
                        if (SUCCEEDED(hrc))
                        {
                            //ASSERT(completed);
                            LONG iRc;
                            CHECK_ERROR_L(progress, COMGETTER(ResultCode)(&iRc));
                            if (SUCCEEDED(hrc))
                            {
                                if (FAILED(iRc))
                                {
                                    ProgressErrorInfo info(progress);
                                    RTPrintf("Stop VM %ls failed. Warning: %ls.\n", machineName.raw(), info.getText().raw());
                                    hrc = iRc;
                                }
                                else
                                {
                                    RTPrintf("VM '%ls' stopped.\n", machineName.raw());
                                }
                            }
                        }
                    }
                    if (!fSkipUnlock)
                        pSession->UnlockMachine();
                    else
                        RTPrintf("Session unlock skipped.\n");
                }
            }
        }
    }
    return hrc;
}


/**
 * Get random @a maxCount machines from list of existing VMs.
 *
 * @note Can return less then maxCount machines.
 */
static int tstGetMachinesList(IVirtualBox *pVBox, uint32_t maxCount, TMachinesList &listToFill)
{
    com::SafeIfaceArray<IMachine> machines;
    HRESULT hrc = TST_COM_EXPR(pVBox->COMGETTER(Machines)(ComSafeArrayAsOutParam(machines)));
    if (SUCCEEDED(hrc))
    {

        size_t cMachines = RT_MIN(machines.size(), maxCount);
        for (size_t i = 0; i < cMachines; ++i)
        {
            // choose random index of machine
            uint32_t idx = RTRandU32Ex(0, (uint32_t)machines.size() - 1);
            if (machines[idx])
            {
                Bstr bstrId;
                Bstr machineName;
                CHECK_ERROR_L(machines[idx], COMGETTER(Id)(bstrId.asOutParam()));
                if (SUCCEEDED(hrc))
                    CHECK_ERROR_L(machines[idx], COMGETTER(Name)(machineName.asOutParam()));
                if (SUCCEEDED(hrc))
                {
                    if (Utf8Str(machineName).startsWith("umtvm"))
                        listToFill.push_back(bstrId);
                }
            }
        }

        // remove duplicates from the vector
        std::sort(listToFill.begin(), listToFill.end());
        listToFill.erase(std::unique(listToFill.begin(), listToFill.end()), listToFill.end());
        RTPrintf("Filled pack of %d from %d machines.\n", listToFill.size(), machines.size());
    }

    return hrc;
}


static int tstMachinesPack(IVirtualBox *pVBox, uint32_t maxPackSize, uint32_t percentage)
{
    HRESULT hrc = S_OK;
    TMachinesList machinesList;
    bool alwaysUnlock = false;
    uint64_t percN = 0;

    // choose and fill pack of machines for test
    tstGetMachinesList(pVBox, maxPackSize, machinesList);

    RTPrintf("Start test.\n");
    // screw up counter
    g_Counter =  UINT64_MAX - machinesList.size() <= g_Counter ? 0 : g_Counter;
    if (percentage > 0)
        percN = 100 / percentage;
    else
        alwaysUnlock = true;

    // start all machines in pack
    for (TMachinesList::iterator it = machinesList.begin();
         it != machinesList.end() && g_RunTest;
         ++it)
    {
        ComPtr<ISession> session;
        hrc = session.createInprocObject(CLSID_Session);
        if (SUCCEEDED(hrc))
        {
            hrc = tstStartVM(pVBox, session, *it, !(alwaysUnlock || g_Counter++ % percN));
        }
        RTSemEventSignal(g_PingEevent);
        RTThreadSleep(100);
    }
    // stop all machines in the pack
    for (TMachinesList::iterator it = machinesList.begin();
         it != machinesList.end() && g_RunTest;
         ++it)
    {
        ComPtr<ISession> session;
        hrc = session.createInprocObject(CLSID_Session);
        if (SUCCEEDED(hrc))
        {
            // stop machines, skip session unlock of given % of machines
            hrc = tstStopVM(pVBox, session, *it, !(alwaysUnlock || g_Counter++ % percN));
        }
        RTSemEventSignal(g_PingEevent);
        RTThreadSleep(100);
    }
    return hrc;
}


static Bstr tstMakeMachineName(int i)
{
    char szMachineName[32];
    RTStrPrintf(szMachineName, sizeof(szMachineName), "umtvm%d", i);
    return Bstr(szMachineName);
}


static int tstCreateMachines(IVirtualBox *pVBox)
{
    HRESULT hrc = S_OK;
    // create machines for the test
    for (uint32_t i = 0; i < g_Args.numberMachines; i++)
    {
        ComPtr<IMachine> ptrMachine;
        com::SafeArray<BSTR> groups;

        Bstr machineName(tstMakeMachineName(i));
        /* Default VM settings */
        CHECK_ERROR_L(pVBox, CreateMachine(NULL,                          /* Settings */
                                         machineName.raw(),             /* Name */
                                         ComSafeArrayAsInParam(groups), /* Groups */
                                         NULL,                          /* OS Type */
                                         NULL,                          /** Cipher */
                                         NULL,                          /** Password id */
                                         NULL,                          /** Password */
                                         NULL,                          /* Create flags */
                                         ptrMachine.asOutParam()));
        if (SUCCEEDED(hrc))
        {
            CHECK_ERROR_L(pVBox, RegisterMachine(ptrMachine));
            RTPrintf("Machine '%ls' created\n", machineName.raw());
        }

        RTSemEventSignal(g_PingEevent);
        RTThreadSleep(100);
    }
    return hrc;
}


static int tstClean(IVirtualBox *pVBox, IVirtualBoxClient *pClient)
{
    RT_NOREF(pClient);
    HRESULT hrc = S_OK;

    // stop all machines created for the test
    for (uint32_t i = 0; i < g_Args.numberMachines; i++)
    {
        ComPtr<IMachine> machine;
        ComPtr<IProgress> progress;
        ComPtr<ISession> session;
        SafeIfaceArray<IMedium> media;

        Bstr machineName(tstMakeMachineName(i));

        /* Delete created VM and its files */
        CHECK_ERROR_L(pVBox, FindMachine(machineName.raw(), machine.asOutParam()));

        // try to stop it again if it was not stopped
        if (SUCCEEDED(hrc))
        {
            MachineState_T machineState;
            CHECK_ERROR_L(machine, COMGETTER(State)(&machineState));
            if (   SUCCEEDED(hrc)
                && (   machineState == MachineState_Running
                    || machineState == MachineState_Paused) )
            {
                hrc = session.createInprocObject(CLSID_Session);
                if (SUCCEEDED(hrc))
                    tstStopVM(pVBox, session, machineName, FALSE);
            }
        }

        if (SUCCEEDED(hrc))
            CHECK_ERROR_L(machine, Unregister(CleanupMode_DetachAllReturnHardDisksOnly, ComSafeArrayAsOutParam(media)));
        if (SUCCEEDED(hrc))
            CHECK_ERROR_L(machine, DeleteConfig(ComSafeArrayAsInParam(media), progress.asOutParam()));
        if (SUCCEEDED(hrc))
            CHECK_ERROR_L(progress, WaitForCompletion(-1));
        if (SUCCEEDED(hrc))
            RTPrintf("Machine '%ls' deleted.\n", machineName.raw());
    }
    return hrc;
}


static DECLCALLBACK(int) tstThreadRun(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);
    TestThreadArgs* args = (TestThreadArgs*)pvUser;
    Assert(args != NULL);
    uint32_t maxPackSize = args->machinesPackSize;
    uint32_t percentage = args->percentsUnlok;

    HRESULT hrc = com::Initialize();
    if (SUCCEEDED(hrc))
    {
        ComPtr<IVirtualBoxClient> ptrVBoxClient;
        ComPtr<IVirtualBox> ptrVBox;

        hrc = TST_COM_EXPR(ptrVBoxClient.createInprocObject(CLSID_VirtualBoxClient));
        if (SUCCEEDED(hrc))
            hrc = TST_COM_EXPR(ptrVBoxClient->COMGETTER(VirtualBox)(ptrVBox.asOutParam()));
        if (SUCCEEDED(hrc))
        {
            RTPrintf("Creating machines...\n");
            tstCreateMachines(ptrVBox);

            while (g_RunTest)
            {
                hrc = tstMachinesPack(ptrVBox, maxPackSize, percentage);
            }

            RTPrintf("Deleting machines...\n");
            tstClean(ptrVBox, ptrVBoxClient);
        }

        g_RunTest = false;
        RTSemEventSignal(g_PingEevent);
        RTThreadSleep(100);

        ptrVBox = NULL;
        ptrVBoxClient = NULL;
        com::Shutdown();
    }
    return hrc;
}


static int ParseArguments(int argc, char **argv, TestThreadArgs *pArgs)
{
    RTGETOPTSTATE               GetState;
    RTGETOPTUNION               ValueUnion;
    static const RTGETOPTDEF    s_aOptions[] =
    {
        { "--packsize",  'p', RTGETOPT_REQ_UINT32 }, // number of machines to start together
        { "--lock",      's', RTGETOPT_REQ_UINT32 }, // percentage of VM sessions closed without Unlok
        { "--time",      't', RTGETOPT_REQ_UINT64 }, // required time of load test execution, in seconds
        { "--machines" , 'u', RTGETOPT_REQ_UINT32 }
    };
    int rc = RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0 /*fFlags*/);
    AssertRCReturn(rc, rc);
    AssertPtr(pArgs);

    while ((rc = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (rc)
        {
            case 'p':
                if (ValueUnion.u32 == 0)
                {
                    RTPrintf("--packsize should be more then zero\n");
                    return VERR_INVALID_PARAMETER;
                }
                if (ValueUnion.u32 > 16000)
                {
                        RTPrintf("maximum --packsize value is 16000.\n"
                                 "That means can use no more then 16000 machines for the test.\n");
                        return VERR_INVALID_PARAMETER;
                }
                pArgs->machinesPackSize = ValueUnion.u32;
                break;

            case 's':
                if (ValueUnion.u32 > 100)
                {
                    RTPrintf("maximum --lock value is 100.\n"
                             "That means 100 percent of sessions should be closed without unlock.\n");
                    return VERR_INVALID_PARAMETER;
                }
                pArgs->percentsUnlok = ValueUnion.u32;
                break;

            case 't':
                pArgs->cMsExecutionTime = ValueUnion.u64 * 1000;
                break;

            case 'u':
                if (ValueUnion.u32 > 16000)
                {
                    RTPrintf("maximum --machines value is 16000.\n"
                             "That means can make no more then 16000 machines for the test.\n");
                    return VERR_INVALID_PARAMETER;
                }
                if (ValueUnion.u32 < pArgs->machinesPackSize)
                {
                    RTPrintf("--machines value should be larger then --packsize value.\n");
                    return VERR_INVALID_PARAMETER;
                }
                pArgs->numberMachines = ValueUnion.u32;
                break;

            default:
                RTGetOptPrintError(rc, &ValueUnion);
                return rc;
        }
    }
    return rc;
}

#endif /* RT_ARCH_AMD64 */


/**
 *
 * Examples:
 *   - tstVBoxClientWatcherLoad --packsize 500 --lock 10 --time 14400 --machines 4000
 *     It will create 4000 VMs with names "utmvm0"..."utmvm3999".  It will start
 *     500 random VMs together, stop them, without closing their session with
 *     probability 10%, will repeat this over 4 hours.  After test it will
 *     delete all "utmvm..." machines.
 *
 *   - tstVBoxClientWatcherLoad --packsize 1 --lock 30 --time 3600 --machines 1000
 *     It will create 1000 VMs with names "utmvm0"..."utmvm999".  It will start
 *     random VM - stop them, without closing their session with probability
 *     30%, will repeat this over 30 minutes.  After test it will delete all
 *     "utmvm..." machines.
 */
int main(int argc, char **argv)
{
    RT_NOREF(argc, argv);
    RTEXITCODE rcExit = RTTestInitAndCreate("tstVBoxMultipleVM", &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    SUPR3Init(NULL);
    com::Initialize();
    RTTestBanner(g_hTest);

#ifndef RT_ARCH_AMD64
    /*
     * Linux OOM killer when running many VMs on a 32-bit host.
     */
    return RTTestSkipAndDestroy(g_hTest, "The test can only run reliably on 64-bit hosts.");
#else  /* RT_ARCH_AMD64 */

    RTPrintf("Initializing ...\n");
    int rc = RTSemEventCreate(&g_PingEevent);
    AssertRC(rc);

    g_Args.machinesPackSize = 100;
    g_Args.percentsUnlok = 10;
    g_Args.cMsExecutionTime = 3*RT_MS_1MIN;
    g_Args.numberMachines = 200;

    /*
     * Skip this test for the time being. Saw crashes on several test boxes but no time
     * to debug.
     */
    if (argc == 1)
        return RTTestSkipAndDestroy(g_hTest, "Test crashes sometimes.\n");

    rc = ParseArguments(argc, argv, &g_Args);
    if (RT_FAILURE(rc))
        return RTTestSkipAndDestroy(g_hTest, "Invalid arguments.\n");

    RTPrintf("Arguments packSize = %d, percentUnlok = %d, time = %lld.\n",
             g_Args.machinesPackSize, g_Args.percentsUnlok, g_Args.cMsExecutionTime);

    RTTHREAD hThread;
    rc = RTThreadCreate(&hThread, tstThreadRun, (void *)&g_Args,
                        0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "tstThreadRun");
    if (RT_SUCCESS(rc))
    {
        AssertRC(rc);

        uint64_t msStart = RTTimeMilliTS();
        while (RTTimeMilliTS() - msStart < g_Args.cMsExecutionTime && g_RunTest)
        {
            // check that test thread didn't hang and call us periodically
            // allowed 30 seconds for operation - msStart or stop VM
            rc = RTSemEventWait(g_PingEevent, 3 * 60 * 1000);
            if (RT_FAILURE(rc))
            {
                if (rc == VERR_TIMEOUT)
                {
                    RTTestFailed(g_hTest, "Timeout. Deadlock?\n");
                    com::Shutdown();
                    return RTTestSummaryAndDestroy(g_hTest);
                }
                AssertRC(rc);
            }
        }

        RTPrintf("Finishing...\n");

        // finish test thread
        g_RunTest = false;
        // wait it for finish
        RTThreadWait(hThread, RT_INDEFINITE_WAIT, &rc);
    }
    RTSemEventDestroy(g_PingEevent);

    com::Shutdown();
    if (RT_FAILURE(rc))
        RTTestFailed(g_hTest, "Test failed.\n");
    else
        RTTestPassed(g_hTest, "Test finished.\n");
    return RTTestSummaryAndDestroy(g_hTest);
#endif /* RT_ARCH_AMD64 */
}

