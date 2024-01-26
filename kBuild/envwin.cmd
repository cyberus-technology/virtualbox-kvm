@echo off
REM $Id: envwin.cmd 3113 2017-10-29 17:16:03Z bird $
REM REM @file
REM Environment setup script.
REM

REM
REM Copyright (c) 2005-2010 knut st. osmundsen <bird-kBuild-spamx@anduin.net>
REM
REM This file is part of kBuild.
REM
REM kBuild is free software; you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation; either version 2 of the License, or
REM (at your option) any later version.
REM
REM kBuild is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with kBuild; if not, write to the Free Software
REM Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
REM

REM
REM Globals
REM
set _KBUILD_CURDIR=%CD%
for /f "tokens=*" %%d in ('cd') do set _KBUILD_CURDIR=%%d

set _KBUILD_PATH=%KBUILD_PATH%
set _KBUILD_BIN_PATH=%KBUILD_BIN_PATH%
set _KBUILD_TYPE=%KBUILD_TYPE%
set _KBUILD_TARGET=%KBUILD_TARGET%
set _KBUILD_TARGET_ARCH=%KBUILD_TARGET_ARCH%
set _KBUILD_TARGET_CPU=%KBUILD_TARGET_CPU%
set _KBUILD_HOST=%KBUILD_HOST%
set _KBUILD_HOST_ARCH=%KBUILD_HOST_ARCH%
set _KBUILD_HOST_CPU=%KBUILD_HOST_CPU%

set _KBUILD_OVERRIDE_TYPE=0
set _KBUILD_OVERRIDE_TARGET=0
set _KBUILD_OVERRIDE_TARGET_ARCH=0

REM
REM Parse the arguments.
REM
REM Note: The 0 argument must be saved as it is also shifted.
REM
set _KBUILD_SELF=%0
set _KBUILD_OPT_FULL=0
set _KBUILD_OPT_LEGACY=0
set _KBUILD_OPT_VAR=
set _KBUILD_OPT_VALUE_ONLY=0
set _KBUILD_SHOW_VAR_PREFIX=
set _KBUILD_OPT_DBG=1
set _KBUILD_OPT_OVERRIDE_ALL=0

:argument_loop
if ".%1" == ".-h"           goto do_help
if ".%1" == "./h"           goto do_help
if ".%1" == "./H"           goto do_help
if ".%1" == ".-h"           goto do_help
if ".%1" == ".-help"        goto do_help
if ".%1" == ".--help"       goto do_help

if ".%1" == ".--win"        goto want_win
if ".%1" == ".-win"         goto want_win
if ".%1" == ".--win32"      goto want_win32_bit
if ".%1" == ".-win32"       goto want_win32_bit
if ".%1" == ".-win64"       goto want_win64_bit
if ".%1" == ".--win64"      goto want_win64_bit
if ".%1" == ".--nt"         goto want_nt
if ".%1" == ".-nt"          goto want_nt
if ".%1" == ".--nt32"       goto want_nt32_bit
if ".%1" == ".-nt32"        goto want_nt32_bit
if ".%1" == ".--nt64"       goto want_nt64_bit
if ".%1" == ".-nt64"        goto want_nt64_bit

if ".%1" == ".--release"    goto want_release
if ".%1" == ".--profile"    goto want_profile
if ".%1" == ".--debug"      goto want_debug

if ".%1" == ".--full"               goto opt_full
if ".%1" == ".--normal"             goto opt_normal
if ".%1" == ".--legacy"             goto opt_legacy
if ".%1" == ".--no-legacy"          goto opt_no_legacy
if ".%1" == ".--debug-script"       goto opt_debug_script
if ".%1" == ".--no-debug-script"    goto opt_no_debug_script
if ".%1" == ".--var"                goto opt_var
if ".%1" == ".--value-only"         goto opt_value_only
if ".%1" == ".--name-and-value"     goto opt_name_and_value
if ".%1" == ".--set"                goto opt_set
if ".%1" == ".--no-set"             goto opt_no_set

goto done_arguments

:want_win
shift
set _KBUILD_TARGET=win
set _KBUILD_OVERRIDE_TARGET=1
goto argument_loop

