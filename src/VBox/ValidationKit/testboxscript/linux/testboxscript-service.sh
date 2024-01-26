#!/bin/sh
## @file
# VirtualBox Validation Kit - TestBoxScript service init script.
#

#
# Copyright (C) 2006-2023 Oracle and/or its affiliates.
#
# This file is part of VirtualBox base platform packages, as
# available from https://www.virtualbox.org.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, in version 3 of the
# License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses>.
#
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
# in the VirtualBox distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#
# SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
#

# chkconfig: 35 35 65
# description: TestBoxScript service
#
### BEGIN INIT INFO
# Provides:       testboxscript-service
# Required-Start: $network
# Required-Stop:
# Default-Start:  2 3 4 5
# Default-Stop:   0 1 6
# Description:    TestBoxScript service
### END INIT INFO


PATH=$PATH:/bin:/sbin:/usr/sbin

#
# Load config and set up defaults.
#
service_name="testboxscript"

[ -r /etc/default/${service_name} ] && . /etc/default/${service_name}
[ -r /etc/conf.d/${service_name}  ] && . /etc/conf.d/${service_name}

if [ -z "${TESTBOXSCRIPT_DIR}" ]; then
    TESTBOXSCRIPT_DIR="/opt/testboxscript"
fi
if [ -z "${TESTBOXSCRIPT_USER}" ]; then
    TESTBOXSCRIPT_USER="vbox"
fi
binary="${TESTBOXSCRIPT_DIR}/testboxscript/testboxscript.py"
binary_real="${TESTBOXSCRIPT_DIR}/testboxscript/testboxscript_real.py"



#
# Detect and abstract distro
#
[ -f /etc/debian_release -a -f /lib/lsb/init-functions ] || NOLSB=yes

system=unknown
if [ -f /etc/redhat-release ]; then
    system=redhat
    PIDFILE="/var/run/${service_name}-service.pid"
elif [ -f /etc/SuSE-release ]; then
    system=suse
    PIDFILE="/var/lock/subsys/${service_name}-service"
elif [ -f /etc/debian_version ]; then
    system=debian
    PIDFILE="/var/run/${service_name}-service"
elif [ -f /etc/gentoo-release ]; then
    system=gentoo
    PIDFILE="/var/run/${service_name}-service"
elif [ -f /etc/arch-release ]; then
     system=arch
     PIDFILE="/var/run/${service_name}-service"
elif [ -f /etc/slackware-version ]; then
    system=slackware
    PIDFILE="/var/run/${service_name}-service"
elif [ -f /etc/lfs-release ]; then
    system=lfs
    PIDFILE="/var/run/${service_name}-service.pid"
else
    system=other
    if [ -d /var/run -a -w /var/run ]; then
        PIDFILE="/var/run/${service_name}-service"
    fi
fi


#
# Generic implementation.
#

## Query daemon status.
# $1 = daemon-user; $2 = binary name
# returns 0 if running, 1 if started but no longer running, 3 if not started.
# When 0 is return the pid variable contains a list of relevant pids.
my_query_status() {
    a_USER="$1";
    a_BINARY="$2";
    pid="";
    if [ -f "${PIDFILE}" -a -s "${PIDFILE}" ]; then
        MY_LINE="";
        read MY_LINE < "${PIDFILE}";
        for MY_PID in `echo $MY_LINE | sed -e 's/[^0123456789 ]/ /g'`;
        do
            if [ "`stat -c '%U' /proc/$MY_PID 2> /dev/null `" = "$a_USER" ]; then
                pid="${pid} ${MY_PID}";
            fi
        done
        if [ -n "${pid}" ]; then
            RETVAL=0;
        else
            RETVAL=1;
        fi
    else
        RETVAL=3
    fi
    return $RETVAL;
}

