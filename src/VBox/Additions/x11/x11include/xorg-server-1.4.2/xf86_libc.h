/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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

/*
 * This file is an attempt to make developing code for the new loadable module
 * architecure simpler. It tries to use macros to hide all libc wrappers so
 * that all that is needed to "port" a module to this architecture is to
 * include this one header file
 *
 * Revision history:
 *
 *
 * 0.4	Apr 12 1997	add the ANSI defines
 * 0.3	Feb 24 1997	handle getenv
 * 0.2	Feb 24 1997	hide few FILE functions
 * 0.1	Feb 24 1997	hide the trivial functions mem* str*
 */

#ifndef	XF86_LIBC_H
#define XF86_LIBC_H 1

#include <X11/Xfuncs.h>
#include <stddef.h>

/*
 * The first set of definitions are required both for modules and
 * libc_wrapper.c.
 */

/*
 * First, the new data types
 *
 * note: if some pointer is declared "opaque" here, pass it between
 * xf86* functions only, and don't rely on it having a whatever internal
 * structure, even if some source file might reveal the existence of
 * such a structure.
 */
typedef void XF86FILE;		/* opaque FILE replacement */
extern  XF86FILE* xf86stdin;
extern  XF86FILE* xf86stdout;
extern  XF86FILE* xf86stderr;

typedef void XF86fpos_t;	/* opaque fpos_t replacement */

#define _XF86NAMELEN	263	/* enough for a larger filename */
				/* (divisble by 8) */
typedef void XF86DIR;		/* opaque DIR replacement */

/* Note: the following is POSIX! POSIX only requires the d_name member. 
 * Normal Unix has often a number of other members, but don't rely on that
 */
struct _xf86dirent {		/* types in struct dirent/direct: */
	char	d_name[_XF86NAMELEN+1];	/* char [MAXNAMLEN]; might be smaller or unaligned */
};
typedef struct _xf86dirent XF86DIRENT;

typedef unsigned long xf86size_t;
typedef signed long xf86ssize_t;
typedef unsigned long xf86dev_t;
typedef unsigned int xf86mode_t;
typedef unsigned int xf86uid_t;
typedef unsigned int xf86gid_t;

struct xf86stat {
    xf86dev_t st_rdev;	/* This is incomplete, and makes assumptions */
};

/* sysv IPC */
typedef int xf86key_t;

/* setjmp/longjmp */
#if defined(__ia64__)
typedef int xf86jmp_buf[1024] __attribute__ ((aligned (16))); /* guarantees 128-bit alignment! */
#else
typedef int xf86jmp_buf[1024];
#endif

/* for setvbuf */
#define XF86_IONBF    1
#define XF86_IOFBF    2
#define XF86_IOLBF    3

/* for open (XXX not complete) */
#define XF86_O_RDONLY	0x0000
#define XF86_O_WRONLY	0x0001
#define XF86_O_RDWR	0x0002
#define XF86_O_CREAT	0x0200

/* for mmap */
#define XF86_PROT_EXEC		0x0001
#define XF86_PROT_READ		0x0002
#define XF86_PROT_WRITE		0x0004
#define XF86_PROT_NONE		0x0008
#define XF86_MAP_FIXED		0x0001
#define XF86_MAP_SHARED		0x0002
#define XF86_MAP_PRIVATE	0x0004
#define XF86_MAP_32BIT	        0x0040
#define XF86_MAP_FAILED		((void *)-1)

/* for fseek */
#define XF86_SEEK_SET	0
#define XF86_SEEK_CUR	1
#define XF86_SEEK_END	2

/* for access */
#define XF86_R_OK       0
#define XF86_W_OK       1
#define XF86_X_OK       2
#define XF86_F_OK       3

/* for chmod */
#define XF86_S_ISUID   04000 /* set user ID on execution */
#define XF86_S_ISGID   02000 /* set group ID on execution */
#define XF86_S_ISVTX   01000 /* sticky bit */
#define XF86_S_IRUSR   00400 /* read by owner */
#define XF86_S_IWUSR   00200 /* write by owner */
#define XF86_S_IXUSR   00100 /* execute/search by owner */
#define XF86_S_IRGRP   00040 /* read by group */
#define XF86_S_IWGRP   00020 /* write by group */
#define XF86_S_IXGRP   00010 /* execute/search by group */
#define XF86_S_IROTH   00004 /* read by others */
#define XF86_S_IWOTH   00002 /* write by others */
#define XF86_S_IXOTH   00001 /* execute/search by others */

/* for mknod */
#define XF86_S_IFREG 0010000
#define XF86_S_IFCHR 0020000
#define XF86_S_IFBLK 0040000
#define XF86_S_IFIFO 0100000

/*
 * errno values
 * They start at 1000 just so they don't match real errnos at all
 */
#define xf86_UNKNOWN		1000
#define xf86_EACCES		1001
#define xf86_EAGAIN		1002
#define xf86_EBADF		1003
#define xf86_EEXIST		1004
#define xf86_EFAULT		1005
#define xf86_EINTR		1006
#define xf86_EINVAL		1007
#define xf86_EISDIR		1008
#define xf86_ELOOP		1009
#define xf86_EMFILE		1010
#define xf86_ENAMETOOLONG	1011
#define xf86_ENFILE		1012
#define xf86_ENOENT		1013
#define xf86_ENOMEM		1014
#define xf86_ENOSPC		1015
#define xf86_ENOTDIR		1016
#define xf86_EPIPE		1017
#define xf86_EROFS		1018
#define xf86_ETXTBSY		1019
#define xf86_ENOTTY		1020
#define xf86_ENOSYS		1021
#define xf86_EBUSY		1022
#define xf86_ENODEV		1023
#define xf86_EIO		1024

#define xf86_ESRCH		1025
#define xf86_ENXIO		1026
#define xf86_E2BIG		1027
#define xf86_ENOEXEC		1028
#define xf86_ECHILD		1029
#define xf86_ENOTBLK		1030
#define xf86_EXDEV		1031
#define xf86_EFBIG		1032
#define xf86_ESPIPE		1033
#define xf86_EMLINK		1034
#define xf86_EDOM		1035
#define xf86_ERANGE		1036
 

/* sysv IPV */
/* xf86shmget() */
#define XF86IPC_CREAT  01000
#define XF86IPC_EXCL   02000
#define XF86IPC_NOWAIT 04000
#define XF86SHM_R           0400         
#define XF86SHM_W           0200            
#define XF86IPC_PRIVATE ((xf86key_t)0)
/* xf86shmat() */
#define XF86SHM_RDONLY      010000      /* attach read-only else read-write */
#define XF86SHM_RND         020000      /* round attach address to SHMLBA */
#define XF86SHM_REMAP       040000      /* take-over region on attach */
/* xf86shmclt() */
#define XF86IPC_RMID 0

/*
 * the rest of this file should only be included for code that is supposed
 * to go into modules
 */

#if !defined(DONT_DEFINE_WRAPPERS)

#undef abort
#define abort()			xf86abort()
#undef abs
#define abs(i)			xf86abs(i)
#undef acos
#define acos(d)			xf86acos(d)
#undef asin
#define asin(d)			xf86asin(d)
#undef atan
#define atan(d)			xf86atan(d)
#undef atan2
#define atan2(d1,d2)		xf86atan2(d1,d2)
#undef atof
#define atof(ccp)		xf86atof(ccp)
#undef atoi
#define atoi(ccp)		xf86atoi(ccp)
#undef atol
#define atol(ccp)		xf86atol(ccp)
#undef bsearch
#define bsearch(a,b,c,d,e)	xf86bsearch(a,b,c,d,e)
#undef ceil
#define ceil(d)			xf86ceil(d)
#undef calloc
#define calloc(I1,I2)		xf86calloc(I1,I2)
#undef clearerr
#define clearerr(FP)		xf86clearerr(FP)
#undef cos
#define cos(d)			xf86cos(d)
#undef exit
#define exit(i)			xf86exit(i)
#undef exp
#define exp(d)			xf86exp(d)
#undef fabs
#define fabs(d)			xf86fabs(d)
#undef fclose
#define fclose(FP)		xf86fclose(FP)
#undef feof
#define feof(FP)		xf86feof(FP)
#undef ferror
#define ferror(FP)		xf86ferror(FP)
#undef fflush
#define fflush(FP)		xf86fflush(FP)
#undef fgetc
#define fgetc(FP)		xf86fgetc(FP)
#undef getc
#define getc(FP)		xf86getc(FP)
#undef fgetpos
#define fgetpos(FP,fpp)		xf86fgetpos(FP,fpp)
#undef fgets
#define fgets(cp,i,FP)		xf86fgets(cp,i,FP)
#undef finite
#define finite(d)		xf86finite(d)
#undef floor
#define floor(d)		xf86floor(d)
#undef fmod
#define fmod(d1,d2)		xf86fmod(d1,d2)
#undef fopen
#define fopen(ccp1,ccp2)	xf86fopen(ccp1,ccp2)
#undef printf
#define printf			xf86printf
#undef fprintf
#define fprintf			xf86fprintf
#undef fputc
#define fputc(i,FP)		xf86fputc(i,FP)
#undef fputs
#define fputs(ccp,FP)		xf86fputs(ccp,FP)
#undef fread
#define fread(vp,I1,I2,FP)	xf86fread(vp,I1,I2,FP)
#undef free
#define free(vp)		xf86free(vp)
#undef freopen
#define freopen(ccp1,ccp2,FP)	xf86freopen(ccp1,ccp2,FP)
#undef frexp
#define frexp(x,exp)            xf86frexp(x,exp)
#undef fscanf
#define fscanf			xf86fscanf
#undef fseek
#define fseek(FP,l,i)		xf86fseek(FP,l,i)
#undef fsetpos
#define fsetpos(FP,cfpp)	xf86fsetpos(FP,cfpp)
#undef ftell
#define ftell(FP)		xf86ftell(FP)
#undef fwrite
#define fwrite(cvp,I1,I2,FP)	xf86fwrite(cvp,I1,I2,FP)
#undef getenv
#define getenv(ccp)		xf86getenv(ccp)
#undef isalnum
#define isalnum(i)		xf86isalnum(i)
#undef isalpha
#define isalpha(i)		xf86isalpha(i)
#undef iscntrl
#define iscntrl(i)		xf86iscntrl(i)
#undef isdigit
#define isdigit(i)		xf86isdigit(i)
#undef isgraph
#define isgraph(i)		xf86isgraph(i)
#undef islower
#define islower(i)		xf86islower(i)
#undef isprint
#define isprint(i)		xf86isprint(i)
#undef ispunct
#define ispunct(i)		xf86ispunct(i)
#undef isspace
#define isspace(i)		xf86isspace(i)
#undef isupper
#define isupper(i)		xf86isupper(i)
#undef isxdigit
#define isxdigit(i)		xf86isxdigit(i)
#undef labs
#define labs(l)			xf86labs(l)
#undef ldexp
#define ldexp(x, exp)           xf86ldexp(x, exp)
#undef log
#define log(d)			xf86log(d)
#undef log10
#define log10(d)		xf86log10(d)
#undef malloc
#define malloc(I)		xf86malloc(I)
#undef memchr
#define memchr(cvp,i,I)		xf86memchr(cvp,i,I)
#undef memcmp
#define memcmp(cvp1,cvp2,I)	xf86memcmp(cvp1,cvp2,I)
#undef memcpy
#define memcpy(vp,cvp,I)	xf86memcpy(vp,cvp,I)
#undef memmove
#define memmove(vp,cvp,I)	xf86memmove(vp,cvp,I)
#undef memset
#define memset(vp,int,I)	xf86memset(vp,int,I)
#undef modf
#define modf(d,dp)		xf86modf(d,dp)
#undef perror
#define perror(ccp)		xf86perror(ccp)
#undef pow
#define pow(d1,d2)		xf86pow(d1,d2)
#undef random
#define random()		xf86random()
#undef realloc
#define realloc(vp,I)		xf86realloc(vp,I)
#undef remove
#define remove(ccp)		xf86remove(ccp)
#undef rename
#define rename(ccp1,ccp2)	xf86rename(ccp1,ccp2)
#undef rewind
#define rewind(FP)		xf86rewind(FP)
#undef setbuf
#define setbuf(FP,cp)		xf86setbuf(FP,cp)
#undef setvbuf
#define setvbuf(FP,cp,i,I)	xf86setvbuf(FP,cp,i,I)
#undef sin
#define sin(d)			xf86sin(d)
#undef snprintf
#define snprintf		xf86snprintf
#undef sprintf
#define sprintf			xf86sprintf
#undef sqrt
#define sqrt(d)			xf86sqrt(d)
#undef sscanf
#define sscanf			xf86sscanf
#undef strcat
#define strcat(cp,ccp)		xf86strcat(cp,ccp)
#undef strcmp
#define strcmp(ccp1,ccp2)	xf86strcmp(ccp1,ccp2)
#undef strcasecmp
#define strcasecmp(ccp1,ccp2)	xf86strcasecmp(ccp1,ccp2)
#undef strcpy
#define strcpy(cp,ccp)		xf86strcpy(cp,ccp)
#undef strcspn
#define strcspn(ccp1,ccp2)	xf86strcspn(ccp1,ccp2)
#undef strerror
#define strerror(i)		xf86strerror(i)
#undef strlcat
#define strlcat(cp,ccp,I)	xf86strlcat(cp,ccp,I)
#undef strlcpy
#define strlcpy(cp,ccp,I)	xf86strlcpy(cp,ccp,I)
#undef strlen
#define strlen(ccp)		xf86strlen(ccp)
#undef strncmp
#define strncmp(ccp1,ccp2,I)	xf86strncmp(ccp1,ccp2,I)
#undef strncasecmp
#define strncasecmp(ccp1,ccp2,I) xf86strncasecmp(ccp1,ccp2,I)
#undef strncpy
#define strncpy(cp,ccp,I)	xf86strncpy(cp,ccp,I)
#undef strpbrk
#define strpbrk(ccp1,ccp2)	xf86strpbrk(ccp1,ccp2)
#undef strchr
#define strchr(ccp,i)		xf86strchr(ccp,i)
#undef strrchr
#define strrchr(ccp,i)		xf86strrchr(ccp,i)
#undef strspn
#define strspn(ccp1,ccp2)	xf86strspn(ccp1,ccp2)
#undef strstr
#define strstr(ccp1,ccp2)	xf86strstr(ccp1,ccp2)
#undef srttod
#define strtod(ccp,cpp)		xf86strtod(ccp,cpp)
#undef strtok
#define strtok(cp,ccp)		xf86strtok(cp,ccp)
#undef strtol
#define strtol(ccp,cpp,i)	xf86strtol(ccp,cpp,i)
#undef strtoul
#define strtoul(ccp,cpp,i)	xf86strtoul(ccp,cpp,i)
#undef tan
#define tan(d)			xf86tan(d)
#undef tmpfile
#define tmpfile()		xf86tmpfile()
#undef tolower
#define tolower(i)		xf86tolower(i)
#undef toupper
#define toupper(i)		xf86toupper(i)
#undef ungetc
#define ungetc(i,FP)		xf86ungetc(i,FP)
#undef vfprintf
#define vfprintf(p,f,a)		xf86vfprintf(p,f,a)
#undef vsnprintf
#define vsnprintf(s,n,f,a)	xf86vsnprintf(s,n,f,a)
#undef vsprintf
#define vsprintf(s,f,a)		xf86vsprintf(s,f,a)
/* XXX Disable assert as if NDEBUG was defined */
/* Some X headers defined this away too */
#undef assert
#define assert(a)		((void)0)
#undef HUGE_VAL
#define HUGE_VAL		xf86HUGE_VAL

#undef hypot
#define hypot(x,y)		xf86hypot(x,y)

#undef qsort
#define qsort(b, n, s, f)	xf86qsort(b, n, s, f)

/* non-ANSI C functions */
#undef opendir
#define opendir(cp)		xf86opendir(cp)
#undef closedir
#define closedir(DP)		xf86closedir(DP)
#undef readdir
#define readdir(DP)		xf86readdir(DP)
#undef rewinddir
#define rewinddir(DP)		xf86rewinddir(DP)
#undef bcopy
#define bcopy(vp,cvp,I)		xf86memmove(cvp,vp,I)
#undef ffs
#define ffs(i)			xf86ffs(i)
#undef strdup
#define strdup(ccp)		xf86strdup(ccp)
#undef bzero
#define bzero(vp,ui)		xf86bzero(vp,ui)
#undef execl
#define execl	        	xf86execl
#undef chmod
#define chmod(a,b)              xf86chmod(a,b)
#undef chown
#define chown(a,b,c)            xf86chown(a,b,c)
#undef geteuid
#define geteuid                 xf86geteuid
#undef getegid
#define getegid                 xf86getegid
#undef getpid
#define getpid                  xf86getpid
#undef mknod
#define mknod(a,b,c)            xf86mknod(a,b,c)
#undef sleep
#define sleep(a)                xf86sleep(a)
#undef mkdir
#define mkdir(a,b)              xf86mkdir(a,b)
#undef getpagesize
#define getpagesize		xf86getpagesize
#undef shmget
#define shmget(a,b,c)		xf86shmget(a,b,c)
#undef shmat
#define shmat(a,b,c)		xf86shmat(a,b,c)
#undef shmdt
#define shmdt(a)		xf86shmdt(a)
#undef shmctl
#define shmctl(a,b,c)		xf86shmctl(a,b,c)

