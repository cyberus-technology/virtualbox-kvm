#!/bin/bash
## @file
# Post installation script template for local gateway image.
#
# Note! This script expects to be running chrooted (inside new sytem).
#

#
# Copyright (C) 2020-2023 Oracle and/or its affiliates.
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
# SPDX-License-Identifier: GPL-3.0-only
#


#
# Globals.
#
MY_TARGET="/mnt/sysimage"
MY_LOGFILE="${MY_TARGET}/var/log/vboxpostinstall.log"
MY_CHROOT_CDROM="/cdrom"
MY_CDROM_NOCHROOT="/tmp/vboxcdrom"
MY_EXITCODE=0
MY_DEBUG="" # "yes"

@@VBOX_COND_HAS_PROXY@@
PROXY="@@VBOX_INSERT_PROXY@@"
export http_proxy="${PROXY}"
export https_proxy="${PROXY}"
echo "HTTP proxy is ${http_proxy}" | tee -a "${MY_LOGFILE}"
echo "HTTPS proxy is ${https_proxy}" | tee -a "${MY_LOGFILE}"
@@VBOX_COND_END@@

#
# Do we need to exec using target bash?  If so, we must do that early
# or ash will bark 'bad substitution' and fail.
#
if [ "$1" = "--need-target-bash" ]; then
    # Try figure out which directories we might need in the library path.
    if [ -z "${LD_LIBRARY_PATH}" ]; then
        LD_LIBRARY_PATH="${MY_TARGET}/lib"
    fi
    for x in \
        ${MY_TARGET}/lib \
        ${MY_TARGET}/usr/lib \
        ${MY_TARGET}/lib/*linux-gnu/ \
        ${MY_TARGET}/lib32/ \
        ${MY_TARGET}/lib64/ \
        ${MY_TARGET}/usr/lib/*linux-gnu/ \
        ${MY_TARGET}/usr/lib32/ \
        ${MY_TARGET}/usr/lib64/ \
        ;
    do
        if [ -e "$x" ]; then LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${x}"; fi;
    done
    export LD_LIBRARY_PATH

    # Append target bin directories to the PATH as busybox may not have tee.
    PATH="${PATH}:${MY_TARGET}/bin:${MY_TARGET}/usr/bin:${MY_TARGET}/sbin:${MY_TARGET}/usr/sbin"
    export PATH

    # Drop the --need-target-bash argument and re-exec.
    shift
    echo "******************************************************************************" >> "${MY_LOGFILE}"
    echo "** Relaunching using ${MY_TARGET}/bin/bash $0 $*" >> "${MY_LOGFILE}"
    echo "**   LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" >> "${MY_LOGFILE}"
    echo "**              PATH=${PATH}" >> "${MY_LOGFILE}"
    exec "${MY_TARGET}/bin/bash" "$0" "$@"
fi


#
# Commands.
#

# Logs execution of a command.
log_command()
{
    echo "--------------------------------------------------" >> "${MY_LOGFILE}"
    echo "** Date:      `date -R`" >> "${MY_LOGFILE}"
    echo "** Executing: $*" >> "${MY_LOGFILE}"
    "$@" 2>&1 | tee -a "${MY_LOGFILE}"
    MY_TMP_EXITCODE="${PIPESTATUS[0]}"      # bashism - whatever.
    if [ "${MY_TMP_EXITCODE}" != "0" ]; then
        if [ "${MY_TMP_EXITCODE}" != "${MY_IGNORE_EXITCODE}" ]; then
            echo "** exit code: ${MY_TMP_EXITCODE}" | tee -a "${MY_LOGFILE}"
            MY_EXITCODE=1;
        else
            echo "** exit code: ${MY_TMP_EXITCODE} (ignored)" | tee -a "${MY_LOGFILE}"
        fi
    fi
}

# Logs execution of a command inside the target.
log_command_in_target()
{
    log_command chroot "${MY_TARGET}" "$@"
}

# Checks if $1 is a command on the PATH inside the target jail.
chroot_which()
{
    for dir in /bin /usr/bin /sbin /usr/sbin;
    do
        if [ -x "${MY_TARGET}${dir}/$1" ]; then
            return 0;
        fi
    done
    return 1;
}

#
# Log header.
#
echo "******************************************************************************" >> "${MY_LOGFILE}"
echo "** VirtualBox Unattended Guest Installation - Late installation actions" >> "${MY_LOGFILE}"
echo "** Date:    `date -R`" >> "${MY_LOGFILE}"
echo "** Started: $0 $*" >> "${MY_LOGFILE}"


#
# We want the ISO available inside the target jail.
#
if [ -d "${MY_TARGET}${MY_CHROOT_CDROM}" ]; then
    MY_RMDIR_TARGET_CDROM=
else
    MY_RMDIR_TARGET_CDROM="yes"
    log_command mkdir -p ${MY_TARGET}${MY_CHROOT_CDROM}
fi

if [ -f "${MY_TARGET}${MY_CHROOT_CDROM}/vboxpostinstall.sh" ]; then
    MY_UNMOUNT_TARGET_CDROM=
    echo "** binding cdrom into jail: already done" | tee -a "${MY_LOGFILE}"
else
    MY_UNMOUNT_TARGET_CDROM="yes"
    log_command mount -o bind "${MY_CDROM_NOCHROOT}" "${MY_TARGET}${MY_CHROOT_CDROM}"
    if [ -f "${MY_TARGET}${MY_CHROOT_CDROM}/vboxpostinstall.sh" ]; then
        echo "** binding cdrom into jail: success"  | tee -a "${MY_LOGFILE}"
    else
        echo "** binding cdrom into jail: failed"   | tee -a "${MY_LOGFILE}"
    fi
    if [ "${MY_DEBUG}" = "yes" ]; then
        log_command find "${MY_TARGET}${MY_CHROOT_CDROM}"
    fi
fi


#
# Debug
#
if [ "${MY_DEBUG}" = "yes" ]; then
    log_command id
    log_command ps
    log_command ps auxwwwf
    log_command env
    log_command df
    log_command mount
    log_command_in_target df
    log_command_in_target mount
    #log_command find /
    MY_EXITCODE=0
fi


#
# Proxy hack for yum
#
@@VBOX_COND_HAS_PROXY@@
echo "" >> "${MY_TARGET}/etc/yum.conf"
echo "proxy=@@VBOX_INSERT_PROXY@@" >> "${MY_TARGET}/etc/yum.conf"
@@VBOX_COND_END@@

#
# Packages needed for GAs.
#
echo "--------------------------------------------------" >> "${MY_LOGFILE}"
echo '** Installing packages for building kernel modules...' | tee -a "${MY_LOGFILE}"
log_command_in_target yum -y install "kernel-devel-$(uname -r)"
log_command_in_target yum -y install "kernel-headers-$(uname -r)"
log_command_in_target yum -y install gcc
log_command_in_target yum -y install binutils
log_command_in_target yum -y install make
log_command_in_target yum -y install dkms
log_command_in_target yum -y install make
log_command_in_target yum -y install bzip2
log_command_in_target yum -y install perl


#
# GAs
#
@@VBOX_COND_IS_INSTALLING_ADDITIONS@@
echo "--------------------------------------------------" >> "${MY_LOGFILE}"
echo '** Installing VirtualBox Guest Additions...' | tee -a "${MY_LOGFILE}"
MY_IGNORE_EXITCODE=2  # returned if modules already loaded and reboot required.
log_command_in_target /bin/bash "${MY_CHROOT_CDROM}/vboxadditions/VBoxLinuxAdditions.run" --nox11
log_command_in_target /bin/bash -c "udevadm control --reload-rules" # GAs doesn't yet do this.
log_command_in_target /bin/bash -c "udevadm trigger"                 # (ditto)
MY_IGNORE_EXITCODE=
log_command_in_target usermod -a -G vboxsf "@@VBOX_INSERT_USER_LOGIN@@"
@@VBOX_COND_END@@

#
# Local gateway support
#
log_command_in_target yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
#log_command_in_target yum -y update
log_command_in_target yum -y install openvpn
log_command_in_target yum -y install connect-proxy
log_command_in_target usermod -a -G wheel "@@VBOX_INSERT_USER_LOGIN@@"

echo "** Creating ${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/cloud-bridge.conf..." | tee -a "${MY_LOGFILE}"
cat >"${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/cloud-bridge.conf" <<'EOT'
# port 1194
# proto udp
port 443
proto tcp-server
dev tap0
secret static.key
keepalive 10 120
compress lz4-v2
push "compress lz4-v2"
persist-key
persist-tun
status /var/log/openvpn-status.log
log-append /var/log/openvpn.log
verb 3
EOT

echo "** Creating ${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/cloud-bridge.sh..." | tee -a "${MY_LOGFILE}"
cat >"${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/cloud-bridge.sh" <<'EOT'
# Initialize variables
br="br0"
tap="tap0"
vnic1=$1
vnic2=$2
vnic2_gw=$3
target_mac=$4

# Install openvpn if it is missing
if ! yum list installed openvpn; then
    sudo yum -y install openvpn
fi

# Let openvpn traffic through Linux firewall
#sudo iptables -I INPUT -p udp --dport 1194 -j ACCEPT
sudo iptables -I INPUT -p tcp --dport 443 -j ACCEPT

# Switch to secondary VNIC
sudo ip route change default via $vnic2_gw dev $vnic2
sudo ip link set dev $vnic1 down

# Bring up the cloud end of the tunnel
sudo openvpn --config cloud-bridge.conf --daemon

# Use target MAC for primary VNIC
sudo ip link set dev $vnic1 address $target_mac

# Bridge tap and primary VNIC
sudo ip link add name $br type bridge
sudo ip link set dev $vnic1 master $br
sudo ip link set dev $tap master $br

# Bring up all interfaces
sudo ip link set dev $tap up
sudo ip link set dev $vnic1 up
sudo ip link set dev $br up
EOT
log_command chmod +x "${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/cloud-bridge.sh"

echo "** Creating ${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/local-bridge.conf..." | tee -a "${MY_LOGFILE}"
cat >"${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/local-bridge.conf" <<'EOT'
dev tap0
# proto udp
# port 1194
proto tcp-client
port 443
persist-key
persist-tun
secret static.key
compress lz4-v2
log-append /var/log/openvpn.log
verb 3
EOT

echo "** Creating ${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/local-bridge.sh..." | tee -a "${MY_LOGFILE}"
cat >"${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/local-bridge.sh" <<'EOT'
echo Complete command line for debugging purposes:
echo $0 $*

# Make sure we are at home
cd ~

# Initialize variables
user=opc
cbr_ip1=$1
cbr_ip2=$2
target_mac=$3
br="br0"
tap="tap0"
eth="enp0s8"

proxy1_ssh=""
proxy2_ssh=""
proxy2_vpn=""
case $4 in
  HTTP | HTTPS)
    proxy1_ssh="connect-proxy -w 30 -H $5:$6 %h %p"
    ;;
  SOCKS | SOCKS5)
    proxy1_ssh="connect-proxy -w 30 -S $5:$6 %h %p"
    ;;
  SOCKS4)
    proxy1_ssh="connect-proxy -w 30 -4 -S $5:$6 %h %p"
    ;;
esac
case $7 in
  HTTP | HTTPS)
    proxy2_ssh="connect-proxy -w 30 -H $8:$9 %h %p"
    proxy2_vpn="--http-proxy $8 $9"
    ;;
  SOCKS | SOCKS5)
    proxy2_ssh="connect-proxy -w 30 -S $8:$9 %h %p"
    proxy2_vpn="--socks-proxy $8 $9"
    ;;
  SOCKS4)
    proxy2_ssh="connect-proxy -w 30 -4 -S $8:$9 %h %p"
    proxy2_vpn="--socks-proxy $8 $9"
    ;;
esac

# Generate pre-shared secret and share it with the server, bypassing proxy if necessary
/usr/sbin/openvpn --genkey --secret static.key
for i in 1 2 3 4
do
  # Go via proxy if set
  scp ${proxy1_ssh:+ -o ProxyCommand="$proxy1_ssh"} static.key cloud-bridge.conf cloud-bridge.sh $user@$cbr_ip1:
  if [ $? -eq 0 ]; then break; fi; sleep 15
  # Go direct even if proxy is set
  scp static.key cloud-bridge.conf cloud-bridge.sh $user@$cbr_ip1:
  if [ $? -eq 0 ]; then proxy1_ssh=""; break; fi; sleep 15
done

# Get metadata info from the cloud bridge
for i in 1 2 3 4; do metadata=$(ssh ${proxy1_ssh:+ -o ProxyCommand="$proxy1_ssh"} $user@$cbr_ip1 sudo oci-network-config) && break || sleep 15; done

# Extract primary VNIC info
vnic1_md=`echo "$metadata"|grep -E "\sUP\s"`
vnic1_dev=`echo $vnic1_md|cut -d ' ' -f 8`
vnic1_mac=`echo $vnic1_md|cut -d ' ' -f 12`
# Extract secondary VNIC info
vnic2_md=`echo "$metadata"|grep -E "\sDOWN\s"`
vnic2_dev=`echo $vnic2_md|cut -d ' ' -f 8`
vnic2_gw=`echo $vnic2_md|cut -d ' ' -f 5`

# Configure secondary VNIC
ssh ${proxy1_ssh:+ -o ProxyCommand="$proxy1_ssh"} $user@$cbr_ip1 sudo oci-network-config -c

# Bring up the cloud bridge
ssh ${proxy2_ssh:+ -o ProxyCommand="$proxy2_ssh"} $user@$cbr_ip2 /bin/sh -x cloud-bridge.sh $vnic1_dev $vnic2_dev $vnic2_gw $target_mac
if [ $? -eq 0 ]
then
  # SSH was able to reach cloud via proxy, establish a tunnel via proxy as well
  sudo /usr/sbin/openvpn $proxy2_vpn --config local-bridge.conf --daemon --remote $cbr_ip2
else
  # Retry without proxy
  ssh $user@$cbr_ip2 /bin/sh -x cloud-bridge.sh $vnic1_dev $vnic2_dev $vnic2_gw $target_mac
  # Establish a tunnel to the cloud bridge
  sudo /usr/sbin/openvpn --config local-bridge.conf --daemon --remote $cbr_ip2
fi

# Bridge the openvpn tap device and the local Ethernet interface
sudo ip link set dev $eth down
sudo ip link add name $br type bridge
sudo ip link set dev $eth master $br
sudo ip link set dev $tap master $br

# Bring up all interfaces
sudo ip link set dev $tap up
sudo ip link set dev $eth up
sudo ip link set dev $br up
EOT
log_command chmod +x "${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/local-bridge.sh"

echo "** Creating ${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/.ssh/config..." | tee -a "${MY_LOGFILE}"
log_command mkdir "${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/.ssh"
cat >"${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/.ssh/config" <<'EOT'
Host *
    StrictHostKeyChecking no
EOT
log_command chmod 400 "${MY_TARGET}/home/@@VBOX_INSERT_USER_LOGIN@@/.ssh/config"

log_command_in_target chown -R @@VBOX_INSERT_USER_LOGIN@@:@@VBOX_INSERT_USER_LOGIN@@ "/home/@@VBOX_INSERT_USER_LOGIN@@"

echo '** Creating /etc/systemd/system/keygen.service...' | tee -a "${MY_LOGFILE}"
cat >"${MY_TARGET}/etc/systemd/system/keygen.service" <<'EOT'
[Unit]
Description=Boot-time ssh key pair generator
After=vboxadd.service

[Service]
ExecStart=/bin/sh -c 'su - vbox -c "cat /dev/zero | ssh-keygen -q -N \\\"\\\""'
ExecStartPost=/bin/sh -c 'VBoxControl guestproperty set "/VirtualBox/Gateway/PublicKey" "`cat ~vbox/.ssh/id_rsa.pub`" --flags TRANSIENT'
Type=oneshot
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOT
log_command chmod 644 "${MY_TARGET}/etc/systemd/system/keygen.service"
log_command_in_target systemctl enable keygen.service

echo '** Creating /etc/sudoers.d/020_vbox_sudo...' | tee -a "${MY_LOGFILE}"
echo "@@VBOX_INSERT_USER_LOGIN@@ ALL=(ALL) NOPASSWD: ALL" > "${MY_TARGET}/etc/sudoers.d/020_vbox_sudo"

#
# Test Execution Service.
#
@@VBOX_COND_IS_INSTALLING_TEST_EXEC_SERVICE@@
echo "--------------------------------------------------" >> "${MY_LOGFILE}"
echo '** Installing Test Execution Service...' | tee -a "${MY_LOGFILE}"
log_command_in_target test "${MY_CHROOT_CDROM}/vboxvalidationkit/linux/@@VBOX_INSERT_OS_ARCH@@/TestExecService"
log_command mkdir -p "${MY_TARGET}/opt/validationkit" "${MY_TARGET}/media/cdrom"
log_command cp -R ${MY_CDROM_NOCHROOT}/vboxvalidationkit/* "${MY_TARGET}/opt/validationkit/"
log_command chmod -R u+rw,a+xr "${MY_TARGET}/opt/validationkit/"
if [ -e "${MY_TARGET}/usr/bin/chcon" -o -e "${MY_TARGET}/bin/chcon" -o -e "${MY_TARGET}/usr/sbin/chcon" -o -e "${MY_TARGET}/sbin/chcon" ]; then
    MY_IGNORE_EXITCODE=1
    log_command_in_target chcon -R -t usr_t "/opt/validationkit/"
    MY_IGNORE_EXITCODE=
fi

# systemd service config:
MY_UNIT_PATH="${MY_TARGET}/lib/systemd/system"
test -d "${MY_TARGET}/usr/lib/systemd/system" && MY_UNIT_PATH="${MY_TARGET}/usr/lib/systemd/system"
if [ -d "${MY_UNIT_PATH}" ]; then
    log_command cp "${MY_TARGET}/opt/validationkit/linux/vboxtxs.service" "${MY_UNIT_PATH}/vboxtxs.service"
    log_command chmod 644 "${MY_UNIT_PATH}/vboxtxs.service"
    log_command_in_target systemctl -q enable vboxtxs

# System V like:
elif [ -e "${MY_TARGET}/etc/init.d/" ]; then

    # Install the script.  On rhel6 scripts are under /etc/rc.d/ with /etc/init.d and /etc/rc?.d being symlinks.
    if [ -d "${MY_TARGET}/etc/rc.d/init.d/" ]; then
        MY_INIT_D_PARENT_PATH="${MY_TARGET}/etc/rc.d"
        log_command ln -s "../../../opt/validationkit/linux/vboxtxs" "${MY_INIT_D_PARENT_PATH}/init.d/"
    else
        MY_INIT_D_PARENT_PATH="${MY_TARGET}/etc"
        log_command ln -s    "../../opt/validationkit/linux/vboxtxs" "${MY_INIT_D_PARENT_PATH}/init.d/"
    fi

    # Use runlevel management script if found.
    if chroot_which chkconfig; then     # Redhat based sysvinit systems
        log_command_in_target chkconfig --add vboxtxs
    elif chroot_which insserv; then     # SUSE-based sysvinit systems
        log_command_in_target insserv vboxtxs
    elif chroot_which update-rc.d; then # Debian/Ubuntu-based systems
        log_command_in_target update-rc.d vboxtxs defaults
    elif chroot_which rc-update; then   # Gentoo Linux
        log_command_in_target rc-update add vboxtxs default
    # Fall back on hardcoded symlinking.
    else
        log_command ln -s "../init.d/vboxtxs" "${MY_INIT_D_PARENT_PATH}/rc0.d/K65vboxtxs"
        log_command ln -s "../init.d/vboxtxs" "${MY_INIT_D_PARENT_PATH}/rc1.d/K65vboxtxs"
        log_command ln -s "../init.d/vboxtxs" "${MY_INIT_D_PARENT_PATH}/rc6.d/K65vboxtxs"
        log_command ln -s "../init.d/vboxtxs" "${MY_INIT_D_PARENT_PATH}/rc2.d/S35vboxtxs"
        log_command ln -s "../init.d/vboxtxs" "${MY_INIT_D_PARENT_PATH}/rc3.d/S35vboxtxs"
        log_command ln -s "../init.d/vboxtxs" "${MY_INIT_D_PARENT_PATH}/rc4.d/S35vboxtxs"
        log_command ln -s "../init.d/vboxtxs" "${MY_INIT_D_PARENT_PATH}/rc5.d/S35vboxtxs"
    fi
else
    echo "** error: Unknown init script system." | tee -a "${MY_LOGFILE}"
fi

@@VBOX_COND_END@@


#
# Run user command.
#
@@VBOX_COND_HAS_POST_INSTALL_COMMAND@@
echo '** Running custom user command ...'      | tee -a "${MY_LOGFILE}"
log_command @@VBOX_INSERT_POST_INSTALL_COMMAND@@
@@VBOX_COND_END@@


#
# Unmount the cdrom if we bound it and clean up the chroot if we set it up.
#
if [ -n "${MY_UNMOUNT_TARGET_CDROM}" ]; then
    echo "** unbinding cdrom from jail..." | tee -a "${MY_LOGFILE}"
    log_command umount "${MY_TARGET}${MY_CHROOT_CDROM}"
fi

if [ -n "${MY_RMDIR_TARGET_CDROM}" ]; then
    log_command rmdir "${MY_TARGET}${MY_CHROOT_CDROM}"
fi


#
# Log footer.
#
echo "******************************************************************************" >> "${MY_LOGFILE}"
echo "** Date:            `date -R`" >> "${MY_LOGFILE}"
echo "** Final exit code: ${MY_EXITCODE}" >> "${MY_LOGFILE}"
echo "******************************************************************************" >> "${MY_LOGFILE}"

exit ${MY_EXITCODE}

