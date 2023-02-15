/*
 * $Id: distrib.c,v 4.29 1999/06/20 09:55:45 ksb Exp $
 *
 * Copyright 1990, 1992 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

#ifndef lint
static char *rcsid =
	"$Id: distrib.c,v 4.29 1999/06/20 09:55:45 ksb Exp $";
#endif /* lint */

/*
 * distrib is a front end for rdist.  distrib reads a configuration file
 * to find out hosts exist, and to find out what type of CPUs these
 * hosts have.  distrib can update a single host, hosts of a certain
 * type, or all hosts listed in the configuration file.  By default,
 * distrib will update all hosts with the same type as the current host.
 *
 * distrib captures these macors for internal use:
 *	RDIST_PATH	the path to rdist for this target
 *	RDISTD_PATH	the -p we must force to rdist
 *	RSH_PATH	the -P path to the remote shell (for rdist)
 * (values are read from the environment for defaults, for the _PATHs)
 *
 * distrib uses m4 to pass the following macros to rdist for use
 * in Distfiles:
 *	HOST		the host to update
 *	SHORTHOST	abbreviated hostname (not use by distrib)
 *	HOSTTYPE	the type of host to update
 *	HOSTOS		an identifier or integer
 *	HASSRC		does the target machine have a /usr/src?
 *
 *	MYTYPE		the type of host running this distrib
 *	MYOS		the OS version of this host
 *
 * The use of m4 also allows the Distfiles to include() other Distfiles.
 *
 *
 * Compile this with -DDEBUG for debugging output.  See the manual page
 * distrib(8L) for more information on the usage of distrib.
 *
 *	Michael J. Spitzer			Kevin S. Braunsdorf
 *	mjs@sequent.com				ksb@fedex.com
 *	Purdue University Computing Center	FedEx
 *	July, 1987				Oct, 1996
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>

#include "machine.h"

#if HAS_NDIR
#include <ndir.h>
#else
#if HAS_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#include "envlist.h"
#include "distrib.h"
#include "main.h"


static char
	acNoMem[] =			"%s: out of memory\n",
	acTemp[MAXPATHLEN+2],		/* a temp dir in /tmp/fdist...	*/
	**ppcRdist;			/* argv for rdist		*/
static int
	fRmDistfile = 0,		/* remove distfile when done	*/
	iFilePos,			/* where the file goes		*/
	iArgLen;			/* the lenght of ppcRdist	*/
static ENVLIST
	*pELCmd;			/* -D's from the command line	*/

static int
	fDebug = 0;			/* should have a debug option	*/

#define MINARGV		32		/* min argv to build for rdist	*/
#define GROWARGV	16		/* every 16 -d options we grow	*/
#define RESARGV		16		/* what Update needs to work	*/


/* create a temp file for the given name				(ksb)
 * return the file descriptor
 */
int
TempFile(pcFile, pcWhere)
char *pcFile;
char *pcWhere;
{
	register int iDir;
	register char *pcEnd;
	auto struct stat stTmp;

	(void)strcpy(pcWhere, acTemp);
	pcEnd = pcWhere + strlen(pcWhere);
	for (iDir = 1; iDir < 32768; ++iDir) {
		(void)sprintf(pcEnd, "/%x/%s", iDir, pcFile);
		if (0 != stat(pcWhere, &stTmp)) {
			break;
		}
	}
	if (32768 == iDir) {
		fprintf(stderr, "%s: I cannot believe how many dirs this file (%s) needs, so I quit.\n", progname, pcFile);
		exit(1);
	}
	(void)sprintf(pcEnd, "/%x", iDir);
	(void)mkdir(pcWhere, 0775);
	(void)sprintf(pcEnd, "/%x/%s", iDir, pcFile);

	return open(pcWhere, O_CREAT|O_RDWR, 0666);
}

/* cleanup our mess in /tmp/fdist...					(ksb)
 * if this fails someone else will rm them for us, so we ignore errors
 */
int
Cleanup(fRm)
int fRm;
{
	register struct DIRECT *dp;
	register DIR *pDITemp;
	register int iDir;
	auto char *pcLocal, *pcDeep;
	auto char acScan[MAXPATHLEN+1];

	(void)strcpy(acScan, acTemp);
	pcLocal = acScan+strlen(acScan);
	for (iDir = 1; iDir < 32768; ++iDir) {
		sprintf(pcLocal, "/%x/", iDir);
		pcDeep = pcLocal + strlen(pcLocal);

		if (0 == (pDITemp = opendir(acScan))) {
			break;
		}
		while (NULL != (dp = readdir(pDITemp))) {
			if ('.' == dp->d_name[0] && ('\000' == dp->d_name[1] ||
			   ('.' == dp->d_name[1] && '\000' == dp->d_name[2]))) {
				continue;
			}
			(void)strcpy(pcDeep, dp->d_name);
			(void)unlink(acScan);
		}
		(void)closedir(pDITemp);
		if (fRm) {
			*--pcDeep = '\000';
			(void)rmdir(acScan);
		}
	}
	*pcLocal = '\000';
	if (fRm) {
		if (fRmDistfile) {
			(void)unlink(pcDistfile);
		}
		(void)rmdir(acScan);
	}

	return 0;
}

/* the user gave us a rdist option to pass, save it			(ksb)
 */
int
KeepDef(pcDef)
char *pcDef;
{
	if (iFilePos >= iArgLen) {
		iArgLen += GROWARGV;
		ppcRdist = (char **)realloc((char *)ppcRdist, sizeof(char *)*iArgLen);
	}
	if ((char **)0 == ppcRdist) {
		fprintf(stderr, acNoMem, progname);
		exit(3);
	}
	ppcRdist[iFilePos++] = pcDef;
	return 0;
}

static char *apcKidM4[] =	/* argv for m4 in _kids_	*/
	{ (char *)0, "m4", "-", (char *)0, (char *)0 };

/* keep an m4 variable for the user					(ksb)
 * of course the config file overrides us, which is wrong LLL
 */
/*ARGSUSED*/
int
KeepM4(_, pcVar)
int _;
char *pcVar;
{
	register char *pcValue, *pcMem;

	if ((char *)0 == (pcValue = strchr(pcVar, '='))) {
		pELCmd = PutEList(pELCmd, pcVar, "`\'");
	} else if ('.' == pcValue[1] && '\000' == pcValue[2]) {
		*pcValue = '\000';
		pELCmd = PutEList(pELCmd, pcVar, (char *)0);
		*pcValue = '=';
	} else {
		*pcValue++ = '\000';
		pcMem = malloc(strlen(pcVar)+1);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acNoMem, progname);
			exit(1);
		}
		pELCmd = PutEList(pELCmd, strcpy(pcMem, pcVar), pcValue);
		*--pcValue = '=';
	}

	return 0;
}

/* build a safe place in /tmp to make special files			(ksb)
 */
int
Init(pcM4Path, pcRdistPath)
char *pcM4Path, *pcRdistPath;
{
	register char *pcEnvPath;

	pELCmd = (ENVLIST *)0;
	(void)strcpy(acTemp, "/tmp/fdistXXXXXX");
	if ((char *)0 == mktemp(acTemp)) {
		fprintf(stderr, "%s: mktemp failed\n", progname);
		exit(4);
	}

	/* N.B.: use umask in your shell, don't change this -- ksb
	 */
	if (0 != mkdir(acTemp, 0777)) {
		fprintf(stderr, "%s: mkdir: %s: %s\n", progname, acTemp, strerror(errno));
		exit(5);
	}
	if ((char *)0 == (pcEnvPath = getenv("M4_PATH"))) {
		pcEnvPath = pcM4Path;
	}
	apcKidM4[0] = pcEnvPath;
	iFilePos = 0;
	iArgLen = MINARGV;
	if ((char **)0 == (ppcRdist = (char **)calloc(iArgLen, sizeof(char *)))) {
		fprintf(stderr, acNoMem, progname);
		exit(3);
	}
	if ((char *)0 == (pcEnvPath = getenv("RDIST_PATH"))) {
		pcEnvPath = pcRdistPath;
	}
	KeepDef(pcEnvPath);
	KeepDef("rdist");
	KeepDef("-f");
	KeepDef("-");
	return 0;
}

/* just pump the data from one place to another, whole thing		(ksb)
 */
static void
CopyFp(fpFrom, fpTo)
FILE *fpFrom, *fpTo;
{
	register int n;
	auto char acCopy[4096];	/* distfiles are short, usually */

	while (0 < (n = fread(acCopy, sizeof(char), sizeof(acCopy), fpFrom))) {
		if (n != fwrite(acCopy, sizeof(char), n, fpTo)) {
			fprintf(stderr, "%s: fwrite: %s\n", progname, strerror(errno));
			exit(9);
		}
	}
}

/* get a line from the stream, grow the buffer if you have to		(ksb)
 */
static char *
GetLine(ppcBuf, puLen, fpFrom)
char **ppcBuf;
unsigned *puLen;
FILE *fpFrom;
{
	register unsigned uCursor;
	register char *pcStart, *pcChop;

	pcStart = ppcBuf[0];
	uCursor = 0;
	while ((char *)0 != fgets(& pcStart[uCursor], puLen[0]-uCursor, fpFrom)) {
		uCursor += strlen(pcStart+uCursor);
		if ((char *)0 != (pcChop = strchr(pcStart, '\n'))) {
			*pcChop = '\000';
			break;
		}
		if (uCursor+1 >= puLen[0]) {
			puLen[0] += 1024;
			*ppcBuf = pcStart = realloc(pcStart, puLen[0]);
		}
	}
	return uCursor == 0 ? (char *)0 : pcStart;
}


/* convert a C string into it's in-core character			(ksb)
 */
char *
CStr(pcDest, pcSrc)
char *pcDest, *pcSrc;
{
	register int num, i;

	while ('\000' != *pcSrc && '\"' != *pcSrc) switch (*pcSrc) {
	default:
		*pcDest++ = *pcSrc++;
		break;
	case '\\':
		switch (*++pcSrc) {
		case '\n':	/* how would this happen? */
			++pcSrc;
			break;
		case 'n':	/* newline */
			*pcDest++ = '\n';
			++pcSrc;
			break;
		case 't':
			*pcDest++ = '\t';
			++pcSrc;
			break;
		case 'b':
			*pcDest++ = '\b';
			++pcSrc;
			break;
		case 'r':
			*pcDest++ = '\r';
			++pcSrc;
			break;
		case 'f':
			*pcDest++ = '\f';
			++pcSrc;
			break;
		case 'v':
			*pcDest++ = '\013';
			++pcSrc;
			break;
		case '\\':
			++pcSrc;
		case '\000':
			*pcDest++ = '\\';
			break;

		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			num = *pcSrc++ - '0';
			for (i = 0; i < 2; i++) {
				if (! isdigit(*pcSrc)) {
					break;
				}
				num <<= 3;
				num += *pcSrc++ - '0';
			}
			*pcDest++ = num;
			break;
		case '8': case '9':
			/* 8 & 9 are bogus octals,
			 * cc makes them literals
			 */
			/*fallthrough*/
		default:
			*pcDest++ = *pcSrc++;
			break;
		}
	}
	*pcDest = '\000';
	return pcSrc;
}

#define strsave(Mpc) strcpy(malloc(strlen((Mpc))+1),(Mpc))

static char
#if defined(DEF_HOSTTYPE)
	acDefHosttype[] = DEF_HOSTTYPE,
#endif
	acTokHost[] = "HOST",
	acTokShort[] = "SHORTHOST",
	acTokHosttype[] = "HOSTTYPE",
	acTokHostos[] = "HOSTOS",
	acTokHassrc[] = "HASSRC",
	acTokMytype[] = "MYTYPE",
	acTokMyOS[] = "MYOS";

static char *pcTemplate;

/* change the column headers from to a new set.				(ksb)
 * used to remove `HOSTOS' and `HASSRC' in some files in place of
 * something useful.  Distrib would be a *LOT* better if we had
 * though of this before we got started! -- ksb
 *	%HOST FILE DEST
 * might be useful under -a
 */
static void
SetTemplate(pcLine)
char *pcLine;
{
	register char *pcMem, *pcCur;
	register int iState;

	if ((char *)0 == (pcMem = strsave(pcLine))) {
		fprintf(stderr, acNoMem, progname);
		exit(1);
	}
	pcTemplate = pcCur = pcLine = pcMem;

	++pcCur;
	for (iState = ' '; '\000' != (*pcLine = *pcCur); ++pcCur) {
		if (isspace(*pcLine)) {
			if (' ' != iState) {
				*pcLine++ = '\000';
				iState = ' ';
			}
			continue;
		}
		iState = '_';
		++pcLine;
	}
	*pcLine++ = '\000';
	if (' ' != iState) {
		*pcLine = '\000';
	}
}

/* set the column header'd macros					(ksb)
 * they are stored '\000' separated in pcTemplate
 */
ENVLIST *
ParseLine(pELBase, pcLine, pcFrom, iLine)
ENVLIST *pELBase;
char *pcLine, *pcFrom;
int iLine;
{
	register char *pcMacro, *pcValue;
	register char c;

	for (pcMacro = pcTemplate; '\000' != *pcLine && '\000' != *pcMacro; pcMacro += strlen(pcMacro)+1) {
		while (isspace(*pcLine))
			++pcLine;
		pcValue = pcLine;
		if ('\"' == *pcLine) {
			++pcValue;
			do
				++pcLine;
			while ('\000' != *pcLine && '\"' != *pcLine);
			if ('\"' != *pcLine) {
				fprintf(stderr, "%s: %s(%d) missing double quote(\")\n", progname, pcFrom, iLine);
				exit(1);
			}
		} else {
			do
				++pcLine;
			while ('\000' != *pcLine && !isspace(*pcLine));
		}
		c = *pcLine;
		*pcLine = '\000';
		if ('\"' != c && '.' == pcValue[0] && '\000' == pcValue[1]) {
			pELBase = PutEList(pELBase, pcMacro, (char *)0);
		} else {
			pELBase = PutEList(pELBase, pcMacro, strsave(pcValue));
		}
		if ('\000' != c)
			++pcLine;
	}
	if ('\n' == *pcLine) {
		++pcLine;
	}
	if ('\000' != *pcLine) {
		fprintf(stderr, "%s: %s(%d) extra junk on the end of the line (%s)\n", progname, pcFrom, iLine, pcLine);
	}
	return pELBase;
}

/* debug a host with this						(ksb)
 */
static int
Dump(pcVar, pcValue)
char *pcVar, *pcValue;
{
	printf("\t%s=%s\n", pcVar, pcValue);
	return 0;
}

/* either open the file given, or search a single alternate path	(ksb)
 * LLL this should use a colon sep list like $PATH
 */
static FILE *
EitherOpen(pcBuf, pcFile)
char *pcBuf, *pcFile;
{
	register FILE *fp;
#ifdef ALT_CONFIG_PATH
	static char
		acAltPath[] = ALT_CONFIG_PATH;
#endif

	(void)strcpy(pcBuf, pcFile);
	if ((FILE *)0 != (fp = fopen(pcBuf, "r"))) {
		return fp;
	}
	if (ENOENT != errno) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcBuf, strerror(errno));
	}
	*pcBuf = '\000';
#ifdef ALT_CONFIG_PATH
	/* don't put a prefix on a full path
	 */
	if ('/' == pcFile[0] || ('.' == pcFile[0] && '/' == pcFile[1])) {
		return fp;
	}
	sprintf(pcBuf, "%s/%s", acAltPath, pcFile);
	if ((FILE *)0 != (fp = fopen(pcBuf, "r"))) {
		return fp;
	}
	if (ENOENT != errno) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcBuf, strerror(errno));
	}
	*pcBuf = '\000';
#endif
	return (FILE *)0;
}

static void McfReplace();
static char acConfig[MAXPATHLEN+1];

/* Read the configuration file into a HOSTLIST table.  The config
 * file is set up like acDefTemplate says.
 */
static void
ReadConfig(ppHL)
HOSTLIST
	**ppHL;				/* thread pointer		*/
{
	register FILE
		*fpConfig;		/* fp for configuration file	*/
	register char
		*pcVar,			/* variable we assign to	*/
		*pcValue,		/* value after \expands		*/
		*pcEq;			/* value in quotes		*/
	auto int
		iLine;			/* which line in the config file*/
	auto ENVLIST
		*pELEnv;		/* the ENVLIST we built		*/
	static char
		*pcLine = (char *)0;	/* line buffer			*/
	static unsigned
		uBufLen = 0;		/* line buffer size		*/
	static char
		acDefCfg[] = CONFIGFILE,	/* default distrib.cf	*/
		acDefMCfg[] = MCONFIGFILE;	/* or meta config name	*/

	fpConfig = (FILE *)0;
	if ((char *)0 == pcConfig) {
		fpConfig = EitherOpen(acConfig, acDefMCfg);
	}
	if ((FILE *)0 == fpConfig) {
		register char *pc;
		pc = pcConfig ? pcConfig : acDefCfg;
		if ((FILE *)0 == (fpConfig = EitherOpen(acConfig, pc))) {
			fprintf(stderr, "%s: config: %s: not found\n", progname, pc);
			exit(2);
		}
	}

	/* if the file we just opened ends in ".mcf" then we should	(ksb)
	 * push it through m4 with our HOSTTYPE defined as MYTYPE.
	 * Check only the tail of the full path, of course.
	 */
	if ((char *)0 == (pcEq = strrchr(acConfig, '/')))
		pcEq = acConfig;
	if ((char *)0 != (pcEq = strrchr(pcEq, '.')) && 0 == strcmp(pcEq+1, "mcf")) {
		McfReplace(fpConfig);
	}

	if ((char *)0 == pcLine) {
		uBufLen = MAXLINELEN+1;
		pcLine = calloc(uBufLen, sizeof(char));
	}
	pELEnv = SaveEList(pELCmd);
	iLine = 0;
	while (!feof(fpConfig) && !ferror(fpConfig)) {
		if (GetLine(& pcLine, & uBufLen, fpConfig) == (char *)0) {
			break;
		}
		++iLine;

		if ('#' == pcLine[0] || pcLine[0] == '\n') {
			continue;
		}
		/* redo template if we find % in column 1
		 */
		if ('%' == pcLine[0]) {
			SetTemplate(pcLine);
			continue;
		}
		/* parse `VAR=value VAR=value' lines
		 */
		if ((char *)0 != (pcEq = strchr(pcLine, '='))) {
			pcVar = pcLine;
			while ('\000' != *pcVar) {
				do {
					*pcEq++ = '\000';
				} while (isspace(*pcEq));

				switch (*pcEq) {
				case '.':	/* unset */
					pELEnv = PutEList(pELEnv, pcVar, (char *)0);
					*pcEq++ = '\000';
					break;
				case '\"':	/* value */
					pcValue = malloc(strlen(pcEq)+1);
					pcEq = CStr(pcValue, pcEq+1);
					if ('\"' != *pcEq++) {
						fprintf(stderr, "%s: %s(%d) missing close quote in assignment to %s\n", progname, acConfig, iLine, pcVar);
						exit(7);
					}
					pELEnv = PutEList(pELEnv, strsave(pcVar), pcValue);
					break;
				case '\000':
					fprintf(stderr, "%s: %s(%d) use `%s=.\' to unset a variable\n", progname, acConfig, iLine, pcVar);
					exit(1);
				default:
					fprintf(stderr, "%s: %s(%d) missing double quotes\n", progname, acConfig, iLine);
					exit(2);
				}
				for (pcVar = pcEq; isspace(*pcVar); ++pcVar)
					;
				pcEq = strchr(pcVar, '=');
				if ((char *)0 == pcEq && '\000' != *pcVar) {
					fprintf(stderr, "%s: %s(%d) trailing junk after `%s'\n", progname, acConfig, iLine, pcVar);
					exit(10);
				}
			}
			continue;
		}
		if (((*ppHL) = (HOSTLIST *) malloc(sizeof(HOSTLIST))) == 0) {
			fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
			exit(7);
		}

		(*ppHL)->pELmy = SaveEList(ParseLine(pELEnv, pcLine, acConfig, iLine));
		if (fDebug) {
			printf("%s(%d):\n", acConfig, iLine);
			ScanEList((*ppHL)->pELmy, Dump);
		}
		ppHL = & (*ppHL)->pHLnext;
	}

	*ppHL = (HOSTLIST *)0;

	if (ferror(fpConfig)) {
		fprintf(stderr, "%s: ferror: %s: %s\n", progname, acConfig, strerror(errno));
		exit(7);
	}
	(void)fclose(fpConfig);
}

/* find the HostList record in the global list				(ksb)
 * Returns (HOSTLIST *)0 if it cannot find the host in the table.
 */
static HOSTLIST *
LookUp(Hosts, pcHostName)
HOSTLIST *
	Hosts;
char *
	pcHostName;
{
	register HOSTLIST *pHLCur;	/* current host table entry	*/
	register char *pcThis;

	for (pHLCur = Hosts; (HOSTLIST *)0 != pHLCur; pHLCur = pHLCur->pHLnext) {
		if ((char *)0 == (pcThis = GetEList(pHLCur->pELmy, acTokHost)))
			continue;
		if (0 == strcmp(pcThis, pcHostName))
			return pHLCur;
	}
	return (HOSTLIST *)0;
}

/* Our host is not in the config file under the name gethostname(2)	(ksb)
 * returned.  Look to see if we are the first component of one of
 * the hosts in the file. (EPIX needs this, BTW)
 *
 * If we are change that name to our name so the default loop works.
 */
static HOSTLIST *
FallBack(Hosts, pcHostName)
HOSTLIST *Hosts;
char *pcHostName;
{
	register HOSTLIST
		*pHLFound,	/* theone we think looks ok		*/
		*pHLScan;	/* current host table entry		*/
	register int l;
	register char *pcThis;

	l = strlen(pcHostName);
	pHLFound = (HOSTLIST *)0;
	for (pHLScan = Hosts; pHLScan != (HOSTLIST *)0; pHLScan = pHLScan->pHLnext) {
		if ((char *)0 == (pcThis = GetEList(pHLScan->pELmy, acTokHost))) {
			continue;
		}
		if (0 != strncmp(pcHostName, pcThis, l) ||
		    !('\000' == pcThis[l] || '.' == pcThis[l])) {
			continue;
		}
		/* found (another?) one
		 */
		if ((HOSTLIST *)0 != pHLFound) {
			fprintf(stderr, "%s: more than on match for `%s\' in host list (%s and %s)\n", progname, pcHostName, pcThis, GetEList(pHLFound->pELmy, acTokHost));
			return (HOSTLIST *)0;
		}
		pHLFound = pHLScan;
	}
	if ((HOSTLIST *)0 == pHLFound) {
		return (HOSTLIST *)0;
	}
	return pHLFound;
}

/*
 * if a string is any of a list, list with pipes, colons or commas	(ksb)
 * return 0 if it *is* in the list, non-zero otherwise.
 * we are case in sensitive, btw.
 */
int
AnyOf(pcElem, pcList)
char *pcElem;
char *pcList;
{
	register char *pcLook;
	register int cmp, ch;

	if ((char *)0 == (pcLook = pcList)) {
		return 1;
	}
	while ('\000' != *pcList) {
		while (',' != *pcLook && '\000' != *pcLook && ':' != *pcLook && '|' != *pcLook) {
			++pcLook;
		}
		ch = *pcLook;
		*pcLook = '\000';
		cmp = strcasecmp(pcElem, pcList);
		if ('\000' != (*pcLook = ch)) {
			++pcLook;
		}
		pcList = pcLook;
		if (0 == cmp) {
			return 0;
		}
	}
	return 1;
}

/* output the given string quoted from the shell			(ksb)
 * (we are in double quotes)
 */
static void
OutQuoted(fp, pcOut, pcQuote)
FILE *fp;			/* the file to output the text to	*/
char *pcOut;			/* what to output			*/
char *pcQuote;			/* how to quote each character		*/
{
	if ((char *)0 == pcQuote)
		pcQuote = "";
	for (;;) { switch (*pcOut) {
		case '\000':
			return;
		case '$':
		case '`':
		case '\"':
		case '\\':
			fprintf(fp, "%s", pcQuote);
			/* fall through */
		default:
			(void)putc(*pcOut++, fp);
			continue;
		}
	}
}


/* output "define(M1,V1)define(M2,V2)...dnl" for this machine		(ksb)
 */
static void
DoDefs(fp, pEL, pHLFrom, pcQuote)
FILE
	*fp;			/* the file to write defines to		*/
ENVLIST
	*pEL;			/* the host macros to send		*/
HOSTLIST
	*pHLFrom;		/* the type of machine from		*/
char
	*pcQuote;		/* how to output special chars		*/
{

	if ((char *)0 == pcQuote) {
		pcQuote = "";
	}

	if ((HOSTLIST *)0 != pHLFrom) {
		register char *pcG;
		pcG = GetEList(pHLFrom->pELmy, acTokHosttype);
		if ((char *)0 != pcG) {
			fprintf(fp, "define(%s,%s`%s\')", acTokMytype, pcQuote, pcG);
#if defined(DEF_HOSTTYPE)
		} else {
			fprintf(fp, "define(%s,%s`%s\')", acTokMytype, pcQuote, acDefHosttype);
#endif
		}
		pcG = GetEList(pHLFrom->pELmy, acTokHostos);
		if ((char *)0 != pcG) {
			fprintf(fp, "define(%s,%s`%s\')", acTokMyOS, pcQuote, pcG);
		}
#if defined(DEF_HOSTTYPE)
	} else {
		fprintf(fp, "define(%s,%s`%s\')", acTokMytype, pcQuote, acDefHosttype);
#endif
	}
	for (/* pEL */; (ENVLIST *)0 != pEL; pEL = pEL->pELnext) {
		if ((char *)0 == pEL->pcvalue)
			continue;
		fprintf(fp, "define(");
		OutQuoted(fp, pEL->pcvar, pcQuote);
		fprintf(fp, ",");
		OutQuoted(fp, pEL->pcvalue, pcQuote);
		fprintf(fp, ")");
	}
}

/* A more generic version of IsSource above, and much more expensive	(ksb)
 * -tSUN5 -G "eval(HOSTOS > 20601)"
 */
static int
IsGuarded(pHL, pcTest, pHLMe)
HOSTLIST *pHL, *pHLMe;
char *pcTest;
{
	auto int aiFd[2];
	auto char acAns[32];
	register int iPid, iCc, fRet;

	fflush(stdout);
	fflush(stderr);
 	if (-1 == pipe(aiFd)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		exit(10);
	}

	switch (iPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(10);
	default:
		close(aiFd[1]);
		/* read the results from the child m4
		 * the empty string means kill the host, strangely
		 */
		fRet = 0;
		while (0 < (iCc = read(aiFd[0], acAns, sizeof(acAns)))) {
			register int i;
			/* anything but spaces and "0"s means it is OK
			 */
			for (i = 0; i < iCc; ++i) {
				if (fVerbose) {
					putc(acAns[i], stderr);
				}
				if (isspace(acAns[i]) || '\n' == acAns[i])
					continue;
				if ('0' == acAns[i])
					continue;
				fRet = 1;
			}
		}
		close(aiFd[0]);
		wait((void *)0);
		return fRet;
	case 0:
		break;
	}
	close(aiFd[0]);
	if (1 != aiFd[1]) {
		(void)dup2(aiFd[1], 1);
		close(aiFd[1]);
	}
	if (fVerbose) {
		fprintf(stderr, "%s: echo \"", progname);
		DoDefs(stderr, pHL->pELmy, pHLMe, "\\");
		OutQuoted(stderr, pcTest, "\\");
		fprintf(stderr, "\" | m4 - |me\n");
		fflush(stderr);
	}
	Child(apcKidM4, pHL, pHLMe);
	DoDefs(stdout, pHL->pELmy, pHLMe, (char *)0);
	fprintf(stdout, "%s", pcTest);
	fflush(stdout);
	fclose(stdout);
	wait((void *)0);
	exit(0);
	/*NOTREACHED*/
}

