# $Id: Makefile,v 4.1 1997/10/26 18:25:09 ksb Exp $
#
#	master source Makefile for msrc/Amdin

GEN=	
SEND=	README Levers Macros Targets \
	bin docs master meta papers platform
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/Admin

HOSTS=	-S
MDEFS=	
DDEFS=	-d"SEND=( ${SEND} ${GEN} )" -d"INTO=${INTO}" ${MDEFS} ${HOSTS}
DEBUG=	-O

LOOP=	for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && make DESTDIR="${DESTDIR}" DEBUG="${DEBUG}" $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | make -f - $@


quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

all: rsource FRC
	${LOOP}

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
