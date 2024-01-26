/** @file
 * VirtualBox - Logging.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_log_h
#define VBOX_INCLUDED_log_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/*
 * Set the default loggroup.
 */
#ifndef LOG_GROUP
# define LOG_GROUP LOG_GROUP_DEFAULT
#endif

#include <iprt/log.h>


/** @defgroup grp_rt_vbox_log    VBox Logging
 * @ingroup grp_rt_vbox
 * @{
 */

/** PC port for debug output */
#define RTLOG_DEBUG_PORT 0x504

/**
 * VirtualBox Logging Groups.
 * (Remember to update LOGGROUP_NAMES!)
 *
 * @remark It should be pretty obvious, but just to have
 *         mentioned it, the values are sorted alphabetically (using the
 *         english alphabet) except for _DEFAULT which is always first.
 *
 *         If anyone might be wondering what the alphabet looks like:
 *              A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _
 */
typedef enum VBOXLOGGROUP
{
    /** The default VBox group. */
    LOG_GROUP_DEFAULT = RTLOGGROUP_FIRST_USER,
    /** Audio mixer group. */
    LOG_GROUP_AUDIO_MIXER,
    /** Audio mixer buffer group. */
    LOG_GROUP_AUDIO_MIXER_BUFFER,
    /** Audio test group. */
    LOG_GROUP_AUDIO_TEST,
    /** Auto-logon group. */
    LOG_GROUP_AUTOLOGON,
    /** CFGM group. */
    LOG_GROUP_CFGM,
    /** CPUM group. */
    LOG_GROUP_CPUM,
    /** CSAM group. */
    LOG_GROUP_CSAM,
    /** Debug Console group. */
    LOG_GROUP_DBGC,
    /** DBGF group. */
    LOG_GROUP_DBGF,
    /** DBGF info group. */
    LOG_GROUP_DBGF_INFO,
    /** The debugger gui. */
    LOG_GROUP_DBGG,
    /** Generic Device group. */
    LOG_GROUP_DEV,
    /** AC97 Device group. */
    LOG_GROUP_DEV_AC97,
    /** ACPI Device group. */
    LOG_GROUP_DEV_ACPI,
    /** AHCI Device group. */
    LOG_GROUP_DEV_AHCI,
    /** APIC Device group. */
    LOG_GROUP_DEV_APIC,
    /** BusLogic SCSI host adapter group. */
    LOG_GROUP_DEV_BUSLOGIC,
    /** DMA Controller group. */
    LOG_GROUP_DEV_DMA,
    /** NS DP8390 Ethernet Device group. */
    LOG_GROUP_DEV_DP8390,
    /** Gigabit Ethernet Device group. */
    LOG_GROUP_DEV_E1000,
    /** Extensible Firmware Interface Device group. */
    LOG_GROUP_DEV_EFI,
    /** USB EHCI Device group. */
    LOG_GROUP_DEV_EHCI,
    /** 3C501 Ethernet Device group. */
    LOG_GROUP_DEV_ELNK,
    /** Floppy Controller Device group. */
    LOG_GROUP_DEV_FDC,
    /** Flash Device group. */
    LOG_GROUP_DEV_FLASH,
    /** Guest Interface Manager Device group. */
    LOG_GROUP_DEV_GIM,
    /** HDA Device group. */
    LOG_GROUP_DEV_HDA,
    /** HDA Codec Device group. */
    LOG_GROUP_DEV_HDA_CODEC,
    /** High Precision Event Timer Device group. */
    LOG_GROUP_DEV_HPET,
    /** IDE Device group. */
    LOG_GROUP_DEV_IDE,
    /** The internal networking IP stack Device group. */
    LOG_GROUP_DEV_INIP,
    /** I/O APIC Device group. */
    LOG_GROUP_DEV_IOAPIC,
    /** IOMMU Device group. */
    LOG_GROUP_DEV_IOMMU,
    /** KeyBoard Controller Device group. */
    LOG_GROUP_DEV_KBD,
    /** Low Pin Count Device group. */
    LOG_GROUP_DEV_LPC,
    /** LsiLogic SCSI controller Device group. */
    LOG_GROUP_DEV_LSILOGICSCSI,
    /** NVMe Device group. */
    LOG_GROUP_DEV_NVME,
    /** USB OHCI Device group. */
    LOG_GROUP_DEV_OHCI,
    /** Parallel Device group */
    LOG_GROUP_DEV_PARALLEL,
    /** PC Device group. */
    LOG_GROUP_DEV_PC,
    /** PC Architecture Device group. */
    LOG_GROUP_DEV_PC_ARCH,
    /** PC BIOS Device group. */
    LOG_GROUP_DEV_PC_BIOS,
    /** PCI Device group. */
    LOG_GROUP_DEV_PCI,
    /** PCI Raw Device group. */
    LOG_GROUP_DEV_PCI_RAW,
    /** PCNet Device group. */
    LOG_GROUP_DEV_PCNET,
    /** PIC Device group. */
    LOG_GROUP_DEV_PIC,
    /** PIT Device group. */
    LOG_GROUP_DEV_PIT,
    /** QEMU firmware config Device group. */
    LOG_GROUP_DEV_QEMUFWCFG,
    /** RTC Device group. */
    LOG_GROUP_DEV_RTC,
    /** SB16 Device group. */
    LOG_GROUP_DEV_SB16,
    /** Serial Device group */
    LOG_GROUP_DEV_SERIAL,
    /** System Management Controller Device group. */
    LOG_GROUP_DEV_SMC,
    /** Trusted Platform Module Device group. */
    LOG_GROUP_DEV_TPM,
    /** VGA Device group. */
    LOG_GROUP_DEV_VGA,
    /** Virtio PCI Device group. */
    LOG_GROUP_DEV_VIRTIO,
    /** Virtio Network Device group. */
    LOG_GROUP_DEV_VIRTIO_NET,
    /** VMM Device group. */
    LOG_GROUP_DEV_VMM,
    /** VMM Device group for backdoor logging. */
    LOG_GROUP_DEV_VMM_BACKDOOR,
    /** VMM Device group for logging guest backdoor logging to stderr. */
    LOG_GROUP_DEV_VMM_STDERR,
    /** VMSVGA Device group. */
    LOG_GROUP_DEV_VMSVGA,
    /** USB xHCI Device group. */
    LOG_GROUP_DEV_XHCI,
    /** Disassembler group. */
    LOG_GROUP_DIS,
    /** Generic driver group. */
    LOG_GROUP_DRV,
    /** ACPI driver group */
    LOG_GROUP_DRV_ACPI,
    /** Audio driver group */
    LOG_GROUP_DRV_AUDIO,
    /** Block driver group. */
    LOG_GROUP_DRV_BLOCK,
    /** Char driver group. */
    LOG_GROUP_DRV_CHAR,
    /** Cloud tunnel driver group. */
    LOG_GROUP_DRV_CTUN,
    /** Disk integrity driver group. */
    LOG_GROUP_DRV_DISK_INTEGRITY,
    /** Video Display driver group. */
    LOG_GROUP_DRV_DISPLAY,
    /** Floppy media driver group. */
    LOG_GROUP_DRV_FLOPPY,
    /** Host Audio driver group. */
    LOG_GROUP_DRV_HOST_AUDIO,
    /** Host Base block driver group. */
    LOG_GROUP_DRV_HOST_BASE,
    /** Host DVD block driver group. */
    LOG_GROUP_DRV_HOST_DVD,
    /** Host floppy block driver group. */
    LOG_GROUP_DRV_HOST_FLOPPY,
    /** Host Parallel Driver group */
    LOG_GROUP_DRV_HOST_PARALLEL,
    /** Host Serial Driver Group */
    LOG_GROUP_DRV_HOST_SERIAL,
    /** The internal networking transport driver group. */
    LOG_GROUP_DRV_INTNET,
    /** ISO (CD/DVD) media driver group. */
    LOG_GROUP_DRV_ISO,
    /** Keyboard Queue driver group. */
    LOG_GROUP_DRV_KBD_QUEUE,
    /** lwIP IP stack driver group. */
    LOG_GROUP_DRV_LWIP,
    /** Video Miniport driver group. */
    LOG_GROUP_DRV_MINIPORT,
    /** Mouse driver group. */
    LOG_GROUP_DRV_MOUSE,
    /** Mouse Queue driver group. */
    LOG_GROUP_DRV_MOUSE_QUEUE,
    /** Named Pipe stream driver group. */
    LOG_GROUP_DRV_NAMEDPIPE,
    /** NAT network transport driver group */
    LOG_GROUP_DRV_NAT,
    /** Raw image driver group */
    LOG_GROUP_DRV_RAW_IMAGE,
    /** SCSI driver group. */
    LOG_GROUP_DRV_SCSI,
    /** Host SCSI driver group. */
    LOG_GROUP_DRV_SCSIHOST,
    /** TCP socket stream driver group. */
    LOG_GROUP_DRV_TCP,
    /** Trusted Platform Module Emulation driver group. */
    LOG_GROUP_DRV_TPM_EMU,
    /** Trusted Platform Module Host driver group. */
    LOG_GROUP_DRV_TPM_HOST,
    /** Async transport driver group */
    LOG_GROUP_DRV_TRANSPORT_ASYNC,
    /** TUN network transport driver group */
    LOG_GROUP_DRV_TUN,
    /** UDP socket stream driver group. */
    LOG_GROUP_DRV_UDP,
    /** UDP tunnet network transport driver group. */
    LOG_GROUP_DRV_UDPTUNNEL,
    /** USB Proxy driver group. */
    LOG_GROUP_DRV_USBPROXY,
    /** VBoxHDD media driver group. */
    LOG_GROUP_DRV_VBOXHDD,
    /** VBox HDD container media driver group. */
    LOG_GROUP_DRV_VD,
    /** The VMNET networking driver group. */
    LOG_GROUP_DRV_VMNET,
    /** VRDE audio driver group. */
    LOG_GROUP_DRV_VRDE_AUDIO,
    /** Virtual Switch transport driver group */
    LOG_GROUP_DRV_VSWITCH,
    /** VUSB driver group */
    LOG_GROUP_DRV_VUSB,
    /** EM group. */
    LOG_GROUP_EM,
    /** FTM group. */
    LOG_GROUP_FTM,
    /** GIM group. */
    LOG_GROUP_GIM,
    /** GMM group. */
    LOG_GROUP_GMM,
    /** Guest control. */
    LOG_GROUP_GUEST_CONTROL,
    /** Guest drag'n drop. */
    LOG_GROUP_GUEST_DND,
    /** GUI group. */
    LOG_GROUP_GUI,
    /** GVMM group. */
    LOG_GROUP_GVMM,
    /** HGCM group */
    LOG_GROUP_HGCM,
    /** HGSMI group */
    LOG_GROUP_HGSMI,
    /** HM group. */
    LOG_GROUP_HM,
    /** IEM group. */
    LOG_GROUP_IEM,
    /** IEM AMD-V group. */
    LOG_GROUP_IEM_SVM,
    /** IEM VT-x group. */
    LOG_GROUP_IEM_VMX,
    /** I/O buffer management group. */
    LOG_GROUP_IOBUFMGMT,
    /** IOM group. */
    LOG_GROUP_IOM,
    /** IOM group, I/O port part. */
    LOG_GROUP_IOM_IOPORT,
    /** IOM group, MMIO part. */
    LOG_GROUP_IOM_MMIO,
    /** XPCOM IPC group. */
    LOG_GROUP_IPC,
    /** lwIP group. */
    LOG_GROUP_LWIP,
    /** lwIP group, api_lib.c API_LIB_DEBUG */
    LOG_GROUP_LWIP_API_LIB,
    /** lwIP group, api_msg.c API_MSG_DEBUG */
    LOG_GROUP_LWIP_API_MSG,
    /** lwIP group, etharp.c ETHARP_DEBUG */
    LOG_GROUP_LWIP_ETHARP,
    /** lwIP group, icmp.c ICMP_DEBUG */
    LOG_GROUP_LWIP_ICMP,
    /** lwIP group, igmp.c IGMP_DEBUG */
    LOG_GROUP_LWIP_IGMP,
    /** lwIP group, inet.c INET_DEBUG */
    LOG_GROUP_LWIP_INET,
    /** lwIP group, IP_DEBUG (sic!) */
    LOG_GROUP_LWIP_IP4,
    /** lwIP group, ip_frag.c IP_REASS_DEBUG (sic!) */
    LOG_GROUP_LWIP_IP4_REASS,
    /** lwIP group, IP6_DEBUG */
    LOG_GROUP_LWIP_IP6,
    /** lwIP group, mem.c MEM_DEBUG */
    LOG_GROUP_LWIP_MEM,
    /** lwIP group, memp.c MEMP_DEBUG */
    LOG_GROUP_LWIP_MEMP,
    /** lwIP group, netif.c NETIF_DEBUG */
    LOG_GROUP_LWIP_NETIF,
    /** lwIP group, pbuf.c PBUF_DEBUG */
    LOG_GROUP_LWIP_PBUF,
    /** lwIP group, raw.c RAW_DEBUG */
    LOG_GROUP_LWIP_RAW,
    /** lwIP group, sockets.c SOCKETS_DEBUG */
    LOG_GROUP_LWIP_SOCKETS,
    /** lwIP group, SYS_DEBUG */
    LOG_GROUP_LWIP_SYS,
    /** lwIP group, TCP_DEBUG */
    LOG_GROUP_LWIP_TCP,
    /** lwIP group, TCP_CWND_DEBUG (congestion window) */
    LOG_GROUP_LWIP_TCP_CWND,
    /** lwIP group, tcp_in.c TCP_FR_DEBUG (fast retransmit) */
    LOG_GROUP_LWIP_TCP_FR,
    /** lwIP group, tcp_in.c TCP_INPUT_DEBUG */
    LOG_GROUP_LWIP_TCP_INPUT,
    /** lwIP group, tcp_out.c TCP_OUTPUT_DEBUG */
    LOG_GROUP_LWIP_TCP_OUTPUT,
    /** lwIP group, TCP_QLEN_DEBUG */
    LOG_GROUP_LWIP_TCP_QLEN,
    /** lwIP group, TCP_RST_DEBUG */
    LOG_GROUP_LWIP_TCP_RST,
    /** lwIP group, TCP_RTO_DEBUG (retransmit) */
    LOG_GROUP_LWIP_TCP_RTO,
    /** lwIP group, tcp_in.c TCP_WND_DEBUG (window updates) */
    LOG_GROUP_LWIP_TCP_WND,
    /** lwIP group, tcpip.c TCPIP_DEBUG */
    LOG_GROUP_LWIP_TCPIP,
    /** lwIP group, timers.c TIMERS_DEBUG */
    LOG_GROUP_LWIP_TIMERS,
    /** lwIP group, udp.c UDP_DEBUG */
    LOG_GROUP_LWIP_UDP,
    /** Main group. */
    LOG_GROUP_MAIN,
    /** Main group, IAdditionsFacility. */
    LOG_GROUP_MAIN_ADDITIONSFACILITY,
    /** Main group, IAppliance. */
    LOG_GROUP_MAIN_APPLIANCE,
    /** Main group, IAudioAdapter. */
    LOG_GROUP_MAIN_AUDIOADAPTER,
    /** Main group, IAudioDevice. */
    LOG_GROUP_MAIN_AUDIODEVICE,
    /** Main group, IAudioSettings. */
    LOG_GROUP_MAIN_AUDIOSETTINGS,
    /** Main group, IBandwidthControl. */
    LOG_GROUP_MAIN_BANDWIDTHCONTROL,
    /** Main group, IBandwidthGroup. */
    LOG_GROUP_MAIN_BANDWIDTHGROUP,
    /** Main group, IBIOSSettings. */
    LOG_GROUP_MAIN_BIOSSETTINGS,
    /** Main group, IBooleanFormValue. */
    LOG_GROUP_MAIN_BOOLEANFORMVALUE,
    /** Main group, ICertificate. */
    LOG_GROUP_MAIN_CERTIFICATE,
    /** Main group, IChoiceFormValue. */
    LOG_GROUP_MAIN_CHOICEFORMVALUE,
    /** Main group, ICloudClient. */
    LOG_GROUP_MAIN_CLOUDCLIENT,
    /** Main group, ICloudMachine. */
    LOG_GROUP_MAIN_CLOUDMACHINE,
    /** Main group, ICloudNetwork. */
    LOG_GROUP_MAIN_CLOUDNETWORK,
    /** Main group, ICloudNetworkEnvironmentInfo */
    LOG_GROUP_MAIN_CLOUDNETWORKENVIRONMENTINFO,
    /** Main group, ICloudNetworkGatewayInfo */
    LOG_GROUP_MAIN_CLOUDNETWORKGATEWAYINFO,
    /** Main group, ICloudProfile. */
    LOG_GROUP_MAIN_CLOUDPROFILE,
    /** Main group, ICloudProfileChangedEvent. */
    LOG_GROUP_MAIN_CLOUDPROFILECHANGEDEVENT,
    /** Main group, ICloudProfileRegisteredEvent. */
    LOG_GROUP_MAIN_CLOUDPROFILEREGISTEREDEVENT,
    /** Main group, ICloudProvider. */
    LOG_GROUP_MAIN_CLOUDPROVIDER,
    /** Main group, ICloudProviderManager. */
    LOG_GROUP_MAIN_CLOUDPROVIDERMANAGER,
    /** Main group, IConsole. */
    LOG_GROUP_MAIN_CONSOLE,
    /** Main group, ICPUProfile. */
    LOG_GROUP_MAIN_CPUPROFILE,
    /** Main group, IDataModel. */
    LOG_GROUP_MAIN_DATAMODEL,
    /** Main group, IDataStream. */
    LOG_GROUP_MAIN_DATASTREAM,
    /** Main group, IDHCPConfig. */
    LOG_GROUP_MAIN_DHCPCONFIG,
    /** Main group, IDHCPGlobalConfig. */
    LOG_GROUP_MAIN_DHCPGLOBALCONFIG,
    /** Main group, IDHCPGroupCondition. */
    LOG_GROUP_MAIN_DHCPGROUPCONDITION,
    /** Main group, IDHCPGroupConfig. */
    LOG_GROUP_MAIN_DHCPGROUPCONFIG,
    /** Main group, IDHCPIndividualConfig. */
    LOG_GROUP_MAIN_DHCPINDIVIDUALCONFIG,
    /** Main group, IDHCPServer. */
    LOG_GROUP_MAIN_DHCPSERVER,
    /** Main group, IDirectory. */
    LOG_GROUP_MAIN_DIRECTORY,
    /** Main group, IDisplay. */
    LOG_GROUP_MAIN_DISPLAY,
    /** Main group, IDisplaySourceBitmap. */
    LOG_GROUP_MAIN_DISPLAYSOURCEBITMAP,
    /** Main group, IDnDBase. */
    LOG_GROUP_MAIN_DNDBASE,
    /** Main group, IDnDSource. */
    LOG_GROUP_MAIN_DNDSOURCE,
    /** Main group, IDnDTarget. */
    LOG_GROUP_MAIN_DNDTARGET,
    /** Main group, IEmulatedUSB. */
    LOG_GROUP_MAIN_EMULATEDUSB,
    /** Main group, IEvent. */
    LOG_GROUP_MAIN_EVENT,
    /** Main group, IEventListener. */
    LOG_GROUP_MAIN_EVENTLISTENER,
    /** Main group, IEventSource. */
    LOG_GROUP_MAIN_EVENTSOURCE,
    /** Main group, IExtPack. */
    LOG_GROUP_MAIN_EXTPACK,
    /** Main group, IExtPackBase. */
    LOG_GROUP_MAIN_EXTPACKBASE,
    /** Main group, IExtPackFile. */
    LOG_GROUP_MAIN_EXTPACKFILE,
    /** Main group, IExtPackManager. */
    LOG_GROUP_MAIN_EXTPACKMANAGER,
    /** Main group, IExtPackPlugIn. */
    LOG_GROUP_MAIN_EXTPACKPLUGIN,
    /** Main group, IFile. */
    LOG_GROUP_MAIN_FILE,
    /** Main group, IForm. */
    LOG_GROUP_MAIN_FORM,
    /** Main group, IFormValue. */
    LOG_GROUP_MAIN_FORMVALUE,
    /** Main group, IFramebuffer. */
    LOG_GROUP_MAIN_FRAMEBUFFER,
    /** Main group, IFramebufferOverlay. */
    LOG_GROUP_MAIN_FRAMEBUFFEROVERLAY,
    /** Main group, IFsInfo. */
    LOG_GROUP_MAIN_FSINFO,
    /** Main group, IFsObjInfo. */
    LOG_GROUP_MAIN_FSOBJINFO,
    /** Main group, IGraphicsAdapter. */
    LOG_GROUP_MAIN_GRAPHICSADAPTER,
    /** Main group, IGuest. */
    LOG_GROUP_MAIN_GUEST,
    /** Main group, IGuestDebugControl. */
    LOG_GROUP_MAIN_GUESTDEBUGCONTROL,
    /** Main group, IGuestDirectory. */
    LOG_GROUP_MAIN_GUESTDIRECTORY,
    /** Main group, IGuestDnDSource. */
    LOG_GROUP_MAIN_GUESTDNDSOURCE,
    /** Main group, IGuestDnDTarget. */
    LOG_GROUP_MAIN_GUESTDNDTARGET,
    /** Main group, IGuestErrorInfo. */
    LOG_GROUP_MAIN_GUESTERRORINFO,
    /** Main group, IGuestFile. */
    LOG_GROUP_MAIN_GUESTFILE,
    /** Main group, IGuestFileEvent. */
    LOG_GROUP_MAIN_GUESTFILEEVENT,
    /** Main group, IGuestFileIOEvent. */
    LOG_GROUP_MAIN_GUESTFILEIOEVENT,
    /** Main group, IGuestFsInfo. */
    LOG_GROUP_MAIN_GUESTFSINFO,
    /** Main group, IGuestFsObjInfo. */
    LOG_GROUP_MAIN_GUESTFSOBJINFO,
    /** Main group, IGuestOSType. */
    LOG_GROUP_MAIN_GUESTOSTYPE,
    /** Main group, IGuestProcess. */
    LOG_GROUP_MAIN_GUESTPROCESS,
    /** Main group, IGuestProcessEvent. */
    LOG_GROUP_MAIN_GUESTPROCESSEVENT,
    /** Main group, IGuestProcessIOEvent. */
    LOG_GROUP_MAIN_GUESTPROCESSIOEVENT,
    /** Main group, IGuestScreenInfo. */
    LOG_GROUP_MAIN_GUESTSCREENINFO,
    /** Main group, IGuestSession. */
    LOG_GROUP_MAIN_GUESTSESSION,
    /** Main group, IGuestSessionEvent. */
    LOG_GROUP_MAIN_GUESTSESSIONEVENT,
    /** Main group, IHost. */
    LOG_GROUP_MAIN_HOST,
    /** Main group, IHostAudioDevice. */
    LOG_GROUP_MAIN_HOSTAUDIODEVICE,
    /** Main group, IHostDrive. */
    LOG_GROUP_MAIN_HOSTDRIVE,
    /** Main group, IHostDriveList. */
    LOG_GROUP_MAIN_HOSTDRIVELIST,
    /** Main group, IHostDrivePartition. */
    LOG_GROUP_MAIN_HOSTDRIVEPARTITION,
    /** Main group, IHostNetworkInterface. */
    LOG_GROUP_MAIN_HOSTNETWORKINTERFACE,
    /** Main group, IHostOnlyNetwork. */
    LOG_GROUP_MAIN_HOSTONLYNETWORK,
    /** Main group, IHostUpdateAgent. */
    LOG_GROUP_MAIN_HOSTUPDATEAGENT,
    /** Main group, IHostUSBDevice. */
    LOG_GROUP_MAIN_HOSTUSBDEVICE,
    /** Main group, IHostUSBDeviceFilter. */
    LOG_GROUP_MAIN_HOSTUSBDEVICEFILTER,
    /** Main group, IHostVideoInputDevice. */
    LOG_GROUP_MAIN_HOSTVIDEOINPUTDEVICE,
    /** Main group, IInternalMachineControl. */
    LOG_GROUP_MAIN_INTERNALMACHINECONTROL,
    /** Main group, IInternalSessionControl. */
    LOG_GROUP_MAIN_INTERNALSESSIONCONTROL,
    /** Main group, IKeyboard. */
    LOG_GROUP_MAIN_KEYBOARD,
    /** Main group, IMachine. */
    LOG_GROUP_MAIN_MACHINE,
    /** Main group, IMachineDebugger. */
    LOG_GROUP_MAIN_MACHINEDEBUGGER,
    /** Main group, IMachineEvent. */
    LOG_GROUP_MAIN_MACHINEEVENT,
    /** Main group, IMedium. */
    LOG_GROUP_MAIN_MEDIUM,
    /** Main group, IMediumAttachment. */
    LOG_GROUP_MAIN_MEDIUMATTACHMENT,
    /** Main group, IMediumFormat. */
    LOG_GROUP_MAIN_MEDIUMFORMAT,
    /** Main group, IMediumIO. */
    LOG_GROUP_MAIN_MEDIUMIO,
    /** Main group, IMouse. */
    LOG_GROUP_MAIN_MOUSE,
    /** Main group, IMousePointerShape. */
    LOG_GROUP_MAIN_MOUSEPOINTERSHAPE,
    /** Main group, INATEngine. */
    LOG_GROUP_MAIN_NATENGINE,
    /** Main group, INATNetwork. */
    LOG_GROUP_MAIN_NATNETWORK,
    /** Main group, INetworkAdapter. */
    LOG_GROUP_MAIN_NETWORKADAPTER,
    /** Main group, INvramStore. */
    LOG_GROUP_MAIN_NVRAMSTORE,
    /** Main group, IParallelPort. */
    LOG_GROUP_MAIN_PARALLELPORT,
    /** Main group, IPCIAddress. */
    LOG_GROUP_MAIN_PCIADDRESS,
    /** Main group, IPCIDeviceAttachment. */
    LOG_GROUP_MAIN_PCIDEVICEATTACHMENT,
    /** Main group, IPerformanceCollector. */
    LOG_GROUP_MAIN_PERFORMANCECOLLECTOR,
    /** Main group, IPerformanceMetric. */
    LOG_GROUP_MAIN_PERFORMANCEMETRIC,
    /** Main group, IProcess. */
    LOG_GROUP_MAIN_PROCESS,
    /** Main group, IProgress. */
    LOG_GROUP_MAIN_PROGRESS,
    /** Main group, IProgressCreatedEvent. */
    LOG_GROUP_MAIN_PROGRESSCREATEDEVENT,
    /** Main group, IProgressEvent. */
    LOG_GROUP_MAIN_PROGRESSEVENT,
    /** Main group, IRangedIntegerFormValue. */
    LOG_GROUP_MAIN_RANGEDINTEGERFORMVALUE,
    /** Main group, IRangedInteger64FormValue. */
    LOG_GROUP_MAIN_RANGEDINTEGER64FORMVALUE,
    /** Main group, IRecordingScreenSettings. */
    LOG_GROUP_MAIN_RECORDINGSCREENSETTINGS,
    /** Main group, IRecordingSettings. */
    LOG_GROUP_MAIN_RECORDINGSETTINGS,
    /** Main group, IReusableEvent. */
    LOG_GROUP_MAIN_REUSABLEEVENT,
    /** Main group, ISerialPort. */
    LOG_GROUP_MAIN_SERIALPORT,
    /** Main group, ISession. */
    LOG_GROUP_MAIN_SESSION,
    /** Main group, ISharedFolder. */
    LOG_GROUP_MAIN_SHAREDFOLDER,
    /** Main group, ISnapshot. */
    LOG_GROUP_MAIN_SNAPSHOT,
    /** Main group, ISnapshotEvent. */
    LOG_GROUP_MAIN_SNAPSHOTEVENT,
    /** Main group, IStorageController. */
    LOG_GROUP_MAIN_STORAGECONTROLLER,
    /** Main group, IStringArray. */
    LOG_GROUP_MAIN_STRINGARRAY,
    /** Main group, IStringFormValue. */
    LOG_GROUP_MAIN_STRINGFORMVALUE,
    /** Main group, ISystemProperties. */
    LOG_GROUP_MAIN_SYSTEMPROPERTIES,
    /** Main group, threaded tasks. */
    LOG_GROUP_MAIN_THREAD_TASK,
    /** Main group, IToken. */
    LOG_GROUP_MAIN_TOKEN,
    /** Main group, ITrustedPlatformModule. */
    LOG_GROUP_MAIN_TRUSTEDPLATFORMMODULE,
    /** Main group, IUefiVariableStore. */
    LOG_GROUP_MAIN_UEFIVARIABLESTORE,
    /** Main group, IUnattended. */
    LOG_GROUP_MAIN_UNATTENDED,
    /** Main group, IUpdateAgent. */
    LOG_GROUP_MAIN_UPDATEAGENT,
    /** Main group, IUpdateAgentAvailableEvent. */
    LOG_GROUP_MAIN_UPDATEAGENTAVAILABLEEVENT,
    /** Main group, IUpdateAgentErrorEvent. */
    LOG_GROUP_MAIN_UPDATEAGENTERROREVENT,
    /** Main group, IUpdateAgentEvent. */
    LOG_GROUP_MAIN_UPDATEAGENTEVENT,
    /** Main group, IUpdateAgentSettingsChangedEvent. */
    LOG_GROUP_MAIN_UPDATEAGENTSETTINGSCHANGEDEVENT,
    /** Main group, IUpdateAgentStateChangedEvent. */
    LOG_GROUP_MAIN_UPDATEAGENTSTATECHANGEDEVENT,
    /** Main group, IUSBController. */
    LOG_GROUP_MAIN_USBCONTROLLER,
    /** Main group, IUSBDevice. */
    LOG_GROUP_MAIN_USBDEVICE,
    /** Main group, IUSBDeviceFilter. */
    LOG_GROUP_MAIN_USBDEVICEFILTER,
    /** Main group, IUSBDeviceFilters. */
    LOG_GROUP_MAIN_USBDEVICEFILTERS,
    /** Main group, IUSBProxyBackend. */
    LOG_GROUP_MAIN_USBPROXYBACKEND,
    /** Main group, IVBoxSVC. */
    LOG_GROUP_MAIN_VBOXSVC,
    /** Main group, IVetoEvent. */
    LOG_GROUP_MAIN_VETOEVENT,
    /** Main group, IVFSExplorer. */
    LOG_GROUP_MAIN_VFSEXPLORER,
    /** Main group, IVirtualBox. */
    LOG_GROUP_MAIN_VIRTUALBOX,
    /** Main group, IVirtualBoxClient. */
    LOG_GROUP_MAIN_VIRTUALBOXCLIENT,
    /** Main group, IVirtualBoxSDS. */
    LOG_GROUP_MAIN_VIRTUALBOXSDS,
    /** Main group, IVirtualSystemDescription. */
    LOG_GROUP_MAIN_VIRTUALSYSTEMDESCRIPTION,
    /** Main group, IVirtualSystemDescriptionForm. */
    LOG_GROUP_MAIN_VIRTUALSYSTEMDESCRIPTIONFORM,
    /** Main group, VMM device interfaces. */
    LOG_GROUP_MAIN_VMMDEVINTERFACES,
    /** Main group, IVRDEServer. */
    LOG_GROUP_MAIN_VRDESERVER,
    /** Main group, IVRDEServerInfo. */
    LOG_GROUP_MAIN_VRDESERVERINFO,
    /** Misc. group intended for external use only. */
    LOG_GROUP_MISC,
    /** MM group. */
    LOG_GROUP_MM,
    /** MM group. */
    LOG_GROUP_MM_HEAP,
    /** MM group. */
    LOG_GROUP_MM_HYPER,
    /** MM Hypervisor Heap group. */
    LOG_GROUP_MM_HYPER_HEAP,
    /** MM Physical/Ram group. */
    LOG_GROUP_MM_PHYS,
    /** MM Page pool group. */
    LOG_GROUP_MM_POOL,
    /** The NAT service group */
    LOG_GROUP_NAT_SERVICE,
    /** NEM group. */
    LOG_GROUP_NEM,
    /** The network adaptor driver group. */
    LOG_GROUP_NET_ADP_DRV,
    /** The DHCP network service deamon. */
    LOG_GROUP_NET_DHCPD,
    /** The network filter driver group. */
    LOG_GROUP_NET_FLT_DRV,
    /** The common network service group */
    LOG_GROUP_NET_SERVICE,
    /** Network traffic shaper driver group. */
    LOG_GROUP_NET_SHAPER,
    /** PATM group. */
    LOG_GROUP_PATM,
    /** PDM group. */
    LOG_GROUP_PDM,
    /** PDM Async completion group. */
    LOG_GROUP_PDM_ASYNC_COMPLETION,
    /** PDM Block cache group. */
    LOG_GROUP_PDM_BLK_CACHE,
    /** PDM critical section group. */
    LOG_GROUP_PDM_CRITSECT,
    /** PDM read/write critical section group. */
    LOG_GROUP_PDM_CRITSECTRW,
    /** PDM Device group. */
    LOG_GROUP_PDM_DEVICE,
    /** PDM Driver group. */
    LOG_GROUP_PDM_DRIVER,
    /** PDM Loader group. */
    LOG_GROUP_PDM_LDR,
    /** PDM Queue group. */
    LOG_GROUP_PDM_QUEUE,
    /** PDM Task group. */
    LOG_GROUP_PDM_TASK,
    /** PDM Thread group. */
    LOG_GROUP_PDM_THREAD,
    /** PGM group. */
    LOG_GROUP_PGM,
    /** PGM dynamic mapping group. */
    LOG_GROUP_PGM_DYNMAP,
    /** PGM physical group. */
    LOG_GROUP_PGM_PHYS,
    /** PGM physical access group. */
    LOG_GROUP_PGM_PHYS_ACCESS,
    /** PGM shadow page pool group. */
    LOG_GROUP_PGM_POOL,
    /** PGM shared paging group. */
    LOG_GROUP_PGM_SHARED,
    /** Audio + video recording. */
    LOG_GROUP_RECORDING,
    /** REM group. */
    LOG_GROUP_REM,
    /** REM disassembly handler group. */
    LOG_GROUP_REM_DISAS,
    /** REM access handler group. */
    LOG_GROUP_REM_HANDLER,
    /** REM I/O port access group. */
    LOG_GROUP_REM_IOPORT,
    /** REM MMIO access group. */
    LOG_GROUP_REM_MMIO,
    /** REM Printf. */
    LOG_GROUP_REM_PRINTF,
    /** REM running group. */
    LOG_GROUP_REM_RUN,
    /** SELM group. */
    LOG_GROUP_SELM,
    /** Shared clipboard host service group. */
    LOG_GROUP_SHARED_CLIPBOARD,
    /** Chromium OpenGL host service group. */
    LOG_GROUP_SHARED_CROPENGL,
    /** Shared folders host service group. */
    LOG_GROUP_SHARED_FOLDERS,
    /** OpenGL host service group. */
    LOG_GROUP_SHARED_OPENGL,
    /** The internal networking service group. */
    LOG_GROUP_SRV_INTNET,
    /** SSM group. */
    LOG_GROUP_SSM,
    /** STAM group. */
    LOG_GROUP_STAM,
    /** SUP group. */
    LOG_GROUP_SUP,
    /** SUPport driver group. */
    LOG_GROUP_SUP_DRV,
    /** TM group. */
    LOG_GROUP_TM,
    /** TRPM group. */
    LOG_GROUP_TRPM,
    /** USB cardreader group. */
    LOG_GROUP_USB_CARDREADER,
    /** USB driver group. */
    LOG_GROUP_USB_DRV,
    /** USBFilter group. */
    LOG_GROUP_USB_FILTER,
    /** USB keyboard device group. */
    LOG_GROUP_USB_KBD,
    /** USB mouse/tablet device group. */
    LOG_GROUP_USB_MOUSE,
    /** MSD USB device group. */
    LOG_GROUP_USB_MSD,
    /** USB remote support. */
    LOG_GROUP_USB_REMOTE,
    /** USB webcam. */
    LOG_GROUP_USB_WEBCAM,
    /** VBox Guest Additions Library. */
    LOG_GROUP_VBGL,
    /** Generic virtual disk layer. */
    LOG_GROUP_VD,
    /** CUE/BIN virtual disk backend. */
    LOG_GROUP_VD_CUE,
    /** DMG virtual disk backend. */
    LOG_GROUP_VD_DMG,
    /** iSCSI virtual disk backend. */
    LOG_GROUP_VD_ISCSI,
    /** Parallels HDD virtual disk backend. */
    LOG_GROUP_VD_PARALLELS,
    /** QCOW virtual disk backend. */
    LOG_GROUP_VD_QCOW,
    /** QED virtual disk backend. */
    LOG_GROUP_VD_QED,
    /** Raw virtual disk backend. */
    LOG_GROUP_VD_RAW,
    /** VDI virtual disk backend. */
    LOG_GROUP_VD_VDI,
    /** VHD virtual disk backend. */
    LOG_GROUP_VD_VHD,
    /** VHDX virtual disk backend. */
    LOG_GROUP_VD_VHDX,
    /** VMDK virtual disk backend. */
    LOG_GROUP_VD_VMDK,
    /** VBox Guest Additions Driver (VBoxGuest). */
    LOG_GROUP_VGDRV,
    /** VM group. */
    LOG_GROUP_VM,
    /** VMM group. */
    LOG_GROUP_VMM,
    /** VRDE group */
    LOG_GROUP_VRDE,
    /** VRDP group */
    LOG_GROUP_VRDP,
    /** VSCSI group */
    LOG_GROUP_VSCSI,
    /** Webservice group. */
    LOG_GROUP_WEBSERVICE
    /* !!!ALPHABETICALLY!!! */
} VBOXLOGGROUP;