:want_win32_bit
shift
set _KBUILD_TARGET=win
set _KBUILD_TARGET_ARCH=x86
set _KBUILD_OVERRIDE_TARGET=1
set _KBUILD_OVERRIDE_TARGET_ARCH=1
goto argument_loop

:want_win64_bit
shift
set _KBUILD_TARGET=win
set _KBUILD_TARGET_ARCH=amd64
set _KBUILD_OVERRIDE_TARGET=1
set _KBUILD_OVERRIDE_TARGET_ARCH=1
goto argument_loop

:want_nt
shift
set _KBUILD_TARGET=nt
set _KBUILD_OVERRIDE_TARGET=1
goto argument_loop

:want_nt32_bit
shift
set _KBUILD_TARGET=nt
set _KBUILD_TARGET_ARCH=x86
set _KBUILD_OVERRIDE_TARGET=1
set _KBUILD_OVERRIDE_TARGET_ARCH=1
goto argument_loop

:want_nt64_bit
shift
set _KBUILD_TARGET=nt
set _KBUILD_TARGET_ARCH=amd64
set _KBUILD_OVERRIDE_TARGET=1
set _KBUILD_OVERRIDE_TARGET_ARCH=1
goto argument_loop

:want_release
shift
set _KBUILD_TYPE=release
set _KBUILD_OVERRIDE_TYPE=1
goto argument_loop

:want_profile
shift
set _KBUILD_TYPE=profile
set _KBUILD_OVERRIDE_TYPE=1
goto argument_loop

:want_debug
shift
set _KBUILD_TYPE=debug
set _KBUILD_OVERRIDE_TYPE=1
goto argument_loop

:opt_full
shift
set _KBUILD_OPT_FULL=1
goto argument_loop

:opt_normal
shift
set _KBUILD_OPT_FULL=0
goto argument_loop

:opt_legacy
shift
set _KBUILD_OPT_LEGACY=1
goto argument_loop

:opt_no_legacy
shift
set _KBUILD_OPT_LEGACY=0
goto argument_loop

:opt_debug_script
shift
set _KBUILD_OPT_DBG=1
goto argument_loop

:opt_no_debug_script
shift
set _KBUILD_OPT_DBG=0
goto argument_loop

:opt_var
shift
if ".%1" == "." echo syntax error: --var is missing it's variable name.
if ".%1" == "." goto failure
set _KBUILD_OPT_VAR=%_KBUILD_OPT_VAR% %1
shift
goto argument_loop

:opt_value_only
shift
set _KBUILD_OPT_VALUE_ONLY=1
goto argument_loop

:opt_name_and_value
shift
set _KBUILD_OPT_VALUE_ONLY=0
goto argument_loop

:opt_set
shift
set _KBUILD_SHOW_VAR_PREFIX=SET %_KBUILD_NON_EXISTING_VAR%
goto argument_loop

:opt_no_set
shift
set _KBUILD_SHOW_VAR_PREFIX=
goto argument_loop


REM #
REM # Syntax
REM #
:do_help
echo kBuild environment setup script for Windows NT.
echo Syntax: envwin.cmd [options] [command to be executed]
echo     or: envwin.cmd [options] --var varname
echo .
echo Options:
echo   --win
echo       Force windows target and host platform.
echo   --win32
echo       Force x86 32-bit windows target platform.
echo   --win64
echo       Force AMD64 64-bit windows target platform.
echo   --nt
echo       Force NT target and host platform.
echo   --nt32
echo       Force x86 32-bit NT target platform.
echo   --nt64
echo       Force AMD64 64-bit NT target platform.
echo   --debug, --release, --profile
echo       Alternative way of specifying KBUILD_TYPE.
echo   --full, --normal
echo       Controls the variable set. Default: --normal
echo   --legacy, --no-legacy
echo       Include legacy variables in result. Default: --legacy
echo   --value-only, --name-and-value
echo       Controls what the result of a --var query. Default: --name-and-value
echo   --set, --no-set
echo       Whether to prefix the variable output with 'SET' or not.
echo       Default: --no-set

goto end

:done_arguments

REM
REM Convert legacy variable names.
REM
if ".%_KBUILD_OPT_OVERRIDE_ALL%" == ".1" goto legacy_convertion_done

