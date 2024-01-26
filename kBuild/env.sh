#!/bin/sh
# $Id: env.sh 3556 2022-02-18 02:02:07Z bird $
## @file
# Environment setup script.
#

#
# Copyright (c) 2005-2010 knut st. osmundsen <bird-kBuild-spamx@anduin.net>
#
# This file is part of kBuild.
#
# kBuild is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# kBuild is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with kBuild; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
#set -x

#
# Check if we're in eval mode or not.
#
ERR_REDIR=1
DBG_REDIR=1
EVAL_OPT=
EVAL_EXPORT="export "
DBG_OPT=
QUIET_OPT=
FULL_OPT=
FULL_WITH_BIN_OPT=
LEGACY_OPT=
VAR_OPT=
VALUE_ONLY_OPT=
EXP_TYPE_OPT=
while test $# -gt 0;
do
    case "$1" in
        "--debug-script")
            DBG_OPT="true"
            ;;
        "--no-debug-script")
            DBG_OPT=
            ;;
        "--quiet")
            QUIET_OPT="true"
            ;;
        "--verbose")
            QUIET_OPT=
            ;;
        "--full")
            FULL_OPT="true"
            ;;
        "--full-with-bin")
            FULL_OPT="true"
            FULL_WITH_BIN_OPT="true"
            ;;
        "--normal")
            FULL_OPT=
            ;;
        "--legacy")
            LEGACY_OPT="true"
            ;;
        "--no-legacy")
            LEGACY_OPT=
            ;;
        "--eval")
            EVAL_OPT="true"
            ERR_REDIR=2
            DBG_REDIR=2
            ;;
        "--set")
            EVAL_OPT="true"
            EVAL_EXPORT=""
            ERR_REDIR=2
            DBG_REDIR=2
            ;;
        "--var")
            shift
            VAR_OPT="${VAR_OPT} $1"
            ERR_REDIR=2
            DBG_REDIR=2
            ;;
        "--value-only")
            VALUE_ONLY_OPT="true"
            ;;
        "--name-and-value")
            VALUE_ONLY_OPT=
            ;;
        "--release")
            EXP_TYPE_OPT=1
            KBUILD_TYPE=release
            BUILD_TYPE=
            ;;
        "--debug")
            EXP_TYPE_OPT=1
            KBUILD_TYPE=debug
            BUILD_TYPE=
            ;;
        "--profile")
            EXP_TYPE_OPT=1
            KBUILD_TYPE=profile
            BUILD_TYPE=
            ;;

        "--help")
            echo "kBuild Environment Setup Script, v0.2.0-pre"
            echo ""
            echo "syntax: $0 [options] [command [args]]"
            echo "    or: $0 [options] --var <varname>"
            echo "    or: $0 [options] --eval"
            echo "    or: $0 [options] --eval --var <varname>"
            echo ""
            echo "The first form will execute the command, or if no command is given start"
            echo "an interactive shell."
            echo "The second form will print the specfified variable(s)."
            echo "The third form will print all exported variables suitable for bourne shell"
            echo "evaluation."
            echo "The forth form will only print the specified variable(s)."
            echo ""
            echo "Options:"
            echo "  --debug, --release, --profile"
            echo "      Alternative way of specifying KBUILD_TYPE."
            echo "  --debug-script, --no-debug-script"
            echo "      Controls debug output. Default: --no-debug-script"
            echo "  --quiet, --verbose"
            echo "      Controls informational output. Default: --verbose"
            echo "  --full, --full-with-bin, --normal"
            echo "      Controls the variable set. Default: --normal"
            echo "  --legacy, --no-legacy"
            echo "      Include legacy variables in result. Default: --no-legacy"
            echo "  --value-only, --name-and-value"
            echo "      Controls what the result of a --var query. Default: --name-and-value"
            echo "  --set, --export"
            echo "      Whether --eval explicitly export the variables. --set is useful for"
            echo "      getting a list of environment vars for a commandline, while --eval"
            echo '      is useful for eval `env.sh`. Default: --export'
            echo ""
            exit 1
            ;;
        *)
            break
            ;;
    esac
    shift
done


