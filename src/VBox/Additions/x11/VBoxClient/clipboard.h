/** $Id: clipboard.h $ */
/** @file
 * Guest Additions - X11 Shared Clipboard - Main header.
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef GA_INCLUDED_SRC_x11_VBoxClient_clipboard_h
#define GA_INCLUDED_SRC_x11_VBoxClient_clipboard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/**
 * Struct keeping a Shared Clipboard context.
 */
struct SHCLCONTEXT
{
    /** Client command context */
    VBGLR3SHCLCMDCTX CmdCtx;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    /** Associated transfer data. */
    SHCLTRANSFERCTX  TransferCtx;
#endif
    /** X11 clipboard context. */
    SHCLX11CTX       X11;
};

#endif /* !GA_INCLUDED_SRC_x11_VBoxClient_clipboard_h */