set _KBUILD_VARS=KBUILD_HOST (%_KBUILD_HOST%) and BUILD_PLATFORM (%BUILD_PLATFORM%)
if not ".%BUILD_PLATFORM%" == "." if not ".%_KBUILD_HOST%" == "." if ".%_KBUILD_HOST%" == ".%BUILD_PLATFORM%" goto legacy_mismatch
if not ".%BUILD_PLATFORM%" == "." set _KBUILD_HOST=%BUILD_PLATFORM%

set _KBUILD_VARS=KBUILD_HOST_ARCH (%_KBUILD_HOST_ARCH%) and BUILD_PLATFORM_ARCH (%BUILD_PLATFORM_ARCH%)
if not ".%BUILD_PLATFORM_ARCH%" == "." if not ".%_KBUILD_HOST_ARCH%" == "." if ".%_KBUILD_HOST_ARCH%" == ".%BUILD_PLATFORM_ARCH%" goto legacy_mismatch
if not ".%BUILD_PLATFORM_ARCH%" == "." set _KBUILD_HOST_ARCH=%BUILD_PLATFORM_ARCH%

set _KBUILD_VARS=KBUILD_HOST_CPU (%_KBUILD_HOST_CPU%) and BUILD_PLATFORM_CPU (%BUILD_PLATFORM_CPU%)
if not ".%BUILD_PLATFORM_CPU%" == "." if not ".%_KBUILD_HOST_CPU%" == "." if ".%_KBUILD_HOST_CPU%" == ".%BUILD_PLATFORM_CPU%" goto legacy_mismatch
if not ".%BUILD_PLATFORM_CPU%" == "." set _KBUILD_HOST_CPU=%BUILD_PLATFORM_CPU%

if ".%_KBUILD_OVERRIDE_TARGET%" == ".1" goto legacy_skip_target
set _KBUILD_VARS=KBUILD_TARGET (%_KBUILD_TARGET%) and BUILD_TARGET (%BUILD_TARGET%)
if not ".%BUILD_TARGET%" == "." if not ".%_KBUILD_TARGET%" == "." if ".%_KBUILD_TARGET%" == ".%BUILD_TARGET%" goto legacy_mismatch
if not ".%BUILD_TARGET%" == "." set _KBUILD_TARGET=%BUILD_TARGET%
:legacy_skip_target

if ".%_KBUILD_OVERRIDE_TARGET%" == ".1" goto legacy_skip_target_arch
set _KBUILD_VARS=KBUILD_TARGET_ARCH (%_KBUILD_TARGET_ARCH%) and BUILD_TARGET_ARCH (%BUILD_TARGET_ARCH%)
if not ".%BUILD_TARGET_ARCH%" == "." if not ".%_KBUILD_TARGET_ARCH%" == "." if ".%_KBUILD_TARGET_ARCH%" == ".%BUILD_TARGET_ARCH%" goto legacy_mismatch
if not ".%BUILD_TARGET_ARCH%" == "." set _KBUILD_TARGET_ARCH=%BUILD_TARGET_ARCH%
:legacy_skip_target_arch

if ".%_KBUILD_OVERRIDE_TARGET%" == ".1" goto legacy_skip_target_cpu
set _KBUILD_VARS=KBUILD_TARGET_CPU (%_KBUILD_TARGET_CPU%) and BUILD_TARGET_CPU (%BUILD_TARGET_CPU%)
if not ".%BUILD_TARGET_CPU%" == "." if not ".%_KBUILD_TARGET_CPU%" == "." if ".%_KBUILD_TARGET_CPU%" == ".%BUILD_TARGET_CPU%" goto legacy_mismatch
if not ".%BUILD_TARGET_CPU%" == "." set _KBUILD_TARGET_CPU=%BUILD_TARGET_CPU%
:legacy_skip_target_cpu

if ".%_KBUILD_OVERRIDE_TARGET%" == ".1" goto legacy_skip_type
set _KBUILD_VARS=KBUILD_TYPE (%_KBUILD_TYPE%) and BUILD_TYPE (%BUILD_TYPE%)
if not ".%BUILD_TYPE%" == "." if not ".%_KBUILD_TYPE%" == "." if ".%_KBUILD_TYPE%" == ".%BUILD_TYPE%" goto legacy_mismatch
if not ".%BUILD_TYPE%" == "." set _KBUILD_TYPE=%BUILD_TYPE%
:legacy_skip_type