/* fake a command like							(ksb)
 * ( echo define(MYTYPE,_) ; cat old-fpConfig ) | m4 | new-fpConfig
 * with the known open (FILE*) fpConfig
 * In the parent process we replace the config file descriptor
 * with a pipe that connects to the new child which pumps the
 * config file through a slaved m4.
 */
static void
McfReplace(fpConfig)
FILE *fpConfig;
{
	auto int aiFd[2];
	register int iPid;

	fflush(stdout);
	fflush(stderr);
 	if (-1 == pipe(aiFd)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		exit(10);
	}

	switch (iPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(10);
	default:
		if (fileno(fpConfig) != dup2(aiFd[0], fileno(fpConfig))) {
			fprintf(stderr, "%s: dup2: %d to %d: %s\n", progname, aiFd[0], fileno(fpConfig), strerror(errno));
			exit(10);
		}
		close(aiFd[0]);
		close(aiFd[1]);
		return;
	case 0:
		break;
	}
	(void)dup2(aiFd[1], 1);
	close(aiFd[1]);
	close(aiFd[0]);
	if (fVerbose) {
		fprintf(stderr, "%s: echo \"", progname);
		DoDefs(stderr, pELCmd, (HOSTLIST *)0, "\\");
		fprintf(stderr, "\" | m4 - %s | me\n", acConfig);
	}
	/* N.B. the nil pointer are OK because we don't call Filter here
	 */
	Child(apcKidM4, (HOSTLIST *)0, (HOSTLIST *)0);
	DoDefs(stdout, pELCmd, (HOSTLIST *)0, (char *)0);
	CopyFp(fpConfig, stdout);
	fflush(stdout);
	fclose(stdout);
	wait((void *)0);
	exit(0);
	/*NOTREACHED*/
}


/* filter the stream (stdin -> stdout) according to the command given	(ksb)
 *  supported filters:
 *	pass	no filter, charout = charin
 *	@file@	replace file name with a m4 processed version
 */
int
Filter(ppcCmd, pHL, pHLFrom)
char
	**ppcCmd;			/* command to filter for	*/
HOSTLIST
	*pHL,				/* the host to send		*/
	*pHLFrom;			/* the type of machine from	*/
{
	register int c;

	if (0 == strcmp("pass", ppcCmd[0])) {
		while (EOF != (c = getchar())) {
			putchar(c);
		}
		return 0;
	}
	if (0 == strcmp("@file@", ppcCmd[0])) {
		auto char acTarget[MAXPATHLEN+2], acThis[MAXPATHLEN+2];
		register int i;
		register char *pcTail;
		auto int iChPid, iPid;
		auto int bHasRoot;
		auto struct stat stTarget;
		auto int fdFile;

		bHasRoot = 0 == geteuid();
		while (EOF != (c = getchar())) {
			if ('@' != c) {
				putchar(c);
				continue;
			}
			i = 0;
			while('@' != (c = getchar()) && EOF != c && '\n' != c) {
				if (MAXPATHLEN == i) {
					fprintf(stderr, "%s: filter: path name too long `%.40s...\n", progname, acTarget);
					exit(4);
				}
				acTarget[i++] = c;
			}
			acTarget[i] = '\000';
			if ('\n' == c) {		/* no match on line */
				printf("@%s\n", acTarget);
				continue;
			}
			if (0 == i) {
				putchar('@');
				continue;
			}
			if ((char *)0 != (pcTail = strrchr(acTarget, '/'))) {
				++pcTail;
			} else {
				pcTail = acTarget;
			}
			fdFile = TempFile(pcTail, acThis);

			(void)fflush(stdout);

			/* <defs> | m4 - $TARGET >$TEMP */
			switch (iPid = fork()) {
			case -1:
				fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
				return 1;
			case 0:			/* child becomes data source */
				apcKidM4[3] = acTarget;

				/* chmod, chown it to source file
				 */
				if (-1 == stat(acTarget, & stTarget)) {
					fprintf(stderr, "%s: stat: %s: %s\n", progname, acTarget, strerror(errno));
					exit(4);
				}
#if HAS_FCHMOD
				if (-1 == fchmod(fdFile, stTarget.st_mode & 0x7777)) {
					fprintf(stderr, "%s: fchmod: %s: %s\n", progname, acThis, strerror(errno));
				}
#else
				if (-1 == chmod(acThis, stTarget.st_mode & 0x7777)) {
					fprintf(stderr, "%s: chmod: %s: %s\n", progname, acThis, strerror(errno));
				}
#endif
#if HAS_FCHOWN
				if (-1 == fchown(fdFile, bHasRoot ? stTarget.st_uid : -1, stTarget.st_gid)) {
					fprintf(stderr, "%s: fchown: %s: %s\n", progname, acThis, strerror(errno));
				}
#else
				if (-1 == chown(acThis, bHasRoot ? stTarget.st_uid : -1, stTarget.st_gid)) {
					fprintf(stderr, "%s: chown: %s: %s\n", progname, acThis, strerror(errno));
				}
#endif
				close(1);
				dup(fdFile);
				close(fdFile);
				iChPid = Child(apcKidM4, pHL, pHLFrom);
				DoDefs(stdout, pHL->pELmy, pHLFrom, (char *)0);
				(void)fflush(stdout);
				(void)fclose(stdout);

				/* wait for the end of pipe to finish
				 */
				while (iChPid != (iPid = wait(0)) && -1 != iPid) {
					;
				}

				exit(0);
				/*NOTREACHED*/
			default: /* parent waits for our child		*/
				break;
			}
			close(fdFile);
			printf("%s", acThis);
			fflush(stdout);
			while (wait(0) != iPid) {
				;
			}
		}
		return 0;
	}
	fprintf(stderr, "%s: unknown filter command: %s\n", progname, ppcCmd[0]);
	return 1;
}

/* read a string from the given file descriptor, but skip leading white	(ksb)
 * and chop off trailing white.  Return about iMax characters.
 */
static char *
InTrim(iFd, iMax)
int iFd;
int iMax;
{
	register char *pcMem, *pcCursor;
	register int cc, iKeep;

	iKeep = iMax;
	pcCursor = pcMem = malloc(iMax+1);
	while (0 < (cc = read(iFd, pcMem, 1))) {
		if (isspace(*pcMem))
			continue;
		++pcCursor;
		--iMax;
		break;
	}
	while (0 < (cc = read(iFd, pcCursor, iMax))) {
		pcCursor += cc;
		iMax -= cc;
		if (0 == iMax)
			break;
	}
	while (iMax < iKeep) {
		if (!isspace(pcCursor[-1])) {
			break;
		}
		--pcCursor, ++iMax;
	}
	*pcCursor = '\000';
	return pcMem;
}

static char *GetM4List();

/* This is where we do the actual updating.  This is called once per	(ksb)
 *	host that is to be updated.
 */
static int
Update(pcRules, pHL, pHLFrom)
char
	*pcRules;			/* the Distfile			*/
