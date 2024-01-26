/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * Copyright (c) 1994-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifndef _COMPILER_H

# define _COMPILER_H

#if defined(__SUNPRO_C)
# define DO_PROTOTYPES
#endif

/* Allow drivers to use the GCC-supported __inline__ and/or __inline. */
# ifndef __inline__
#  if defined(__GNUC__)
    /* gcc has __inline__ */
#  elif defined(__HIGHC__)
#   define __inline__ _Inline
#  else
#   define __inline__ /**/
#  endif
# endif /* __inline__ */
# ifndef __inline
#  if defined(__GNUC__)
    /* gcc has __inline */
#  elif defined(__HIGHC__)
#   define __inline _Inline
#  else
#   define __inline /**/
#  endif
# endif /* __inline */

# if defined(IODEBUG) && defined(__GNUC__)
#  define outb RealOutb
#  define outw RealOutw
#  define outl RealOutl
#  define inb RealInb
#  define inw RealInw
#  define inl RealInl
# endif

# if defined(QNX4) /* Do this for now to keep Watcom happy */
#  define outb outp
#  define outw outpw
#  define outl outpd 
#  define inb inp
#  define inw inpw
#  define inl inpd

/* Define the ffs function for inlining */
extern int ffs(unsigned long);
#  pragma aux ffs_ = \
        "bsf edx, eax"          \
        "jnz bits_set"          \
        "xor eax, eax"          \
        "jmp exit1"             \
        "bits_set:"             \
        "mov eax, edx"          \
        "inc eax"               \
        "exit1:"                \
        __parm [eax]            \
        __modify [eax edx]      \
        __value [eax]           \
        ;
# endif

# if defined(__SUNPRO_C)
#  define DO_PROTOTYPES
# endif

# if defined(NO_INLINE) || defined(DO_PROTOTYPES)

#  if !defined(__arm__)
#   if !defined(__sparc__) && !defined(__sparc) && !defined(__arm32__) \
      && !(defined(__alpha__) && defined(linux)) \
      && !(defined(__ia64__) && defined(linux)) \

extern void outb(unsigned short, unsigned char);
extern void outw(unsigned short, unsigned short);
extern void outl(unsigned short, unsigned int);
extern unsigned int inb(unsigned short);
extern unsigned int inw(unsigned short);
extern unsigned int inl(unsigned short);

#   else /* __sparc__,  __arm32__, __alpha__*/

extern void outb(unsigned long, unsigned char);
extern void outw(unsigned long, unsigned short);
extern void outl(unsigned long, unsigned int);
extern unsigned int inb(unsigned long);
extern unsigned int inw(unsigned long);
extern unsigned int inl(unsigned long);

#   endif /* __sparc__,  __arm32__, __alpha__ */
#  endif /* __arm__ */

extern unsigned long ldq_u(unsigned long *);
extern unsigned long ldl_u(unsigned int *);
extern unsigned long ldw_u(unsigned short *);
extern void stq_u(unsigned long, unsigned long *);
extern void stl_u(unsigned long, unsigned int *);
extern void stw_u(unsigned long, unsigned short *);
extern void mem_barrier(void);
extern void write_mem_barrier(void);
extern void stl_brx(unsigned long, volatile unsigned char *, int);
extern void stw_brx(unsigned short, volatile unsigned char *, int);
extern unsigned long ldl_brx(volatile unsigned char *, int);
extern unsigned short ldw_brx(volatile unsigned char *, int);

# endif

# ifndef NO_INLINE
#  ifdef __GNUC__
#   if (defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && (defined(__alpha__))

#    ifdef linux
/* for Linux on Alpha, we use the LIBC _inx/_outx routines */
/* note that the appropriate setup via "ioperm" needs to be done */
/*  *before* any inx/outx is done. */

extern void (*_alpha_outb)(char val, unsigned long port);
static __inline__ void
outb(unsigned long port, unsigned char val)
{
    _alpha_outb(val, port);
}

extern void (*_alpha_outw)(short val, unsigned long port);
static __inline__ void
outw(unsigned long port, unsigned short val)
{
    _alpha_outw(val, port);
}

extern void (*_alpha_outl)(int val, unsigned long port);
static __inline__ void
outl(unsigned long port, unsigned int val)
{
    _alpha_outl(val, port);
}

extern unsigned int (*_alpha_inb)(unsigned long port);
static __inline__ unsigned int
inb(unsigned long port)
{
  return _alpha_inb(port);
}

extern unsigned int (*_alpha_inw)(unsigned long port);
static __inline__ unsigned int
inw(unsigned long port)
{
  return _alpha_inw(port);
}

extern unsigned int (*_alpha_inl)(unsigned long port);
static __inline__ unsigned int
inl(unsigned long port)
{
  return _alpha_inl(port);
}

#    endif /* linux */

#    if (defined(__FreeBSD__) || defined(__OpenBSD__)) \
      && !defined(DO_PROTOTYPES)

/* for FreeBSD and OpenBSD on Alpha, we use the libio (resp. libalpha) */
/*  inx/outx routines */
/* note that the appropriate setup via "ioperm" needs to be done */
/*  *before* any inx/outx is done. */

extern void outb(unsigned int port, unsigned char val);
extern void outw(unsigned int port, unsigned short val);
extern void outl(unsigned int port, unsigned int val);
extern unsigned char inb(unsigned int port);
extern unsigned short inw(unsigned int port);
extern unsigned int inl(unsigned int port);

#    endif /* (__FreeBSD__ || __OpenBSD__ ) && !DO_PROTOTYPES */


#if defined(__NetBSD__)
#include <machine/pio.h>
#endif /* __NetBSD__ */

/*
 * inline functions to do unaligned accesses
 * from linux/include/asm-alpha/unaligned.h
 */

/*
 * EGCS 1.1 knows about arbitrary unaligned loads.  Define some
 * packed structures to talk about such things with.
 */

struct __una_u64 { unsigned long  x __attribute__((packed)); };
struct __una_u32 { unsigned int   x __attribute__((packed)); };
struct __una_u16 { unsigned short x __attribute__((packed)); };

/*
 * Elemental unaligned loads 
 */
/* let's try making these things static */

static __inline__ unsigned long ldq_u(unsigned long * r11)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	const struct __una_u64 *ptr = (const struct __una_u64 *) r11;
	return ptr->x;
#    else
	unsigned long r1,r2;
	__asm__("ldq_u %0,%3\n\t"
		"ldq_u %1,%4\n\t"
		"extql %0,%2,%0\n\t"
		"extqh %1,%2,%1"
		:"=&r" (r1), "=&r" (r2)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(const unsigned long *)(7+(char *) r11)));
	return r1 | r2;