set _KBUILD_VARS=KBUILD_PATH (%_KBUILD_PATH%) and PATH_KBUILD (%PATH_KBUILD%)
if not ".%PATH_KBUILD%" == "." if not ".%_KBUILD_PATH%" == "." if ".%_KBUILD_PATH%" == ".%PATH_KBUILD%" goto legacy_mismatch
if not ".%PATH_KBUILD%" == "." set _KBUILD_PATH=%PATH_KBUILD%
goto legacy_convertion_done

:legacy_mismatch
echo error: %_KBUILD_VARS% disagree.
goto failed

:legacy_convertion_done


REM
REM Check for illegal target/platforms.
REM
:target_and_platform
if "%_KBUILD_TARGET" == "win32"  goto illegal_target
if "%_KBUILD_TARGET" == "win64"  goto illegal_target
if "%_KBUILD_HOST"   == "win32"  goto illegal_host
if "%_KBUILD_HOST"   == "win64"  goto illegal_host
goto target_and_platform_ok

:illegal_target
echo error: KBUILD_TARGET=%KBUILD_TARGET% is no longer valid.
echo        Only 'win' and 'nt' are permitted for targeting microsoft windows.
goto failed

:illegal_host
echo error: KBUILD_HOST=%KBUILD_HOST is no longer valid.
echo        Only 'win' and 'nt' are permitted for building on microsoft windows.
goto failed

:target_and_platform_ok

REM
REM Find kBuild.
REM
REM We'll try determin the location of this script first and use that
REM as a starting point for guessing the kBuild directory.
REM

REM Check if set via KBUILD_PATH.
if not ".%_KBUILD_PATH%" == "."  if exist %_KBUILD_PATH%\footer.kmk goto found_kbuild

REM Determin a correct self - by %0
if exist "%_KBUILD_SELF%" goto found_self
set _KBUILD_SELF=%_KBUILD_SELF%.cmd
if exist "%_KBUILD_SELF%" goto found_self

REM Determin a correct self - by the PATH
REM This is very verbose because nested for loops didn't work out.
for /f "tokens=1  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=2  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=3  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=4  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=5  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=6  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=7  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=8  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=9  delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=10 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=11 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=12 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=13 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=14 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=15 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=16 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=17 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=18 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=19 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
for /f "tokens=20 delims=;" %%d in ("%PATH%") do set _KBUILD_SELF=%%d\envwin.cmd
if exist "%_KBUILD_SELF%" goto found_self
goto try_by_pwd

:found_self
cd "%_KBUILD_SELF%\.."
for /f "tokens=*" %%d in ('cd') do set _KBUILD_PATH=%%d
cd "%_KBUILD_CURDIR%"
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild

REM Try relative to the current directory.
:try_by_pwd
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild
set _KBUILD_PATH=%_KBUILD_CURDIR%
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild
set _KBUILD_PATH=%_KBUILD_CURDIR%\kBuild
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild
set _KBUILD_PATH=%_KBUILD_CURDIR%\..\kBuild
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild
set _KBUILD_PATH=%_KBUILD_CURDIR%\..\..\kBuild
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild
set _KBUILD_PATH=%_KBUILD_CURDIR%\..\..\..\kBuild
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild
set _KBUILD_PATH=%_KBUILD_CURDIR%\..\..\..\..\kBuild
if exist "%_KBUILD_PATH%\footer.kmk"    goto found_kbuild
echo kBuild: Can't find the kBuild directory!
set CURDIR=
goto failed

:found_kbuild
cd "%_KBUILD_PATH%"
for /f "tokens=*" %%d in ('cd') do set _KBUILD_PATH=%%d
cd "%_KBUILD_CURDIR%"
if ".%_KBUILD_OPT_DBG%" == ".1" echo dbg: _KBUILD_PATH=%_KBUILD_PATH%


REM
REM Type - release is the default.
REM
if not ".%_KBUILD_TYPE%" == "."         goto have_type
set _KBUILD_TYPE=release
:have_type
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_TYPE=%_KBUILD_TYPE%


