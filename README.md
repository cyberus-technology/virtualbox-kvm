# VirtualBox KVM

This repository contains an adapted version of the open source virtualization
tool VirtualBox called VirtualBox KVM.
VirtualBox KVM uses Linux KVM as the underlying hypervisor.

## What to expect

The basic look and feel of VirtualBox KVM will be the same as with an
conventional VirtualBox. The user is able to boot the same guest VMs in their
existing VirtualBox configuration.

Nonetheless, there are the following benefits of using VirtualBox KVM compared
to the conventional VirtualBox:

* VirtualBox can run in parallel to QEMU/KVM
* VirtualBox kernel driver (`vboxdrv`) is not required
* Modern virtualization features supported by KVM are automatically used (e.g.
  APICv)
* KVM is part of the Linux kernel and therefore always directly available with
  every kernel update

Due to the replacement of the underlying hypervisor, there will be differences
in the guest performance. Performance differences heavily depend on the guest
workload.

## How to use

Prebuilt packages for the latest development commit of of VirtualBox KVM can be
found at https://github.com/cyberus-technology/virtualbox-kvm/releases/tag/dev-build

Optionally download the sha384sums and validate.

```shell
$ sha384sum -c dev-build.tar.gz.sha384sums
dev-build.tar.gz: OK
```

Unpack the archive and run the VirtualBox binary.

```shell
$ tar xvzf dev-build.tar.gz
$ out/linux.amd64/release/bin/VirtualBox
```

## Building from source

The process of building VirtualBox from source can be found
[on virtualbox.org](https://www.virtualbox.org/wiki/Linux%20build%20instructions) and only
minor adjustments are required to build VirtualBox with KVM as a backend.

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
$ ./configure --with-kvm --disable-kmods --disable-docs --disable-hardening --disable-java
$ source ./env.sh
$ kmk
$ out/linux.amd64/release/bin/VirtualBox
```

The noticeable difference to the official build process is the addition of
`--with-kvm` when calling `./configure`.

## Known issues and limitations

* Currently, Intel x86_64 is the only supported host platform.
  * AMD will most likely work too but is considered experimental at the moment.
* Linux is required as a host operating system for building and running
  VirtualBox KVM.
* Starting with Intel Tiger Lake (11th Gen Core processors) or newer, split lock
  detection must be turned off in the host system. This can be achieved using
  the Linux kernel command line parameter `split_lock_detect=off` or using the
  `split_lock_mitigate` sysctl.

## How to engage

If you would like to use VirtualBox with KVM or if you have a need for custom
virtualization solutions, we are happy to provide guidance and engineering
services. Please reach out to us via our
[support form](https://cyberus-technology.de/contact) or via e-mail at
<service@cyberus-technology.de>.

If you encounter any issues please use the provided issue template and describe
your problem as detailed as possible.