#
# Deal with legacy environment variables.
#
if test -n "$PATH_KBUILD"; then
    if test -n "$KBUILD_PATH"  -a  "$KBUILD_PATH" != "$PATH_KBUILD"; then
        echo "$0: error: KBUILD_PATH ($KBUILD_PATH) and PATH_KBUILD ($PATH_KBUILD) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_PATH=$PATH_KBUILD
fi
if test -n "$PATH_KBUILD_BIN"; then
    if test -n "$KBUILD_BIN_PATH"  -a  "$KBUILD_BIN_PATH" != "$PATH_KBUILD_BIN"; then
        echo "$0: error: KBUILD_BIN_PATH ($KBUILD_BIN_PATH) and PATH_KBUILD_BIN ($PATH_KBUILD_BIN) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_BIN_PATH=$PATH_KBUILD_BIN
fi

if test -n "$BUILD_TYPE"; then
    if test -n "$KBUILD_TYPE"  -a  "$KBUILD_TYPE" != "$BUILD_TYPE"; then
        echo "$0: error: KBUILD_TYPE ($KBUILD_TYPE) and BUILD_TYPE ($BUILD_TYPE) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_TYPE=$BUILD_TYPE
fi

if test -n "$BUILD_PLATFORM"; then
    if test -n "$KBUILD_HOST"  -a  "$KBUILD_HOST" != "$BUILD_PLATFORM"; then
        echo "$0: error: KBUILD_HOST ($KBUILD_HOST) and BUILD_PLATFORM ($BUILD_PLATFORM) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_HOST=$BUILD_PLATFORM
fi
if test -n "$BUILD_PLATFORM_ARCH"; then
    if test -n "$KBUILD_HOST_ARCH"  -a  "$KBUILD_HOST_ARCH" != "$BUILD_PLATFORM_ARCH"; then
        echo "$0: error: KBUILD_HOST_ARCH ($KBUILD_HOST_ARCH) and BUILD_PLATFORM_ARCH ($BUILD_PLATFORM_ARCH) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_HOST_ARCH=$BUILD_PLATFORM_ARCH
fi
if test -n "$BUILD_PLATFORM_CPU"; then
    if test -n "$KBUILD_HOST_CPU"  -a  "$KBUILD_HOST_CPU" != "$BUILD_PLATFORM_CPU"; then
        echo "$0: error: KBUILD_HOST_CPU ($KBUILD_HOST_CPU) and BUILD_PLATFORM_CPU ($BUILD_PLATFORM_CPU) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_HOST_CPU=$BUILD_PLATFORM_CPU
fi

if test -n "$BUILD_TARGET"; then
    if test -n "$KBUILD_TARGET"  -a  "$KBUILD_TARGET" != "$BUILD_TARGET"; then
        echo "$0: error: KBUILD_TARGET ($KBUILD_TARGET) and BUILD_TARGET ($BUILD_TARGET) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_TARGET=$BUILD_TARGET
fi
if test -n "$BUILD_TARGET_ARCH"; then
    if test -n "$KBUILD_TARGET_ARCH"  -a  "$KBUILD_TARGET_ARCH" != "$BUILD_TARGET_ARCH"; then
        echo "$0: error: KBUILD_TARGET_ARCH ($KBUILD_TARGET_ARCH) and BUILD_TARGET_ARCH ($BUILD_TARGET_ARCH) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_TARGET_ARCH=$BUILD_TARGET_ARCH
fi
if test -n "$BUILD_TARGET_CPU"; then
    if test -n "$KBUILD_TARGET_CPU"  -a  "$KBUILD_TARGET_CPU" != "$BUILD_TARGET_CPU"; then
        echo "$0: error: KBUILD_TARGET_CPU ($KBUILD_TARGET_CPU) and BUILD_TARGET_CPU ($BUILD_TARGET_CPU) disagree." 1>&${ERR_REDIR}
        sleep 1
        exit 1
    fi
    KBUILD_TARGET_CPU=$BUILD_TARGET_CPU
fi


#
# Set default build type.
#
if test -z "$KBUILD_TYPE"; then
    KBUILD_TYPE=release
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_TYPE=$KBUILD_TYPE" 1>&${DBG_REDIR}

