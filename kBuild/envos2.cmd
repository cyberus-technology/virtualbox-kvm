/*
echo this is a rexx script!
cancel & quit & exit
*/
/* $Id: envos2.cmd 2413 2010-09-11 17:43:04Z bird $ */
/** @file
 * Environment setup script for OS/2.
 */

/*
 *
 * Copyright (c) 1999-2010 knut st. osmundsen <bird-kBuild-spamx@anduin.net>
 *
 * This file is part of kBuild.
 *
 * kBuild is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kBuild is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kBuild; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*
 * Setup the usual suspects.
 */
Address CMD '@echo off';
signal on novalue name NoValueHandler
if (RxFuncQuery('SysLoadFuncs') = 1) then
do
    call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs';
    call SysLoadFuncs;
end

/*
 * Apply the CMD.EXE workaround.
 */
call FixCMDEnv;

/*
 * Globals
 */
skBuildPath       = EnvGet("KBUILD_PATH");
skBuildBinPath    = EnvGet("KBUILD_BIN_PATH");
skBuildType       = EnvGet("KBUILD_TYPE");
skBuildTarget     = EnvGet("KBUILD_TARGET");
skBuildTargetArch = EnvGet("KBUILD_TARGET_ARCH");
skBuildTargetCpu  = EnvGet("KBUILD_TARGET_CPU");
skBuildHost       = EnvGet("KBUILD_HOST");
skBuildHostArch   = EnvGet("KBUILD_HOST_ARCH");
skBuildHostCpu    = EnvGet("KBUILD_HOST_CPU");

/*
 * Process arguments.
 */
fOptFull = 0
fOptLegacy = 0
fOptDbg = 0
fOptQuiet = 0
sOptVars = ""
fOptValueOnly = 0
sShowVarPrefix = "";
fOptOverrideAll = 0
fOptOverrideType = 0;
fSetType = 0;
fOptOverrideTarget = 0;
fOptOverrideTargetArch = 0;
fOptDefault = 0;

