
@VBOX_PRODUCT@ for Oracle Solaris (TM) Operating System
--------------------------------------------------------

Upgrading:
----------

If you have an existing VirtualBox installation and you are upgrading to
a newer version of VirtualBox, please uninstall the previous version
before installing a newer one. Please refer to the "Uninstalling" section
at the end of this document for details.


Installing:
-----------

After extracting the contents of the tar.gz file perform the following steps:

1. Login as root using the "su" command.

2. Install the VirtualBox package:

      pkgadd -d VirtualBox-@VBOX_VERSION_STRING@-SunOS-@KBUILD_TARGET_ARCH@-r@VBOX_SVN_REV@.pkg

      To perform an unattended (non-interactive) installation of this
      package, add "-n -a autoresponse SUNWvbox" (without quotes)
      to the end of the above pkgadd command.

3. For each package, the installer will ask you to "Select package(s) you
   wish to process". In response, type "1".

4. Type "y" when asked about continuing the installation.

At this point, all the required files should be installed on your system.
You can launch VirtualBox by running 'VirtualBox' from the terminal.


Uninstalling:
-------------

To remove VirtualBox from your system, perform the following steps:

1. Login as root using the "su" command.

2. To remove VirtualBox, run the command:
        pkgrm SUNWvbox

