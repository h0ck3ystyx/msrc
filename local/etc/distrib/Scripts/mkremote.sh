#!/bin/sh
# $Id: mkremote.sh,v 4.0 1996/02/27 21:54:31 kb207252 Exp $
# Make a Makefile on stdout that will build the remote versions
# of all the m4 processed files.  Call this file `Make.remote' and use
#	make -f Make.remote HOSTTYPE
# to build expansions for the given HOSTTYPE
#

PROGNAME=`basename $0 .sh`
TMP=/tmp/remmk$$
CLN=/tmp/clnmk$$

# find files we need to run
if [ -f Makefile ] 
then
	MF=Makefile
elif [ -f makefile ]
then
	MF=makefile
else
	echo 1>&2 "$PROGNAME: where is the Makefile?"
	exit 1
fi

if [ -f Distfile ]
then
	DF=Distfile
elif [ -f distfile ]
then
	DF=distfile
else
	echo 1>&2 "$PROGNAME: where is the Distfile?"
	exit 1
fi

[ -f Make.host ] || {
	echo 1>&2 "$PROGNAME: where is Make.host?"
	exit 1
}

# find the files to build, of course we'd like to parse the install ...;
# line too, but rdist can have all sorts of macro games with m4. dnl
# (Yeah, we want the newlines in the output text.)
BUILD="`sed -n \
	-e ':loop' \
	-e '/@/{' \
		-e 'h' \
		-e 's/[^@]*@\([^@]*\)@\(.*\)/\1/p' \
		-e 'g' \
		-e 's/[^@]*@\([^@]*\)@\(.*\)/\2/' \
		-e 'b loop' \
	-e '}' < Distfile`"
RBUILD=`echo "$BUILD" | sed -e 's/$/.HOSTTYPE/'`
export BUILD RBUILD CLN

# output a makefile that can be used on a remote UNIX host
echo "#!/bin/make -f"
echo "# made on `date` by ${USER-${LOGNAME}} (${NAME})"
echo ""

# make gets an error if we give it a child, so...
( cat $MF;
  cat - <<!
_remote: FRC
	echo -n "	rm -f" >$CLN
	echo "all: \`echo -n \\ HOSTTYPE | distrib -f - -E \${HOSTS}\`" ;\\
	for i in \`distrib -H \${HOSTS}\` ; do \\
		h=\`echo HOSTTYPE | distrib -E -f - -m \$\$i\` ;\\
		echo "" ;\\
		echo "\\\`\$\$h: '"\$\$RBUILD | distrib -E -f - -m \$\$i ;\\
		echo "" ;\\
		for f in \$\$BUILD ; do \\
			echo "\$\$f.\$\$h: \$\$f" ;\\
			echo -n " \$\$f.\$\$h" >>$CLN ;\\
			distrib -x -n -E -f \$\$f -m \$\$i  2>&1 >/dev/null | sed -e 's/distrib: //' -e 's/\\\$\$/\$\$\$\$/g' -e 's/^/	/' -e '\$\$!s/\$\$/ \\\\/' -e '\$\$s/\$\$/ > \\\$\$@/';\\
		done ;\\
	done
	echo ""
	echo "clean: FRC"
	cat $CLN
	echo ""
	echo ""
	echo "FRC:"
!
) > $TMP
make -s $MPASS -f $TMP _remote
rm -f $TMP

exit 0