#    endif
}

static __inline__ unsigned long ldl_u(unsigned int * r11)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	const struct __una_u32 *ptr = (const struct __una_u32 *) r11;
	return ptr->x;
#    else
	unsigned long r1,r2;
	__asm__("ldq_u %0,%3\n\t"
		"ldq_u %1,%4\n\t"
		"extll %0,%2,%0\n\t"
		"extlh %1,%2,%1"
		:"=&r" (r1), "=&r" (r2)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(const unsigned long *)(3+(char *) r11)));
	return r1 | r2;
#    endif
}

static __inline__ unsigned long ldw_u(unsigned short * r11)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	const struct __una_u16 *ptr = (const struct __una_u16 *) r11;
	return ptr->x;
#    else
	unsigned long r1,r2;
	__asm__("ldq_u %0,%3\n\t"
		"ldq_u %1,%4\n\t"
		"extwl %0,%2,%0\n\t"
		"extwh %1,%2,%1"
		:"=&r" (r1), "=&r" (r2)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(const unsigned long *)(1+(char *) r11)));
	return r1 | r2;
#    endif
}

/*
 * Elemental unaligned stores 
 */

static __inline__ void stq_u(unsigned long r5, unsigned long * r11)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	struct __una_u64 *ptr = (struct __una_u64 *) r11;
	ptr->x = r5;
#    else
	unsigned long r1,r2,r3,r4;

	__asm__("ldq_u %3,%1\n\t"
		"ldq_u %2,%0\n\t"
		"insqh %6,%7,%5\n\t"
		"insql %6,%7,%4\n\t"
		"mskqh %3,%7,%3\n\t"
		"mskql %2,%7,%2\n\t"
		"bis %3,%5,%3\n\t"
		"bis %2,%4,%2\n\t"
		"stq_u %3,%1\n\t"
		"stq_u %2,%0"
		:"=m" (*r11),
		 "=m" (*(unsigned long *)(7+(char *) r11)),
		 "=&r" (r1), "=&r" (r2), "=&r" (r3), "=&r" (r4)
		:"r" (r5), "r" (r11));
#    endif
}

static __inline__ void stl_u(unsigned long r5, unsigned int * r11)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	struct __una_u32 *ptr = (struct __una_u32 *) r11;
	ptr->x = r5;
#    else
	unsigned long r1,r2,r3,r4;

	__asm__("ldq_u %3,%1\n\t"
		"ldq_u %2,%0\n\t"
		"inslh %6,%7,%5\n\t"
		"insll %6,%7,%4\n\t"
		"msklh %3,%7,%3\n\t"
		"mskll %2,%7,%2\n\t"
		"bis %3,%5,%3\n\t"
		"bis %2,%4,%2\n\t"
		"stq_u %3,%1\n\t"
		"stq_u %2,%0"
		:"=m" (*r11),
		 "=m" (*(unsigned long *)(3+(char *) r11)),
		 "=&r" (r1), "=&r" (r2), "=&r" (r3), "=&r" (r4)
		:"r" (r5), "r" (r11));
#    endif
}

static __inline__ void stw_u(unsigned long r5, unsigned short * r11)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	struct __una_u16 *ptr = (struct __una_u16 *) r11;
	ptr->x = r5;
#    else
	unsigned long r1,r2,r3,r4;

	__asm__("ldq_u %3,%1\n\t"
		"ldq_u %2,%0\n\t"
		"inswh %6,%7,%5\n\t"
		"inswl %6,%7,%4\n\t"
		"mskwh %3,%7,%3\n\t"
		"mskwl %2,%7,%2\n\t"
		"bis %3,%5,%3\n\t"
		"bis %2,%4,%2\n\t"
		"stq_u %3,%1\n\t"
		"stq_u %2,%0"
		:"=m" (*r11),
		 "=m" (*(unsigned long *)(1+(char *) r11)),
		 "=&r" (r1), "=&r" (r2), "=&r" (r3), "=&r" (r4)
		:"r" (r5), "r" (r11));
#    endif
}

/* to flush the I-cache before jumping to code which just got loaded */
#    define PAL_imb 134
#    define istream_mem_barrier() \
	__asm__ __volatile__("call_pal %0 #imb" : : "i" (PAL_imb) : "memory")
#    define mem_barrier()        __asm__ __volatile__("mb"  : : : "memory")
#    ifdef __ELF__
#     define write_mem_barrier()  __asm__ __volatile__("wmb" : : : "memory")
#    else  /*  ECOFF gas 2.6 doesn't know "wmb" :-(  */
#     define write_mem_barrier()  mem_barrier()
#    endif


#   elif defined(linux) && defined(__ia64__) 
 
#    include <inttypes.h>

#    include <sys/io.h>

struct __una_u64 { uint64_t x __attribute__((packed)); };
struct __una_u32 { uint32_t x __attribute__((packed)); };
struct __una_u16 { uint16_t x __attribute__((packed)); };

static __inline__ unsigned long
__uldq (const unsigned long * r11)
{
	const struct __una_u64 *ptr = (const struct __una_u64 *) r11;
	return ptr->x;
}

static __inline__ unsigned long
__uldl (const unsigned int * r11)
{
	const struct __una_u32 *ptr = (const struct __una_u32 *) r11;
	return ptr->x;
}

static __inline__ unsigned long
__uldw (const unsigned short * r11)
{
	const struct __una_u16 *ptr = (const struct __una_u16 *) r11;
	return ptr->x;
}

static __inline__ void
__ustq (unsigned long r5, unsigned long * r11)
{
	struct __una_u64 *ptr = (struct __una_u64 *) r11;
	ptr->x = r5;
}

static __inline__ void
__ustl (unsigned long r5, unsigned int * r11)
{
	struct __una_u32 *ptr = (struct __una_u32 *) r11;
	ptr->x = r5;
}

static __inline__ void
__ustw (unsigned long r5, unsigned short * r11)
{
	struct __una_u16 *ptr = (struct __una_u16 *) r11;
	ptr->x = r5;
}

#    define ldq_u(p)	__uldq(p)
#    define ldl_u(p)	__uldl(p)
#    define ldw_u(p)	__uldw(p) 
#    define stq_u(v,p)	__ustq(v,p)
#    define stl_u(v,p)	__ustl(v,p)
#    define stw_u(v,p)	__ustw(v,p)