REM
REM Host platform - windows
REM
if not ".%_KBUILD_HOST%" == "."         goto have_host
set _KBUILD_HOST=win
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_HOST=%_KBUILD_HOST%
:have_host

if not ".%_KBUILD_HOST_ARCH%" == "."    goto have_host_arch
REM try guess from _KBUILD_HOST_CPU
if ".%KBUILD_HOST_CPU%" == ".i386"      set _KBUILD_HOST_ARCH=x86
if ".%KBUILD_HOST_CPU%" == ".i486"      set _KBUILD_HOST_ARCH=x86
if ".%KBUILD_HOST_CPU%" == ".i586"      set _KBUILD_HOST_ARCH=x86
if ".%KBUILD_HOST_CPU%" == ".i686"      set _KBUILD_HOST_ARCH=x86
if ".%KBUILD_HOST_CPU%" == ".i786"      set _KBUILD_HOST_ARCH=x86
if ".%KBUILD_HOST_CPU%" == ".i886"      set _KBUILD_HOST_ARCH=x86
if ".%KBUILD_HOST_CPU%" == ".i986"      set _KBUILD_HOST_ARCH=x86
if ".%KBUILD_HOST_CPU%" == ".k8"        set _KBUILD_HOST_ARCH=amd64
if ".%KBUILD_HOST_CPU%" == ".k9"        set _KBUILD_HOST_ARCH=amd64
if ".%KBUILD_HOST_CPU%" == ".k10"       set _KBUILD_HOST_ARCH=amd64
if not ".%_KBUILD_HOST_ARCH%" == "."    goto have_host_arch
REM try guess from PROCESSOR_ARCHITEW6432 and PROCESSOR_ARCHITECTURE
set _KBUILD_TMP=%PROCESSOR_ARCHITECTURE%
if not ".%PROCESSOR_ARCHITEW6432%" == "." set _KBUILD_TMP=%PROCESSOR_ARCHITEW6432%
if "%_KBUILD_TMP%" == "x86"             set _KBUILD_HOST_ARCH=x86
if "%_KBUILD_TMP%" == "X86"             set _KBUILD_HOST_ARCH=x86
if "%_KBUILD_TMP%" == "amd64"           set _KBUILD_HOST_ARCH=amd64
if "%_KBUILD_TMP%" == "Amd64"           set _KBUILD_HOST_ARCH=amd64
if "%_KBUILD_TMP%" == "AMD64"           set _KBUILD_HOST_ARCH=amd64
if "%_KBUILD_TMP%" == "x64"             set _KBUILD_HOST_ARCH=amd64
if "%_KBUILD_TMP%" == "X64"             set _KBUILD_HOST_ARCH=amd64
if not ".%_KBUILD_HOST_ARCH%" == "."    goto have_host_arch
echo error: Cannot figure KBUILD_HOST_ARCH!
goto failed
:have_host_arch
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_HOST_ARCH=%_KBUILD_HOST_ARCH%

if not ".%_KBUILD_HOST_CPU%" == "."     goto have_host_cpu
set _KBUILD_HOST_CPU=blend
:have_host_cpu
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_HOST_CPU=%_KBUILD_HOST_CPU%

REM
REM The target platform.
REM Defaults to the host when not specified.
REM
if not ".%_KBUILD_TARGET%" == "."       goto have_target
set _KBUILD_TARGET=%_KBUILD_HOST%
:have_target
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_TARGET=%_KBUILD_TARGET%

if not ".%_KBUILD_TARGET_ARCH%" == "."  goto have_target_arch
set _KBUILD_TARGET_ARCH=%_KBUILD_HOST_ARCH%
:have_target_arch
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_TARGET_ARCH=%_KBUILD_TARGET_ARCH%

if not ".%_KBUILD_TARGET_CPU%" == "."  goto have_target_cpu
if ".%_KBUILD_TARGET_ARCH%" == ".%_KBUILD_HOST_ARCH%" set _KBUILD_TARGET_CPU=%_KBUILD_HOST_CPU%
if not ".%_KBUILD_TARGET_CPU%" == "."  goto have_target_cpu
set _KBUILD_TARGET_CPU=%_KBUILD_HOST_CPU%
:have_target_cpu
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_TARGET_CPU=%_KBUILD_TARGET_CPU%