## Starts detached daeamon in screen or tmux.
# $1 = daemon-user; $2+ = daemon and its arguments
my_start_daemon() {
    a_USER="$1"
    shift
    if touch "${PIDFILE}" && chown "${a_USER}" -- "${PIDFILE}"; then
        ARGS=""
        while [ $# -gt 0 ];
        do
            ARGS="$ARGS '$1'";
            shift
        done
        ARGS="$ARGS --pidfile '$PIDFILE'";
        if type screen > /dev/null; then
            su - "${a_USER}" -c "screen -S ${service_name} -d -m ${ARGS}";
        elif type tmux > /dev/null; then
            su - "${a_USER}" -c "tmux new-session -AdD -s ${service_name} ${ARGS}";
        else
            echo "Need screen or tmux, please install!"
            exit 1
        fi
        RETVAL=$?;
        if [ $RETVAL -eq 0 ]; then
            sleep 0.6;
            if [ ! -s "$PIDFILE" ]; then sleep 1; fi
            if [ ! -s "$PIDFILE" ]; then sleep 2; fi
            if [ ! -s "$PIDFILE" ]; then sleep 3; fi
            if [ -s "$PIDFILE" ]; then
                RETVAL=0;
            else
                RETVAL=1;
            fi
        else
            fail_msg "su failed with exit code $RETVAL";
        fi
    else
        fail_msg "Failed to create pid file and change it's ownership to ${a_USER}."
        RETVAL=1;
    fi
    return $RETVAL;
}

## Stops the daemon.
# $1 = daemon-user; $2 = binary name
my_stop_daemon() {
    a_USER="$1";
    a_BINARY="$2";
    my_query_status "$a_USER" "$a_BINARY"
    RETVAL=$?
    if [ $RETVAL -eq 0 -a -n "$pid" ]; then
        kill $pid;
    fi
    sleep 0.6
    if my_query_status "$a_USER" "$a_BINARY"; then sleep 1; fi
    if my_query_status "$a_USER" "$a_BINARY"; then sleep 2; fi
    if my_query_status "$a_USER" "$a_BINARY"; then sleep 3; fi
    if ! my_query_status "$a_USER" "$a_BINARY"; then
        rm -f -- "${PIDFILE}"
        return 0;
    fi
    return 1;
}

if [ -z "$NOLSB" ]; then
    . /lib/lsb/init-functions
    fail_msg() {
        echo ""
        log_failure_msg "$1"
    }
    succ_msg() {
        log_success_msg " done."
    }
    begin_msg() {
        log_daemon_msg "$@"
    }
else
    fail_msg() {
        echo " ...fail!"
        echo "$@"
    }
    succ_msg() {
        echo " ...done."
    }
    begin_msg() {
        echo -n "$1"
    }
fi

#
# System specific overrides.
#

if [ "$system" = "redhat" ]; then
    . /etc/init.d/functions
    if [ -n "$NOLSB" ]; then
        fail_msg() {
            echo_failure
            echo
        }
        succ_msg() {
            echo_success
            echo
        }
        begin_msg() {
            echo -n "$1"
        }
    fi
fi

if [ "$system" = "suse" ]; then
    . /etc/rc.status
    if [ -n "$NOLSB" ]; then
        fail_msg() {
            rc_failed 1
            rc_status -v
        }
        succ_msg() {
            rc_reset
            rc_status -v
        }
        begin_msg() {
            echo -n "$1"
        }
    fi
fi

if [ "$system" = "debian" ]; then
    # Share my_start_daemon and my_stop_daemon with gentoo
    if [ -n "$NOLSB" ]; then
        fail_msg() {
            echo " ...fail!"
        }
        succ_msg() {
            echo " ...done."
        }
        begin_msg() {
            echo -n "$1"
       }
    fi
fi

if [ "$system" = "gentoo" ]; then
    if [ -f /sbin/functions.sh ]; then
        . /sbin/functions.sh
    elif [ -f /etc/init.d/functions.sh ]; then
        . /etc/init.d/functions.sh
    fi
    # Share my_start_daemon and my_stop_daemon with debian.
    if [ -n "$NOLSB" ]; then
        if [ "`which $0`" = "/sbin/rc" ]; then
            shift
        fi
    fi
fi

if [ "$system" = "debian" -o  "$system" = "gentoo" ]; then
    #my_start_daemon() {
    #    usr="$1"
    #    shift
    #    bin="$1"
    #    shift
    #    echo usr=$usr
    #    start-stop-daemon --start --background  --pidfile "${PIDFILE}" --make-pidfile --chuid "${usr}" --user "${usr}" \
    #        --exec $bin -- $@
    #}
    my_stop_daemon() {
        a_USER="$1"
        a_BINARY="$2"
        start-stop-daemon --stop --user "${a_USER}" --pidfile "${PIDFILE}"
        RETVAL=$?
        rm -f "${PIDFILE}"
        return $RETVAL
    }
fi

if [ "$system" = "arch" ]; then
    USECOLOR=yes
    . /etc/rc.d/functions
    if [ -n "$NOLSB" ]; then
        fail_msg() {
            stat_fail
        }
        succ_msg() {
            stat_done
        }
        begin_msg() {
            stat_busy "$1"
        }
    fi
fi

if [ "$system" = "lfs" ]; then
    . /etc/rc.d/init.d/functions
    if [ -n "$NOLSB" ]; then
        fail_msg() {
            echo_failure
        }
        succ_msg() {
            echo_ok
        }
        begin_msg() {
            echo $1
        }
    fi
fi

#
# Implement the actions.
#
check_single_user() {
    if [ -n "$2" ]; then
        fail_msg "TESTBOXSCRIPT_USER must not contain multiple users!"
        exit 1
    fi
}

#
# Open ports at the firewall:
#   6000..6100 / TCP for VRDP
#   5000..5032 / TCP for netperf
#   5000..5032 / UDP for netperf
#
set_iptables() {
    if [ -x /sbin/iptables ]; then
        I="/sbin/iptables -j ACCEPT -A INPUT -m state --state NEW"
        if ! /sbin/iptables -L INPUT | grep -q "testsuite vrdp"; then
            $I -m tcp -p tcp --dport 6000:6100 -m comment --comment "testsuite vrdp"
        fi
        if ! /sbin/iptables -L INPUT | grep -q "testsuite perftcp"; then
            $I -m tcp -p tcp --dport 5000:5032 -m comment --comment "testsuite perftcp"
        fi
        if ! /sbin/iptables -L INPUT | grep -q "testsuite perfudp"; then
            $I -m udp -p udp --dport 5000:5032 -m comment --comment "testsuite perfudp"
        fi
    fi
}


start() {
    if [ ! -f "${PIDFILE}" ]; then
        begin_msg "Starting TestBoxScript";

        #
        # Verify config and installation.
        #
        if [ ! -d "$TESTBOXSCRIPT_DIR"  -o  ! -r "$binary"  -o  ! -r "$binary_real" ]; then
            fail_msg "Cannot find TestBoxScript installation under '$TESTBOXSCRIPT_DIR'!"
            exit 0;
        fi
        ## @todo check ownership (for upgrade purposes)
        check_single_user $TESTBOXSCRIPT_USER

        #
        # Open some ports in the firewall
        # Allows to access VMs remotely by VRDP, netperf
        #
        set_iptables

        #
        # Set execute bits to make installation (unzip) easier.
        #
        chmod a+x  > /dev/null 2>&1  \
            "${binary}" \
            "${binary_real}" \
            "${TESTBOXSCRIPT_DIR}/linux/amd64/TestBoxHelper" \
            "${TESTBOXSCRIPT_DIR}/linux/x86/TestBoxHelper"

        #
        # Start the daemon as the specified user.
        #
        PARAMS=""
        if [ "${TESTBOXSCRIPT_HWVIRT}"        = "yes" ]; then PARAMS="${PARAMS} --hwvirt"; fi
        if [ "${TESTBOXSCRIPT_HWVIRT}"        = "no"  ]; then PARAMS="${PARAMS} --no-hwvirt"; fi
        if [ "${TESTBOXSCRIPT_NESTED_PAGING}" = "yes" ]; then PARAMS="${PARAMS} --nested-paging"; fi
        if [ "${TESTBOXSCRIPT_NESTED_PAGING}" = "no"  ]; then PARAMS="${PARAMS} --no-nested-paging"; fi
        if [ "${TESTBOXSCRIPT_IOMMU}"         = "yes" ]; then PARAMS="${PARAMS} --io-mmu"; fi
        if [ "${TESTBOXSCRIPT_IOMMU}"         = "no"  ]; then PARAMS="${PARAMS} --no-io-mmu"; fi
        if [ "${TESTBOXSCRIPT_SPB}"           = "yes" ]; then PARAMS="${PARAMS} --spb"; fi
        if [ -n "${TESTBOXSCRIPT_SYSTEM_UUID}"     ]; then PARAMS="${PARAMS} --system-uuid '${TESTBOXSCRIPT_SYSTEM_UUID}'"; fi
        if [ -n "${TESTBOXSCRIPT_TEST_MANAGER}"    ]; then PARAMS="${PARAMS} --test-manager '${TESTBOXSCRIPT_TEST_MANAGER}'"; fi
        if [ -n "${TESTBOXSCRIPT_SCRATCH_ROOT}"    ]; then PARAMS="${PARAMS} --scratch-root '${TESTBOXSCRIPT_SCRATCH_ROOT}'"; fi

        if [ -n "${TESTBOXSCRIPT_BUILDS_PATH}"     ]; then PARAMS="${PARAMS} --builds-path '${TESTBOXSCRIPT_BUILDS_PATH}'"; fi
        if [ -n "${TESTBOXSCRIPT_BUILDS_TYPE}"     ]; then PARAMS="${PARAMS} --builds-server-type   '${TESTBOXSCRIPT_BUILDS_TYPE}'"; fi
        if [ -n "${TESTBOXSCRIPT_BUILDS_NAME}"     ]; then PARAMS="${PARAMS} --builds-server-name   '${TESTBOXSCRIPT_BUILDS_NAME}'"; fi
        if [ -n "${TESTBOXSCRIPT_BUILDS_SHARE}"    ]; then PARAMS="${PARAMS} --builds-server-share  '${TESTBOXSCRIPT_BUILDS_SHARE}'"; fi
        if [ -n "${TESTBOXSCRIPT_BUILDS_USER}"     ]; then PARAMS="${PARAMS} --builds-server-user   '${TESTBOXSCRIPT_BUILDS_USER}'"; fi
        if [ -n "${TESTBOXSCRIPT_BUILDS_PASSWD}"   ]; then PARAMS="${PARAMS} --builds-server-passwd '${TESTBOXSCRIPT_BUILDS_PASSWD}'"; fi
        if [ -n "${TESTBOXSCRIPT_BUILDS_MOUNTOPT}" ]; then PARAMS="${PARAMS} --builds-server-mountopt '${TESTBOXSCRIPT_BUILDS_MOUNTOPT}'"; fi
        if [ -n "${TESTBOXSCRIPT_TESTRSRC_PATH}"   ]; then PARAMS="${PARAMS} --testrsrc-path '${TESTBOXSCRIPT_TESTRSRC_PATH}'"; fi
        if [ -n "${TESTBOXSCRIPT_TESTRSRC_TYPE}"   ]; then PARAMS="${PARAMS} --testrsrc-server-type   '${TESTBOXSCRIPT_TESTRSRC_TYPE}'"; fi
        if [ -n "${TESTBOXSCRIPT_TESTRSRC_NAME}"   ]; then PARAMS="${PARAMS} --testrsrc-server-name   '${TESTBOXSCRIPT_TESTRSRC_NAME}'"; fi
        if [ -n "${TESTBOXSCRIPT_TESTRSRC_SHARE}"  ]; then PARAMS="${PARAMS} --testrsrc-server-share  '${TESTBOXSCRIPT_TESTRSRC_SHARE}'"; fi
        if [ -n "${TESTBOXSCRIPT_TESTRSRC_USER}"   ]; then PARAMS="${PARAMS} --testrsrc-server-user   '${TESTBOXSCRIPT_TESTRSRC_USER}'"; fi
        if [ -n "${TESTBOXSCRIPT_TESTRSRC_PASSWD}" ]; then PARAMS="${PARAMS} --testrsrc-server-passwd '${TESTBOXSCRIPT_TESTRSRC_PASSWD}'"; fi
        if [ -n "${TESTBOXSCRIPT_TESTRSRC_MOUNTOPT}" ]; then PARAMS="${PARAMS} --testrsrc-server-mountopt '${TESTBOXSCRIPT_TESTRSRC_MOUNTOPT}'"; fi

        if [ -n "${TESTBOXSCRIPT_PYTHON}" ]; then
            my_start_daemon "${TESTBOXSCRIPT_USER}" "${TESTBOXSCRIPT_PYTHON}" "${binary}" ${PARAMS}
        else
            my_start_daemon "${TESTBOXSCRIPT_USER}" "${binary}" ${PARAMS}
        fi
        RETVAL=$?

        if [ $RETVAL -eq 0 ]; then
            succ_msg
        else
            fail_msg
        fi
    else
        succ_msg "Already running."
        RETVAL=0
    fi
    return $RETVAL
}

stop() {
    if [ -f "${PIDFILE}" ]; then
        begin_msg "Stopping TestBoxScript";
        my_stop_daemon "${TESTBOXSCRIPT_USER}" "${binary}"
        RETVAL=$?
        if [ $RETVAL -eq 0 ]; then
            succ_msg
        else
            fail_msg
        fi
    else
        RETVAL=0
    fi
    return $RETVAL
}

restart() {
    stop && sleep 1 && start
}

status() {
    echo -n "Checking for TestBoxScript"
    my_query_status "${TESTBOXSCRIPT_USER}" "${binary}"
    RETVAL=$?
    if [ ${RETVAL} -eq 0 ]; then
        echo " ...running"
    elif [ ${RETVAL} -eq 3 ]; then
        echo " ...stopped"
    elif [ ${RETVAL} -eq 1 ]; then
        echo " ...started but not running"
    else
        echo " ...unknown status '${RETVAL}'"
    fi
}


#
# main().
#
case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    force-reload)
        restart
        ;;
    status)
        status
        ;;
    setup)
        ;;
    cleanup)
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
esac

exit $RETVAL

