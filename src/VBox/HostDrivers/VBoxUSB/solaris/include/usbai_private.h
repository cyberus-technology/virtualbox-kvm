/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009-2010 Oracle Corporation. All rights reserved.
 * Use is subject to license terms.
 */

#ifndef    _SYS_USB_USBA_USBAI_PRIVATE_H
#define    _SYS_USB_USBA_USBAI_PRIVATE_H

/*
 * Unstable interfaces not part of USBAI but used by Solaris client drivers.
 * These interfaces may not be present in future releases and are highly
 * unstable.
 *
 * Status key:
 *    C = Remove from Sun client drivers before removing from this file
 *    D = May be needed by legacy (DDK) drivers.
 */

#ifdef    __cplusplus
extern "C" {
#endif

/*
 * convenience function for getting default config index
 * as saved in usba_device structure
 *
 * Status: C
 */
uint_t usb_get_current_cfgidx(dev_info_t *);

/*
 * Usb logging, debug and console message handling.
 */
typedef struct usb_log_handle *usb_log_handle_t;

#define    USB_LOG_L0    0    /* warnings, console & syslog buffer */
#define    USB_LOG_L1    1    /* errors, syslog buffer */
#define    USB_LOG_L2    2    /* recoverable errors, debug only */
#define    USB_LOG_L3    3    /* interesting data, debug only */
#define    USB_LOG_L4    4    /* tracing, debug only */

#define    USB_CHK_BASIC  0             /* Empty mask.    Basics always done. */
#define    USB_CHK_SERIAL 0x00000001    /* Compare device serial numbers. */
#define    USB_CHK_CFG    0x00000002    /* Compare raw config clouds. */
#define    USB_CHK_VIDPID 0x00000004    /* Compare product and vendor ID. */
#define    USB_CHK_ALL    0xFFFFFFFF    /* Perform maximum checking. */

int usb_check_same_device(dev_info_t        *dip,
                          usb_log_handle_t   log_handle,
                          int                log_level,
                          int                log_mask,
                          uint_t             check_mask,
                          char              *device_string);

/*
 * **************************************************************************
 * Serialization functions remaining Contracted Consolidation Private
 * **************************************************************************
 */

/* This whole section: status: C and D. */

/*
 * opaque serialization handle.
 *    Used by all usb_serialization routines.
 *
 *    This handle is opaque to the client driver.
 */
typedef struct usb_serialization *usb_serialization_t;

/*
 * usb_init_serialization
 *    setup for serialization
 *
 * ARGUMENTS:
 *    s_dip        - devinfo pointer
 *    flag         - USB_INIT_SER_CHECK_SAME_THREAD
 *                   when set, usb_release_access() will verify that the same
 *                   thread releases access. If not, a console warning will
 *                   be issued but access will be released anyways.
 *
 * RETURNS:
 *    usb_serialization handle
 *
 */
usb_serialization_t usb_init_serialization(dev_info_t    *s_dip,
                                           uint_t         flag);

#define    USB_INIT_SER_CHECK_SAME_THREAD    1

/* fini for serialization */
void usb_fini_serialization(usb_serialization_t usb_serp);

/*
 * Various ways of calling usb_serialize_access. These correspond to
 * their cv_*wait* function counterparts for usb_serialize_access.
 */
#define USB_WAIT            0
#define USB_WAIT_SIG        1
#define USB_TIMEDWAIT       2
#define USB_TIMEDWAIT_SIG   3

/*
 * usb_serialize_access:
 *    acquire serialized access
 *
 * ARGUMENTS:
 *    usb_serp      - usb_serialization handle
 *    how_to_wait   - Which cv_*wait* function to wait for condition.
 *       USB_WAIT:           use cv_wait
 *       USB_WAIT_SIG:       use cv_wait_sig
 *       USB_TIMEDWAIT:      use cv_timedwait
 *       USB_TIMEDWAIT_SIG:  use cv_timedwait_sig
 *    delta_timeout - Time in ms from current time to timeout.  Checked
 *                    only if USB_TIMEDWAIT or USB_TIMEDWAIT_SIG
 *                    specified in how_to_wait.
 * RETURNS:
 *    Same as values returned by cv_*wait* functions,
 *    except for when how_to_wait == USB_WAIT, where 0 is always returned.
 *    For calls where a timeout or signal could be expected, use this value
 *    to tell whether a kill(2) signal or timeout occurred.
 */
int usb_serialize_access(usb_serialization_t    usb_serp,
                         uint_t                 how_to_wait,
                         uint_t                 delta_timeout);

/*
 * usb_release_access:
 *    release serialized access
 *
 * ARGUMENTS:
 *    usb_serp    - usb_serialization handle
 */
void usb_release_access(usb_serialization_t usb_serp);

#ifdef __cplusplus
}
#endif

#endif    /* _SYS_USB_USBA_USBAI_PRIVATE_H */

