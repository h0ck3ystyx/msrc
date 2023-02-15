#!mkcmd
# $Id: rcsvg.m,v 2.15 2000/07/30 22:26:03 ksb Exp $
#
# command line options for RCS rev grab -- a snapshot program
# for a product kept under RCS			(ksb@cc.purdue.edu)
#
# N.B.: `version' refers to the product `revision' refers
# to any single source file that composes the product.

%i
static char rcsid[] =
	"$Id: rcsvg.m,v 2.15 2000/07/30 22:26:03 ksb Exp $";

extern int errno;
%%

from '<sys/param.h>'
from '<sys/stat.h>'
from '<sys/file.h>'
from '<errno.h>'
from '<unistd.h>'

from '"machine.h"'
from '"util.h"'

require "std_help.m" "std_version.m" "std_control.m"

basename "rcsvg" ""
getenv "RCSVG"

integer variable "iUmask" {
	before '%n = umask(022);umask(%n);'
}

char* variable "pcVersion" {
	param "version"
	help "the version of the product we want to stage"
}

left "pcVersion" {
	update "/* got them */"
	abort 'fprintf(stderr, "%%s: must provide a version string\\n", %b);'
}

char* 'S' {
	named "pcStage"
	param "stage"
	help "the staging area we should construct/update"
}

boolean 'R' {
	named "fLinkRCS"
	help "build a symbolic link back to each RCS directory"
}

boolean 'm' {
	named "fMode"
	help "take modes from the RCS delta files for destination"
}
boolean 'f' {
	named "fUnlink"
	help "forces delete of out of sync files before checkout"
}

after {
	named "PostInit"
	update "if (%rRn) {%n();}"
}

zero {
	update '%Ren(".");'
}

every {
	param "source"
	named "CopyOut"
	help "a top-level source directory for the product"
}

# bit mask of type of failures found
#	1	RCS/file,v not readable
#	2	found a character or block special file
#	8	found a trash file in RCS
#
integer variable "iRet" {
	init "0"
}

boolean 'c' {
	named "fCmp"
	help "do not change files which compare to previously staged files"
}

exit {
	update "exit(iRet);"
}

from '"machine.h"'

%i
static char acSource[MAXPATHLEN+1], acStage[MAXPATHLEN+1];

static char acLinkPrefix[MAXPATHLEN+1], *pcLinkPrefix = (char *)0;
%%

%c
/* make sure we can lay hands on the path to the source for the RCS	(ksb)
 * links back to the source under -R.
 * If ksh (or any parent) provides a "better" name for the pwd then
 * we should take that path.
 */
void
PostInit()
{
	register char *pcPwd;
	auto struct stat stSys, stEnv;
	extern char *getenv();

	if ((char *)0 == getcwd(acLinkPrefix, sizeof(acLinkPrefix))) {
		fprintf(stderr, "%s: getcwd: %s\n", progname, strerror(errno));
		exit(1);
	}
	if ((char *)0 != (pcPwd = getenv("PWD")) && strlen(pcPwd) < strlen(acLinkPrefix) && -1 != stat(pcPwd, & stEnv) && -1 != stat(acLinkPrefix, & stSys)) {
		if (stSys.st_ino == stEnv.st_ino && stSys.st_dev == stEnv.st_dev) {
			(void)strcpy(acLinkPrefix, pcPwd);
		}
	}
	pcLinkPrefix = acLinkPrefix+strlen(acLinkPrefix);
}

/* In this alphasorted list of (struct direct *) [some nil] find	(ksb)
 * (via a binary search) the named file... return a pointer to
 * the entry so we may remove it in the caller
 *
 * This allows the caller to `shoot' the return array from scandir
 * a few files at a time and still have a list of the files left.
 */
