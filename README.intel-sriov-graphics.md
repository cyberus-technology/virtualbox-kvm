# Introduction

This document describes how to setup and use Intel SR-IOV graphics
virtualization with the KVM backend for VirtualBox. The hardware graphics
acceleration is not integrated in the GUI yet and requires manual setup. Yet, we
deem it good enough for people to try this.

This is a feature which is currently under active development, everything noted here should be
considered experimental.

**Note**: [dev-20240828](https://github.com/cyberus-technology/virtualbox-kvm/releases/tag/dev-20240828) was the last
release of our KVM backend supporting SR-IOV graphics. Make sure to use that for SR-IOV graphics experiments.

# Requirements

- Intel CPU with SR-IOV graphics support
   - [Intel Knowledge Base Article]( https://www.intel.com/content/www/us/en/support/articles/000093216/graphics/processor-graphics.html#primary-content)
- Ubuntu 22.04 as host OS
- VT-d must be enabled in BIOS.
- IOMMU must be enabled in the host Linux kernel.
- A Windows 10 or Windows 11 VM with __all__ updates installed
- Make sure the SR-IOV capability is present:
    - `lspci | grep VGA`, remember the BDF for the device, we use 0:2.0 in this tutorial
    - `sudo lspci -s 0:2.0 -v | grep SR-IOV`

# Host Setup

The host needs a special Linux kernel which includes the SR-IOV graphics patches from Intel. These patches
have not been upstreamed yet. In addition, the kernel needs to be booted with custom command line
options in order for SR-IOV graphics to function properly.

## Installation with pre-built kernel packages

We currently don't provide binary kernel packages. You can find older versions
for quick testing in a [previous
release](https://github.com/cyberus-technology/virtualbox-kvm/releases/tag/dev-20240307). Note
that these packages are built for Ubuntu 22.04 and have not been tested with any
other Linux distribution. These images are not signed.

## Installation with self-compiled kernel

- `sudo apt install flex bison elfutils dwarves dpkg-dev debhelper libelf-dev`
- `mkdir kernel-intel-sriov`
- `cd kernel-intel-sriov`
- `git clone https://github.com/intel/linux-intel-lts.git --branch lts-v6.6.15-linux-240219T085932Z --single-branch`
- `cd linux-intel-lts`
- `make olddefconfig`
- execute:
    - `scripts/config --disable SYSTEM_TRUSTED_KEYS`
    - `scripts/config --disable SYSTEM_REVOCATION_KEYS`
- `make deb-pkg LOCALVERSION="-sriov" -j$(nproc)`
- `cd ..`
- `sudo dpkg -i *.deb`

## Preparing the Linux kernel for SR-IOV graphics

- Edit `/etc/defaults/grub`
- Add `i915.enable_guc=3 i915.max_vfs=7 split_lock_detect=off` to `GRUB_CMDLINE_LINUX`
- Set `GRUB_DEFAULT` to `Advanced options for Ubuntu>Ubuntu, with Linux 6.6.15-sriov`
- Execute `update-grub` as root
- Edit `/etc/security/limits.conf` (required because VFIO needs more locked memory than configured as default)
	- add:
	  - `*  soft  memlock  unlimited`
	  - `*  hard  memlock  unlimited`
- Reboot

## Verify Host Setup

Make sure you have booted the correct Linux kernel; `uname -r` should provide the following output: `6.6.15-sriov`.
Check `sudo dmesg` for the following output which indicates that SR-IOV has been set up successfully:

```
i915 0000:00:02.0: enabling device (0006 -> 0007)
i915 0000:00:02.0: Running in SR-IOV PF mode
i915 0000:00:02.0: [drm] VT-d active for gfx access
```

# Setting up the vGPUs

- Execute the following commands as root:
    - `lspci | grep VGA`, remember the BDF for the device, we use 0:2.0 in this tutorial
    - `lspci -s 0:2.0 -n`, remember device/vendor id
    - `echo "<vendor-id> <device-id>" > /sys/bus/pci/drivers/vfio-pci/new_id`
    - `echo 7 > /sys/class/drm/card0/device/sriov_numvfs`
    - `lspci -v -s 0:2.1`, verify vfio-pci driver is in use
    - `chmod 0666 /dev/vfio/*`

# Configuring vGPUs

- Configure ICH9 Chipset: `VBoxManage modifyvm <vm name> --chipset=ICH9`
- Attach the vGPU: `VBoxManage modifyvm <vm name> --attachvfio /sys/bus/pci/devices/0000\:00\:02.1` (no trailing slash)
- Change display adapter: `VBoxManage modifyvm <vm name> --graphicscontroller vga-virtiogpu`
- Boot the VM
- Verify that there are 2 new display adapters in device manager
- Install latest Intel GPU driver or wait for Windows to install it automatically
- Verify in Task Manager that GPU0 is present (Performance Tab)
- Download and extract [the Intel Display Virtualization Drivers](https://www.intel.com/content/www/us/en/download/806254/nswe-display-virtualization-drivers-for-meteor-lake-ps-pv-meteor-lake-u-h-pv-and-raptor-lake-ps-beta.html)
- Open Powershell as Admin
- Change the execution policy: `Set-ExecutionPolicy -ExecutionPolicy AllSigned -Scope CurrentUser`
- Execute `DVInstaller.ps1`
- VM will reboot
- Verify in Device Manager that both "Intel Iris Xe Graphics" and "DVServerUMD" are present in display adapters
- Enjoy your GPU-accelerated VM ;)

# Limitations

- Host suspend/resume while GPU acceleration is in use is unsupported and will result in broken VM graphics
- PIIX3 chipset is unsupported
- Multiple virtual monitors are unsupported
- Automatic display resizing is not supported

# Troubleshooting

- A vGPU can be removed from a VM via:
    - `VBoxManage modifyvm <vm name> --detachvfio /sys/bus/pci/devices/0000\:00\:02.1` (no trailing slash)
    - `VBoxManage modifyvm <vm name> --graphicscontroller vboxsvga`
- After host suspend/resume, the graphics may recover when pressing `Win + Ctrl + Shift + B`. Some windows may stay black, in which case minimizing/maximizing may fix the problem.