REM
REM Calc KBUILD_BIN_PATH and the new PATH value.
REM
if not ".%_KBUILD_BIN_PATH%" == "."     goto have_kbuild_bin_path
set _KBUILD_BIN_PATH=%_KBUILD_PATH%\bin\win.%_KBUILD_HOST_ARCH%
:have_kbuild_bin_path
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_BIN_PATH=%_KBUILD_BIN_PATH%

set _KBUILD_NEW_PATH=%_KBUILD_BIN_PATH%;%PATH%
if ".%_KBUILD_OPT_DBG%" == ".1"         echo dbg: _KBUILD_NEW_PATH=%_KBUILD_NEW_PATH%

REM
REM Sanity check.
REM
if not exist "%_KBUILD_BIN_PATH%" goto missing_bin_path
if not exist "%_KBUILD_BIN_PATH%" goto done_chekcing_for_tools
set _KBUILD_TMP=kmk kDepPre kDepIDB kmk_append kmk_ash kmk_cat kmk_cp kmk_echo kmk_install kmk_ln kmk_mkdir kmk_mv kmk_rm kmk_rmdir kmk_sed
for %%i in ( %_KBUILD_TMP% ) do if not exist "%_KBUILD_BIN_PATH%\%%i.exe" echo warning: The %%i program doesn't exist for this platform. (%_KBUILD_BIN_PATH%\%%i.exe)"
goto done_chekcing_for_tools

:missing_bin_path
echo warning: The bin directory for this platform doesn't exist. (%_KBUILD_BIN_PATH%)
:done_chekcing_for_tools

REM
REM The environment is in place, now take the requested action.
REM
if ".%_KBUILD_OPT_VAR%" == "." goto exec_or_setup_env
goto show_variable

REM
REM Show a set of variables.
REM
REM Note: 4nt doesn't grok the setlocal delayed expansion option.
REM       So, we'll have to identify which shell we're running.
REM
:show_variable
for %%i in ( %_KBUILD_OPT_VAR% ) do if "%%i" == "all" goto show_all_variables

if not ".%_4VER%" == "." goto 4nt
setlocal ENABLEDELAYEDEXPANSION
if errorlevel 1 goto no_delay_expansion

for %%i in ( %_KBUILD_OPT_VAR% ) do (
    set _KBUILD_VAR=%%i
    set _KBUILD_TMP=
    if "%%i" == "KBUILD_PATH"         set _KBUILD_TMP=%_KBUILD_PATH%
    if "%%i" == "KBUILD_BIN_PATH"     set _KBUILD_TMP=%_KBUILD_BIN_PATH%
    if "%%i" == "KBUILD_TYPE"         set _KBUILD_TMP=%_KBUILD_TYPE%
    if "%%i" == "KBUILD_HOST"         set _KBUILD_TMP=%_KBUILD_HOST%
    if "%%i" == "KBUILD_HOST_ARCH"    set _KBUILD_TMP=%_KBUILD_HOST_ARCH%
    if "%%i" == "KBUILD_HOST_CPU"     set _KBUILD_TMP=%_KBUILD_HOST_CPU%
    if "%%i" == "KBUILD_TARGET"       set _KBUILD_TMP=%_KBUILD_TARGET%
    if "%%i" == "KBUILD_TARGET_ARCH"  set _KBUILD_TMP=%_KBUILD_TARGET_ARCH%
    if "%%i" == "KBUILD_TARGET_CPU"   set _KBUILD_TMP=%_KBUILD_TARGET_CPU%
    if ".!_KBUILD_TMP!" == "."                      goto varible_not_found
    if not ".%_KBUILD_OPT_VALUE_ONLY%" == ".1"      echo %_KBUILD_SHOW_VAR_PREFIX%%%i=!_KBUILD_TMP!
    if ".%_KBUILD_OPT_VALUE_ONLY%" == ".1"          echo !_KBUILD_TMP!
)

endlocal
goto end

:no_delay_expansion
echo error: Unable to enable delayed expansion in the shell.

