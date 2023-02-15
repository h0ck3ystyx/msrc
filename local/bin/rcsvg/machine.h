/* leverage for platfrom specific OS features
 * $Id: machine.h,v 2.8 1998/02/12 20:23:40 ksb Exp $
 */

/* do we have scandir,readdir and friends in <sys/dir.h>, else look for
 * <ndir.h>
 */
#if !defined(HAS_NDIR)
#define HAS_NDIR        (defined(ETA10)||defined(V386))
#endif

#if !defined(HAS_DIRENT)
#define	HAS_DIRENT	(defined(HPUX8)||defined(IBMR2)||defined(SUN5))
#endif

/* if we have BSDDIR which name is the type? (struct direct) or (struct dirent)
 */
#if !defined(DIRENT)
#if defined(IBMR2) || defined(HPUX8) || defined(SUN5)
#define DIRENT		dirent
#else
#define DIRENT		direct
#endif
#endif	/* need to scandir */

/* we used to use pD->d_namlen -- under EPIX -systype bsd we still can!
 */
#if !defined(D_NAMELEN)
#if HAS_DIRENT
#define D_NAMELEN(MpD)	((MpD)->d_reclen - DIRENTSIZE(0))
#else
#define D_NAMELEN(MpD)	((MpD)->d_namlen)
#endif
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(SUN5)||defined(NETBSD)||defined(FREEBSD)||defined(LINUX))
#endif

extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if HAS_NDIR
#include <ndir.h>
#else
#if HAS_DIRENT
#if defined(IBMR2)
#include <sys/dir.h>
#else
#include <sys/dirent.h>
#endif
#else
#include <sys/dir.h>
#endif
#endif