static struct DIRENT **
FindDEnt(pcName, ppDE, n)
char *pcName;
struct DIRENT **ppDE;
int n;
{
	register int i, mid;
	register int cmp;

	if (0 == n)
		return (struct DIRENT **)0;
	for (i = 0; i < n; ++i) {
		if ((struct DIRENT *)0 != ppDE[i])
			break;
	}
	if (n == i) {
		return (struct DIRENT **)0;
	}

	if (0 == strcmp(pcName, (ppDE[i])->d_name)) {
		return & ppDE[i];
	}

	do {
		--n;
		if ((struct DIRENT *)0 != ppDE[n])
			break;
	} while (n > i);
	if (0 == strcmp(pcName, (ppDE[n])->d_name)) {
		return & ppDE[n];
	}

	while (i != n) {
		mid = (n+i)/2;
		while (mid < n && (struct DIRENT *)0 == ppDE[mid])
			++mid;
		if (mid == n) {
			mid = (n+i)/2;
			while (mid > i && (struct DIRENT *)0 == ppDE[mid])
				--mid;
			if (mid == i)
				break;
		}
		cmp = strcmp(pcName, (ppDE[mid])->d_name);
		if (cmp == 0) {
			return & ppDE[mid];
		}
		if (cmp < 0) {
			n = mid;
		} else {
			i = mid;
		}
		if (i+1 == n)
			break;
	}
	return (struct DIRENT **)0;
}


/* blow off ., and ..							(ksb)
 */
int
TlSelect(pDE)
struct DIRENT *pDE;
{
	return !('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] || ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])));
}

/* scan a top level directory for sub directories and RCS caches	(ksb)
 * take apropos action in each case (skip . & .., of course)
 *
 * check to make sure the source directory exists and has /RCS in it.
 * make the stage area
 * scan the files in the source dir and dispatch them
 */
void
TopLevel(pcSTail, pcDTail, pcLinkDest)
char *pcSTail, *pcDTail, *pcLinkDest;
{
	register struct DIRENT *pDE;
	register int i;
	register char *pcRTail;
	register char *pcComma;
	register struct DIRENT **ppDEDink;
	auto int nO, nV, iLen;
	auto struct DIRENT **ppDEOut, **ppDEVees;
	auto char acRCScv[MAXPATHLEN+10];
	auto struct stat stThis, stComVee;
	auto char acLink[MAXPATHLEN+1];
	auto int fSawCache, fMadeRCS;
	extern int alphasort();

	*pcSTail = '\000';
	if (0 != VrfyDir(acSource, 0)) {
		return ;
	}
	(void)strcpy(acRCScv, acSource);
	pcRTail = acRCScv + strlen(acRCScv);
	*pcRTail++ = '/';
	*pcRTail++ = 'R';
	*pcRTail++ = 'C';
	*pcRTail++ = 'S';
	*pcRTail = '\000';
	if (0 != VrfyDir(acRCScv, 0)) {
		fprintf(stderr, "%s: %s has no RCS directory [%s] (warning only)\n", progname, acSource, acRCScv);
		nV = 0;
	} else if (-1 == (nV = scandir(acRCScv, &ppDEVees, TlSelect, alphasort))) {
		fprintf(stderr, "%s: scandir: %s: %s\n", progname, acSource, strerror(errno));
		return;
	}

	*pcDTail = '\000';
	if (0 != VrfyDir(acStage, 1)) {
		return;
	}

	if (-1 == (nO = scandir(acSource, &ppDEOut, TlSelect, alphasort))) {
		fprintf(stderr, "%s: scandir: %s: %s\n", progname, acSource, strerror(errno));
		return;
	}

	*pcDTail++ = '/';
	*pcSTail++ = '/';
	*pcRTail++ = '/';

	/* Looking in the RCS directory first:
	 *	look for non-plain files
	 *	look for non-,v files
	 *	look for file,v we can't read
	 *	loof for "RCS,v" which is an artifact of "rcs -i *", ouch!
	 *	look for ,semaphore files
	 * then copy the son of a gun to the stage area.
	 */
	for (i = 0; i < nV; ++i) {
		(void)strcpy(pcRTail, ppDEVees[i]->d_name);
		if (-1 == stat(acRCScv, &stThis)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, acSource, strerror(errno));
			continue;
		}
		switch (stThis.st_mode & S_IFMT) {
		case 0:
		case S_IFREG:
			/* handled below */
			break;
		case S_IFDIR:
			fprintf(stderr, "%s: %s is a subdirectory of an RCS cache directory (ignored)\n", progname, acRCScv);
			iRet |= 1;
			continue;
		default:
			fprintf(stderr, "%s: %s delta file with funny type\n", progname, acRCScv);
			iRet |= 2;
			continue;
		}
		if ((char *)0 == (pcComma = strrchr(ppDEVees[i]->d_name, ','))) {
			fprintf(stderr, "%s: %s should not live under a RCS directory (no ,v extender)\n", progname, acRCScv);
			iRet |= 8;
			continue;
		}
		if ('v' != pcComma[1] && '\000' != pcComma[2]) {
			fprintf(stderr, "%s: %s should not live under a RCS directory (extender \"%s\")\n", progname, acRCScv, pcComma);
			iRet |= 8;
			continue;
		}
		if (0 != access(acRCScv, R_OK)) {
			fprintf(stderr, "%s: %s cache file is not readable!\n", progname, acRCScv);
			iRet |= 1;
			continue;
		}

		/* is not -R we don't complain, I guess that's OK -- ksb
		 */
		if (fLinkRCS && 0 == strcmp("RCS,v", ppDEVees[i]->d_name)) {
			fprintf(stderr, "%s: %s cache file would clobber RCS backlink\n", progname, acRCScv);
			iRet |= 17;
		}

		/* file is a ,semaphore file if RCS/file,v also exists
		 * I think some versions of RCS must do this with dots.
		 */
		if (',' == ppDEVees[i]->d_name[0]) {
			fprintf(stderr, "%s: %s might be a semaphore file!\n", progname, acRCScv);
		}

		/* find the file in acSource list and rm it
		 */
		*pcComma = '\000';
		(void)strcpy(pcDTail, ppDEVees[i]->d_name);
		ppDEDink = FindDEnt(ppDEVees[i]->d_name, ppDEOut, nO);
		*pcComma = ',';
		if ((struct DIRENT **)0 != ppDEDink) {
			*ppDEDink = (struct DIRENT *)0;
		}
		Extract(acRCScv, acStage, 07777 & stThis.st_mode);
	}

