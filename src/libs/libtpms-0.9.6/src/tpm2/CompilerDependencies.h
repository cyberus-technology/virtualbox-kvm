/********************************************************************************/
/*										*/
/*			   Compiler Dependencies  				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CompilerDependencies.h $	*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2016 - 2019				*/
/*										*/
/********************************************************************************/

#ifndef COMPILERDEPEDENCIES_H
#define COMPILERDEPEDENCIES_H

/* kgold - Not in the original code.  A user reported that it was required for a non-Visual Studio
   environment.
*/

#ifdef TPM_WINDOWS
#include <windows.h>
#include <winsock.h>
#endif

/* 5.10	CompilerDependencies.h */
#ifdef GCC
#   undef _MSC_VER
#   undef WIN32
#endif

#ifdef _MSC_VER

// These definitions are for the Microsoft compiler Endian conversion for aligned structures
#   define REVERSE_ENDIAN_16(_Number) _byteswap_ushort(_Number)
#   define REVERSE_ENDIAN_32(_Number) _byteswap_ulong(_Number)
#   define REVERSE_ENDIAN_64(_Number) _byteswap_uint64(_Number)

// Avoid compiler warning for in line of stdio (or not)

// #define _NO_CRT_STDIO_INLINE

// This macro is used to handle LIB_EXPORT of function and variable names in lieu of a .def
// file. Visual Studio requires that functions be explicitly exported and imported.

#   define LIB_EXPORT __declspec(dllexport) // VS compatible version
#   define LIB_IMPORT __declspec(dllimport)

// This is defined to indicate a function that does not return. Microsoft compilers do not
// support the _Noretrun() function parameter.

#   define NORETURN  __declspec(noreturn)
#   if _MSC_VER >= 1400     // SAL processing when needed
#       include <sal.h>
#   endif
#   ifdef _WIN64
#       define _INTPTR 2
#   else
#       define _INTPTR 1
#   endif
#   define NOT_REFERENCED(x)   (x)

// Lower the compiler error warning for system include files. They tend not to be that clean and
// there is no reason to sort through all the spurious errors that they generate when the normal
// error level is set to /Wall

#   define _REDUCE_WARNING_LEVEL_(n)		\
    __pragma(warning(push, n))

// Restore the compiler warning level

#   define _NORMAL_WARNING_LEVEL_		\
    __pragma(warning(pop))
#   include <stdint.h>
#endif 	// _MSC_VER

#ifndef _MSC_VER
#ifndef WINAPI
#   define WINAPI
#endif
#   define __pragma(x)
    /* libtpms added begin */
#   if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR >= 2)
#     define REVERSE_ENDIAN_16(_Number) __builtin_bswap16(_Number)
#     define REVERSE_ENDIAN_32(_Number) __builtin_bswap32(_Number)
#     define REVERSE_ENDIAN_64(_Number) __builtin_bswap64(_Number)
#   else
#     if defined __linux__ || defined __CYGWIN__
#       include <byteswap.h>
#       define REVERSE_ENDIAN_16(_Number) bswap_16(_Number)
#       define REVERSE_ENDIAN_32(_Number) bswap_32(_Number)
#       define REVERSE_ENDIAN_64(_Number) bswap_64(_Number)
#     elif defined __OpenBSD__
#       include <endian.h>
#       define REVERSE_ENDIAN_16(_Number) swap16(_Number)
#       define REVERSE_ENDIAN_32(_Number) swap32(_Number)
#       define REVERSE_ENDIAN_64(_Number) swap64(_Number)
#     elif defined __APPLE__
#       include <libkern/OSByteOrder.h>
#       define REVERSE_ENDIAN_16(_Number) _OSSwapInt16(_Number)
#       define REVERSE_ENDIAN_32(_Number) _OSSwapInt32(_Number)
#       define REVERSE_ENDIAN_64(_Number) _OSSwapInt64(_Number)
#     elif defined __FreeBSD__
#       include <sys/endian.h>
#       define REVERSE_ENDIAN_16(_Number) bswap16(_Number)
#       define REVERSE_ENDIAN_32(_Number) bswap32(_Number)
#       define REVERSE_ENDIAN_64(_Number) bswap64(_Number)
#     else
#       error Unsupported OS
#     endif
#   endif
    /* libtpms added end */
#endif
#if defined(__GNUC__)
#      define NORETURN                     __attribute__((noreturn))
#      include <stdint.h>
#endif

// Things that are not defined should be defined as NULL
#ifndef NORETURN
#   define NORETURN
#endif
#ifndef LIB_EXPORT
#   define LIB_EXPORT
#endif
#ifndef LIB_IMPORT
#   define LIB_IMPORT
#endif
#ifndef _REDUCE_WARNING_LEVEL_
#   define _REDUCE_WARNING_LEVEL_(n)
#endif
#ifndef _NORMAL_WARNING_LEVEL_
#   define _NORMAL_WARNING_LEVEL_
#endif
#ifndef NOT_REFERENCED
#   define  NOT_REFERENCED(x) (x = x)
#endif
#ifdef _POSIX_
typedef int SOCKET;
#endif
// #ifdef TPM_POSIX
// typedef int SOCKET;
// #endif
#endif // _COMPILER_DEPENDENCIES_H_