#
# Determin the host platform.
#
# The CPU isn't important, only the other two are.  But, since the cpu,
# arch and platform (and build type) share a common key space, try make
# sure any new additions are unique. (See header.kmk, KBUILD_OSES/ARCHES.)
#
if test -z "$KBUILD_HOST"; then
    KBUILD_HOST=`uname`
    case "$KBUILD_HOST" in
        Darwin|darwin)
            KBUILD_HOST=darwin
            ;;

        DragonFly)
            KBUILD_HOST=dragonfly
            ;;

        freebsd|FreeBSD|FREEBSD)
            KBUILD_HOST=freebsd
            ;;

        GNU)
            KBUILD_HOST=gnuhurd
            ;;

        GNU/kFreeBSD)
            KBUILD_HOST=gnukfbsd
            ;;

        GNU/kNetBSD|GNU/NetBSD)
            KBUILD_HOST=gnuknbsd
            ;;

        Haiku)
            KBUILD_HOST=haiku
            ;;

        linux|Linux|GNU/Linux|LINUX)
            KBUILD_HOST=linux
            ;;

        netbsd|NetBSD|NETBSD)
            KBUILD_HOST=netbsd
            ;;

        openbsd|OpenBSD|OPENBSD)
            KBUILD_HOST=openbsd
            ;;

        os2|OS/2|OS2)
            KBUILD_HOST=os2
            ;;

        SunOS)
            KBUILD_HOST=solaris
            ;;

        WindowsNT|CYGWIN_NT-*)
            KBUILD_HOST=win
            ;;

        *)
            echo "$0: unknown os $KBUILD_HOST" 1>&${ERR_REDIR}
            sleep 1
            exit 1
            ;;
    esac
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_HOST=$KBUILD_HOST" 1>&${DBG_REDIR}

if test -z "$KBUILD_HOST_ARCH"; then
    # Try deduce it from the cpu if given.
    if test -n "$KBUILD_HOST_CPU"; then
        case "$KBUILD_HOST_CPU" in
            i[3456789]86)
                KBUILD_HOST_ARCH='x86'
                ;;
            k8|k8l|k9|k10)
                KBUILD_HOST_ARCH='amd64'
                ;;
        esac
    fi
