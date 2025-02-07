# KVM Backend for VirtualBox

This repository contains a KVM backend for the open source virtualization
tool VirtualBox. With this backend, Linux KVM is used as the underlying hypervisor.

*Note:* VirtualBox is a trademark owned by Oracle. This project has no affiliation with Oracle.

## What to expect

The basic look and feel will be largely unchanged. The user is able to
boot the same guest VMs as in their existing configuration.

Nonetheless, there are the following benefits of using the KVM backend:

* VirtualBox VMs can run in parallel to QEMU/KVM
* VirtualBox kernel driver (`vboxdrv`) is not required
* Modern virtualization features supported by KVM are automatically used (e.g.
  APICv)
* KVM is part of the Linux kernel and therefore always directly available with
  every kernel update

Due to the replacement of the underlying hypervisor, there will be differences
in the guest performance. Performance differences heavily depend on the guest
workload.

## How to use

There are no prebuilt packages of the resulting program and it needs to be built from
source. The process of building VirtualBox from source can be found
[on virtualbox.org](https://www.virtualbox.org/wiki/Linux%20build%20instructions) and only
minor adjustments are required to integrate the KVM backend.

On a fresh install of Ubuntu 22.04, you can use the following command to install
all prerequisites via `apt`:

```shell
apt install acpica-tools chrpath doxygen g++-multilib libasound2-dev libcap-dev \
        libcurl4-openssl-dev libdevmapper-dev libidl-dev libopus-dev libpam0g-dev \
        libpulse-dev libqt5opengl5-dev libqt5x11extras5-dev qttools5-dev libsdl1.2-dev libsdl-ttf2.0-dev \
        libssl-dev libvpx-dev libxcursor-dev libxinerama-dev libxml2-dev libxml2-utils \
        libxmu-dev libxrandr-dev make nasm python3-dev python2-dev qttools5-dev-tools \
        texlive texlive-fonts-extra texlive-latex-extra unzip xsltproc \
        \
        default-jdk libstdc++5 libxslt1-dev linux-kernel-headers makeself \
        mesa-common-dev subversion yasm zlib1g-dev glslang-tools \
        libc6-dev-i386 lib32stdc++6 libtpms-dev
```

Newer GCC versions (>= 12) might cause build issues. The command above installs a
compatible version on Ubuntu 22.04.

After having all the prerequisites installed, the build process can be condensed
to the following steps:

```shell
$ # Download the VirtualBox 7.1.6 source package from Oracle.
$ tar xf VirtualBox-7.1.6a.tar.bz2
$ git clone https://github.com/cyberus-technology/virtualbox-kvm vbox-kvm
$ cd VirtualBox-7.1.6
$ git init
$ git add *
$ git commit -m "VirtualBox vanilla code"
$ git am ../vbox-kvm/patches/*.patch
$ ./configure --with-kvm --disable-kmods --disable-docs --disable-hardening --disable-java
$ source ./env.sh
$ kmk
```

The noticeable difference to the official build process is the addition of
`--with-kvm` when calling `./configure`.

**Note:** These instructions are intended for local building and testing
purposes only. There are more considerations when packaging for a
distribution. We do not advise or recommend instructions for packaging at this
time.

## Known issues and limitations

* Currently, Intel x86_64 is the only supported host platform.
  * AMD will most likely work too but is considered experimental at the moment.
  * Processor support for the `XSAVE` instruction is required. This implies a
    2nd Gen Core processor or newer.
* Linux is required as a host operating system for building and running the KVM
  backend.
* Starting with Intel Tiger Lake (11th Gen Core processors) or newer, split lock
  detection must be turned off in the host system. This can be achieved using
  the Linux kernel command line parameter `split_lock_detect=off` or using the
  `split_lock_mitigate` sysctl.

## Networking

The new KVM backend utilizes the `--driverless` mode of VirtualBox. Some setups
that require kernel module support will not work in this mode and prevent the
VM from starting. Specifically, the Bridged adapter and "NAT Network" modes do
not work. Only regular NAT is easily supported. More complex setups will need
manual configuration, e.g., using `tun`/`tap` devices.

## USB pass-through

USB device pass-through is supported. Some `udev` rules are required to
trigger the creation of fitting device nodes, though. VirtualBox provides the
`out/linux.amd64/bin/VBoxCreateUSBNode.sh` script to create the right nodes.
Distribution-provided packages will usually take care of the
required setup. The following is a short summary of the additional configuration
steps, which might differ based on the distribution. Be sure to know what you
are doing when following these steps. This can potentially interfere with
existing installations.

* Create a group `vboxusers` and add your user to the group. Remember group
  changes need a re-login or `newgrp` to take effect.
```shell
sudo groupadd -r vboxusers -U <username>
```

* Place the `VBoxCreateUSBNode.sh` script in a system accessible folder
```shell
sudo mkdir /usr/lib/virtualbox
sudo cp out/release/linux.amd64/bin/VBoxCreateUSBNode.sh /usr/lib/virtualbox
sudo chown -R root:vboxusers /usr/lib/virtualbox
```

* Create a `udev` rule file (e.g. `60-vboxusb.rules`) in `/etc/udev/rules.d/`
  with the following entries:
```shell
SUBSYSTEM=="usb_device", ACTION=="add", RUN+="/usr/lib/virtualbox/VBoxCreateUSBNode.sh $major $minor $attr{bDeviceClass}"
SUBSYSTEM=="usb", ACTION=="add", ENV{DEVTYPE}=="usb_device", RUN+="/usr/lib/virtualbox/VBoxCreateUSBNode.sh $major $minor $attr{bDeviceClass}"
SUBSYSTEM=="usb_device", ACTION=="remove", RUN+="/usr/lib/virtualbox/VBoxCreateUSBNode.sh --remove $major $minor"
SUBSYSTEM=="usb", ACTION=="remove", ENV{DEVTYPE}=="usb_device", RUN+="/usr/lib/virtualbox/VBoxCreateUSBNode.sh --remove $major $minor"
```

* Reload the udev rules
```shell
sudo systemctl reload systemd-udevd
```

## SR-IOV Graphics Virtualization

If you want to use the graphics virtualization features of modern Intel processors, please refer to our
[SR-IOV Graphics Virtualization HowTo](README.intel-sriov-graphics.md).

## Nested Virtualization

If you want to use nested virtualization, you need to run the following command:
```shell
VBoxManage modifyvm <vm_name> --nested-hw-virt on
```

Please note that nested virtualization is only supported for Intel CPUs. Nested virtualization on AMD is currently unsupported.

## How to engage

If you would like to use our KVM backend or if you have a need for custom
virtualization solutions, we are happy to provide guidance and engineering
services. Please reach out to us via our
[support form](https://cyberus-technology.de/contact) or via e-mail at
<service@cyberus-technology.de>.

If you encounter any issues please use the provided issue template and describe
your problem as detailed as possible.

## Licensing

This source code is released under the GPLv3, the same license terms as the
original VirtualBox Open Source release it is derived from. See the `LICENSE`
file and the upstream `COPYING` file for details. Make sure to follow
[licensing conditions](https://www.virtualbox.org/wiki/Licensing_FAQ) when
redistributing.
