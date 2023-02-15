#	$Id: Makefile,v 4.5 2000/06/21 19:40:17 ksb Exp $
#
#	meta Makefile for distrib

# Example Scripts
GEN=	main.c main.h
SEND=	INSTALL README TODO Make.host \
	distrib.man distrib.c distrib.h distrib.m \
	envlist.c envlist.h machine.h
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/etc/distrib

HOSTS= -S
MDEFS=
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}


LOOP=	-for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && ${MAKE} DESTDIR=${DESTDIR} $@"' ;\
	done

quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

all: rsource FRC
	${LOOP}

main.h: main.c

main.c: distrib.m
	mkcmd std_help.m std_version.m distrib.m
	-(cmp -s prog.c main.c || (mv prog.c main.c && echo main.c updated))
	-(cmp -s prog.h main.h || (mv prog.h main.h && echo main.h updated))
	rm -rf prog.[ch]

calls: msource FRC
	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - calls

clean: FRC
	${LOOP}
	rm -f ${GEN} Makefile.bak a.out core errs lint.out tags

deinstall: FRC
	echo "Unsafe to deinstall on every host."
	false

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
	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - $@

source: rsource
	${LOOP}

rsource: Distfile msource
	distrib ${DDEFS}

msource: ${SOURCE}

spotless: clean FRC
	rcsclean ${SOURCE}

tags: msource FRC
	ctags -t *.c *.h

${SOURCE}:
	co -q $@

FRC:
