/* $Id: machine.h,v 4.11 1999/01/10 01:26:04 ksb Exp $
 * machine depend config						(ksb)
 */
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif

#if !defined(HAS_FCHOWN)
#define HAS_FCHOWN	!(defined(ETA10)||defined(V386))
#endif
#if !defined(HAS_FCHMOD)
#define HAS_FCHMOD	HAS_FCHOWN
#endif

#if !defined(HAS_NDIR)
#define HAS_NDIR	(defined(ETA10)||defined(V386))
#endif

#if !defined(HAS_DIRENT)
#define HAS_DIRENT	(defined(EPIX)||defined(SUN5))
#endif

#if !defined(DIRECT)
#if defined(IBMR2) || defined(EPIX) || defined(SUN5) || defined(PARAGON)
#define DIRECT		dirent
#else
#define DIRECT		direct
#endif
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD)||defined(LINUX))
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if !defined(USE_STDLIB)
#define USE_STDLIB	(defined(IBMR2)||defined(PTX)||defined(FREEBSD)||defined(ULTRIX))
#endif

#if !defined(USE_UNISTD_H)
#define USE_UNISTD_H	(defined(FREEBSD)||defined(HPUX9)||defined(HPUX10)||defined(BSDI))
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX)||defined(ULTRIX))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX)&&!defined(BSDI))
#endif

#if !defined(USE_STRINGS)
#define USE_STRINGS	0
#endif

#if USE_STDLIB || defined(__STDC__)
#include <stdlib.h>
#endif

#if USE_UNISTD_H
#include <unistd.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif
