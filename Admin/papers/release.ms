.\" $Id: release.ms,v 1.4 1997/10/26 18:03:36 ksb Exp $

.de {V			\" start a verbatim listing
.	KS		\" we want to keep the lines together
.	nf		\" do not fill the lines in
.	in +0.5i	\" indent 1/2 an inch
.	ft CW		\" use a constant width font
..

.de }V			\" end a verbatim listing
.	ft P		\" switch back to Roman font
.	in -0.5i	\" end the indentation
.	fi		\" switch fill mode back on
.	KE		\" end the keep together area
..


.de LI			\" List Item
.	IP \fB\\$1\fP	\" Indent paragraph and bold the item
.	PP
..


.TL
How to build a release from meta source.
.AI
Federal Express Corp.
.DA March 1997

.AB
Given that one can maintain source and binary files on a local
heterogeneous network using the meta source system 
our attention must eventually turn to replicating our work at
remote institutions.
It is clear that limited distributions, in both size and scope,
are better than a simple tar archive of all of /usr/msrc.
A system for extracting the relevant files with
the current contents to replicate
a known release is described.
.AE

.NH 1
Our Level

.PP
As in the previous papers in this series we are working with 4 levels
of source maintainance: files, products, releases and systems.  In
this paper we concentrate on building the release to be merged into
a production (or test) system.

.PP
Given that we've used the previous two levels (RCS and rcsvg) to
construct our products (binarys, documents, libraries and the like)
we can leverage those to build a unified release package.

.NH 1
The Goal

.PP
If we can extract a structure that replicates on the parts of /usr/msrc
that are need to build and install the products in the package we
are done.

.NH 1
Details

.PP
This is a cookie cutter walk through to build your release.


.NH 2
Build the box

.PP
Build a directory under /usr/msrc/Pkgs.

.PP
Build a README with the list of all the source directories you are
sending and what _version_ of each you are including.

.PP
Build directory structure, RCS, bin/RCS, lib/RCS if you have source
under "bin" and "lib".  Add "etc/RCS" and the like for any other
source structures you need.  You have to start at the common ancestor
of all the source directories you are sending.

.PP
Build the install notes file in INSTALL.

.NH 2
Build the recipees

.PP
Build a package makefile with 
.sp
	cp /usr/msrc/Admin/meta/Makefile Makefile
.br
	vi Makefile
.sp
in you package directory.  Edit the Makefile and replace
all the (standard) %% text %% tokens.

.PP
Replicate the ${STAGE}/BIN/PROD stanza for each source directory
you are including.
Decide on the release name for this directory as of this minute
(release "7.10" might be "Seven" or "April97" or "FCS" or "Silver").
Put the number in for "Release name".

.PP
Build a distfile using:
.sp
	cp /usr/msrc/Admin/meta/Distfile Distfile
.br
	vi Distfile
.sp
Remember to include any files in the second stanza we need on
the platform host.

.PP
Build a directory level meta makefile for use on the remote msrc
machine:
.sp
	cp /usr/msrc/Admin/meta/toplevel Make.meta
.br
	vi Make.meta
.sp
Editing (its %%text%% stuff) to descend to each product directory.
If order is important put them in the correct order, else use
something clear.

.PP
Build a directory level host makefile for use on each platform,
same as the meta one above, but start with:
.sp
	cp /usr/msrc/Admin/master/toplevel Make.host
.br
	vi Make.host
.sp

.NH 2
Set the stone

.PP
Check in to RCS all the makefiles, INSTALL, README, and such with the
symbolic name you picked for "Release name" above.  This is the level
two control loop for the release subsystem, of course.  Maintain a lock
on the Makefile.

.PP
Go through each product directory to be sure the version names you
picked for them are defined in RCS symbolic names (check contents
later).  We usually use 
.sp
	rcsdiff -q -r$NAME RCS/*
.sp
in each directory.

.NH 2
Dry run

.PP
In the package directory run
.sp
	make stage
.sp
Look for any gross errors on stderr and repair them.  Make clean is
almost always a good idea when you are testing.

.PP
It is safe to make stage and clean to the default stage area since
you hold a lock on the Makefile.  If you don't hold a lock on the
Makefile you are a bad person and need to confess to someone smarter.

.PP
A final sanity check before we build: make clean, and run
.sp
	make STAGE=/tmp/$$.dir
.sp
to build a copy in another name (this assures that you didn't hard code
any paths).  Clean up the directory with
.sp
	make STAGE=/tmp/$$.dir clean
.sp

.NH 1
Make is shine

.PP
Now you can build a gzip'd tar file from the staged directory to
share with the whole world.  Remember that most people like to see
.sp
	package-release.tgz
.sp
now days.

.NH 1
Still to do

.PP
I still need to document and release the link to the GNU configure
packages like "screen" and "gzip".   
