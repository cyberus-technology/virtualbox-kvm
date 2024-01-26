/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_ansic.h,v 3.53 2003/10/28 18:36:37 tsi Exp $ */
/*
 * Copyright 1997-2003 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef _XF86_ANSIC_H
#define _XF86_ANSIC_H

/* Handle <stdarg.h> */

#ifndef IN_MODULE
# include <stdarg.h>
#else /* !IN_MODULE */
# ifndef __OS2ELF__
#  include <stdarg.h>
# else /* __OS2ELF__ */
   /* EMX/gcc_elf under OS/2 does not have native header files */
#  if !defined (_VA_LIST)
#   define _VA_LIST
    typedef char *va_list;
#  endif
#  define _VA_ROUND(t) ((sizeof (t) + 3) & -4)
#  if !defined (va_start)
#   define va_start(ap,v) ap = (va_list)&v + ((sizeof (v) + 3) & -4)
#   define va_end(ap) (ap = 0, (void)0)
#   define va_arg(ap,t) (ap += _VA_ROUND (t), *(t *)(ap - _VA_ROUND (t)))
#  endif
# endif /* __OS2ELF__ */
#endif /* IN_MODULE */

/*
 * The first set of definitions are required both for modules and
 * libc_wrapper.c.
 */

#if defined(XFree86LOADER) || defined(NEED_XF86_TYPES)

#if !defined(SYSV) && !defined(SVR4) && !defined(Lynx) || \
	defined(__SCO__) || defined(__UNIXWARE__)
#define HAVE_VSSCANF
#define HAVE_VFSCANF
#endif 

#ifndef NULL
#if (defined(SVR4) || defined(SYSV)) && !defined(__GNUC__)
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif
#ifndef EOF
#define EOF (-1)
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/* <limits.h> stuff */
#define x_BITSPERBYTE 8
#define x_BITS(type)  (x_BITSPERBYTE * (int)sizeof(type))
#define x_SHORTBITS x_BITS(short)
#define x_INTBITS x_BITS(int)
#define x_LONGBITS x_BITS(long)
#ifndef SHRT_MIN
#define SHRT_MIN ((short)(1 << (x_SHORTBITS - 1)))
#endif

#ifndef FONTMODULE
#include "misc.h"
#endif
#include "xf86_libc.h"
#ifndef SHRT_MAX
#define SHRT_MAX ((short)~SHRT_MIN)
#endif
#ifndef USHRT_MAX
#define USHRT_MAX ((unsigned short)~0)
#endif
#ifndef MINSHORT
#define MINSHORT SHRT_MIN
#endif
#ifndef MAXSHORT
#define MAXSHORT SHRT_MAX
#endif
#ifndef INT_MIN
#define INT_MIN (1 << (x_INTBITS - 1))
#endif
#ifndef INT_MAX
#define INT_MAX (~INT_MIN)
#endif
#ifndef UINT_MAX
#define UINT_MAX (~0)
#endif
#ifndef MININT
#define MININT INT_MIN
#endif
#ifndef MAXINT
#define MAXINT INT_MAX
#endif
#ifndef LONG_MIN
#define LONG_MIN ((long)(1 << (x_LONGBITS - 1)))
#endif
#ifndef LONG_MAX
#define LONG_MAX ((long)~LONG_MIN)
#endif
#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)~0UL)
#endif
#ifndef MINLONG
#define MINLONG LONG_MIN
#endif
#ifndef MAXLONG
#define MAXLONG LONG_MAX
#endif

#endif /* XFree86LOADER || NEED_XF86_TYPES */

#if defined(XFree86LOADER) || defined(NEED_XF86_PROTOTYPES)
/*
 * ANSI C compilers only.
 */

/* ANSI C emulation library */

extern void xf86abort(void);
extern int xf86abs(int);
extern double xf86acos(double);
extern double xf86asin(double);
extern double xf86atan(double);
extern double xf86atan2(double,double);
extern double xf86atof(const char*);
extern int xf86atoi(const char*);
extern long xf86atol(const char*);
extern void *xf86bsearch(const void *, const void *, xf86size_t, xf86size_t,
			 int (*)(const void *, const void *));