HOSTLIST
	*pHL,				/* the host to send		*/
	*pHLFrom;			/* the type of machine from	*/
{
	register int
		iPid;			/* our child...			*/
	auto int iChPid;
	static char
		*apcFilter[] =		/* argv for builtin filter	*/
			{ (char *)0, "@file@", "-", (char *)0 };
	register char *pcThis;
	register char *pcEnv;
	register int i, iF;
	register char **ppcMyRdist;

	/* new in distrib 4.10, if we can't find HOST use SHORTHOST:
	 * this might break rdist'd -m pass through, or not...
	 */
	pcThis = GetEList(pHL->pELmy, acTokHost);
	if ((char *)0 == pcThis) {
		pcThis = GetEList(pHL->pELmy, acTokShort);
	}
	if (fHosts) {
		if ((char *)0 != pcThis) {
			printf("%s\n", pcThis);
		}
		return 0;
	}

	ppcMyRdist = (char **)calloc(iFilePos+RESARGV, sizeof(char *));
	if ((char **)0 == ppcMyRdist) {
		fprintf(stderr, acNoMem, progname);
		exit(2);
	}

	/* build a argv for rdist, we lever some special macros
	 * (viz. RDIST_PATH) for each machine without changing the
	 * argv formed globally.
	 */
	pcEnv = GetM4List(pHL, "RDIST_PATH");
	ppcMyRdist[0] = ((char *)0 != pcEnv) ? pcEnv : ppcRdist[0];
	iF = i = 1;
	ppcMyRdist[i++] = ppcRdist[iF++];

	/* lever RDISTD_PATH and RSH_PATH
	 * rdist wants -P rsh_path before -f -, sigh.
	 */
	pcEnv = GetM4List(pHL, "RSH_PATH");
	if ((char *)0 == pcEnv) {
		pcEnv = pcRemsh;
	}
	if ((char *)0 != pcEnv || (char *)0 != (pcEnv = getenv("RSH_PATH"))) {
		ppcMyRdist[i++] = "-P";
		ppcMyRdist[i++] = pcEnv;
	}
	pcEnv = GetM4List(pHL, "RDISTD_PATH");
	if ((char *)0 == pcEnv) {
		pcEnv = pcRemoteD;
	}
	if ((char *)0 != pcEnv || (char *)0 != (pcEnv = getenv("RDISTD_PATH"))) {
		ppcMyRdist[i++] = "-p";
		ppcMyRdist[i++] = pcEnv;
	}

	/* copy in the rest
	 */
	while (iF < iFilePos) {
		ppcMyRdist[i++] = ppcRdist[iF++];
	}
	if ((char *)0 != pcOnly) {
		if ((char *)0 == pcThis) {
			fprintf(stderr, "%s: unnamed host cannot be updated under -m\n", progname);
			return 1;
		}
		ppcMyRdist[i++] = "-m";
		ppcMyRdist[i++] = pcThis;
	}
	ppcMyRdist[i] = (char *)0;


	/* clone <data source for defines> | m4 [| rdist [-p $rdistd...]]
	 */
	switch (iPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		return 1;
	case 0:			/* child becomes data source */
		apcKidM4[3] = pcRules;
		if (fVerbose) {
			register char **ppc;

			fprintf(stderr, "%s: echo \"", progname);
			DoDefs(stderr, pHL->pELmy, pHLFrom, "\\");
			fprintf(stderr, "dnl\" |\n");

			fprintf(stderr, "%s: ", progname);
			for (ppc = apcKidM4+1; (char *)0 != *ppc; ++ppc) {
				fprintf(stderr, "%s ", *ppc);
			}
			if (!fExam) {
				fprintf(stderr, "| \n%s: ", progname);
				for (ppc = ppcMyRdist+1; (char *)0 != *ppc; ++ppc) {
					fprintf(stderr, "%s ", *ppc);
				}
			}
			fprintf(stderr, "\n");
		}

		if (!fExam) {
			iChPid = Child(ppcMyRdist, pHL, pHLFrom);
			(void) Child(apcFilter, pHL, pHLFrom);
			(void) Child(apcKidM4, pHL, pHLFrom);
		} else if (fFilter) {
			iChPid = Child(apcFilter, pHL, pHLFrom);
			(void) Child(apcKidM4, pHL, pHLFrom);
		} else {
			iChPid = Child(apcKidM4, pHL, pHLFrom);
		}
		DoDefs(stdout, pHL->pELmy, pHLFrom, (char *)0);
		(void)fflush(stdout);
		(void)fclose(stdout);

		/*
		 * wait for the end of pipe to finish
		 */
		while (iChPid != (iPid = wait(0)) && -1 != iPid) {
			;
		}

		exit(0);
		/*NOTREACHED*/
	default:		/* parent waits for our child		*/
		while (wait(0) != iPid) {
			;
		}
		break;
	}
	Cleanup(0);
	free((void *)ppcMyRdist);
	return 0;
}


/* Push a child onto our output stream					(ksb)
 * Returns pid of child, -1 on error
 */
int
Child(ppcArgv, pHL, pHLFrom)
char
	**ppcArgv;			/* process to spawn		*/
HOSTLIST
	*pHL,				/* the host to send		*/
	*pHLFrom;			/* the type of machine from	*/
{
	register int iPid;
	auto int aiFd[2];
	extern char **environ;

 	if (-1 == pipe(aiFd)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		return -1;
	}

	switch (iPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		return -1;
	case 0:
		(void)close(aiFd[1]);
		(void)close(0);
		if (0 != dup(aiFd[0]))
			exit(2);
		(void)close(aiFd[0]);
		if ((char *)0 == ppcArgv[0]) {
			/* internal filter routine (ksb)
			 */
			exit(Filter(ppcArgv+1, pHL, pHLFrom));
		}
		(void)execve(*ppcArgv, ppcArgv+1, environ);
		fprintf(stderr, "%s: execve: %s: %s\n", progname, *ppcArgv, strerror(errno));
		exit(1);
	default:
		break;
	}
	(void)close(aiFd[0]);
	(void)close(1);
	if (1 != dup(aiFd[1]))
		exit(2);
	(void)close(aiFd[1]);
	return iPid;
}


static HOSTLIST
	*pHLMe,				/* this host, if in the file	*/
	*pHLAllHosts;			/* the host table		*/

/* Only if the host has the macro in it's list get the value from m4.	(ksb)
 * This is a long way to run but it makes something work which I needed.
 */
static char *
GetM4List(pHL, pcMacro)
HOSTLIST *pHL;
char *pcMacro;
{
	register char *pcExpr, *pcRet;
	auto int aiFd[2];
	register int iPid, i;

	if ((char *)0 == (pcExpr = GetEList(pHL->pELmy, pcMacro))) {
		return (char *)0;
	}

	/* OK, build a slave pipeline:  <DoDefs> | m4 -> back to us
	 */
	fflush(stdout);
	fflush(stderr);
 	if (-1 == pipe(aiFd)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		exit(10);
	}

	switch (iPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(10);
	default:
		close(aiFd[1]);
		/* read the results of all the work */
		pcRet = InTrim(aiFd[0], MAXPATHLEN|3);
		close(aiFd[0]);
		while (-1 != (i = wait((void *)0))) {
			if (iPid == i)
				break;
		}
		return pcRet;
	case 0:
		break;
	}
	(void)dup2(aiFd[1], 1);
	close(aiFd[1]);
	close(aiFd[0]);
	if (fVerbose) {
		fprintf(stderr, "%s: m4 <<! |me\n", progname);
		DoDefs(stderr, pHL->pELmy, pHLMe, "\\");
		fprintf(stderr, "dnl\n%s\n!\n", pcMacro);
	}
	/* N.B. the nil pointer are OK because we don't call Filter here
	 */
	Child(apcKidM4, (HOSTLIST *)0, pHLMe);
	DoDefs(stdout, pHL->pELmy, pHLMe, (char *)0);
	fprintf(stdout, "%s\n", pcMacro);
	fclose(stdout);
	wait((void *)0);
	exit(0);
	/*NOTREACHED*/
}