#undef S_ISUID
#define S_ISUID XF86_S_ISUID
#undef S_ISGID
#define S_ISGID XF86_S_ISGID
#undef S_ISVTX
#define S_ISVTX XF86_S_ISVTX
#undef S_IRUSR
#define S_IRUSR XF86_S_IRUSR
#undef S_IWUSR
#define S_IWUSR XF86_S_IWUSR
#undef S_IXUSR
#define S_IXUSR XF86_S_IXUSR
#undef S_IRGRP
#define S_IRGRP XF86_S_IRGRP
#undef S_IWGRP
#define S_IWGRP XF86_S_IWGRP
#undef S_IXGRP
#define S_IXGRP XF86_S_IXGRP
#undef S_IROTH
#define S_IROTH XF86_S_IROTH
#undef S_IWOTH
#define S_IWOTH XF86_S_IWOTH
#undef S_IXOTH
#define S_IXOTH XF86_S_IXOTH
#undef S_IFREG
#define S_IFREG XF86_S_IFREG
#undef S_IFCHR
#define S_IFCHR XF86_S_IFCHR
#undef S_IFBLK
#define S_IFBLK XF86_S_IFBLK
#undef S_IFIFO
#define S_IFIFO XF86_S_IFIFO

/* some types */
#undef FILE
#define FILE			XF86FILE
#undef fpos_t
#define fpos_t			XF86fpos_t
#undef DIR
#define DIR			XF86DIR
#undef DIRENT
#define DIRENT			XF86DIRENT
#undef size_t
#define size_t			xf86size_t
#undef ssize_t
#define ssize_t			xf86ssize_t
#undef dev_t
#define dev_t                   xf86dev_t
#undef mode_t
#define mode_t                  xf86mode_t
#undef uid_t
#define uid_t                   xf86uid_t
#undef gid_t
#define gid_t                   xf86gid_t
#undef stat_t
#define stat_t			struct xf86stat

#undef ulong
#define ulong			unsigned long

/*
 * There should be no need to #undef any of these.  If they are already
 * defined it is because some illegal header has been included.
 */

/* some vars */
#undef stdin
#define	stdin			xf86stdin
#undef stdout
#define stdout			xf86stdout
#undef stderr
#define stderr			xf86stderr

#undef SEEK_SET
#define SEEK_SET		XF86_SEEK_SET
#undef SEEK_CUR
#define SEEK_CUR		XF86_SEEK_CUR
#undef SEEK_END
#define SEEK_END		XF86_SEEK_END

/*
 * XXX Basic I/O functions BAD,BAD,BAD!
 */
#define open			xf86open
#define close(a)		xf86close(a)
#define lseek(a,b,c)		xf86lseek(a,b,c)
#if !defined(__DragonFly__)
#define ioctl(a,b,c)		xf86ioctl(a,b,c)
#endif
#define read(a,b,c)		xf86read(a,b,c)
#define write(a,b,c)		xf86write(a,b,c)
#define mmap(a,b,c,d,e,f)	xf86mmap(a,b,c,d,e,f)
#define munmap(a,b)		xf86munmap(a,b)
#define stat(a,b)               xf86stat(a,b)
#define fstat(a,b)              xf86fstat(a,b)
#define access(a,b)             xf86access(a,b)
#undef O_RDONLY
#define O_RDONLY		XF86_O_RDONLY
#undef O_WRONLY
#define O_WRONLY		XF86_O_WRONLY
#undef O_RDWR
#define O_RDWR			XF86_O_RDWR
#undef O_CREAT
#define O_CREAT			XF86_O_CREAT
#undef PROT_EXEC
#define PROT_EXEC		XF86_PROT_EXEC
#undef PROT_READ
#define PROT_READ		XF86_PROT_READ
#undef PROT_WRITE
#define PROT_WRITE		XF86_PROT_WRITE
#undef PROT_NONE
#define PROT_NONE		XF86_PROT_NONE
#undef MAP_FIXED
#define MAP_FIXED		XF86_MAP_FIXED
#undef MAP_SHARED
#define MAP_SHARED		XF86_MAP_SHARED
#undef MAP_PRIVATE
#define MAP_PRIVATE		XF86_MAP_PRIVATE
#undef MAP_FAILED
#define MAP_FAILED		XF86_MAP_FAILED
#undef R_OK
#define R_OK                    XF86_R_OK
#undef W_OK
#define W_OK                    XF86_W_OK
#undef X_OK
#define X_OK                    XF86_X_OK
#undef F_OK
#define F_OK                    XF86_F_OK
#undef errno
#define errno			xf86errno
#undef putchar
#define putchar(i)		xf86fputc(i, xf86stdout)
#undef puts
#define puts(s)			xf86fputs(s, xf86stdout)

#undef EACCES
#define EACCES		xf86_EACCES
#undef EAGAIN
#define EAGAIN		xf86_EAGAIN
#undef EBADF
#define EBADF		xf86_EBADF
#undef EEXIST
#define EEXIST		xf86_EEXIST
#undef EFAULT
#define EFAULT		xf86_EFAULT
#undef EINTR
#define EINTR		xf86_EINTR
#undef EINVAL
#define EINVAL		xf86_EINVAL
#undef EISDIR
#define EISDIR		xf86_EISDIR
#undef ELOOP
#define ELOOP		xf86_ELOOP
#undef EMFILE
#define EMFILE		xf86_EMFILE
#undef ENAMETOOLONG
#define ENAMETOOLONG	xf86_ENAMETOOLONG
#undef ENFILE
#define ENFILE		xf86_ENFILE
#undef ENOENT
#define ENOENT		xf86_ENOENT
#undef ENOMEM
#define ENOMEM		xf86_ENOMEM
#undef ENOSPC
#define ENOSPC		xf86_ENOSPC
#undef ENOTDIR
#define ENOTDIR		xf86_ENOTDIR
#undef EPIPE
#define EPIPE		xf86_EPIPE
#undef EROFS
#define EROFS		xf86_EROFS
#undef ETXTBSY
#define ETXTBSY		xf86_ETXTBSY
#undef ENOTTY
#define ENOTTY		xf86_ENOTTY
#undef ENOSYS
#define ENOSYS		xf86_ENOSYS
#undef EBUSY
#define EBUSY		xf86_EBUSY
#undef ENODEV
#define ENODEV		xf86_ENODEV
#undef EIO
#define EIO		xf86_EIO

/* IPC stuff */
#undef SHM_RDONLY
#define SHM_RDONLY XF86SHM_RDONLY
#undef SHM_RND
#define SHM_RND XF86SHM_RND
#undef SHM_REMAP
#define SHM_REMAP XF86SHM_REMAP
#undef IPC_RMID
#define IPC_RMID XF86IPC_RMID
#undef IPC_CREAT
#define IPC_CREAT XF86IPC_CREAT
#undef IPC_EXCL
#define IPC_EXCL XF86IPC_EXCL
#undef PC_NOWAIT
#define IPC_NOWAIT XF86IPC_NOWAIT
#undef SHM_R
#define SHM_R XF86SHM_R
#undef SHM_W
#define SHM_W XF86SHM_W
#undef IPC_PRIVATE
#define IPC_PRIVATE XF86IPC_PRIVATE

/* Some ANSI macros */
#undef FILENAME_MAX
#define FILENAME_MAX		1024

#if (defined(sun) && defined(__SVR4)) 
# define _FILEDEFED /* Already have FILE defined, don't redefine it */
#endif

#endif /* !DONT_DEFINE_WRAPPERS */

#if (!defined(DONT_DEFINE_WRAPPERS) || defined(DEFINE_SETJMP_WRAPPERS))
#undef setjmp
#define setjmp(a)               xf86setjmp_macro(a)
#undef longjmp
#define longjmp(a,b)            xf86longjmp(a,b) 
#undef jmp_buf
#define jmp_buf                 xf86jmp_buf
#endif

#endif /* XF86_LIBC_H */