	/* OK look through the files left after we skimmed off the
	 * RCS delta files and check for
	 *	comma-v files not under RCS (Geeze)
	 *	RCS dir to ignore
	 *	sub dirs to look in
	 *	symbolic links to copy [see ExtLink]
	 */
	fMadeRCS = fSawCache = 0;
	pcRTail -= 4;
	for (i = 0; i < nO; ++i) {
		if ((struct DIRENT *)0 == ppDEOut[i]) {
			continue;
		}
		/* if this is an RCS delta file at the wrong level...
		 */
		if ((char *)0 != (pcComma = strrchr(ppDEOut[i]->d_name, ',')) && 'v' == pcComma[1] && '\000' == pcComma[2]) {
			*pcRTail = '\000';
			*pcComma = '\000';
			(void)strcpy(pcDTail, ppDEOut[i]->d_name);
			*pcComma = ',';
		} else {
			pcComma = (char *)0;
			(void)sprintf(pcRTail, "%s,v", ppDEOut[i]->d_name);
			(void)strcpy(pcDTail, ppDEOut[i]->d_name);
		}
		(void)strcpy(pcSTail, ppDEOut[i]->d_name);

		if (-1 == lstat(acSource, &stThis)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, acSource, strerror(errno));
			continue;
		}
		switch (stThis.st_mode & S_IFMT) {
			register char *pcLnText;
			auto char acLCS[MAXPATHLEN+2];
		case 0:
		case S_IFREG:
			/* handled below */
			break;
		case S_IFDIR:
			/* RCS dir is extracted by us, do not decend
			 */
			if (0 != strcmp("RCS", ppDEOut[i]->d_name)) {
				auto char acLinkDest[MAXPATHLEN+4];
				sprintf(acLinkDest, "%s%s", '/' == acSource[0] ? "" : "../", pcLinkDest);
				TopLevel(pcSTail+strlen(pcSTail), pcDTail+strlen(pcDTail), acLinkDest);
				continue;
			}
			if (!fLinkRCS) {
				continue;
			}

			(void)strcpy(pcDTail, "RCS");
			*--pcRTail = '\000';

			if ('.' == acRCScv[0] && ('\000' == acRCScv[1] || '/' == acRCScv[1])) {
				(void)strcpy(pcLinkPrefix, acRCScv+1);
				pcLnText = acLinkPrefix;
			} else {
				pcLnText = acRCScv;
			}
			sprintf(acLCS, "%s%s/RCS", pcLinkDest, pcLnText);

			/* ZZZ read existing link? */
			if (fVerbose) {
				printf("ln -s %s %s\n", acLCS, acStage);
			}
			if (fExec && -1 == symlink(acLCS, acStage)) {
				fprintf(stderr, "%s: symlink: %s to %s: %s\n", progname, acLCS, acStage, strerror(errno));
			}
			*pcRTail++ = '/';
			fMadeRCS = 1;
			continue;
		case S_IFLNK:
			if ((char *)0 != pcComma) {
				fprintf(stderr, "%s: %s is a link to a delta file at the wrong level, save us all\n", progname, acSource);
				continue;
			}
			/* copy the link but check for leading '/'
			 * and '../..' out of scope
			 */
			/* link case */
			iLen = readlink(acSource, acLink, MAXPATHLEN);
			if (-1 == iLen) {
				fprintf(stderr, "%s: readlink: %s: %s\n", progname, acSource, strerror(errno));
				iRet |= 16;
				continue;
			}
			acLink[iLen] = '\000';
			ExtLink(acLink, acStage);
			continue;
		default:
			fprintf(stderr, "%s: %s ignored due to funny node type\n", progname, acSource);
			continue;
		}
		/* plain file case, might be a "file,v" or a "file"
		 */