#    ifndef __INTEL_COMPILER  
#      define mem_barrier()        __asm__ __volatile__ ("mf" ::: "memory")
#      define write_mem_barrier()  __asm__ __volatile__ ("mf" ::: "memory")
#    else
#      include "ia64intrin.h"
#      define mem_barrier() __mf()
#      define write_mem_barrier() __mf()
#    endif

/*
 * This is overkill, but for different reasons depending on where it is used.
 * This is thus general enough to be used everywhere cache flushes are needed.
 * It doesn't handle memory access serialisation by other processors, though.
 */
#    ifndef __INTEL_COMPILER
#       define ia64_flush_cache(Addr) \
	__asm__ __volatile__ ( \
		"fc.i %0;;;" \
		"sync.i;;;" \
		"mf;;;" \
		"srlz.i;;;" \
		:: "r"(Addr) : "memory")
#    else
#      define ia64_flush_cache(Addr) { \
        __fc(Addr);\
        __synci();\
        __mf();\
        __isrlz();\
       }
#    endif
#    undef outb
#    undef outw
#    undef outl
#    undef inb
#    undef inw
#    undef inl
extern void outb(unsigned long port, unsigned char val);
extern void outw(unsigned long port, unsigned short val);
extern void outl(unsigned long port, unsigned int val);
extern unsigned int inb(unsigned long port);
extern unsigned int inw(unsigned long port);
extern unsigned int inl(unsigned long port);
 
#   elif defined(linux) && defined(__amd64__) 
 
#    include <inttypes.h>

#    define ldq_u(p)	(*((unsigned long  *)(p)))
#    define ldl_u(p)	(*((unsigned int   *)(p)))
#    define ldw_u(p)	(*((unsigned short *)(p)))
#    define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#    define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#    define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
  
#    define mem_barrier() \
       __asm__ __volatile__ ("lock; addl $0,0(%%rsp)": : :"memory")
#    define write_mem_barrier() \
       __asm__ __volatile__ ("": : :"memory")


static __inline__ void
outb(unsigned short port, unsigned char val)
{
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
}


