#!/bin/sh
# $Id: expound.sh,v 4.0 1996/02/27 21:51:28 kb207252 Exp $
# Output the Make.hosts file and Distfile for each host we goto.
# This allows the source master to see what is going on, kinda.
#
# ZZZ we could do the same trick mkremote.sh uses to get all the
# m4 expanded files and show them all?

PROGNAME=`basename $0 .sh`
TMP=/tmp/expnd$$

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
( cat $MF; echo ""; echo "_expound: FRC"; echo '	distrib -H ${HOSTS}') | make -s -f - _expound  > $TMP

if [ $# -eq 0 ]
then
	echo "We send source to:"
	sed -e "s/^/	/" < $TMP
	echo -n "press return to continue: "
	read foo
else
	LIST=""
	for HOST
	do
		if [ -z "`grep -e $HOST $TMP`" ]
		then
			echo 1>&2 "$PROGNAME: we don't send to $HOST"
		else
			LIST="$LIST
$HOST"
		fi
	done
	echo "$LIST" > $TMP
fi

for HOST in `cat $TMP`
do
	(
		echo "$HOST Makefile:"
		distrib -E -m $HOST -f Make.host
		echo "$HOST Distfile:"
		distrib -E -m $HOST -f "$DF"
	) | ${PAGER-${more-more}}
done

rm -f $TMP
exit 0