#if !defined(R_OK)
#define R_OK	4
#endif

/* a host is a source host if the MACRO HASSRC is set			(ksb)
 */
static int
IsSource(pHL)
HOSTLIST *pHL;
{
	return (char *)0 != GetEList(pHL->pELmy, acTokHassrc);
}


static char acDistfile[] = DISTFILE, acOtherDistfile[] = OTHERDISTFILE;
static char acDefTemplate[132+1];

/* after the options are parsed make sure they make some sense		(ksb)
 * If the distfile is "-" copy it to /usr/tmp/dstrXXXX so we
 * and scan it many times.
 */
int
Post(pcDashO)
char *pcDashO;
{
	auto char acOurName[MAXHOSTNAMELEN+1];	/* our host name	*/

	/* Check to see if they gave us an alternate Distfile, if not,
	 * use one of the defaults.  If none of these exist it will
	 * be caught below.
	 */
	if ((char *)0 == pcDistfile) {
		if (fCmdLine) {
			/* we will build one later */;
		} else if (fHosts) {
			/* we never look at it */;
		} else if (0 == access(acDistfile, R_OK)) {
			pcDistfile = acDistfile;
		} else {
			pcDistfile = acOtherDistfile;
		}
	} else if (0 == strcmp("-", pcDistfile)) {
		static char acTemplate[] = "/dstrXXXXXX";
		register FILE *fpCopy;

		fRmDistfile = 1;

		/* length of /tmp + "/" + template + "\000" + 4 for "/tmp"
		 */
		pcDistfile = malloc(strlen(acTemp)+1+strlen(acTemplate)+1+4);
		if ((char *)0 == pcDistfile) {
			fprintf(stderr, acNoMem, progname);
			exit(1);
		}
		if (pcDistfile == mktemp(strcat(strcpy(pcDistfile, acTemp), acTemplate))) {
			/*we win*/;
		} else if (pcDistfile != mktemp(strcat(strcpy(pcDistfile, "/tmp") , acTemplate))) {
			fprintf(stderr, "%s: mktemp failed on %s\n", progname, pcDistfile);
			exit(2);
		}

		if ((FILE *)0 == (fpCopy = fopen(pcDistfile, "w"))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcDistfile, strerror(errno));
			exit(3);
		}

		CopyFp(stdin, fpCopy);
		(void)fclose(fpCopy);
		(void)clearerr(stdin);
	} if (fHosts) {
		/* we ignore pcDistfile silently */;
	}

	/* add any single letter rdist options we got
	 */
	if ('\000' != acRdistOpts[1]) {
		KeepDef(acRdistOpts);
	}

	/* add any dash o rdist options we got
	 */
	if ((char *) 0 != pcDashO && '\000' != pcDashO[0]) {
		KeepDef("-o");
		KeepDef(pcDashO);
	}

	/* Make sure we can read the Distfile
	 * (this prevents so many errors, one for each rdist we fork -- ksb)
	 */
	if (!fCmdLine && !fHosts && access(pcDistfile, R_OK) != 0) {
		fprintf(stderr, "%s: access: %s: %s\n", progname, pcDistfile, strerror(errno));
		exit(2);
	}

	if (fExam && !fFilter && (char *)0 != pcDistfile) {
		register char *pcSlash;

		if ((char *)0 == (pcSlash = strrchr(pcDistfile, '/'))) {
			pcSlash = pcDistfile;
		} else {
			++pcSlash;
		}
		if (0 == strcmp(acDistfile, pcSlash) || 0 == strcmp(acOtherDistfile, pcSlash)) {
			fFilter = 1;
		}
	}

	/* Read in the configuration file with all the hosts in it
	 */
	sprintf(acDefTemplate, "%%%s %s %s %s %s\n", acTokHost, acTokShort, acTokHosttype, acTokHostos, acTokHassrc);
	SetTemplate(acDefTemplate);
	ReadConfig(& pHLAllHosts);

	/* Find out our host name and our value for MYTYPE
	 */
	if (gethostname(acOurName, sizeof(acOurName)) != 0) {
		fprintf(stderr, "%s: gethostname: %s\n", progname, strerror(errno));
		exit(4);
	}
	if ((HOSTLIST *)0 == (pHLMe = LookUp(pHLAllHosts, acOurName))) {
		pHLMe = FallBack(pHLAllHosts, acOurName);
	}

	return 0;
}

/* update a given host							(ksb)
 */
int
UpHost(pcHosts)
char *pcHosts;
{
	register char *pcHost;
	register HOSTLIST *pHL;	/* current host we are looking at	*/
	register int l;
	register int iRet;
	register char *pcFound;

	iRet = 0;
	while ((char *)0 != pcHosts && '\000' != *pcHosts) {
		pcHost = pcHosts;
		if ((char *)0 != (pcHosts = strchr(pcHosts, ','))) {
			*pcHosts++ = '\000';
		}

		/* rdist has a botch we work around, the -m option to rdist just
		 * does a strcmp on the host names, so we have to use the HOST
		 * macro in the command line to rdist and hope the Distfile we
		 * use uses HOST, not SHORTHOST.  *Sigh*  (ksb)
		 */
		l = strlen(pcHost);
		for (pHL = pHLAllHosts; (HOSTLIST *)0 != pHL; pHL = pHL->pHLnext) {
			if ((char *)0 == (pcFound = GetEList(pHL->pELmy, acTokHost))) {
				continue;
			}
			if (0 != strncmp(pcHost, pcFound, l) || !('\000' == pcFound[l] || '.' == pcFound[l])) {
				continue;
			}
			break;
		}
		if ((HOSTLIST *)0 == pHL) {
			fprintf(stderr, "%s: %s: unknown host\n", progname, pcHost);
			++iRet;
		} else if (fSource && !IsSource(pHL)) {
			fprintf(stderr, "%s: %s: not a source host\n", progname, pcHost);
			++iRet;
		} else if ((char *)0 != pcGuard && !IsGuarded(pHL, pcGuard, pHLMe)) {
			fprintf(stderr, "%s: %s: killed by guard\n", progname, pcHost);
			++iRet;

		} else if ((char *)0 != pcType) {
			if ((char *)0 == (pcFound = GetEList(pHL->pELmy, acTokHosttype))) {
				fprintf(stderr, "%s: %s: has no defined %s\n", progname, pcHost, acTokHosttype);
				++iRet;
			} else if (0 != AnyOf(pcFound, pcType)) {
				fprintf(stderr, "%s: %s: wrong cpu type (%s)\n", progname, pcHost, pcFound);
				++iRet;
			} else {
				iRet += Update(pcDistfile, pHL, pHLMe);
			}
		} else if ((char *)0 == (pcFound = GetEList(pHL->pELmy, acTokHost))) {
			fprintf(stderr, "%s: %s: has no defined HOST (name)\n", progname, pcHost);
			++iRet;
		} else {
			iRet += Update(pcDistfile, pHL, pHLMe);
		}
	}
	return iRet;
}