		/* We found a file,v maybe there is a co'd one as well.
		 * We'll extract the file,v and ignore any co'd copy.
		 */
		if ((char *)0 != pcComma) {
			if (fLinkRCS && 0 == strcmp("RCS,v", ppDEOut[i]->d_name)) {
				fprintf(stderr, "%s: %s cache file would clobber RCS backlink\n", progname, acSource);
				iRet |= 17;
				continue;
			}
			Extract(acSource, acStage, 07777 & stThis.st_mode);
			*pcComma = '\000';
			ppDEDink = FindDEnt(ppDEOut[i]->d_name, ppDEOut, nO);
			*pcComma = ',';
			if ((struct DIRENT **)0 == ppDEDink) {
				*ppDEDink = (struct DIRENT *)0;
			}
			fSawCache = 1;
			continue;
		}

		/* the other order, we found "file" look for "file,v"
		 */
		ppDEDink = FindDEnt(pcRTail, ppDEOut, nO);
		if ((struct DIRENT **)0 == ppDEDink) {
			fprintf(stderr, "%s: %s ignored (no RCS cache)\n", progname, acSource);
			continue;
		}
		*ppDEDink = (struct DIRENT *)0;
		Extract(acRCScv, acStage, 07777 & stThis.st_mode);
		fSawCache = 1;
		continue;
	}

	/* We need an RCS back link and saw a cache file in . and
	 * didn't see an RCS directory.  Make RCS@->here --ksb
	 * If you have a mix of cached,v in . and RCS you loose (can
	 * only see the RCS ones).
	 * [NO!  I'm not putting links in a real directory for you.]
	 */
	*--pcSTail = '\000';
	if (fLinkRCS && fSawCache && !fMadeRCS) {
		auto char acLCS[MAXPATHLEN+2];

		*pcLinkPrefix = '\000';
		(void)strcpy(pcDTail, "RCS");
		if (fVerbose) {
			printf("ln -s %s %s\n", acLinkPrefix, acStage);
		}
		if (fExec && -1 == symlink(acLinkPrefix, acStage)) {
			fprintf(stderr, "%s: symlink: %s to %s: %s\n", progname, acLinkPrefix, acStage, strerror(errno));
		}
	}
	*--pcDTail = '\000';
}

/* start the whole thing off in a top level directory			(ksb)
 */
void
CopyOut(pcSource)
char *pcSource;
{
	if ((char *)0 == pcStage) {
		(void)sprintf(acStage, "/tmp/%s", pcVersion);
	} else {
		(void)strcpy(acStage, pcStage);
	}

	(void)strcpy(acSource, pcSource);
	TopLevel(acSource+strlen(acSource), acStage+strlen(acStage), "");
}
%%