parse arg sArgs
do while (sArgs <> '')
    parse value sArgs with sArg sRest
    say 'sArgs='sArgs';'
    say ' sArg='sArg';'
    say 'sRest='sRest';'

    select
        when (sArg = "--debug-script") then do
            fOptDbg = 1;
        end
        when (sArg = "--no-debug-script") then do
            fOptDbg = 0;
        end
        when (sArg = "--quiet") then do
            fOptQuiet = 1;
        end
        when (sArg = "--verbose") then do
            fOptQuiet = 0;
        end
        when (sArg = "--full") then do
            fOptFull = 1;
        end
        when (sArg = "--normal") then do
            fOptFull = 0;
        end
        when (sArg = "--legacy") then do
            fOptLegacy = 1;
        end
        when (sArg = "--no-legacy") then do
            fOptLegacy = 0;
        end
        when (sArg = "--eval") then do
            say "error: --eval is not supported on OS/2."
        end
        when (sArg = "--var") then do
            parse value sRest with sVar sRest2
            sRest = sRest2;
            if (sVar = '') then do
                say "syntax error: --var is missing the variable name";
                call SysSleep 1
                exit 1;
            end
            if (sVar = "all" | sOptVars = "all") then
                sOptVars = "all";
            else
                sOptVars = sOptVars || " " || sVar;
        end
        when (sArg = "--set") then do
            sShowVarPrefix = "SET ";
        end
        when (sArg = "--no-set") then do
            sShowVarPrefix = "";
        end
        when (sArg = "--value-only") then do
            fOptValueOnly = 1;
        end
        when (sArg = "--name-and-value") then do
            fOptValueOnly = 0;
        end
        when (sArg = "--release") then do
            fOptOverrideType = 1;
            fSetType = 1;
            skBuildType = 'release';
        end
        when (sArg = "--debug") then do
            fOptOverrideType = 1;
            fSetType = 1;
            skBuildType = 'debug';
        end
        when (sArg = "--profile") then do
            fOptOverrideType = 1;
            fSetType = 1;
            skBuildType = 'profile';
        end
        when (sArg = "--defaults") then do
            fOptOverrideAll = 1;
            skBuildType = "";
            skBuildTarget = "";
            skBuildTargetArch = "";
            skBuildTargetCpu = "";
            skBuildHost = "";
            skBuildHostArch = "";
            skBuildHostCpu = "";
            skBuildPath = "";
            skBuildBinPath = "";
        end

        when (sArg = "--help" | sArg = "-h" | sArg = "-?" | sArg = "/?" | sArg = "/h") then do
            say "kBuild Environment Setup Script, v0.1.4"
            say ""
            say "syntax: envos2.cmd [options] [command [args]]"
            say "    or: envos2.cmd [options] --var <varname>"
            say ""
            say "The first form will execute the command, or if no command is given"
            say "modify the current invoking shell."
            say "The second form will print the specfified variable(s)."
            say ""
            say "Options:"
            say "  --debug, --release, --profile"
            say "      Alternative way of specifying KBUILD_TYPE."
            say "  --defaults"
            say "      Enforce defaults for all the KBUILD_* values."
            say "  --debug-script, --no-debug-script"
            say "      Controls debug output. Default: --no-debug-script"
            say "  --quiet, --verbose"
            say "      Controls informational output. Default: --verbose"
            say "  --full, --normal"
            say "      Controls the variable set. Default: --normal"
            say "  --legacy, --no-legacy"
            say "      Include legacy variables in result. Default: --no-legacy"
            say "  --value-only, --name-and-value"
            say "      Controls what the result of a --var query. Default: --name-and-value"
            say "  --set, --no-set"
            say "      Whether to prefix the variable output with 'SET'."
            say "      Default: --no-set"
            say ""
            exit 1
        end

        when (sArg = "--") then do
            sArgs = sRest;
            leave;
        end

        when (left(sArg, 2) = '--') then do
            say 'syntax error: unknown option: '||sArg
            call SysSleep 1
            exit 1
        end

        otherwise do
            leave
        end
    end
    sArgs = sRest;
end
sCommand = strip(sArgs);

/*
 * Deal with legacy environment variables.
 */