static __inline__ void
outw(unsigned short port, unsigned short val)
{
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(unsigned short port)
{
   unsigned char ret;
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inw(unsigned short port)
{
   unsigned short ret;
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inl(unsigned short port)
{
   unsigned int ret;
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

#   elif (defined(linux) || defined(Lynx) || defined(sun) || defined(__OpenBSD__) || defined(__FreeBSD__)) && defined(__sparc__)

#    if !defined(Lynx)
#     ifndef ASI_PL
#      define ASI_PL 0x88
#     endif

#     define barrier() __asm__ __volatile__(".word 0x8143e00a": : :"memory")

static __inline__ void
outb(unsigned long port, unsigned char val)
{
	__asm__ __volatile__("stba %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (port), "i" (ASI_PL));
	barrier();
}

static __inline__ void
outw(unsigned long port, unsigned short val)
{
	__asm__ __volatile__("stha %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (port), "i" (ASI_PL));
	barrier();
}

static __inline__ void
outl(unsigned long port, unsigned int val)
{
	__asm__ __volatile__("sta %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (port), "i" (ASI_PL));
	barrier();
}

static __inline__ unsigned int
inb(unsigned long port)
{
	unsigned int ret;
	__asm__ __volatile__("lduba [%1] %2, %0"
			     : "=r" (ret)
			     : "r" (port), "i" (ASI_PL));
	return ret;
}

static __inline__ unsigned int
inw(unsigned long port)
{
	unsigned int ret;
	__asm__ __volatile__("lduha [%1] %2, %0"
			     : "=r" (ret)
			     : "r" (port), "i" (ASI_PL));
	return ret;
}

static __inline__ unsigned int
inl(unsigned long port)
{
	unsigned int ret;
	__asm__ __volatile__("lda [%1] %2, %0"
			     : "=r" (ret)
			     : "r" (port), "i" (ASI_PL));
	return ret;
}

static __inline__ unsigned char
xf86ReadMmio8(__volatile__ void *base, const unsigned long offset)
{
	unsigned long addr = ((unsigned long)base) + offset;
	unsigned char ret;

	__asm__ __volatile__("lduba [%1] %2, %0"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_PL));
	return ret;
}

static __inline__ unsigned short
xf86ReadMmio16Be(__volatile__ void *base, const unsigned long offset)
{
	unsigned long addr = ((unsigned long)base) + offset;
	unsigned short ret;

	__asm__ __volatile__("lduh [%1], %0"
			     : "=r" (ret)
			     : "r" (addr));
	return ret;
}

static __inline__ unsigned short
xf86ReadMmio16Le(__volatile__ void *base, const unsigned long offset)
{
	unsigned long addr = ((unsigned long)base) + offset;
	unsigned short ret;

	__asm__ __volatile__("lduha [%1] %2, %0"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_PL));
	return ret;
}

static __inline__ unsigned int
xf86ReadMmio32Be(__volatile__ void *base, const unsigned long offset)
{
	unsigned long addr = ((unsigned long)base) + offset;
	unsigned int ret;

	__asm__ __volatile__("ld [%1], %0"
			     : "=r" (ret)
			     : "r" (addr));
	return ret;
}

static __inline__ unsigned int
xf86ReadMmio32Le(__volatile__ void *base, const unsigned long offset)
{
	unsigned long addr = ((unsigned long)base) + offset;
	unsigned int ret;

	__asm__ __volatile__("lda [%1] %2, %0"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_PL));
	return ret;
}

static __inline__ void
xf86WriteMmio8(__volatile__ void *base, const unsigned long offset,
	       const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("stba %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (addr), "i" (ASI_PL));
	barrier();
}

static __inline__ void
xf86WriteMmio16Be(__volatile__ void *base, const unsigned long offset,
		  const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("sth %0, [%1]"
			     : /* No outputs */
			     : "r" (val), "r" (addr));
	barrier();
}

static __inline__ void
xf86WriteMmio16Le(__volatile__ void *base, const unsigned long offset,
		  const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("stha %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (addr), "i" (ASI_PL));
	barrier();
}

static __inline__ void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
		  const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("st %0, [%1]"
			     : /* No outputs */
			     : "r" (val), "r" (addr));
	barrier();
}

static __inline__ void
xf86WriteMmio32Le(__volatile__ void *base, const unsigned long offset,
		  const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("sta %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (addr), "i" (ASI_PL));
	barrier();
}

static __inline__ void
xf86WriteMmio8NB(__volatile__ void *base, const unsigned long offset,
		 const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("stba %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (addr), "i" (ASI_PL));
}

static __inline__ void
xf86WriteMmio16BeNB(__volatile__ void *base, const unsigned long offset,
		    const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("sth %0, [%1]"
			     : /* No outputs */
			     : "r" (val), "r" (addr));
}

static __inline__ void
xf86WriteMmio16LeNB(__volatile__ void *base, const unsigned long offset,
		    const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("stha %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (addr), "i" (ASI_PL));
}

static __inline__ void
xf86WriteMmio32BeNB(__volatile__ void *base, const unsigned long offset,
		    const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("st %0, [%1]"
			     : /* No outputs */
			     : "r" (val), "r" (addr));
}

static __inline__ void
xf86WriteMmio32LeNB(__volatile__ void *base, const unsigned long offset,
		    const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("sta %0, [%1] %2"
			     : /* No outputs */
			     : "r" (val), "r" (addr), "i" (ASI_PL));
}

#    endif	/* !Lynx */

/*
 * EGCS 1.1 knows about arbitrary unaligned loads.  Define some
 * packed structures to talk about such things with.
 */

#    if defined(__arch64__) || defined(__sparcv9)
struct __una_u64 { unsigned long  x __attribute__((packed)); };
#    endif
struct __una_u32 { unsigned int   x __attribute__((packed)); };
struct __una_u16 { unsigned short x __attribute__((packed)); };

static __inline__ unsigned long ldq_u(unsigned long *p)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
#     if defined(__arch64__) || defined(__sparcv9)
	const struct __una_u64 *ptr = (const struct __una_u64 *) p;
#     else
	const struct __una_u32 *ptr = (const struct __una_u32 *) p;
#     endif
	return ptr->x;
#    else
	unsigned long ret;
	memmove(&ret, p, sizeof(*p));
	return ret;
#    endif
}

static __inline__ unsigned long ldl_u(unsigned int *p)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	const struct __una_u32 *ptr = (const struct __una_u32 *) p;
	return ptr->x;
#    else
	unsigned int ret;
	memmove(&ret, p, sizeof(*p));
	return ret;
#    endif
}

static __inline__ unsigned long ldw_u(unsigned short *p)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	const struct __una_u16 *ptr = (const struct __una_u16 *) p;
	return ptr->x;
#    else
	unsigned short ret;
	memmove(&ret, p, sizeof(*p));
	return ret;
#    endif
}

static __inline__ void stq_u(unsigned long val, unsigned long *p)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
#     if defined(__arch64__) || defined(__sparcv9)
	struct __una_u64 *ptr = (struct __una_u64 *) p;
#     else
	struct __una_u32 *ptr = (struct __una_u32 *) p;
#     endif
	ptr->x = val;
#    else
	unsigned long tmp = val;
	memmove(p, &tmp, sizeof(*p));
#    endif
}

static __inline__ void stl_u(unsigned long val, unsigned int *p)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	struct __una_u32 *ptr = (struct __una_u32 *) p;
	ptr->x = val;
#    else
	unsigned int tmp = val;
	memmove(p, &tmp, sizeof(*p));
#    endif
}

static __inline__ void stw_u(unsigned long val, unsigned short *p)
{
#    if defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC_MINOR__ >= 91))
	struct __una_u16 *ptr = (struct __una_u16 *) p;
	ptr->x = val;
#    else
	unsigned short tmp = val;
	memmove(p, &tmp, sizeof(*p));
#    endif
}

#    define mem_barrier()         /* XXX: nop for now */
#    define write_mem_barrier()   /* XXX: nop for now */

#   elif defined(__mips__) || (defined(__arm32__) && !defined(__linux__))
#    ifdef __arm32__
#     define PORT_SIZE long
#    else
#     define PORT_SIZE short
#    endif

unsigned int IOPortBase;  /* Memory mapped I/O port area */

static __inline__ void
outb(unsigned PORT_SIZE port, unsigned char val)
{
	*(volatile unsigned char*)(((unsigned PORT_SIZE)(port))+IOPortBase) = val;
}

static __inline__ void
outw(unsigned PORT_SIZE port, unsigned short val)
{
	*(volatile unsigned short*)(((unsigned PORT_SIZE)(port))+IOPortBase) = val;
}

static __inline__ void
outl(unsigned PORT_SIZE port, unsigned int val)
{
	*(volatile unsigned int*)(((unsigned PORT_SIZE)(port))+IOPortBase) = val;
}

static __inline__ unsigned int
inb(unsigned PORT_SIZE port)
{
	return *(volatile unsigned char*)(((unsigned PORT_SIZE)(port))+IOPortBase);
}

static __inline__ unsigned int
inw(unsigned PORT_SIZE port)
{
	return *(volatile unsigned short*)(((unsigned PORT_SIZE)(port))+IOPortBase);
}

static __inline__ unsigned int
inl(unsigned PORT_SIZE port)
{
	return *(volatile unsigned int*)(((unsigned PORT_SIZE)(port))+IOPortBase);
}


#    if defined(__mips__)
static __inline__ unsigned long ldq_u(unsigned long * r11)
{
	unsigned long r1;
	__asm__("lwr %0,%2\n\t"
		"lwl %0,%3\n\t"
		:"=&r" (r1)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(unsigned long *)(3+(char *) r11)));
	return r1;
}

static __inline__ unsigned long ldl_u(unsigned int * r11)
{
	unsigned long r1;
	__asm__("lwr %0,%2\n\t"
		"lwl %0,%3\n\t"
		:"=&r" (r1)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(unsigned long *)(3+(char *) r11)));
	return r1;
}

static __inline__ unsigned long ldw_u(unsigned short * r11)
{
	unsigned long r1;
	__asm__("lwr %0,%2\n\t"
		"lwl %0,%3\n\t"
		:"=&r" (r1)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(unsigned long *)(1+(char *) r11)));
	return r1;
}

#     ifdef linux	/* don't mess with other OSs */

/*
 * EGCS 1.1 knows about arbitrary unaligned loads (and we don't support older
 * versions anyway. Define some packed structures to talk about such things
 * with.
 */

struct __una_u32 { unsigned int   x __attribute__((packed)); };
struct __una_u16 { unsigned short x __attribute__((packed)); };

static __inline__ void stw_u(unsigned long val, unsigned short *p)
{
	struct __una_u16 *ptr = (struct __una_u16 *) p;
	ptr->x = val;
}

static __inline__ void stl_u(unsigned long val, unsigned int *p)
{
	struct __una_u32 *ptr = (struct __una_u32 *) p;
	ptr->x = val;
}

