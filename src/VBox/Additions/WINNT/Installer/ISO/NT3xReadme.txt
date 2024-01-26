While the VirtualBox guest additions does not officially support NT 3.x some
of the core drivers and tools happens to work.  However, the installer binary
(VBoxWindowsAdditions-x86.exe) does not run on anything older than NT 4,
making it hard to extract (VBoxWindowsAdditions-x86.exe /extract /D=C:\dir)
the necessary files.  So, the 32-bit drivers and components that might work
on NT 3.x are provided here for convenience.

The VBoxAddInstallNt3x.exe program is a simple command line installation
utility for NT 3.x guest additions.  It automats the steps detailed below.


NT 3.51:
-------

Add VBoxGuest using registry editor (regedt32.exe):
  - Copy the VBoxGuest.sys file to %SystemRoot%\system32\drivers.
  - Go to HKEY_LOCAL_MACHINE/System/CurrentControlSet/Service/
  - Create VBoxGuest key with the following values:
         DisplayName (REG_SZ) = "VBoxGuest Support Driver"
     ErrorControl (REG_DWORD) = 0x1
               Group (REG_SZ) = "System"
    ImagePath (REG_EXPAND_SZ) = System32\drivers\VBoxGuest.sys
            Start (REG_DWORD) = 0
             Type (REG_DWORD) = 0x1
  - Reboot.
  - Open the "Devices" in the "Control Panel", locate the VBoxGuest driver
    and check that it started fine.

  If FAT file system s/VBoxGuest.sys/VBoxGst.sys/ above.


Install VBoxService by copying it to %SystemRoot%\system32 and run
"VBoxService --register".  Go to "Services" in the "Control Panel" and modify
the "VirtualBox Guest Additions Service" to startup "Automatic".  If VBoxGuest
is already started you can start the service (also possible using
"net start VBoxService").

Install VBoxMouseNT.sys by copying it %SystemRoot%\system32\drivers and using
the registry editor (regedt32.exe) to set (create it if necessary) value
HKEY_LOCAL_MACHINE/System/CurrentControlSet/Service/i8042prt/ImagePath
(REG_EXPAND_SZ) to "System32\drivers\VBoxGuestNT.sys".  (The i8042prt driver
is the NT PS/2 mouse + keyboard driver and VBoxGuestNT.sys replaces it.)


NT 3.50
-------

Same as for 3.51.


NT 3.1
------

Does not currently work.