if (\fOptOverrideAll) then do
    if (EnvGet("PATH_KBUILD") <> '') then do
        if (skBuildPath <> '' & skBuildPath <> EnvGet("PATH_KBUILD")) then do
            say "error: KBUILD_PATH ("||skBuildPath||") and PATH_KBUILD ("||EnvGet("PATH_KBUILD")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildPath = EnvGet("PATH_KBUILD");
    end

    if (EnvGet("PATH_KBUILD_BIN") <> '') then do
        if (skBuildPath <> '' & skBuildBinPath <> EnvGet("PATH_KBUILD_BIN")) then do
            say "error: KBUILD_BIN_PATH ("||skBuildBinPath||") and PATH_KBUILD_BIN ("||EnvGet("PATH_KBUILD_BIN")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildBinPath = EnvGet("PATH_KBUILD_BIN");
    end

    if (EnvGet("BUILD_TYPE") <> '') then do
        if (skBuildType <> '' & skBuildType <> EnvGet("BUILD_TYPE")) then do
            say "error: KBUILD_TYPE ("||skBuildType||") and BUILD_TYPE ("||EnvGet("BUILD_TYPE")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildType = EnvGet("BUILD_TYPE");
    end

    if (EnvGet("BUILD_TARGET") <> '') then do
        if (skBuildTarget <> '' & skBuildTarget <> EnvGet("BUILD_TARGET")) then do
            say "error: KBUILD_TARGET ("||skBuildTarget||") and BUILD_TARGET ("||EnvGet("BUILD_TARGET")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildTarget = EnvGet("BUILD_TARGET");
    end

    if (EnvGet("BUILD_TARGET_ARCH") <> '') then do
        if (skBuildTargetArch <> '' & skBuildTargetArch <> EnvGet("BUILD_TARGET_ARCH")) then do
            say "error: KBUILD_TARGET_ARCH ("||skBuildTargetArch ||") and BUILD_TARGET_ARCH ("||EnvGet("BUILD_TARGET_ARCH")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildTargetArch = EnvGet("BUILD_TARGET_ARCH");
    end

    if (EnvGet("BUILD_TARGET_CPU") <> '') then do
        if (skBuildTargetCpu <> '' & skBuildTargetCpu <> EnvGet("BUILD_TARGET_CPU")) then do
            say "error: KBUILD_TARGET_CPU ("||skBuildTargetCpu ||") and BUILD_TARGET_CPU ("||EnvGet("BUILD_TARGET_CPU")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildTargetCpu = EnvGet("BUILD_TARGET_CPU");
    end

    if (EnvGet("BUILD_PLATFORM") <> '') then do
        if (skBuildHost <> '' & skBuildHost <> EnvGet("BUILD_PLATFORM")) then do
            say "error: KBUILD_HOST ("||skBuildHost||") and BUILD_PLATFORM ("||EnvGet("BUILD_PLATFORM")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        if (skBuildHost = '' & EnvGet("BUILD_PLATFORM") = "OS2") then do
            say "error: BUILD_PLATFORM=OS2! Please unset it or change it to 'os2'."
            call SysSleep 1;
            exit 1;
        end
        skBuildHost = EnvGet("BUILD_PLATFORM");
    end

    if (EnvGet("BUILD_PLATFORM_ARCH") <> '') then do
        if (skBuildHostArch <> '' & skBuildHostArch <> EnvGet("BUILD_PLATFORM_ARCH")) then do
            say "error: KBUILD_HOST_ARCH ("||skBuildHostArch ||") and BUILD_PLATFORM_ARCH ("||EnvGet("BUILD_PLATFORM_ARCH")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildHostArch = EnvGet("BUILD_PLATFORM_ARCH");
    end

    if (EnvGet("BUILD_PLATFORM_CPU") <> '') then do
        if (skBuildHostCpu <> '' & skBuildHostCpu <> EnvGet("BUILD_PLATFORM_CPU")) then do
            say "error: KBUILD_HOST_CPU ("||skBuildHostCpu ||") and BUILD_PLATFORM_CPU ("||EnvGet("BUILD_PLATFORM_CPU")||") disagree."
            call SysSleep 1;
            exit 1;
        end
        skBuildHostCpu = EnvGet("BUILD_PLATFORM_CPU");
    end
end

/*
 * Set default build type.
 */
if (skBuildType = '') then
    skBuildType = 'release';
if (fOptDbg <> 0) then say "dbg: KBUILD_TYPE="||skBuildType

/*
 * Determin the host platform (OS/2)
 */
if (skBuildHost = '') then
    skBuildHost = 'os2';
if (fOptDbg <> 0) then say "dbg: KBUILD_HOST="||skBuildHost

if (skBuildHostArch = '') then do
    select
        when (skBuildHostCpu = 'i386',
            | skBuildHostCpu = 'i486',
            | skBuildHostCpu = 'i586',
            | skBuildHostCpu = 'i686',
            | skBuildHostCpu = 'i786',
            | skBuildHostCpu = 'i886',
            | skBuildHostCpu = 'i986') then do
            skBuildHostArch = "x86";
        end
        otherwise do
            skBuildHostArch = "x86";
        end
    end
end
if (fOptDbg <> 0) then say "dbg: KBUILD_HOST_ARCH="||skBuildHostArch

if (skBuildHostCpu = '') then
    skBuildHostCpu = 'blend';
if (fOptDbg <> 0) then say "dbg: KBUILD_HOST_CPU="||skBuildHostCpu


/*
 * The target platform.
 * Defaults to the host when not specified.
 */
if (skBuildTarget = '') then
    skBuildTarget = skBuildHost;
if (fOptDbg <> 0) then say "dbg: KBUILD_TARGET="||skBuildTarget

if (skBuildTargetArch = '') then
    skBuildTargetArch = skBuildHostArch;