#       if X_BYTE_ORDER == X_BIG_ENDIAN
static __inline__ unsigned int
xf86ReadMmio32Be(__volatile__ void *base, const unsigned long offset)
{
	unsigned long addr = ((unsigned long)base) + offset;
	unsigned int ret;

	__asm__ __volatile__("lw %0, 0(%1)"
			     : "=r" (ret)
			     : "r" (addr));
	return ret;
}

static __inline__ void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
		  const unsigned int val)
{
	unsigned long addr = ((unsigned long)base) + offset;

	__asm__ __volatile__("sw %0, 0(%1)"
			     : /* No outputs */
			     : "r" (val), "r" (addr));
}
#      endif

#      define mem_barrier() \
        __asm__ __volatile__(					\
		"# prevent instructions being moved around\n\t"	\
       		".set\tnoreorder\n\t"				\
		"# 8 nops to fool the R4400 pipeline\n\t"	\
		"nop;nop;nop;nop;nop;nop;nop;nop\n\t"		\
		".set\treorder"					\
		: /* no output */				\
		: /* no input */				\
		: "memory")
#      define write_mem_barrier() mem_barrier()

#     else  /* !linux */

#      define stq_u(v,p)	stl_u(v,p)
#      define stl_u(v,p)	(*(unsigned char *)(p)) = (v); \
			(*(unsigned char *)(p)+1) = ((v) >> 8);  \
			(*(unsigned char *)(p)+2) = ((v) >> 16); \
			(*(unsigned char *)(p)+3) = ((v) >> 24)

#      define stw_u(v,p)	(*(unsigned char *)(p)) = (v); \
				(*(unsigned char *)(p)+1) = ((v) >> 8)

#      define mem_barrier()   /* NOP */
#     endif /* !linux */
#    endif /* __mips__ */

#    if defined(__arm32__)
#     define ldq_u(p)	(*((unsigned long  *)(p)))
#     define ldl_u(p)	(*((unsigned int   *)(p)))
#     define ldw_u(p)	(*((unsigned short *)(p)))
#     define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#     define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#     define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#     define mem_barrier()	/* NOP */
#     define write_mem_barrier()	/* NOP */
#    endif /* __arm32__ */

#   elif (defined(Lynx) || defined(linux) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)) && defined(__powerpc__)

#    ifndef MAP_FAILED
#     define MAP_FAILED ((void *)-1)
#    endif

extern volatile unsigned char *ioBase;

#if defined(linux) && defined(__powerpc64__)
# include <linux/version.h>
# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#  include <asm/memory.h>
# endif
#endif /* defined(linux) && defined(__powerpc64__) */
#ifndef eieio /* We deal with arch-specific eieio() routines above... */
# define eieio() __asm__ __volatile__ ("eieio" ::: "memory")
#endif /* eieio */

static __inline__ unsigned char
xf86ReadMmio8(__volatile__ void *base, const unsigned long offset)
{
        register unsigned char val;
        __asm__ __volatile__(
                        "lbzx %0,%1,%2\n\t"
                        "eieio"
                        : "=r" (val)
                        : "b" (base), "r" (offset),
                        "m" (*((volatile unsigned char *)base+offset)));
        return val;
}

static __inline__ unsigned short
xf86ReadMmio16Be(__volatile__ void *base, const unsigned long offset)
{
        register unsigned short val;
        __asm__ __volatile__(
                        "lhzx %0,%1,%2\n\t"
                        "eieio"
                        : "=r" (val)
                        : "b" (base), "r" (offset),
                        "m" (*((volatile unsigned char *)base+offset)));
        return val;
}

static __inline__ unsigned short
xf86ReadMmio16Le(__volatile__ void *base, const unsigned long offset)
{
        register unsigned short val;
        __asm__ __volatile__(
                        "lhbrx %0,%1,%2\n\t"
                        "eieio"
                        : "=r" (val)
                        : "b" (base), "r" (offset),
                        "m" (*((volatile unsigned char *)base+offset)));
        return val;
}

static __inline__ unsigned int
xf86ReadMmio32Be(__volatile__ void *base, const unsigned long offset)
{
        register unsigned int val;
        __asm__ __volatile__(
                        "lwzx %0,%1,%2\n\t"
                        "eieio"
                        : "=r" (val)
                        : "b" (base), "r" (offset),
                        "m" (*((volatile unsigned char *)base+offset)));
        return val;
}

static __inline__ unsigned int
xf86ReadMmio32Le(__volatile__ void *base, const unsigned long offset)
{
        register unsigned int val;
        __asm__ __volatile__(
                        "lwbrx %0,%1,%2\n\t"
                        "eieio"
                        : "=r" (val)
                        : "b" (base), "r" (offset),
                        "m" (*((volatile unsigned char *)base+offset)));
        return val;
}

static __inline__ void
xf86WriteMmioNB8(__volatile__ void *base, const unsigned long offset,
		 const unsigned char val)
{
        __asm__ __volatile__(
                        "stbx %1,%2,%3\n\t"
                        : "=m" (*((volatile unsigned char *)base+offset))
                        : "r" (val), "b" (base), "r" (offset));
}

static __inline__ void
xf86WriteMmioNB16Le(__volatile__ void *base, const unsigned long offset,
		    const unsigned short val)
{
        __asm__ __volatile__(
                        "sthbrx %1,%2,%3\n\t"
                        : "=m" (*((volatile unsigned char *)base+offset))
                        : "r" (val), "b" (base), "r" (offset));
}

static __inline__ void
xf86WriteMmioNB16Be(__volatile__ void *base, const unsigned long offset,
		    const unsigned short val)
{
        __asm__ __volatile__(
                        "sthx %1,%2,%3\n\t"
                        : "=m" (*((volatile unsigned char *)base+offset))
                        : "r" (val), "b" (base), "r" (offset));
}

static __inline__ void
xf86WriteMmioNB32Le(__volatile__ void *base, const unsigned long offset,
		    const unsigned int val)
{
        __asm__ __volatile__(
                        "stwbrx %1,%2,%3\n\t"
                        : "=m" (*((volatile unsigned char *)base+offset))
                        : "r" (val), "b" (base), "r" (offset));
}

static __inline__ void
xf86WriteMmioNB32Be(__volatile__ void *base, const unsigned long offset,
		    const unsigned int val)
{
        __asm__ __volatile__(
                        "stwx %1,%2,%3\n\t"
                        : "=m" (*((volatile unsigned char *)base+offset))
                        : "r" (val), "b" (base), "r" (offset));
}

static __inline__ void
xf86WriteMmio8(__volatile__ void *base, const unsigned long offset,
               const unsigned char val)
{
        xf86WriteMmioNB8(base, offset, val);
        eieio();
}

