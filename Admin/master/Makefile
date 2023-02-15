# $Id: Makefile,v 2.6 2000/06/17 22:01:45 ksb Exp $
#
#	master source Makefile for %% product name %%

GEN=	%% stuff we are called to make, usually empty or for GNU a tar.gz %%
SEND=	%% :r !echo *| fmt %%
SOURCE=	Makefile Distfile ${SEND}
INTO=	%% /usr/src/local/bin/foo %%

HOSTS=	-S
MDEFS=	%% -DNEW="-o $$USER -g staff" %%
DDEFS=	-d"SEND=( ${SEND} ${GEN} )" -d"INTO=${INTO}" ${MDEFS} ${HOSTS}
DEBUG=	-O

LOOP=	for i in `distrib -H ${MDEFS} ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && ${MAKE} DESTDIR="${DESTDIR}" DEBUG="${DEBUG}" $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - $@


quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

all: rsource FRC
	${LOOP}

# 6dd %% any local targets we need to distrib files go here %%
# use "${HERE}" to make local targets from Make.host (this is _rare_)
# Or use a cp command to copy GNU source from the ftp spool.
# ${GEN}:
# 	${HERE} %% cp /home/dspd6/ftp/pub/gnu/$@ . %%

calls: msource FRC
	${HERE}

clean: FRC
	${LOOP}
	rm -f ${GEN} Makefile.bak a.out core errs lint.out tags

deinstall: FRC
	${LOOP}

depend: FRC
	${LOOP}

dirs: FRC
	${LOOP}

distrib: FRC
	${LOOP}

install: rsource FRC
	${LOOP}

lint: FRC
	${LOOP}

mkcat: FRC
	${LOOP}

print: msource FRC
	${HERE}

source: rsource
	${LOOP}

rsource: Distfile msource ${GEN}
	distrib ${DDEFS}

msource: ${SOURCE}

spotless: clean FRC
	rcsclean ${SOURCE}

tags: msource FRC
	${HERE}

${SOURCE}:
	co -q $@

FRC:
