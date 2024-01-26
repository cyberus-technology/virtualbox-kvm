/* Disabled from config/defaults/pcbios.h */

#undef SANBOOT_PROTO_ISCSI
#undef SANBOOT_PROTO_AOE
#undef SANBOOT_PROTO_IB_SRP
#undef SANBOOT_PROTO_FCP
#undef SANBOOT_PROTO_HTTP

#undef USB_HCD_XHCI
#undef USB_HCD_EHCI
#undef USB_HCD_UHCI
#undef USB_KEYBOARD
#undef USB_BLOCK

#undef IMAGE_ELF
#undef IMAGE_MULTIBOOT
#undef IMAGE_EFI
#undef IMAGE_SCRIPT
#undef IMAGE_BZIMAGE
#undef IMAGE_ELTORITO
#undef REBOOT_CMD
#undef CPUID_CMD

/* Disabled from config/general.h */

#undef CRYPTO_80211_WEP
#undef CRYPTO_80211_WPA
#undef CRYPTO_80211_WPA2
#undef IWMGMT_CMD
#undef MENU_CMD

/* Ensure ROM banner is not displayed */

#undef ROM_BANNER_TIMEOUT
#define ROM_BANNER_TIMEOUT 0

/*
 * Disable the autboot device filtering because the PXE ROM is not part of a PCI device
 * and it would disable autoboot.
 */
#undef AUTOBOOT_ROM_FILTER

/* Ensure that some things really are disabled to stay in the size limits. */
#undef CONSOLE_SERIAL
#undef CONSOLE_SYSLOG
#undef CONSOLE_EFI
#undef CONSOLE_LINUX
#undef CONSOLE_VMWARE
#undef CONSOLE_DEBUGCON
#undef CONSOLE_VESAFB
#undef CONSOLE_FRAMEBUFFER

#undef NET_PROTO_IPV6
#undef NET_PROTO_STP
#undef NET_PROTO_LACP

#undef DOWNLOAD_PROTO_HTTP
#undef DOWNLOAD_PROTO_HTTPS
#undef DOWNLOAD_PROTO_NFS
#undef DOWNLOAD_PROTO_SLAM

#undef PCI_SETTINGS
#undef VMWARE_SETTINGS
#undef CPUID_SETTINGS
#undef MEMMAP_SETTINGS
#undef VRAM_SETTINGS
#undef ACPI_SETTINGS

#undef HTTP_AUTH_BASIC
#undef HTTP_AUTH_DIGEST
#undef HTTP_AUTH_NTLM
#undef HTTP_ENC_PEERDIST
#undef HTTP_HACK_GCE

#undef TIMER_RDTSC
#undef TIMER_EFI
#undef TIMER_LINUX
#undef TIMER_ACPI

#undef IMAGE_PNM
#undef IMAGE_PNG

#undef SANBOOT_CMD
#undef IFMGMT_CMD
#undef CONFIG_CMD
#undef NVO_CMD
#undef PXE_CMD
#undef DHCP_CMD
#undef ROUTE_CMD
#undef LOGIN_CMD
#undef SYNC_CMD