:4nt
for %%i in ( %_KBUILD_OPT_VAR% ) do (
    set _KBUILD_VAR=%%i
    set _KBUILD_TMP=
    if "%%i" == "KBUILD_PATH"         set _KBUILD_TMP=%_KBUILD_PATH%
    if "%%i" == "KBUILD_BIN_PATH"     set _KBUILD_TMP=%_KBUILD_BIN_PATH%
    if "%%i" == "KBUILD_TYPE"         set _KBUILD_TMP=%_KBUILD_TYPE%
    if "%%i" == "KBUILD_HOST"         set _KBUILD_TMP=%_KBUILD_HOST%
    if "%%i" == "KBUILD_HOST_ARCH"    set _KBUILD_TMP=%_KBUILD_HOST_ARCH%
    if "%%i" == "KBUILD_HOST_CPU"     set _KBUILD_TMP=%_KBUILD_HOST_CPU%
    if "%%i" == "KBUILD_TARGET"       set _KBUILD_TMP=%_KBUILD_TARGET%
    if "%%i" == "KBUILD_TARGET_ARCH"  set _KBUILD_TMP=%_KBUILD_TARGET_ARCH%
    if "%%i" == "KBUILD_TARGET_CPU"   set _KBUILD_TMP=%_KBUILD_TARGET_CPU%
    if ".%_KBUILD_TMP%" == "."                      goto varible_not_found
    if not ".%_KBUILD_OPT_VALUE_ONLY%" == ".1"      echo %_KBUILD_SHOW_VAR_PREFIX%%i=%_KBUILD_TMP%
    if ".%_KBUILD_OPT_VALUE_ONLY%" == ".1"          echo %_KBUILD_TMP%
)
goto end

:varible_not_found
echo error: Unknown variable %_KBUILD_VAR% specified in --var request.
goto failed

:show_all_variables
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_PATH=%_KBUILD_PATH%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_BIN_PATH=%_KBUILD_BIN_PATH%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_TYPE=%_KBUILD_TYPE%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_HOST=%_KBUILD_HOST%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_HOST_ARCH=%_KBUILD_HOST_ARCH%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_HOST_CPU=%_KBUILD_HOST_CPU%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_TARGET=%_KBUILD_TARGET%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_TARGET_ARCH=%_KBUILD_TARGET_ARCH%
echo %_KBUILD_SHOW_VAR_PREFIX%KBUILD_TARGET_CPU=%_KBUILD_TARGET_CPU%
goto end

REM
REM Setup environment for the current shell or execute a command.
REM
REM Note: We use setlocal if we're going to execute a command as we
REM       don't want the environment of the invoking shell to be changed.
REM
:exec_or_setup_env
if not ".%1" == "." setlocal

REM The PATH is always set.
set PATH=%_KBUILD_NEW_PATH%

REM Clear up anything that should be overridden.
if not "%_KBUILD_OPT_OVERRIDE_ALL%" == "1" goto skip_override_all
set KBUILD_TYPE=
set KBUILD_HOST=
set KBUILD_HOST_ARCH=
set KBUILD_HOST_CPU=
set KBUILD_TARGET=
set KBUILD_TARGET_ARCH=
set KBUILD_TARGET_CPU=
set KBUILD_PATH=
set KBUILD_BIN_PATH=
if not "%_KBUILD_OPT_LEGACY%" == "1" goto skip_override_all
set BUILD_TYPE=
set BUILD_PLATFORM=
set BUILD_PLATFORM_ARCH=
set BUILD_PLATFORM_CPU=
set BUILD_TARGET=
set BUILD_TARGET_ARCH=
set BUILD_TARGET_CPU=
set PATH_KBUILD=
set PATH_KBUILD_BIN=
:skip_override_all

REM Specific overrides, these implicitly deletes the legacy variable.
if "%_KBUILD_OVERRIDE_TARGET%" == "1"       set KBUILD_TARGET=%_KBUILD_TARGET%
if "%_KBUILD_OVERRIDE_TARGET%" == "1"       set BUILD_TARGET=
if "%_KBUILD_OVERRIDE_TARGET_ARCH%" == "1"  set KBUILD_TARGET_ARCH=%_KBUILD_TARGET_ARCH%
if "%_KBUILD_OVERRIDE_TARGET_ARCH%" == "1"  set BUILD_TARGET_ARCH=
if "%_KBUILD_OVERRIDE_TYPE%" == "1"         set KBUILD_TYPE=%_KBUILD_TYPE%
if "%_KBUILD_OVERRIDE_TYPE%" == "1"         set BUILD_TYPE=