if (fOptDbg <> 0) then say "dbg: KBUILD_TARGET_ARCH="||skBuildTargetArch

if (skBuildTargetCpu = '') then do
    if (skBuildTargetArch = skBuildHostArch) then
        skBuildTargetCpu = skBuildHostCpu;
    else
        skBuildTargetCpu = "blend";
end
if (fOptDbg <> 0) then say "dbg: KBUILD_TARGET_CPU="||skBuildTargetCpu


/*
 * Determin KBUILD_PATH from the script location and calc KBUILD_BIN_PATH from there.
 */
if (skBuildPath = '') then do
    skBuildPath = GetScriptDir()
end
skBuildPath = translate(skBuildPath, '/', '\')
if (  FileExists(skBuildPath||"/footer.kmk") = 0,
    | FileExists(skBuildPath||"/header.kmk") = 0,
    | FileExists(skBuildPath||"/rules.kmk") = 0) then do
    say "error: KBUILD_PATH ("skBuildPath||") is not pointing to a popluated kBuild directory."
    call SysSleep 1
    exit 1
end
if (fOptDbg <> 0) then say "dbg: KBUILD_PATH="||skBuildPath;

if (skBuildBinPath = '') then do
    skBuildBinPath = skBuildPath||'/bin/'||skBuildHost||'.'||skBuildHostArch;
end
skBuildBinPath = translate(skBuildBinPath, '/', '\')
if (fOptDbg <> 0) then say "dbg: KBUILD_BIN_PATH="||skBuildBinPath;

/*
 * Add the bin/x.y/ directory to the PATH and BEGINLIBPATH.
 * NOTE! Once bootstrapped this is the only thing that is actually necessary.
 */
sOldPath = EnvGet("PATH");
call EnvAddFront 0, "PATH", translate(skBuildBinPath, '\', '/');
sNewPath = EnvGet("PATH");
call EnvSet 0, "PATH", sOldPath;
if (fOptDbg <> 0) then say "dbg: PATH="||sNewPath;

sOldBeginLibPath = EnvGet("BEGINLIBPATH");
call EnvAddFront 0, "BEGINLIBPATH", translate(skBuildBinPath, '\', '/');
sNewBeginLibPath = EnvGet("BEGINLIBPATH");
call EnvSet 0, "BEGINLIBPATH", sOldBeginLibPath;
if (fOptDbg <> 0) then say "dbg: BEGINLIBPATH="||sNewBeginLibPath;

/*
 * Sanity check
 */
if (DirExists(skBuildBinPath) = 0) then
    say "warning: The bin directory for the build platform doesn't exist. ("||skBuildBinPath||")";
else
do
    sPrograms = "kmk kDepPre kDepIDB kmk_append kmk_ash kmk_cat kmk_cp kmk_echo kmk_install kmk_ln kmk_mkdir kmk_mv kmk_redirect kmk_rm kmk_rmdir kmk_sed";
    do i = 1 to words(sPrograms)
        sProgram = word(sPrograms, i);
        if (FileExists(skBuildBinPath||"\"||sProgram||".exe") = 0) then
            say "warning: The "||sProgram||" program doesn't exit for this platform. ("||skBuildBinPath||")";
    end
end


/*
 * The environment is in place, now take the requested action.
 */
iRc = 0;
if (sOptVars <> '') then
do
    if (sOptVars = "all") then
        sOptVars = "KBUILD_PATH KBUILD_BIN_PATH KBUILD_TYPE ",
                || "KBUILD_TARGET KBUILD_TARGET_ARCH KBUILD_TARGET_CPU ",
                || "KBUILD_HOST KBUILD_HOST_ARCH KBUILD_HOST_CPU ";

    /* Echo variable values or variable export statements. */
    do i = 1 to words(sOptVars)
        sVar = word(sOptVars, i)
        sVal = '';
        select
            when (sVar = "PATH") then               sVal = sNewPath;
            when (sVar = "BEGINLIBPATH") then       sVal = sNewBeginLibPath;
            when (sVar = "KBUILD_PATH") then        sVal = skBuildPath;
            when (sVar = "KBUILD_BIN_PATH") then    sVal = skBuildBinPath;
            when (sVar = "KBUILD_TYPE") then        sVal = skBuildType;
            when (sVar = "KBUILD_HOST") then        sVal = skBuildHost;
            when (sVar = "KBUILD_HOST_ARCH") then   sVal = skBuildHostArch;
            when (sVar = "KBUILD_HOST_CPU") then    sVal = skBuildHostCpu;
            when (sVar = "KBUILD_TARGET") then      sVal = skBuildTarget;
            when (sVar = "KBUILD_TARGET_ARCH") then sVal = skBuildTargetArch;
            when (sVar = "KBUILD_TARGET_CPU") then  sVal = skBuildTargetCpu;
            otherwise do
                say "error: Unknown variable "||sVar||" specified in --var request."
                call SysSleep 1
                exit 1
            end

        end
        if (fOptValueOnly <> 0) then
            say sVal
        else
            say sShowVarPrefix||sVar||"="||sVal;
    end
end
else
do
    /* Wipe out all variables - legacy included - with --default. */
    if (fOptOverrideAll <> 0) then do
        call EnvSet 0, KBUILD_PATH, ''
        call EnvSet 0, KBUILD_BIN_PATH, ''
        call EnvSet 0, KBUILD_HOST, ''
        call EnvSet 0, KBUILD_HOST_ARCH, ''
        call EnvSet 0, KBUILD_HOST_CPU, ''
        call EnvSet 0, KBUILD_TARGET, ''
        call EnvSet 0, KBUILD_TARGET_ARCH, ''
        call EnvSet 0, KBUILD_TARGET_CPU, ''

        call EnvSet 0, PATH_KBUILD, ''
        call EnvSet 0, PATH_KBUILD_BIN, ''
        call EnvSet 0, BUILD_PLATFORM, ''
        call EnvSet 0, BUILD_PLATFORM_ARCH, ''
        call EnvSet 0, BUILD_PLATFORM_CPU, ''
        call EnvSet 0, BUILD_TARGET, ''
        call EnvSet 0, BUILD_TARGET_ARCH, ''
        call EnvSet 0, BUILD_TARGET_CPU, ''
    end

    /* Export the variables. */
    call EnvSet 0, "PATH", sNewPath
    call EnvSet 0, "BEGINLIBPATH", sNewBeginLibPath
    if (fOptOverrideType <> 0) then call EnvSet 0, "KBUILD_TYPE", skBuildType
    if (fOptFull <> 0) then do
        call EnvSet 0, KBUILD_PATH,         skBuildPath
        call EnvSet 0, KBUILD_HOST,         skBuildHost
        call EnvSet 0, KBUILD_HOST_ARCH,    skBuildHostArch
        call EnvSet 0, KBUILD_HOST_CPU,     skBuildHostCpu
        call EnvSet 0, KBUILD_TARGET,       skBuildTarget
        call EnvSet 0, KBUILD_TARGET_ARCH,  skBuildTargetArch
        call EnvSet 0, KBUILD_TARGET_CPU,   skBuildTargetCpu

        if (fOptLegacy <> 0) then do
            call EnvSet 0, PATH_KBUILD,         skBuildPath
            call EnvSet 0, BUILD_PLATFORM,      skBuildHost
            call EnvSet 0, BUILD_PLATFORM_ARCH, skBuildHostArch
            call EnvSet 0, BUILD_PLATFORM_CPU,  skBuildHostCpu
            call EnvSet 0, BUILD_TARGET,        skBuildTarget
            call EnvSet 0, BUILD_TARGET_ARCH,   skBuildTargetArch
            call EnvSet 0, BUILD_TARGET_CPU,    skBuildTargetCpu
        end
    end

    /*
     * Execute left over arguments.
     */
    if (strip(sCommand) <> '') then do
        if (fOptQuiet <> 0) then say "info: Executing command: "|| sCommand
        address CMD sCommand
        iRc = rc;
        if (fOptQuiet <> 0 & iRc <> 0) then say "info: rc="||iRc||": "|| sCommand
    end
end
if (fOptDbg <> 0) then say "dbg: finished (rc="||rc||")"
exit (iRc);


/*******************************************************************************
*   Procedure Section                                                          *
*******************************************************************************/

/**
 * Give the script syntax
 */
syntax: procedure
    say 'syntax: envos2.cmd [command to be executed and its arguments]'
    say ''
return 0;


/**
 * No value handler
 */
NoValueHandler:
    say 'NoValueHandler: line 'SIGL;
exit(16);



/**
 * Add sToAdd in front of sEnvVar.
 * Note: sToAdd now is allowed to be alist!
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddFront: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with an ';'. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove ';' at start of sToAdd */
    if (substr(sToAdd, 1, 1) = ';') then
        sToAdd = substr(sToAdd, 2);

    /* loop thru sToAdd */
    rc = 0;
    i = length(sToAdd);
    do while i > 1 & rc = 0
        j = lastpos(sSeparator, sToAdd, i-1);
        rc = EnvAddFront2(fRM, sEnvVar, substr(sToAdd, j+1, i - j), sSeparator);
        i = j;
    end

return rc;

/**
 * Add sToAdd in front of sEnvVar.
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddFront2: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with a separator. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove the separator at start of sToAdd */
    if (substr(sToAdd, 1, 1) = sSeparator) then
        sToAdd = substr(sToAdd, 2);

    /* Get original variable value */
    sOrgEnvVar = EnvGet(sEnvVar);

    /* Remove previously sToAdd if exists. (Changing sOrgEnvVar). */
    i = pos(translate(sToAdd), translate(sOrgEnvVar));
    if (i > 0) then
        sOrgEnvVar = substr(sOrgEnvVar, 1, i-1) || substr(sOrgEnvVar, i + length(sToAdd));

    /* set environment */
    if (fRM) then
        return EnvSet(0, sEnvVar, sOrgEnvVar);
return EnvSet(0, sEnvVar, sToAdd||sOrgEnvVar);


/**
 * Add sToAdd as the end of sEnvVar.
 * Note: sToAdd now is allowed to be alist!
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddEnd: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with a separator. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove ';' at start of sToAdd */
    if (substr(sToAdd, 1, 1) = sSeparator) then
        sToAdd = substr(sToAdd, 2);

    /* loop thru sToAdd */
    rc = 0;
    i = length(sToAdd);
    do while i > 1 & rc = 0
        j = lastpos(sSeparator, sToAdd, i-1);
        rc = EnvAddEnd2(fRM, sEnvVar, substr(sToAdd, j+1, i - j), sSeparator);
        i = j;
    end

return rc;


/**
 * Add sToAdd as the end of sEnvVar.
 *
 * Known features: Don't remove sToAdd from original value if sToAdd
 *                 is at the end and don't end with a ';'.
 */
EnvAddEnd2: procedure
    parse arg fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* checks that sToAdd ends with a separator. Adds one if not. */
    if (substr(sToAdd, length(sToAdd), 1) <> sSeparator) then
        sToAdd = sToAdd || sSeparator;

    /* check and evt. remove separator at start of sToAdd */
    if (substr(sToAdd, 1, 1) = sSeparator) then
        sToAdd = substr(sToAdd, 2);

    /* Get original variable value */
    sOrgEnvVar = EnvGet(sEnvVar);

    if (sOrgEnvVar <> '') then
    do
        /* Remove previously sToAdd if exists. (Changing sOrgEnvVar). */
        i = pos(translate(sToAdd), translate(sOrgEnvVar));
        if (i > 0) then
            sOrgEnvVar = substr(sOrgEnvVar, 1, i-1) || substr(sOrgEnvVar, i + length(sToAdd));

        /* checks that sOrgEnvVar ends with a separator. Adds one if not. */
        if (sOrgEnvVar = '') then
            if (right(sOrgEnvVar,1) <> sSeparator) then
                sOrgEnvVar = sOrgEnvVar || sSeparator;
    end

    /* set environment */
    if (fRM) then return EnvSet(0, sEnvVar, sOrgEnvVar);
return EnvSet(0, sEnvVar, sOrgEnvVar||sToAdd);


/**
 * Sets sEnvVar to sValue.
 */
EnvSet: procedure
    parse arg fRM, sEnvVar, sValue

    /* if we're to remove this, make valuestring empty! */
    if (fRM) then
        sValue = '';
    sEnvVar = translate(sEnvVar);

    /*
     * Begin/EndLibpath fix:
     *      We'll have to set internal these using both commandline 'SET'
     *      and internal VALUE in order to export it and to be able to
     *      get it (with EnvGet) again.
     */
    if ((sEnvVar = 'BEGINLIBPATH') | (sEnvVar = 'ENDLIBPATH')) then
    do
        if (length(sValue) >= 1024) then
            say 'Warning: 'sEnvVar' is too long,' length(sValue)' char.';
        return SysSetExtLibPath(sValue, substr(sEnvVar, 1, 1));
    end

    if (length(sValue) >= 1024) then
    do
        say 'Warning: 'sEnvVar' is too long,' length(sValue)' char.';
        say '    This may make CMD.EXE unstable after a SET operation to print the environment.';
    end
    sRc = VALUE(sEnvVar, sValue, 'OS2ENVIRONMENT');
return 0;


/**
 * Gets the value of sEnvVar.
 */
EnvGet: procedure
    parse arg sEnvVar
    if ((translate(sEnvVar) = 'BEGINLIBPATH') | (translate(sEnvVar) = 'ENDLIBPATH')) then
        return SysQueryExtLibPath(substr(sEnvVar, 1, 1));
return value(sEnvVar,, 'OS2ENVIRONMENT');


/**
 * Checks if a file exists.
 * @param   sFile       Name of the file to look for.
 * @param   sComplain   Complaint text. Complain if non empty and not found.
 * @returns TRUE if file exists.
 *          FALSE if file doesn't exists.
 */
FileExists: procedure
    parse arg sFile, sComplain
    rc = stream(sFile, 'c', 'query exist');
    if ((rc = '') & (sComplain <> '')) then
        say sComplain ''''sFile'''.';
return rc <> '';


/**
 * Checks if a directory exists.
 * @param   sDir        Name of the directory to look for.
 * @param   sComplain   Complaint text. Complain if non empty and not found.
 * @returns TRUE if file exists.
 *          FALSE if file doesn't exists.
 */
DirExists: procedure
    parse arg sDir, sComplain
    rc = SysFileTree(sDir, 'sDirs', 'DO');
    if (rc = 0 & sDirs.0 = 1) then
        return 1;
    if (sComplain <> '') then
        say sComplain ''''sDir'''.';
return 0;


/**
 *  Workaround for bug in CMD.EXE.
 *  It messes up when REXX have expanded the environment.
 */
FixCMDEnv: procedure
/* do this anyway
    /* check for 4OS2 first */
    Address CMD 'set 4os2test_env=%@eval[2 + 2]';
    if (value('4os2test_env',, 'OS2ENVIRONMENT') = '4') then
        return 0;
*/

    /* force environment expansion by setting a lot of variables and freeing them.
     * ~6600 (bytes) */
    do i = 1 to 100
        Address CMD '@set dummyenvvar'||i'=abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    end
    do i = 1 to 100
        Address CMD '@set dummyenvvar'||i'=';
    end
return 0;


/**
 * Translate a string to lower case.
 */
ToLower: procedure
    parse arg sString
return translate(sString, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ');


/**
 * Gets the script directory.
 */
GetScriptDir: procedure
    /*
     * Assuming this script is in the root directory, we can determing
     * the abs path to it by using the 'parse source' feature in rexx.
     */
    parse source . . sScript
    sScriptDir = filespec('drive', sScript) || strip(filespec('path', sScript), 'T', '\');
return ToLower(sScriptDir);