fi
if test -z "$KBUILD_HOST_ARCH"; then
    # Use uname -m or isainfo (lots of guesses here, please help clean this up...)
    if test "$KBUILD_HOST" = "solaris"; then
        KBUILD_HOST_ARCH=`isainfo | cut -f 1 -d ' '`

    else
        KBUILD_HOST_ARCH=`uname -m`
    fi
    case "$KBUILD_HOST_ARCH" in
        x86_64|AMD64|amd64|k8|k8l|k9|k10)
            KBUILD_HOST_ARCH='amd64'
            # Try detect debian x32.
            if test "$KBUILD_HOST" = "linux"; then 
                if test -z "${DEB_HOST_ARCH}"; then
                    DEB_HOST_ARCH=`dpkg-architecture -qDEB_HOST_ARCH 2> /dev/null`;
                    if test -z "${DEB_HOST_ARCH}"; then
                        DEB_HOST_ARCH=`dpkg --print-architecture 2> /dev/null`;
                    fi
                fi
                case "${DEB_HOST_ARCH}" in
                    "x32")
                        KBUILD_HOST_ARCH=x32
                        ;;
                    "") case "`uname -v`" in
                            *Debian*+x32+*) KBUILD_HOST_ARCH=x32 ;;
                        esac
                        ;;
                esac
            fi
            ;;
        x86|i86pc|ia32|i[3456789]86|BePC|i[3456789]86-AT[3456789]86)
            KBUILD_HOST_ARCH='x86'
            ;;
        alpha)
            KBUILD_HOST_ARCH='alpha'
            ;;
        aarch32|arm|arm1|arm2|arm3|arm6|armv1|armv2|armv3*|armv4*|armv5*|armv6*|armv7*|armv8*)
            KBUILD_HOST_ARCH='arm32'
            ;;
        aarch64*|arm64) # (Apple M1 is arm64.)
            KBUILD_HOST_ARCH='arm64'
            ;;
        hppa32|parisc32|parisc)
            KBUILD_HOST_ARCH='hppa32'
            ;;
        hppa64|parisc64)
            KBUILD_HOST_ARCH='hppa64'
            ;;
        ia64)
            KBUILD_HOST_ARCH='ia64'
            ;;
        m68k)
            KBUILD_HOST_ARCH='m68k'
            ;;
        mips32|mips)
            KBUILD_HOST_ARCH='mips32'
            ;;
        mips64)
            KBUILD_HOST_ARCH='mips64'
            ;;
        ppc32|ppc|powerpc)
            KBUILD_HOST_ARCH='ppc32'
            ;;
        ppc64|ppc64le|powerpc64|powerpc64le)
            KBUILD_HOST_ARCH='ppc64'
            ;;
        riscv64*)
            KBUILD_HOST_ARCH='riscv64'
            ;;
        riscv32*|riscv)
            KBUILD_HOST_ARCH='riscv32'
            ;;
        s390)
            KBUILD_HOST_ARCH='s390'
            ;;
        s390x)
            KBUILD_HOST_ARCH='s390x'
            ;;
	sh|sh2|sh2a|sh3|sh3|sh4|sh4a|sh4al|sh4al-dsp|shmedia)
	    KBUILD_HOST_ARCH='sh32'
	    ;;
        sh64)
	    KBUILD_HOST_ARCH='sh64'
	    ;;
        sparc32|sparc|sparcv8|sparcv7|sparcv8e)
            KBUILD_HOST_ARCH='sparc32'
            ;;
        sparc64|sparcv9)
            KBUILD_HOST_ARCH='sparc64'
            ;;

        *)  echo "$0: unknown cpu/arch - $KBUILD_HOST_ARCH" 1>&${ERR_REDIR}
            sleep 1
            exit 1
            ;;
    esac

fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_HOST_ARCH=$KBUILD_HOST_ARCH" 1>&${DBG_REDIR}

if test -z "$KBUILD_HOST_CPU"; then
    KBUILD_HOST_CPU="blend"
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_HOST_CPU=$KBUILD_HOST_CPU" 1>&${DBG_REDIR}

#
# The target platform.
# Defaults to the host when not specified.
#
if test -z "$KBUILD_TARGET"; then
    KBUILD_TARGET="$KBUILD_HOST"
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_TARGET=$KBUILD_TARGET" 1>&${DBG_REDIR}

if test -z "$KBUILD_TARGET_ARCH"; then
    KBUILD_TARGET_ARCH="$KBUILD_HOST_ARCH"
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_TARGET_ARCH=$KBUILD_TARGET_ARCH" 1>&${DBG_REDIR}

if test -z "$KBUILD_TARGET_CPU"; then
    if test "$KBUILD_TARGET_ARCH" = "$KBUILD_HOST_ARCH"; then
        KBUILD_TARGET_CPU="$KBUILD_HOST_CPU"
    else
        KBUILD_TARGET_CPU="blend"
    fi
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_TARGET_CPU=$KBUILD_TARGET_CPU" 1>&${DBG_REDIR}

#
# Determin executable extension and path separator.
#
_SUFF_EXE=
_PATH_SEP=":"
case "$KBUILD_HOST" in
    os2|win|nt)
        _SUFF_EXE=".exe"
        _PATH_SEP=";"
        ;;
esac

#
# Determin KBUILD_PATH from the script location and calc KBUILD_BIN_PATH from there.
#
if test -z "$KBUILD_PATH"; then
    KBUILD_PATH=`dirname "$0"`
    KBUILD_PATH=`cd "$KBUILD_PATH" ; /bin/pwd`
fi
if test ! -f "$KBUILD_PATH/footer.kmk" -o ! -f "$KBUILD_PATH/header.kmk" -o ! -f "$KBUILD_PATH/rules.kmk"; then
    echo "$0: error: KBUILD_PATH ($KBUILD_PATH) is not pointing to a popluated kBuild directory." 1>&${ERR_REDIR}
    sleep 1
    exit 1
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_PATH=$KBUILD_PATH" 1>&${DBG_REDIR}

if test -z "$KBUILD_BIN_PATH"; then
    KBUILD_BIN_PATH="${KBUILD_PATH}/bin/${KBUILD_HOST}.${KBUILD_HOST_ARCH}"
fi
test -n "$DBG_OPT" && echo "dbg: KBUILD_BIN_PATH=${KBUILD_BIN_PATH}" 1>&${DBG_REDIR}

#
# Add the bin/x.y/ directory to the PATH.
# NOTE! Once bootstrapped this is the only thing that is actually necessary.
#
PATH="${KBUILD_BIN_PATH}${_PATH_SEP}$PATH"
test -n "$DBG_OPT" && echo "dbg: PATH=$PATH" 1>&${DBG_REDIR}

#
# Sanity and x bits.
#
if test ! -d "${KBUILD_BIN_PATH}/"; then
    echo "$0: warning: The bin directory for this platform doesn't exist. (${KBUILD_BIN_PATH}/)" 1>&${ERR_REDIR}
else
    for prog in kmk kDepPre kDepIDB kmk_append kmk_ash kmk_cat kmk_cp kmk_echo kmk_install kmk_ln kmk_mkdir kmk_mv kmk_rm kmk_rmdir kmk_sed;
    do
        chmod a+x ${KBUILD_BIN_PATH}/${prog} > /dev/null 2>&1
        if test ! -f "${KBUILD_BIN_PATH}/${prog}${_SUFF_EXE}"; then
            echo "$0: warning: The ${prog} program doesn't exist for this platform. (${KBUILD_BIN_PATH}/${prog}${_SUFF_EXE})" 1>&${ERR_REDIR}
        fi
    done
fi

#
# The environment is in place, now take the requested action.
#
MY_RC=0
if test -n "${VAR_OPT}"; then
    # Echo variable values or variable export statements.
    for var in ${VAR_OPT};
    do
        val=
        case "$var" in
            PATH)
                val=$PATH
                ;;
            KBUILD_PATH)
                val=$KBUILD_PATH
                ;;
            KBUILD_BIN_PATH)
                val=$KBUILD_BIN_PATH
                ;;
            KBUILD_HOST)
                val=$KBUILD_HOST
                ;;
            KBUILD_HOST_ARCH)
                val=$KBUILD_HOST_ARCH
                ;;
            KBUILD_HOST_CPU)
                val=$KBUILD_HOST_CPU
                ;;
            KBUILD_TARGET)
                val=$KBUILD_TARGET
                ;;
            KBUILD_TARGET_ARCH)
                val=$KBUILD_TARGET_ARCH
                ;;
            KBUILD_TARGET_CPU)
                val=$KBUILD_TARGET_CPU
                ;;
            KBUILD_TYPE)
                val=$KBUILD_TYPE
                ;;
            *)
                echo "$0: error: Unknown variable $var specified in --var request." 1>&${ERR_REDIR}
                sleep 1
                exit 1
                ;;
        esac

        if test -n "$EVAL_OPT"; then
            echo "${EVAL_EXPORT} $var=$val"
        else
            if test -n "$VALUE_ONLY_OPT"; then
                echo "$val"
            else
                echo "$var=$val"
            fi
        fi
    done