if not "%_KBUILD_OPT_FULL%" == "1"          goto env_setup_done
set KBUILD_PATH=%_KBUILD_PATH%
set KBUILD_BIN_PATH=%_KBUILD_BIN_PATH%
set KBUILD_TYPE=%_KBUILD_TYPE%
set KBUILD_HOST=%_KBUILD_HOST%
set KBUILD_HOST_ARCH=%_KBUILD_HOST_ARCH%
set KBUILD_HOST_CPU=%_KBUILD_HOST_CPU%
set KBUILD_TARGET=%_KBUILD_TARGET%
set KBUILD_TARGET_ARCH=%_KBUILD_TARGET_ARCH%
set KBUILD_TARGET_CPU=%_KBUILD_TARGET_CPU%

if not "%_KBUILD_OPT_LEGACY%" == "1"        goto env_setup_done
set PATH_KBUILD=%_KBUILD_PATH%
set PATH_KBUILD_BIN=%_KBUILD_BIN_PATH%
set BUILD_TYPE=%_KBUILD_TYPE%
set BUILD_PLATFORM=%_KBUILD_HOST%
set BUILD_PLATFORM_ARCH=%_KBUILD_HOST_ARCH%
set BUILD_PLATFORM_CPU=%_KBUILD_HOST_CPU%
set BUILD_TARGET=%_KBUILD_TARGET%
set BUILD_TARGET_ARCH=%_KBUILD_TARGET_ARCH%
set BUILD_TARGET_CPU=%_KBUILD_TARGET_CPU%

:env_setup_done
if ".%1" == "." goto end

REM Execute the specified command
if ".%_KBUILD_OPT_DBG%" == ".1" echo dbg: Executing: %1 %2 %3 %4 %5 %6 %7 %8 %9
set _KBUILD_CLEAN_GOTO=exec_command & goto cleanup
:exec_command
SET _KBUILD_CLEAN_GOTO=
%1 %2 %3 %4 %5 %6 %7 %8 %9
endlocal
goto end_no_exit



REM
REM All exit paths leads to 'end' or 'failed' depending on
REM which exit code is desire. This is required as we're manually
REM performing environment cleanup (setlocal/endlocal is crap).
REM
:cleanup
set _KBUILD_CURDIR=
set _KBUILD_PATH=
set _KBUILD_BIN_PATH=
set _KBUILD_NEW_PATH=
set _KBUILD_TYPE=
set _KBUILD_TARGET=
set _KBUILD_TARGET_ARCH=
set _KBUILD_TARGET_CPU=
set _KBUILD_HOST=
set _KBUILD_HOST_ARCH=
set _KBUILD_HOST_CPU=
set _KBUILD_OPT_OVERRIDE_ALL=
set _KBUILD_OVERRIDE_TYPE=
set _KBUILD_OVERRIDE_TARGET=
set _KBUILD_OVERRIDE_TARGET_ARCH=

set _KBUILD_SELF=
set _KBUILD_OPT_FULL=
set _KBUILD_OPT_LEGACY=
set _KBUILD_OPT_VAR=
set _KBUILD_OPT_VALUE_ONLY=
set _KBUILD_SHOW_VAR_PREFIX=
set _KBUILD_OPT_DBG=
set _KBUILD_OPT_OVERRIDE_ALL=

set _KBUILD_TMP=
set _KBUILD_VAR=
set _KBUILD_VARS=
goto %_KBUILD_CLEAN_GOTO%

:failed
set _KBUILD_CLEAN_GOTO=failed_done & goto cleanup
:failed_done
set _KBUILD_CLEAN_GOTO=
exit /b 1

:end
set _KBUILD_CLEAN_GOTO=end_done & goto cleanup
:end_done
set _KBUILD_CLEAN_GOTO=
exit /b 0

:end_no_exit