/* if no hosts update all hosts of our type, else ...			(ksb)
 */
int
Zero()
{
	register HOSTLIST *pHL;	/* current host we are looking at	*/
	register int i = 0;
	register char *pcFound, *pcMyType;

	if ((HOSTLIST *)0 == pHLMe || (char *)0 == (pcMyType = GetEList(pHLMe->pELmy, acTokHosttype))) {
#if defined(DEF_HOSTTYPE)
		pcMyType = acDefHosttype;
#endif
	}

	if (fCmdLine) {
		fprintf(stderr, "%s: -c: need files and host\n", progname);
		i = 1;
	} else if ((char *)0 != pcOnly) {
		/* the exit trap for only will catch this */
	} else if ((char *)0 != pcType) {
		for (pHL = pHLAllHosts; pHL != (HOSTLIST *)0; pHL = pHL->pHLnext) {
			if ((char *)0 == (pcFound = GetEList(pHL->pELmy, acTokHosttype)))
				continue;
			if (0 != AnyOf(pcFound, pcType))
				continue;
			if (fSource && !IsSource(pHL))
				continue;
			if ((char *)0 != pcGuard && !IsGuarded(pHL, pcGuard, pHLMe))
				continue;
			i += Update(pcDistfile, pHL, pHLMe);
		}
	} else if (fSource || (char *)0 != pcGuard) {
		for (pHL = pHLAllHosts; pHL != (HOSTLIST *)0; pHL = pHL->pHLnext) {
			if (fSource && !IsSource(pHL))
				continue;
			if ((char *)0 != pcGuard && !IsGuarded(pHL, pcGuard, pHLMe))
				continue;
			i += Update(pcDistfile, pHL, pHLMe);
		}
	} else if (fAll) {
		for (pHL = pHLAllHosts; pHL != (HOSTLIST *)0; pHL = pHL->pHLnext) {
			i += Update(pcDistfile, pHL, pHLMe);
		}
	} else if ((char *)0 == pcMyType) {
		fprintf(stderr, "%s: I don't know my own %s\n", progname, acTokHosttype);
		i = 1;
	} else {
		/* by default do all of this type, but not us
		 * hosts with no type don't count!
		 */
		for (pHL = pHLAllHosts; pHL != (HOSTLIST *)0; pHL = pHL->pHLnext) {
			if (0 == fMyself && pHL == pHLMe) {
				continue;
			}
			if ((char *)0 == (pcFound = GetEList(pHL->pELmy, acTokHosttype))) {
				continue;
			}
			if (0 != strcmp(pcFound, pcMyType)) {
				continue;
			}
			i += Update(pcDistfile, pHL, pHLMe);
		}
	}
	return i;
}


/* look at the rest of the command line as a micro distfile		(ksb)
 */
int
DoCmd(argc, argv)
int argc;
char **argv;
{
	auto char *pcDest, *pcTo;
	auto FILE *fp;
	auto int i;
	auto char acCDist[MAXPATHLEN+1];
	static char *apcFix[3];

	switch (argc) {
	case 0:
		fprintf(stderr, "%s: -c: usage files [host:path]\n", progname);
		exit(1);
	case 1:
		apcFix[0] = argv[0];
		apcFix[1] = acTokHost;
		apcFix[2] = (char *)0;
		argc = 2;
		argv = apcFix;
		break;
	default:
		break;
	}

	if ((char *)0 != pcDistfile) {
		fprintf(stderr, "%s: -f %s ignored\n", progname, pcDistfile);
	}

	(void)strcpy(acCDist, "/tmp/cdistXXXXXX");
	if ((char *)0 == (pcDistfile = mktemp(acCDist))) {
		fprintf(stderr, "%s: mktemp failed\n", progname);
		exit(4);
	}
	if ((FILE *)0 == (fp = fopen(pcDistfile, "w"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcDistfile, strerror(errno));
		exit(2);
	}

	pcTo = argv[--argc];
	argv[argc] = (char *)0;
	if ((char *)0 == (pcDest = strchr(pcTo, ':'))) {
		pcDest = "";
	} else {
		*pcDest++ = '\000';
	}
	fprintf(fp, "(");
	while (0 != argc--) {
		fprintf(fp, " %s", argv[argc]);
	}
	fprintf(fp, " ) -> ( %s )\n", pcTo);
	fprintf(fp, "\tinstall %s%s;\n", fBinary ? "-b " : "", pcDest);
	if ((char *)0 != pcSpecial) {
		fprintf(fp, "\tspecial %s;\n", pcSpecial);
	}

	(void)fclose(fp);
	fCmdLine = 0;
	if ((char *)0 != pcOnly)
		i = UpHost(pcOnly);
	else
		i = Zero();
	(void)unlink(pcDistfile);
	Cleanup(0);
	return i;
}


/* clue the driver in about our default configuration file		(ksb)
 * (note that the default template has a \n on the end)
 */
int
ShowVer(pcPath)
char *pcPath;
{
	static char acNot[] = "nonexistant ";
	register char *pcTemp;

	/* post looks up a bunch of stuff we'd like to know
	 * but we have to keep the other stuff from complaining
	 */
	++fHosts;
	Post((char *)0);
	--fHosts;

	printf("%s: using configuration from `%s'\n", progname, acConfig);
	printf("%s: default column headers: %s", progname, acDefTemplate);
#if defined(DEF_HOSTTYPE)
	if ((HOSTLIST *)0 != pHLMe && ((char *)0 != (pcTemp = GetEList(pHLMe->pELmy, acTokHosttype)))) {
		if (0 == strcmp(pcTemp, acDefHosttype)) {
			printf("%s: compiled %s matches configured %s\n", progname, acTokHosttype, pcTemp);
		} else {
			printf("%s: compiled %s (%s) doesn't match configured %s\n", progname, acTokHosttype, acDefHosttype, pcTemp);
		}
	} else {
		printf("%s: compiled %s is %s\n", progname, acTokHosttype, acDefHosttype);
	}
#endif
	/* show paths to rdist and m4
	 */
	if ((char *)0 != pcPath && ! ('.' == pcPath[0] && '\000' == pcPath[1])) {
		printf("%s: library path \"%s\"\n", progname, pcPath);
	}
	if ((char **)0 != ppcRdist && (char *)0 != ppcRdist[0]) {
		printf("%s: rdist binary is %s\"%s\"\n", progname, (-1 == access(ppcRdist[0], 0)) ? acNot : "", ppcRdist[0]);
	}
	printf("%s: m4 binary is %s\"%s\"\n", progname, (-1 == access(apcKidM4[0], 0)) ? acNot : "", apcKidM4[0]);
	return 0;
}
