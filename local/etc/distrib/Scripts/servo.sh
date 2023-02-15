#!/bin/sh
# $Id: servo.sh,v 4.0 1996/02/27 21:51:28 kb207252 Exp $
# Run a shell command in every remote source directory.
#

PROGNAME=`basename $0 .sh`
TMP=/tmp/servo$$

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

# make gets an error if we give it a child, so...
( cat $MF;
  cat - <<!
_servo: FRC
	for i in \`distrib -H \${HOSTS}\` ; do \\
		echo \$\$i:; \\
		rsh \$\$i -n sh -c '"PATH=/usr/local/bin:/usr/local/etc:\$\$PATH && export PATH && cd \${INTO} && $*" '\$\$i ; \\
	done
!
) > $TMP
make $MPASS -f $TMP _servo
rm -f $TMP

exit 0