else
    if test -n "$EVAL_OPT"; then
        # Echo statements for the shell to evaluate.
        test -n "$DBG_OPT" && echo "dbg: echoing exported variables" 1>&${DBG_REDIR}
        echo "${EVAL_EXPORT} PATH=${PATH}"
        test -n "${FULL_OPT}" -o "${EXP_TYPE_OPT}" && echo "${EVAL_EXPORT} KBUILD_TYPE=${KBUILD_TYPE}"
        if test -n "${FULL_OPT}"; then
            echo "${EVAL_EXPORT} KBUILD_PATH=${KBUILD_PATH}"
            if test -n "{FULL_WITH_BIN_OPT}"; then
                echo "${EVAL_EXPORT} KBUILD_BIN_PATH=${KBUILD_BIN_PATH}"
            fi
            echo "${EVAL_EXPORT} KBUILD_HOST=${KBUILD_HOST}"
            echo "${EVAL_EXPORT} KBUILD_HOST_ARCH=${KBUILD_HOST_ARCH}"
            echo "${EVAL_EXPORT} KBUILD_HOST_CPU=${KBUILD_HOST_CPU}"
            echo "${EVAL_EXPORT} KBUILD_TARGET=${KBUILD_TARGET}"
            echo "${EVAL_EXPORT} KBUILD_TARGET_ARCH=${KBUILD_TARGET_ARCH}"
            echo "${EVAL_EXPORT} KBUILD_TARGET_CPU=${KBUILD_TARGET_CPU}"

            if test -n "${LEGACY_OPT}"; then
                echo "${EVAL_EXPORT} PATH_KBUILD=${KBUILD_PATH}"
                if test -n "${FULL_WITH_BIN_OPT}"; then
                    echo "${EVAL_EXPORT} PATH_KBUILD_BIN=${KBUILD_PATH_BIN}"
                fi
                echo "${EVAL_EXPORT} BUILD_TYPE=${KBUILD_TYPE}"
                echo "${EVAL_EXPORT} BUILD_PLATFORM=${KBUILD_HOST}"
                echo "${EVAL_EXPORT} BUILD_PLATFORM_ARCH=${KBUILD_HOST_ARCH}"
                echo "${EVAL_EXPORT} BUILD_PLATFORM_CPU=${KBUILD_HOST_CPU}"
                echo "${EVAL_EXPORT} BUILD_TARGET=${KBUILD_TARGET}"
                echo "${EVAL_EXPORT} BUILD_TARGET_ARCH=${KBUILD_TARGET_ARCH}"
                echo "${EVAL_EXPORT} BUILD_TARGET_CPU=${KBUILD_TARGET_CPU}"
            fi
        fi
    else
        # Export variables.
        export PATH
        test -n "${FULL_OPT}" -o "${EXP_TYPE_OPT}" && export KBUILD_TYPE
        if test -n "${FULL_OPT}"; then
            export KBUILD_PATH
            if test -n "${FULL_WITH_BIN_OPT}"; then
                export KBUILD_BIN_PATH
            fi
            export KBUILD_HOST
            export KBUILD_HOST_ARCH
            export KBUILD_HOST_CPU
            export KBUILD_TARGET
            export KBUILD_TARGET_ARCH
            export KBUILD_TARGET_CPU

            if test -n "${LEGACY_OPT}"; then
                export PATH_KBUILD=$KBUILD_PATH
                if test -n "${FULL_WITH_BIN_OPT}"; then
                    export PATH_KBUILD_BIN=$KBUILD_BIN_PATH
                fi
                export BUILD_TYPE=$KBUILD_TYPE
                export BUILD_PLATFORM=$KBUILD_HOST
                export BUILD_PLATFORM_ARCH=$KBUILD_HOST_ARCH
                export BUILD_PLATFORM_CPU=$KBUILD_HOST_CPU
                export BUILD_TARGET=$KBUILD_TARGET
                export BUILD_TARGET_ARCH=$KBUILD_TARGET_ARCH
                export BUILD_TARGET_CPU=$KBUILD_TARGET_CPU
            fi
        fi

        # Execute command or spawn shell.
        if test $# -eq 0; then
            test -z "${QUIET_OPT}" && echo "$0: info: Spawning work shell..." 1>&${ERR_REDIR}
            if test "$TERM" != 'dumb'  -a  -n "$BASH"; then
                export PS1='\[\033[01;32m\]\u@\h \[\033[01;34m\]\W \$ \[\033[00m\]'
            fi
            $SHELL -i
            MY_RC=$?
        else
            test -z "${QUIET_OPT}" && echo "$0: info: Executing command: $*" 1>&${ERR_REDIR}
            $*
            MY_RC=$?
            test -z "${QUIET_OPT}" -a "$MY_RC" -ne 0 && echo "$0: info: rc=$MY_RC: $*" 1>&${ERR_REDIR}
        fi
    fi
fi
test -n "$DBG_OPT" && echo "dbg: finished (rc=$MY_RC)" 1>&${DBG_REDIR}
exit $MY_RC