extern double xf86ceil(double);
extern void* xf86calloc(xf86size_t,xf86size_t);
extern void xf86clearerr(XF86FILE*);
extern double xf86cos(double);
extern void xf86exit(int);
extern double xf86exp(double);
extern double xf86fabs(double);
extern int xf86fclose(XF86FILE*);
extern int xf86feof(XF86FILE*);
extern int xf86ferror(XF86FILE*);
extern int xf86fflush(XF86FILE*);
extern int xf86fgetc(XF86FILE*);
extern int xf86getc(XF86FILE*);
extern int xf86fgetpos(XF86FILE*,XF86fpos_t*);
extern char* xf86fgets(char*,INT32,XF86FILE*);
extern int xf86finite(double);
extern double xf86floor(double);
extern double xf86fmod(double,double);
extern XF86FILE* xf86fopen(const char*,const char*);
extern double xf86frexp(double, int*);
extern int xf86printf(const char*,...);
extern int xf86fprintf(XF86FILE*,const char*,...);
extern int xf86fputc(int,XF86FILE*);
extern int xf86fputs(const char*,XF86FILE*);
extern xf86size_t xf86fread(void*,xf86size_t,xf86size_t,XF86FILE*);
extern void xf86free(void*);
extern XF86FILE* xf86freopen(const char*,const char*,XF86FILE*);
#if defined(HAVE_VFSCANF) || !defined(NEED_XF86_PROTOTYPES)
extern int xf86fscanf(XF86FILE*,const char*,...);
#else
extern int xf86fscanf(/*XF86FILE*,const char*,char *,char *,char *,char *,
			char *,char *,char *,char *,char *,char * */);
#endif
extern int xf86fseek(XF86FILE*,long,int);
extern int xf86fsetpos(XF86FILE*,const XF86fpos_t*);
extern long xf86ftell(XF86FILE*);
extern xf86size_t xf86fwrite(const void*,xf86size_t,xf86size_t,XF86FILE*);
extern char* xf86getenv(const char*);
extern int xf86isalnum(int);
extern int xf86isalpha(int);
extern int xf86iscntrl(int);
extern int xf86isdigit(int);
extern int xf86isgraph(int);
extern int xf86islower(int);
extern int xf86isprint(int);
extern int xf86ispunct(int);
extern int xf86isspace(int);
extern int xf86isupper(int);
extern int xf86isxdigit(int);
extern long xf86labs(long);
extern double xf86ldexp(double,int);
extern double xf86log(double);
extern double xf86log10(double);
extern void* xf86malloc(xf86size_t);
extern void* xf86memchr(const void*,int,xf86size_t);
extern int xf86memcmp(const void*,const void*,xf86size_t);
extern void* xf86memcpy(void*,const void*,xf86size_t);
extern void* xf86memmove(void*,const void*,xf86size_t);
extern void* xf86memset(void*,int,xf86size_t);
extern double xf86modf(double,double*);
extern void xf86perror(const char*);
extern double xf86pow(double,double);
extern void xf86qsort(void*, xf86size_t, xf86size_t, 
                      int(*)(const void*, const void*));
extern void* xf86realloc(void*,xf86size_t);
extern int xf86remove(const char*);
extern int xf86rename(const char*,const char*);
extern void xf86rewind(XF86FILE*);
extern int xf86setbuf(XF86FILE*,char*);
extern int xf86setvbuf(XF86FILE*,char*,int,xf86size_t);
extern double xf86sin(double);
extern int xf86sprintf(char*,const char*,...);
extern int xf86snprintf(char*,xf86size_t,const char*,...);
extern double xf86sqrt(double);
#if defined(HAVE_VSSCANF) || !defined(NEED_XF86_PROTOTYPES)
extern int xf86sscanf(char*,const char*,...);
#else
extern int xf86sscanf(/*char*,const char*,char *,char *,char *,char *,
			char *,char *,char *,char *,char *,char * */);
