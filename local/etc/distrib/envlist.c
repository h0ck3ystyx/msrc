/* $Id: envlist.c,v 4.1 1995/05/02 01:58:34 kb207252 Exp $
 * keep track of a list of variable/value pairs				(ksb)
 */
#include <stdio.h>
#include "envlist.h"

extern char *malloc();
extern char *progname;

/* save the current env list						(ksb)
 */
ENVLIST *
SaveEList(pELSrc)
register ENVLIST *pELSrc;
{
	register ENVLIST *pEL;

	for (pEL = pELSrc; (ENVLIST *)0 != pEL; pEL = pEL->pELnext) {
		pEL->fused = 1;
	}
	return pELSrc;
}

/* update an Env list with a new value, or rm a var if			(ksb)
 * value == (char *)0
 *  empty -> build new one
 *  us -> if not used replace, else clone and replace, or delete
 *  before us insert or return success (deleted)
 *  after us, recurse and modify next ptr (if not used)
 *  or clone this node
 */
ENVLIST *
PutEList(pELSrc, pcVar, pcValue)
ENVLIST *pELSrc;
char *pcVar, *pcValue;
{
	register ENVLIST *pELMake;
	register int i;

	if ((ENVLIST *)0 == pELSrc) {				/* empty */
		/* make it */;
	} else if (0 == (i = strcmp(pELSrc->pcvar, pcVar))) {	/* us	*/
		/* this one */
		if (0 == pELSrc->fused) {
			pELSrc->pcvalue = pcValue;
			return pELSrc;
		}
		if ((char *)0 == pcValue) {			/* ~us	*/
			return pELSrc->pELnext;
		}
		pELSrc = pELSrc->pELnext;
		/* make a new one with the same next ptr */
	} else if (i < 0) {
		if ((char *)0 == pcValue)	/* not in list		*/
			return pELSrc;
		/* insert it */
	} else {				/* check list after us	*/
		pELMake = PutEList(pELSrc->pELnext, pcVar, pcValue);
		if (pELSrc->pELnext == pELMake) {
			/* no change in list */
			return pELSrc;
		}
		if (0 == pELSrc->fused) {
			pELSrc->pELnext = pELMake;
			return pELSrc;
		}
		/* clone us */
		pcVar = pELSrc->pcvar;
		pcValue = pELSrc->pcvalue;
		pELSrc = pELMake;
	}
	if ((char *)0 == pcValue) {		/* ~unknown		*/
		/* could be an error in some apps XXX
		 */
		return pELSrc;
	}
	if ((ENVLIST *)0 == (pELMake = (ENVLIST *) malloc(sizeof(ENVLIST)))) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(1);
	}
	pELMake->pcvar = pcVar;
	pELMake->pcvalue = pcValue;
	pELMake->pELnext = pELSrc;
	pELMake->fused = 0;
	return pELMake;
}


/* scan an env list with a use function					(ksb)
 */
int
ScanEList(pEL, pfi)
ENVLIST *pEL;
int (*pfi)();
{
	register int i;

	for (/*empty*/; (ENVLIST *)0 != pEL; pEL = pEL->pELnext) {
		if (0 != (i = (*pfi)(pEL->pcvar, pEL->pcvalue)))
			return i;
	}
	return 0;
}

/* Get a value from the list of values				(ksb)
 */
char *
GetEList(pEL, pcName)
ENVLIST *pEL;
char *pcName;
{
	for (/*empty*/; (ENVLIST *)0 != pEL; pEL = pEL->pELnext) {
		if (0 == strcmp(pcName, pEL->pcvar)) {
			return pEL->pcvalue;
		}
	}
	return (char *)0;
}
