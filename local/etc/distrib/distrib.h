/*
 * $Id: distrib.h,v 4.5 1996/08/23 15:57:14 kb207252 Exp $
 *
 * you must include <sys/param.h> before this file
 */

/*
 *  include file for distrib.
 *
 *  Michael J. Spiitzer, Purdue University Computing Center	(mjs)
 */
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9)
#if !defined(HPUX)
#define HPUX	1
#endif
#endif

/*
 * Directory to search if we can't find the given config file
 * (see the -C option)
 */
#if !defined(ALT_CONFIG_PATH)
#define ALT_CONFIG_PATH	"/usr/local/lib/distrib"
#endif

/*
 * Location of the configuration file
 */
#if !defined(MCONFIGFILE)
#define	MCONFIGFILE	"distrib.mcf"
#endif
#if !defined(CONFIGFILE)
#define	CONFIGFILE	"distrib.cf"
#endif

/*
 * default Distfile to search for
 */
#define DISTFILE	"Distfile"

/*
 * look for this file if DISTFILE doesn't exist
 */
#define OTHERDISTFILE 	"distfile"

/*
 * Maximum host name length (for pre-4.3 systems)
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

/* sysv machines do not define MAXPATHLEN
 */
#ifndef MAXPATHLEN
#define MAXPATHLEN	1024
#endif

/* Maximum length of machine CPU type
 */
#define MAXTYPELEN 	64

/* Maximum length of OS name
 */
#define MAXOSLEN 	MAXTYPELEN 

/* Maximum line length in the configuration file
 */
#define MAXLINELEN	128

/* Comment character in configuration file
 */
#define COMMENT		'#'

/* size of the buffer where we hold the options for rdist
 */
#define MAXRDISTOPT	10

/*
 * host information table
 */
typedef struct hostlist {
	struct hostlist *pHLnext;		/* next entry in chain	*/
	ENVLIST *pELmy;				/* my environment list	*/
} HOSTLIST;

/*
 * Extern function declarations for functions in distrib.c
 */
extern int	Zero(), UpHost(), Post();