/** @def VBOX_LOGGROUP_NAMES
 * VirtualBox Logging group names.
 *
 * Must correspond 100% to LOGGROUP!
 * Don't forget commas!
 *
 * @remark It should be pretty obvious, but just to have
 *         mentioned it, the values are sorted alphabetically (using the
 *         english alphabet) except for _DEFAULT which is always first.
 *
 *         If anyone might be wondering what the alphabet looks like:
 *              a b c d e f g h i j k l m n o p q r s t u v w x y z
 */
#define VBOX_LOGGROUP_NAMES \
{                   \
    RT_LOGGROUP_NAMES, \
    "DEFAULT", \
    "AUDIO_MIXER", \
    "AUDIO_MIXER_BUFFER", \
    "AUDIO_TEST", \
    "AUTOLOGON", \
    "CFGM", \
    "CPUM", \
    "CSAM", \
    "DBGC", \
    "DBGF", \
    "DBGF_INFO", \
    "DBGG", \
    "DEV", \
    "DEV_AC97", \
    "DEV_ACPI", \
    "DEV_AHCI", \
    "DEV_APIC", \
    "DEV_BUSLOGIC", \
    "DEV_DMA", \
    "DEV_DP8390", \
    "DEV_E1000", \
    "DEV_EFI", \
    "DEV_EHCI", \
    "DEV_ELNK", \
    "DEV_FDC", \
    "DEV_FLASH", \
    "DEV_GIM", \
    "DEV_HDA", \
    "DEV_HDA_CODEC", \
    "DEV_HPET", \
    "DEV_IDE", \
    "DEV_INIP", \
    "DEV_IOAPIC", \
    "DEV_IOMMU", \
    "DEV_KBD", \
    "DEV_LPC", \
    "DEV_LSILOGICSCSI", \
    "DEV_NVME", \
    "DEV_OHCI", \
    "DEV_PARALLEL", \
    "DEV_PC", \
    "DEV_PC_ARCH", \
    "DEV_PC_BIOS", \
    "DEV_PCI", \
    "DEV_PCI_RAW", \
    "DEV_PCNET", \
    "DEV_PIC", \
    "DEV_PIT", \
    "DEV_QEMUFWCFG", \
    "DEV_RTC", \
    "DEV_SB16", \
    "DEV_SERIAL", \
    "DEV_SMC", \
    "DEV_TPM", \
    "DEV_VGA", \
    "DEV_VIRTIO", \
    "DEV_VIRTIO_NET", \
    "DEV_VMM", \
    "DEV_VMM_BACKDOOR", \
    "DEV_VMM_STDERR", \
    "DEV_VMSVGA", \
    "DEV_XHCI", \
    "DIS", \
    "DRV", \
    "DRV_ACPI", \
    "DRV_AUDIO", \
    "DRV_BLOCK", \
    "DRV_CHAR", \
    "DRV_CTUN", \
    "DRV_DISK_INTEGRITY", \
    "DRV_DISPLAY", \
    "DRV_FLOPPY", \
    "DRV_HOST_AUDIO", \
    "DRV_HOST_BASE", \
    "DRV_HOST_DVD", \
    "DRV_HOST_FLOPPY", \
    "DRV_HOST_PARALLEL", \
    "DRV_HOST_SERIAL", \
    "DRV_INTNET", \
    "DRV_ISO", \
    "DRV_KBD_QUEUE", \
    "DRV_LWIP", \
    "DRV_MINIPORT", \
    "DRV_MOUSE", \
    "DRV_MOUSE_QUEUE", \
    "DRV_NAMEDPIPE", \
    "DRV_NAT", \
    "DRV_RAW_IMAGE", \
    "DRV_SCSI", \
    "DRV_SCSIHOST", \
    "DRV_TCP", \
    "DRV_TPM_EMU", \
    "DRV_TPM_HOST", \
    "DRV_TRANSPORT_ASYNC", \
    "DRV_TUN", \
    "DRV_UDP", \
    "DRV_UDPTUNNEL", \
    "DRV_USBPROXY", \
    "DRV_VBOXHDD", \
    "DRV_VD", \
    "DRV_VMNET", \
    "DRV_VRDE_AUDIO", \
    "DRV_VSWITCH", \
    "DRV_VUSB", \
    "EM", \
    "FTM", \
    "GIM", \
    "GMM", \
    "GUEST_CONTROL", \
    "GUEST_DND", \
    "GUI", \
    "GVMM", \
    "HGCM", \
    "HGSMI", \
    "HM", \
    "IEM", \
    "IEM_SVM", \
    "IEM_VMX", \
    "IOBUFMGMT", \
    "IOM", \
    "IOM_IOPORT", \
    "IOM_MMIO", \
    "IPC", \
    "LWIP", \
    "LWIP_API_LIB", \
    "LWIP_API_MSG", \
    "LWIP_ETHARP", \
    "LWIP_ICMP", \
    "LWIP_IGMP", \
    "LWIP_INET", \
    "LWIP_IP4", \
    "LWIP_IP4_REASS", \
    "LWIP_IP6", \
    "LWIP_MEM", \
    "LWIP_MEMP", \
    "LWIP_NETIF", \
    "LWIP_PBUF", \
    "LWIP_RAW", \
    "LWIP_SOCKETS", \
    "LWIP_SYS", \
    "LWIP_TCP", \
    "LWIP_TCP_CWND", \
    "LWIP_TCP_FR", \
    "LWIP_TCP_INPUT", \
    "LWIP_TCP_OUTPUT", \
    "LWIP_TCP_QLEN", \
    "LWIP_TCP_RST", \
    "LWIP_TCP_RTO", \
    "LWIP_TCP_WND", \
    "LWIP_TCPIP", \
    "LWIP_TIMERS", \
    "LWIP_UDP", \
    "MAIN", \
    "MAIN_ADDITIONSFACILITY", \
    "MAIN_APPLIANCE", \
    "MAIN_AUDIOADAPTER", \
    "MAIN_AUDIODEVICE", \
    "MAIN_AUDIOSETTINGS", \
    "MAIN_BANDWIDTHCONTROL", \
    "MAIN_BANDWIDTHGROUP", \
    "MAIN_BIOSSETTINGS", \
    "MAIN_BOOLEANFORMVALUE", \
    "MAIN_CERTIFICATE", \
    "MAIN_CHOICEFORMVALUE", \
    "MAIN_CLOUDCLIENT", \
    "MAIN_CLOUDMACHINE", \
    "MAIN_CLOUDNETWORK", \
    "MAIN_CLOUDNETWORKENVIRONMENTINFO", \
    "MAIN_CLOUDNETWORKGATEWAYINFO", \
    "MAIN_CLOUDPROFILE", \
    "MAIN_CLOUDPROFILECHANGEDEVENT",   \
    "MAIN_CLOUDPROFILEREGISTEREDEVENT",   \
    "MAIN_CLOUDPROVIDER", \
    "MAIN_CLOUDPROVIDERMANAGER", \
    "MAIN_CONSOLE", \
    "MAIN_CPUPROFILE", \
    "MAIN_DATAMODEL", \
    "MAIN_DATASTREAM", \
    "MAIN_DHCPCONFIG", \
    "MAIN_DHCPGLOBALCONFIG", \
    "MAIN_DHCPGROUPCONDITION", \
    "MAIN_DHCPGROUPCONFIG", \
    "MAIN_DHCPINDIVIDUALCONFIG", \
    "MAIN_DHCPSERVER", \
    "MAIN_DIRECTORY", \
    "MAIN_DISPLAY", \
    "MAIN_DISPLAYSOURCEBITMAP", \
    "MAIN_DNDBASE", \
    "MAIN_DNDSOURCE", \
    "MAIN_DNDTARGET", \
    "MAIN_EMULATEDUSB", \
    "MAIN_EVENT", \
    "MAIN_EVENTLISTENER", \
    "MAIN_EVENTSOURCE", \
    "MAIN_EXTPACK", \
    "MAIN_EXTPACKBASE", \
    "MAIN_EXTPACKFILE", \
    "MAIN_EXTPACKMANAGER", \
    "MAIN_EXTPACKPLUGIN", \
    "MAIN_FILE", \
    "MAIN_FORM", \
    "MAIN_FORMVALUE", \
    "MAIN_FRAMEBUFFER", \
    "MAIN_FRAMEBUFFEROVERLAY", \
    "MAIN_FSINFO", \
    "MAIN_FSOBJINFO", \
    "MAIN_GRAPHICSADAPTER", \
    "MAIN_GUEST", \
    "MAIN_GUESTDEBUGCONTROL", \
    "MAIN_GUESTDIRECTORY", \
    "MAIN_GUESTDNDSOURCE", \
    "MAIN_GUESTDNDTARGET", \
    "MAIN_GUESTERRORINFO", \
    "MAIN_GUESTFILE", \
    "MAIN_GUESTFILEEVENT", \
    "MAIN_GUESTFILEIOEVENT", \
    "MAIN_GUESTFSINFO", \
    "MAIN_GUESTFSOBJINFO", \
    "MAIN_GUESTOSTYPE", \
    "MAIN_GUESTPROCESS", \
    "MAIN_GUESTPROCESSEVENT", \
    "MAIN_GUESTPROCESSIOEVENT", \
    "MAIN_GUESTSCREENINFO", \
    "MAIN_GUESTSESSION", \
    "MAIN_GUESTSESSIONEVENT", \
    "MAIN_HOST", \
    "MAIN_HOSTAUDIODEVICE", \
    "MAIN_HOSTDRIVE", \
    "MAIN_HOSTDRIVELIST", \
    "MAIN_HOSTDRIVEPARTITION", \
    "MAIN_HOSTNETWORKINTERFACE", \
    "MAIN_HOSTONLYNETWORK", \
    "MAIN_HOSTUPDATEAGENT", \
    "MAIN_HOSTUSBDEVICE", \
    "MAIN_HOSTUSBDEVICEFILTER", \
    "MAIN_HOSTVIDEOINPUTDEVICE", \
    "MAIN_INTERNALMACHINECONTROL", \
    "MAIN_INTERNALSESSIONCONTROL", \
    "MAIN_KEYBOARD", \
    "MAIN_MACHINE", \
    "MAIN_MACHINEDEBUGGER", \
    "MAIN_MACHINEEVENT", \
    "MAIN_MEDIUM", \
    "MAIN_MEDIUMATTACHMENT", \
    "MAIN_MEDIUMFORMAT", \
    "MAIN_MEDIUMIO", \
    "MAIN_MOUSE", \
    "MAIN_MOUSEPOINTERSHAPE", \
    "MAIN_NATENGINE", \
    "MAIN_NATNETWORK", \
    "MAIN_NETWORKADAPTER", \
    "MAIN_NVRAMSTORE", \
    "MAIN_PARALLELPORT", \
    "MAIN_PCIADDRESS", \
    "MAIN_PCIDEVICEATTACHMENT", \
    "MAIN_PERFORMANCECOLLECTOR", \
    "MAIN_PERFORMANCEMETRIC", \
    "MAIN_PROCESS", \
    "MAIN_PROGRESS", \
    "MAIN_PROGRESSCREATEDEVENT", \
    "MAIN_PROGRESSEVENT", \
    "MAIN_RANGEDINTEGER64FORMVALUE", \
    "MAIN_RANGEDINTEGERFORMVALUE", \
    "MAIN_RECORDINGSCREENSETTINGS", \
    "MAIN_RECORDINGSETTINGS", \
    "MAIN_REUSABLEEVENT", \
    "MAIN_SERIALPORT", \
    "MAIN_SESSION", \
    "MAIN_SHAREDFOLDER", \
    "MAIN_SNAPSHOT", \
    "MAIN_SNAPSHOTEVENT", \
    "MAIN_STORAGECONTROLLER", \
    "MAIN_STRINGARRAY", \
    "MAIN_STRINGFORMVALUE", \
    "MAIN_SYSTEMPROPERTIES", \
    "MAIN_THREAD_TASK", \
    "MAIN_TOKEN", \
    "MAIN_TRUSTEDPLATFORMMODULE", \
    "MAIN_UEFIVARIABLESTORE", \
    "MAIN_UNATTENDED", \
    "MAIN_UPDATEAGENT", \
    "MAIN_UPDATEAGENTAVAILABLEEVENT", \
    "MAIN_UPDATEAGENTERROREVENT", \
    "MAIN_UPDATEAGENTEVENT", \
    "MAIN_UPDATEAGENTSETTINGSCHANGEDEVENT", \
    "MAIN_UPDATEAGENTSTATECHANGEDEVENT", \
    "MAIN_USBCONTROLLER", \
    "MAIN_USBDEVICE", \
    "MAIN_USBDEVICEFILTER", \
    "MAIN_USBDEVICEFILTERS", \
    "MAIN_USBPROXYBACKEND", \
    "MAIN_VBOXSVC", \
    "MAIN_VETOEVENT", \
    "MAIN_VFSEXPLORER", \
    "MAIN_VIRTUALBOX", \
    "MAIN_VIRTUALBOXCLIENT", \
    "MAIN_VIRTUALBOXSDS", \
    "MAIN_VIRTUALSYSTEMDESCRIPTION", \
    "MAIN_VIRTUALSYSTEMDESCRIPTIONFORM", \
    "MAIN_VMMDEVINTERFACES", \
    "MAIN_VRDESERVER", \
    "MAIN_VRDESERVERINFO", \
    "MISC", \
    "MM", \
    "MM_HEAP", \
    "MM_HYPER", \
    "MM_HYPER_HEAP", \
    "MM_PHYS", \
    "MM_POOL", \
    "NAT_SERVICE", \
    "NEM", \
    "NET_ADP_DRV", \
    "NET_DHCPD",   \
    "NET_FLT_DRV", \
    "NET_SERVICE", \
    "NET_SHAPER", \
    "PATM", \
    "PDM", \
    "PDM_ASYNC_COMPLETION", \
    "PDM_BLK_CACHE", \
    "PDM_CRITSECT", \
    "PDM_CRITSECTRW", \
    "PDM_DEVICE", \
    "PDM_DRIVER", \
    "PDM_LDR", \
    "PDM_QUEUE", \
    "PDM_TASK", \
    "PDM_THREAD", \
    "PGM", \
    "PGM_DYNMAP", \
    "PGM_PHYS", \
    "PGM_PHYS_ACCESS", \
    "PGM_POOL", \
    "PGM_SHARED", \
    "RECORDING", \
    "REM", \
    "REM_DISAS", \
    "REM_HANDLER", \
    "REM_IOPORT", \
    "REM_MMIO", \
    "REM_PRINTF", \
    "REM_RUN", \
    "SELM", \
    "SHARED_CLIPBOARD", \
    "SHARED_CROPENGL", \
    "SHARED_FOLDERS", \
    "SHARED_OPENGL", \
    "SRV_INTNET", \
    "SSM", \
    "STAM", \
    "SUP", \
    "SUP_DRV", \
    "TM", \
    "TRPM", \
    "USB_CARDREADER", \
    "USB_DRV", \
    "USB_FILTER", \
    "USB_KBD", \
    "USB_MOUSE", \
    "USB_MSD", \
    "USB_REMOTE", \
    "USB_WEBCAM", \
    "VBGL", \
    "VD", \
    "VD_CUE", \
    "VD_DMG", \
    "VD_ISCSI", \
    "VD_PARALLELS", \
    "VD_QCOW", \
    "VD_QED", \
    "VD_RAW", \
    "VD_VDI", \
    "VD_VHD", \
    "VD_VHDX", \
    "VD_VMDK", \
    "VGDRV", \
    "VM", \
    "VMM", \
    "VRDE", \
    "VRDP", \
    "VSCSI", \
    "WEBSERVICE", \
}

/** @} */
#endif /* !VBOX_INCLUDED_log_h */
