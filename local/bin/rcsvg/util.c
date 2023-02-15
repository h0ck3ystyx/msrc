/* $Id: util.c,v 2.8 1998/01/29 18:19:17 ksb Exp $
 *
 * routines to manage file extraction and directory creation
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include "main.h"
#include "machine.h"

extern int errno;

/* is this path really a directory?  If is doesn't exist mkdir it,	(ksb)
 * if it does exist and is not a directory moan and croak.
 */
int
VrfyDir(pcDir, fBuild)
char *pcDir;
int fBuild;
{
	auto struct stat stDir;

	if (-1 == stat(pcDir, & stDir)) {
		if (ENOENT != errno) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pcDir, strerror(errno));
			return 2;
		}
		if (!fBuild) {
			return 2;
		}
		if (fVerbose) {
			printf("mkdir %s\n", pcDir);
		}
		if (fExec) {
			if (-1 == mkdir(pcDir, 0777)) {
				fprintf(stderr, "%s: mkdir: %s: %s\n", progname, pcDir, strerror(errno));
				return 1;
			}
		}
	} else if (S_IFDIR != (stDir.st_mode & S_IFMT)) {
		fprintf(stderr, "%s: %s is not a directory\n", progname, pcDir);
		return 2;
	}
	return 0;
}

/* check out the file to the named location				(ksb)
 */
int
Extract(pcDelta, pcDest, mMode)
char *pcDelta, *pcDest;
int mMode;
{
	auto char acCmd[MAXPATHLEN*2 + 400];

	if (fCmp) {
		(void)sprintf(acCmd, "co -q -p -r%s %s | cmp -s - %s", pcVersion, pcDelta, pcDest);
		if (fVerbose) {
			printf("%s || {\n", acCmd);
		}
		if (fExec && 0 == system(acCmd)) {
			if (fVerbose) {
				printf("}\n", acCmd);
			}
			return 0;
		}
	}

	if (fUnlink) {
		if (fVerbose) {
			printf("rm -f %s\n", pcDest);
		}
		if (fExec && -1 == unlink(pcDest) && ENOENT != errno) {
			fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcDest, strerror(errno));
		}
	}
	(void)sprintf(acCmd, "exec co -q -p -r%s %s >%s", pcVersion, pcDelta, pcDest);
	if (fVerbose) {
		printf("co -q -p -r%s %s >%s\n", pcVersion, pcDelta, pcDest);
	}
	mMode &= ~iUmask;
	if (fVerbose && fMode && 0 != (mMode &~ 0111)) {
		printf("chmod %04o %s\n", mMode, pcDest);
	}
	if (fCmp && fVerbose) {
		printf("}\n", acCmd);
	}
	if (!fExec) {
		return 0;
	}
	if (0 == system(acCmd)) {
		if (fMode && 0 != (mMode &~ 0111) && -1 == chmod(pcDest, mMode)) {
			fprintf(stderr, "%s: chmod: %s: %s\n", progname, pcDest, strerror(errno));
			return 1;
		}
		return 0;
	}

	/* revision not found?
	 */
	if (fVerbose) {
		printf("rm -f %s\n", pcDest);
	}
	if (fExec && -1 == unlink(pcDest)) {
		fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcDest, strerror(errno));
	}
	return 1;
}

/* we found a symbolic link to replicate, but it might pass through	(ksb)
 * a fixed path (/...) or climb out of the subtree we are replicating
 */
int
ExtLink(pcLinkText, pcWhere)
char *pcLinkText, *pcWhere;
{
	auto struct stat stLink;
	auto char acHas[MAXPATHLEN+1];
	auto int iLen;

	if (-1 != lstat(pcWhere, &stLink)) {
		if (S_IFLNK == (stLink.st_mode & S_IFMT) && -1 != (iLen = readlink(pcWhere, acHas, MAXPATHLEN))) {
			acHas[iLen] = '\000';
			if (0 == strcmp(acHas, pcLinkText))
				return 0;
		}
		if (fVerbose) {
			printf("rm -f %s\n", pcWhere);
		}
		if (fExec && -1 == unlink(pcWhere)) {
			fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcWhere, strerror(errno));
			return 1;
		}
	}
	if ('/' == pcLinkText[0]) {
		fprintf(stderr, "%s: %s: symbolic link runs through root\n", progname, pcWhere);
	}
	/* XXX check ... count -- req. recursive counter and stuff
	 */
	if (fVerbose) {
		printf("ln -s '%s' %s\n", pcLinkText, pcWhere);
	}
	if (fExec) {
		if (-1 == symlink(pcLinkText, pcWhere)) {
			fprintf(stderr, "%s: symlink: %s: %s\n", progname, pcWhere, strerror(errno));
			return 1;
		}
	}
	return 0;
}
