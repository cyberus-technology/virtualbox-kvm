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

There are no prebuilt packages of VirtualBox KVM and it needs to be build from
source. The process of building VirtualBox from source can be found
[on virtualbox.org](https://www.virtualbox.org/wiki/Linux%20build%20instructions) and only
minor adjustments are required to build VirtualBox with KVM as a backend.

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
