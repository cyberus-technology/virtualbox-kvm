
@VBOX_PRODUCT@ for Oracle Solaris 11 (TM) Operating System
----------------------------------------------------------------

Installing:
-----------

After extracting the contents of the tar.xz file install the VirtualBox
package with the following command:

    $ sudo pkg install -g VirtualBox-@VBOX_VERSION_STRING@-SunOS-@KBUILD_TARGET_ARCH@-r@VBOX_SVN_REV@.p5p virtualbox

Of course you can add options for performing the install in a different boot
environment or in a separate Solaris install.

Normally you need to reboot the system to load the drivers which have been
added by the VirtualBox package.

If you want to have VirtualBox immediately usable on your system you can run
the script /opt/VirtualBox/ipsinstall.sh which sets up everything immediately.

At this point, all the required files should be installed on your system.
You can launch VirtualBox by running 'VirtualBox' from the terminal.


Upgrading:
----------

If you want to upgrade from an older to a newer version of the VirtualBox IPS
package you can use the following command after extracting the contents of the
tar.xz file:

    $ sudo pkg update -g VirtualBox-@VBOX_VERSION_STRING@-SunOS-@KBUILD_TARGET_ARCH@-r@VBOX_SVN_REV@.p5p virtualbox

If you want to upgrade from the SysV package of VirtualBox to the IPS one,
please uninstall the previous package before installing the IPS one. Please
refer to the "Uninstalling" and "Installing" sections of this document for
details.

It is your responsibility to ensure that no VirtualBox VMs or other related
activities are running. One possible way is using the command pgrep VBoxSVC. If
this shows no output then it is safe to upgrade VirtualBox.


Uninstalling:
-------------

To remove VirtualBox from your system, run the following command:

    $ sudo pkg uninstall virtualbox

It is your responsibility to ensure that no VirtualBox VMs or other related
activities are running. One possible way is using the command pgrep VBoxSVC. If
this shows no output then it is safe to uninstall VirtualBox.