static __inline__ void
xf86WriteMmio16Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned short val)
{
        xf86WriteMmioNB16Le(base, offset, val);
        eieio();
}

static __inline__ void
xf86WriteMmio16Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned short val)
{
        xf86WriteMmioNB16Be(base, offset, val);
        eieio();
}

static __inline__ void
xf86WriteMmio32Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
        xf86WriteMmioNB32Le(base, offset, val);
        eieio();
}

static __inline__ void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
        xf86WriteMmioNB32Be(base, offset, val);
        eieio();
}


static __inline__ void
outb(unsigned short port, unsigned char value)
{
        if(ioBase == MAP_FAILED) return;
        xf86WriteMmio8((void *)ioBase, port, value);
}

static __inline__ void
outw(unsigned short port, unsigned short value)
{
        if(ioBase == MAP_FAILED) return;
        xf86WriteMmio16Le((void *)ioBase, port, value);
}

static __inline__ void
outl(unsigned short port, unsigned int value)
{
        if(ioBase == MAP_FAILED) return;
        xf86WriteMmio32Le((void *)ioBase, port, value);
}

static __inline__ unsigned int
inb(unsigned short port)
{
        if(ioBase == MAP_FAILED) return 0;
        return xf86ReadMmio8((void *)ioBase, port);
}

static __inline__ unsigned int
inw(unsigned short port)
{
        if(ioBase == MAP_FAILED) return 0;
        return xf86ReadMmio16Le((void *)ioBase, port);
}

static __inline__ unsigned int
inl(unsigned short port)
{
        if(ioBase == MAP_FAILED) return 0;
        return xf86ReadMmio32Le((void *)ioBase, port);
}

#    define ldq_u(p)	ldl_u(p)
#    define ldl_u(p)	((*(unsigned char *)(p))	| \
			(*((unsigned char *)(p)+1)<<8)	| \
			(*((unsigned char *)(p)+2)<<16)	| \
			(*((unsigned char *)(p)+3)<<24))
#    define ldw_u(p)	((*(unsigned char *)(p)) | \
			(*((unsigned char *)(p)+1)<<8))

#    define stq_u(v,p)	stl_u(v,p)
#    define stl_u(v,p)	(*(unsigned char *)(p)) = (v); \
				(*((unsigned char *)(p)+1)) = ((v) >> 8);  \
				(*((unsigned char *)(p)+2)) = ((v) >> 16); \
				(*((unsigned char *)(p)+3)) = ((v) >> 24)
#    define stw_u(v,p)	(*(unsigned char *)(p)) = (v); \
				(*((unsigned char *)(p)+1)) = ((v) >> 8)

#    define mem_barrier()	eieio()
#    define write_mem_barrier()	eieio()

#elif defined(__arm__) && defined(__linux__)

#define ldq_u(p)	(*((unsigned long  *)(p)))
#define ldl_u(p)	(*((unsigned int   *)(p)))
#define ldw_u(p)	(*((unsigned short *)(p)))
#define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#define mem_barrier()   /* NOP */
#define write_mem_barrier()   /* NOP */

/* for Linux on ARM, we use the LIBC inx/outx routines */
/* note that the appropriate setup via "ioperm" needs to be done */
/*  *before* any inx/outx is done. */

#include <sys/io.h>

static __inline__ void
xf_outb(unsigned short port, unsigned char val)
{
    outb(val, port);
}

static __inline__ void
xf_outw(unsigned short port, unsigned short val)
{
    outw(val, port);
}

static __inline__ void
xf_outl(unsigned short port, unsigned int val)
{
    outl(val, port);
}

#define outb xf_outb
#define outw xf_outw
#define outl xf_outl

#define arm_flush_cache(addr)						\
do {									\
  register unsigned long _beg __asm ("a1") = (unsigned long) (addr);	\
  register unsigned long _end __asm ("a2") = (unsigned long) (addr) + 4;\
  register unsigned long _flg __asm ("a3") = 0;				\
  __asm __volatile ("swi 0x9f0002		@ sys_cacheflush"	\
    : "=r" (_beg)							\
    : "0" (_beg), "r" (_end), "r" (_flg));				\
} while (0)

#   else /* ix86 */

#    define ldq_u(p)	(*((unsigned long  *)(p)))
#    define ldl_u(p)	(*((unsigned int   *)(p)))
#    define ldw_u(p)	(*((unsigned short *)(p)))
#    define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#    define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#    define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#    define mem_barrier()   /* NOP */
#    define write_mem_barrier()   /* NOP */

#    if !defined(__SUNPRO_C)
#    if !defined(FAKEIT) && !defined(__mc68000__) && !defined(__arm__) && !defined(__sh__) && !defined(__hppa__) && !defined(__s390__)
#     ifdef GCCUSESGAS

/*
 * If gcc uses gas rather than the native assembler, the syntax of these
 * inlines has to be different.		DHD
 */

static __inline__ void
outb(unsigned short port, unsigned char val)
{
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
}


