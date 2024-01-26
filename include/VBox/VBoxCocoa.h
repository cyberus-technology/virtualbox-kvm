/** @file
 * VBoxCocoa Helper
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_VBoxCocoa_h
#define VBOX_INCLUDED_VBoxCocoa_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** Macro which add a typedef of the given Cocoa class in an appropriate form
 * for the current context. This means void* in the C/CPP context and
 * NSWhatever* in the ObjC/ObjCPP context. Use
 * NativeNSWhateverRef/ConstNativeNSWhateverRef when you reference the Cocoa
 * type somewhere. The use of this prevents extensive casting of void* to the
 * right type in the Cocoa context. */
#ifdef __OBJC__
# define ADD_COCOA_NATIVE_REF(CocoaClass) \
    @class CocoaClass; \
    typedef CocoaClass *Native##CocoaClass##Ref; \
    typedef const CocoaClass *ConstNative##CocoaClass##Ref
#else  /* !__OBJC__ */
# define ADD_COCOA_NATIVE_REF(CocoaClass) \
    typedef void *Native##CocoaClass##Ref; \
    typedef const void *ConstNative##CocoaClass##Ref
#endif /* !__OBJC__ */


/*
 * Objective-C++ Helpers.
 */
#if defined(__OBJC__) && defined (__cplusplus)

/* Global includes */
# import <Foundation/NSAutoreleasePool.h>

/** Helper class for automatic creation & destroying of a cocoa auto release
 *  pool. */
class CocoaAutoreleasePool
{
public:
    inline CocoaAutoreleasePool()
    {
         mPool = [[NSAutoreleasePool alloc] init];
    }
    inline ~CocoaAutoreleasePool()
    {
        [mPool release];
    }

private:
    NSAutoreleasePool *mPool;
};

#endif /* __OBJC__ && __cplusplus */

#endif /* !VBOX_INCLUDED_VBoxCocoa_h */

