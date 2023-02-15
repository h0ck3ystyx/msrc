/* $Id: envlist.h,v 4.2 1996/03/19 01:07:29 kb207252 Exp $
 * we just keep these in a list, linear insert is OK for <10 things	(ksb)
 */
typedef struct ELnode {
	short int fused;
	char *pcvar;
	char *pcvalue;
	struct ELnode *pELnext;
} ENVLIST;

extern ENVLIST *SaveEList();
extern ENVLIST *PutEList();
extern int ScanEList();
extern char *GetEList();