static __inline__ void
outw(unsigned short port, unsigned short val)
{
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(unsigned short port)
{
   unsigned char ret;
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inw(unsigned short port)
{
   unsigned short ret;
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inl(unsigned short port)
{
   unsigned int ret;
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

#     else	/* GCCUSESGAS */

static __inline__ void
outb(unsigned short port, unsigned char val)
{
  __asm__ __volatile__("out%B0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outw(unsigned short port, unsigned short val)
{
  __asm__ __volatile__("out%W0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
  __asm__ __volatile__("out%L0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(unsigned short port)
{
  unsigned char ret;
  __asm__ __volatile__("in%B0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ unsigned int
inw(unsigned short port)
{
  unsigned short ret;
  __asm__ __volatile__("in%W0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ unsigned int
inl(unsigned short port)
{
  unsigned int ret;
  __asm__ __volatile__("in%L0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

#     endif /* GCCUSESGAS */

#    else /* !defined(FAKEIT) && !defined(__mc68000__)  && !defined(__arm__) && !defined(__sh__) && !defined(__hppa__)*/

static __inline__ void
outb(unsigned short port, unsigned char val)
{
}

static __inline__ void
outw(unsigned short port, unsigned short val)
{
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
}

static __inline__ unsigned int
inb(unsigned short port)
{
  return 0;
}

static __inline__ unsigned int
inw(unsigned short port)
{
  return 0;
}

static __inline__ unsigned int
inl(unsigned short port)
{
  return 0;
}

#    endif /* FAKEIT */
#    endif /* __SUNPRO_C */

#   endif /* ix86 */

#  else /* !GNUC */
#   if !defined(QNX4)
#    if defined(__STDC__) && (__STDC__ == 1)
#     ifndef asm
#      define asm __asm
#     endif
#    endif
#    ifndef SCO325
#     if defined(__UNIXWARE__)
#     /* avoid including <sys/types.h> for <sys/inline.h> on UnixWare */
#      define ushort unsigned short
#      define ushort_t unsigned short
#      define ulong unsigned long
#      define ulong_t unsigned long
#      define uint_t unsigned int
#      define uchar_t unsigned char
#     endif /* __UNIXWARE__ */
#     if !defined(sgi) && !defined(__SUNPRO_C)
#      include <sys/inline.h>
#     endif
#    else
#     include "scoasm.h"
#    endif
#    if (!defined(__HIGHC__) && !defined(sgi) && !defined(__SUNPRO_C)) || \
	defined(__USLC__)
#     pragma asm partial_optimization outl
#     pragma asm partial_optimization outw
#     pragma asm partial_optimization outb
#     pragma asm partial_optimization inl
#     pragma asm partial_optimization inw
#     pragma asm partial_optimization inb
#    endif
#   endif
#   define ldq_u(p)	(*((unsigned long  *)(p)))
#   define ldl_u(p)	(*((unsigned int   *)(p)))
#   define ldw_u(p)	(*((unsigned short *)(p)))
#   define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#   define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#   define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#   define mem_barrier()   /* NOP */
#   define write_mem_barrier()   /* NOP */
#  endif /* __GNUC__ */

#  if defined(QNX4)
#   include <sys/types.h>
extern unsigned  inb(unsigned port);
extern unsigned  inw(unsigned port);
extern unsigned  inl(unsigned port);
extern void outb(unsigned port, unsigned val);
extern void outw(unsigned port, unsigned val);
extern void outl(unsigned port, unsigned val);
#  endif /* QNX4 */

#  if defined(IODEBUG) && defined(__GNUC__)
#   undef inb
#   undef inw
#   undef inl
#   undef outb
#   undef outw
#   undef outl
#   define inb(a) __extension__ ({unsigned char __c=RealInb(a); ErrorF("inb(0x%03x) = 0x%02x\t@ line %4d, file %s\n", a, __c, __LINE__, __FILE__);__c;})
#   define inw(a) __extension__ ({unsigned short __c=RealInw(a); ErrorF("inw(0x%03x) = 0x%04x\t@ line %4d, file %s\n", a, __c, __LINE__, __FILE__);__c;})
#   define inl(a) __extension__ ({unsigned int __c=RealInl(a); ErrorF("inl(0x%03x) = 0x%08x\t@ line %4d, file %s\n", a, __c, __LINE__, __FILE__);__c;})

#   define outb(a,b) (ErrorF("outb(0x%03x, 0x%02x)\t@ line %4d, file %s\n", a, b, __LINE__, __FILE__),RealOutb(a,b))
#   define outw(a,b) (ErrorF("outw(0x%03x, 0x%04x)\t@ line %4d, file %s\n", a, b, __LINE__, __FILE__),RealOutw(a,b))
#   define outl(a,b) (ErrorF("outl(0x%03x, 0x%08x)\t@ line %4d, file %s\n", a, b, __LINE__, __FILE__),RealOutl(a,b))
#  endif

# endif /* NO_INLINE */

# ifdef __alpha__
/* entry points for Mmio memory access routines */
extern int (*xf86ReadMmio8)(void *, unsigned long);
extern int (*xf86ReadMmio16)(void *, unsigned long);
#  ifndef STANDALONE_MMIO
extern int (*xf86ReadMmio32)(void *, unsigned long);
#  else
/* Some DRI 3D drivers need MMIO_IN32. */
static __inline__ int
xf86ReadMmio32(void *Base, unsigned long Offset)
{
	__asm__ __volatile__("mb"  : : : "memory");
	return *(volatile unsigned int*)((unsigned long)Base+(Offset));
}
#  endif
extern void (*xf86WriteMmio8)(int, void *, unsigned long);
extern void (*xf86WriteMmio16)(int, void *, unsigned long);
extern void (*xf86WriteMmio32)(int, void *, unsigned long);
extern void (*xf86WriteMmioNB8)(int, void *, unsigned long);
extern void (*xf86WriteMmioNB16)(int, void *, unsigned long);
extern void (*xf86WriteMmioNB32)(int, void *, unsigned long);
extern void xf86JensenMemToBus(char *, long, long, int);
extern void xf86JensenBusToMem(char *, char *, unsigned long, int);
extern void xf86SlowBCopyFromBus(unsigned char *, unsigned char *, int);
extern void xf86SlowBCopyToBus(unsigned char *, unsigned char *, int);

/* Some macros to hide the system dependencies for MMIO accesses */
/* Changed to kill noise generated by gcc's -Wcast-align */
#  define MMIO_IN8(base, offset) (*xf86ReadMmio8)(base, offset)
#  define MMIO_IN16(base, offset) (*xf86ReadMmio16)(base, offset)
#  ifndef STANDALONE_MMIO
#   define MMIO_IN32(base, offset) (*xf86ReadMmio32)(base, offset)
#  else
#   define MMIO_IN32(base, offset) xf86ReadMmio32(base, offset)
#  endif

#  if defined (JENSEN_SUPPORT)
#   define MMIO_OUT32(base, offset, val) \
    (*xf86WriteMmio32)((CARD32)(val), base, offset)
#   define MMIO_ONB32(base, offset, val) \
    (*xf86WriteMmioNB32)((CARD32)(val), base, offset)
#  else
#   define MMIO_OUT32(base, offset, val) \
    do { \
	write_mem_barrier(); \
	*(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset)) = (val); \
    } while (0)
#   define MMIO_ONB32(base, offset, val) \
	*(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset)) = (val)
#  endif

#  define MMIO_OUT8(base, offset, val) \
    (*xf86WriteMmio8)((CARD8)(val), base, offset)
#  define MMIO_OUT16(base, offset, val) \
    (*xf86WriteMmio16)((CARD16)(val), base, offset)
#  define MMIO_ONB8(base, offset, val) \
    (*xf86WriteMmioNB8)((CARD8)(val), base, offset)
#  define MMIO_ONB16(base, offset, val) \
    (*xf86WriteMmioNB16)((CARD16)(val), base, offset)
#  define MMIO_MOVE32(base, offset, val) \
    MMIO_OUT32(base, offset, val)

# elif defined(__powerpc__)  
 /* 
  * we provide byteswapping and no byteswapping functions here
  * with byteswapping as default, 
  * drivers that don't need byteswapping should define PPC_MMIO_IS_BE 
  */
#  define MMIO_IN8(base, offset) xf86ReadMmio8(base, offset)
#  define MMIO_OUT8(base, offset, val) \
    xf86WriteMmio8(base, offset, (CARD8)(val))
#  define MMIO_ONB8(base, offset, val) \
    xf86WriteMmioNB8(base, offset, (CARD8)(val))

#  if defined(PPC_MMIO_IS_BE) /* No byteswapping */
#   define MMIO_IN16(base, offset) xf86ReadMmio16Be(base, offset)
#   define MMIO_IN32(base, offset) xf86ReadMmio32Be(base, offset)
#   define MMIO_OUT16(base, offset, val) \
    xf86WriteMmio16Be(base, offset, (CARD16)(val))
#   define MMIO_OUT32(base, offset, val) \
    xf86WriteMmio32Be(base, offset, (CARD32)(val))
#   define MMIO_ONB16(base, offset, val) \
    xf86WriteMmioNB16Be(base, offset, (CARD16)(val))
#   define MMIO_ONB32(base, offset, val) \
    xf86WriteMmioNB32Be(base, offset, (CARD32)(val))
#  else /* byteswapping is the default */
#   define MMIO_IN16(base, offset) xf86ReadMmio16Le(base, offset)
#   define MMIO_IN32(base, offset) xf86ReadMmio32Le(base, offset)
#   define MMIO_OUT16(base, offset, val) \
     xf86WriteMmio16Le(base, offset, (CARD16)(val))
#   define MMIO_OUT32(base, offset, val) \
     xf86WriteMmio32Le(base, offset, (CARD32)(val))
#   define MMIO_ONB16(base, offset, val) \
     xf86WriteMmioNB16Le(base, offset, (CARD16)(val))
#   define MMIO_ONB32(base, offset, val) \
     xf86WriteMmioNB32Le(base, offset, (CARD32)(val))
#  endif

#  define MMIO_MOVE32(base, offset, val) \
       xf86WriteMmio32Be(base, offset, (CARD32)(val))

static __inline__ void ppc_flush_icache(char *addr)
{
	__asm__ volatile (
		"dcbf 0,%0;" 
		"sync;" 
		"icbi 0,%0;" 
		"sync;" 
		"isync;" 
		: : "r"(addr) : "memory");
}

# elif defined(__sparc__) || defined(sparc) || defined(__sparc)
 /*
  * Like powerpc, we provide byteswapping and no byteswapping functions
  * here with byteswapping as default, drivers that don't need byteswapping
  * should define SPARC_MMIO_IS_BE (perhaps create a generic macro so that we
  * do not need to use PPC_MMIO_IS_BE and the sparc one in all the same places
  * of drivers?).
  */
#  define MMIO_IN8(base, offset) xf86ReadMmio8(base, offset)
#  define MMIO_OUT8(base, offset, val) \
    xf86WriteMmio8(base, offset, (CARD8)(val))
#  define MMIO_ONB8(base, offset, val) \
    xf86WriteMmio8NB(base, offset, (CARD8)(val))

#  if defined(SPARC_MMIO_IS_BE) /* No byteswapping */
#   define MMIO_IN16(base, offset) xf86ReadMmio16Be(base, offset)
#   define MMIO_IN32(base, offset) xf86ReadMmio32Be(base, offset)
#   define MMIO_OUT16(base, offset, val) \
     xf86WriteMmio16Be(base, offset, (CARD16)(val))
#   define MMIO_OUT32(base, offset, val) \
     xf86WriteMmio32Be(base, offset, (CARD32)(val))
#   define MMIO_ONB16(base, offset, val) \
     xf86WriteMmio16BeNB(base, offset, (CARD16)(val))
#   define MMIO_ONB32(base, offset, val) \
     xf86WriteMmio32BeNB(base, offset, (CARD32)(val))
#  else /* byteswapping is the default */
#   define MMIO_IN16(base, offset) xf86ReadMmio16Le(base, offset)
#   define MMIO_IN32(base, offset) xf86ReadMmio32Le(base, offset)
#   define MMIO_OUT16(base, offset, val) \
     xf86WriteMmio16Le(base, offset, (CARD16)(val))
#   define MMIO_OUT32(base, offset, val) \
     xf86WriteMmio32Le(base, offset, (CARD32)(val))
#   define MMIO_ONB16(base, offset, val) \
     xf86WriteMmio16LeNB(base, offset, (CARD16)(val))
#   define MMIO_ONB32(base, offset, val) \
     xf86WriteMmio32LeNB(base, offset, (CARD32)(val))
#  endif

#  define MMIO_MOVE32(base, offset, val) \
       xf86WriteMmio32Be(base, offset, (CARD32)(val))

# else /* !__alpha__ && !__powerpc__ && !__sparc__ */

#  define MMIO_IN8(base, offset) \
	*(volatile CARD8 *)(((CARD8*)(base)) + (offset))
#  define MMIO_IN16(base, offset) \
	*(volatile CARD16 *)(void *)(((CARD8*)(base)) + (offset))
#  define MMIO_IN32(base, offset) \
	*(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset))
#  define MMIO_OUT8(base, offset, val) \
	*(volatile CARD8 *)(((CARD8*)(base)) + (offset)) = (val)
#  define MMIO_OUT16(base, offset, val) \
	*(volatile CARD16 *)(void *)(((CARD8*)(base)) + (offset)) = (val)
#  define MMIO_OUT32(base, offset, val) \
	*(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset)) = (val)
#  define MMIO_ONB8(base, offset, val) MMIO_OUT8(base, offset, val) 
#  define MMIO_ONB16(base, offset, val) MMIO_OUT16(base, offset, val) 
#  define MMIO_ONB32(base, offset, val) MMIO_OUT32(base, offset, val) 

#  define MMIO_MOVE32(base, offset, val) MMIO_OUT32(base, offset, val)

# endif /* __alpha__ */

/*
 * With Intel, the version in os-support/misc/SlowBcopy.s is used.
 * This avoids port I/O during the copy (which causes problems with
 * some hardware).
 */
# ifdef __alpha__
#  define slowbcopy_tobus(src,dst,count) xf86SlowBCopyToBus(src,dst,count)
#  define slowbcopy_frombus(src,dst,count) xf86SlowBCopyFromBus(src,dst,count)
# else /* __alpha__ */
#  define slowbcopy_tobus(src,dst,count) xf86SlowBcopy(src,dst,count)
#  define slowbcopy_frombus(src,dst,count) xf86SlowBcopy(src,dst,count)
# endif /* __alpha__ */

#endif /* _COMPILER_H */