#endif
extern char* xf86strcat(char*,const char*);
extern char* xf86strchr(const char*, int c);
extern int xf86strcmp(const char*,const char*);
extern int xf86strcasecmp(const char*,const char*);
extern char* xf86strcpy(char*,const char*);
extern xf86size_t xf86strcspn(const char*,const char*);
extern char* xf86strerror(int);
extern xf86size_t xf86strlcat(char*,const char*,xf86size_t);
extern xf86size_t xf86strlcpy(char*,const char*,xf86size_t);
extern xf86size_t xf86strlen(const char*);
extern char* xf86strncat(char *, const char *, xf86size_t);
extern int xf86strncmp(const char*,const char*,xf86size_t);
extern int xf86strncasecmp(const char*,const char*,xf86size_t);
extern char* xf86strncpy(char*,const char*,xf86size_t);
extern char* xf86strpbrk(const char*,const char*);
extern char* xf86strrchr(const char*,int);
extern xf86size_t xf86strspn(const char*,const char*);
extern char* xf86strstr(const char*,const char*);
extern double xf86strtod(const char*,char**);
extern char* xf86strtok(char*,const char*);
extern long xf86strtol(const char*,char**,int);
extern unsigned long xf86strtoul(const char*,char**,int);
extern double xf86tan(double);
extern XF86FILE* xf86tmpfile(void);
extern char* xf86tmpnam(char*);
extern int xf86tolower(int);
extern int xf86toupper(int);
extern int xf86ungetc(int,XF86FILE*);
extern int xf86vfprintf(XF86FILE*,const char*,va_list);
extern int xf86vsprintf(char*,const char*,va_list);
extern int xf86vsnprintf(char*,xf86size_t,const char*,va_list);

extern int xf86open(const char*, int,...);
extern int xf86close(int);
extern long xf86lseek(int, long, int);
extern int xf86ioctl(int, unsigned long, pointer);
extern xf86ssize_t xf86read(int, void *, xf86size_t);
extern xf86ssize_t xf86write(int, const void *, xf86size_t);
extern void* xf86mmap(void*, xf86size_t, int, int, int, xf86size_t /* off_t */);
extern int xf86munmap(void*, xf86size_t);
extern int xf86stat(const char *, struct xf86stat *);
extern int xf86fstat(int, struct xf86stat *);
extern int xf86access(const char *, int);
extern int xf86errno;
extern int xf86GetErrno(void);

extern double xf86HUGE_VAL;

extern double xf86hypot(double,double);

/* non-ANSI C functions */
extern XF86DIR* xf86opendir(const char*);
extern int xf86closedir(XF86DIR*);
extern XF86DIRENT* xf86readdir(XF86DIR*);
extern void xf86rewinddir(XF86DIR*);
extern void xf86bcopy(const void*,void*,xf86size_t);
extern int xf86ffs(int);
extern char* xf86strdup(const char*);
extern void xf86bzero(void*,unsigned int);
extern int xf86execl(const char *, const char *, ...);
extern long xf86fpossize(void);
extern int xf86chmod(const char *, xf86mode_t);
extern int xf86chown(const char *, xf86uid_t, xf86gid_t);
extern xf86uid_t xf86geteuid(void);
extern xf86gid_t xf86getegid(void);
extern int xf86getpid(void);
extern int xf86mknod(const char *, xf86mode_t, xf86dev_t);
extern int xf86mkdir(const char *, xf86mode_t);
unsigned int xf86sleep(unsigned int seconds);
/* sysv IPC */
extern int xf86shmget(xf86key_t key, int size, int xf86shmflg);
extern char * xf86shmat(int id, char *addr, int xf86shmflg);
extern int xf86shmdt(char *addr);
extern int xf86shmctl(int id, int xf86cmd, pointer buf);

extern int xf86setjmp(xf86jmp_buf env);
extern int xf86setjmp0(xf86jmp_buf env);
extern int xf86setjmp1(xf86jmp_buf env, int);
extern int xf86setjmp1_arg2(void);
extern int xf86setjmperror(xf86jmp_buf env);
extern int xf86getjmptype(void);
extern void xf86longjmp(xf86jmp_buf env, int val);
#define xf86setjmp_macro(env) \
	(xf86getjmptype() == 0 ? xf86setjmp0((env)) : \
	(xf86getjmptype() == 1 ? xf86setjmp1((env), xf86setjmp1_arg2()) : \
		xf86setjmperror((env))))

#else /* XFree86LOADER || NEED_XF86_PROTOTYPES */
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#ifdef HAVE_SYSV_IPC
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include <sys/stat.h>
#define stat_t struct stat
#endif /* XFree86LOADER || NEED_XF86_PROTOTYPES */

/*
 * These things are always required by drivers (but not by libc_wrapper.c),
 * even for a static server because some OSs don't provide them.
 */

extern int xf86getpagesize(void);
extern void xf86usleep(unsigned long);
extern void xf86getsecs(long *, long *);
#ifndef DONT_DEFINE_WRAPPERS
#undef getpagesize
#define getpagesize()		xf86getpagesize()
#undef usleep
#define usleep(ul)		xf86usleep(ul)
#undef getsecs
#define getsecs(a, b)		xf86getsecs(a, b)
#endif
#endif /* _XF86_ANSIC_H */
