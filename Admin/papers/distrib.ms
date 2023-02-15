.\" $Id: distrib.ms,v 1.3 1997/10/26 18:05:51 ksb Exp $

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
Distrib: A master-slave source system for
.UX
Systems
.br
(``Distrib 96'')
.AU
Kevin Braunsdorf
.AI
Federal Express Corporation
.AB
Maintaining source and binary files on a heterogeneous network
using straight \fIrdist\fP(1) is time consuming and error prone.
This paper presents a tool and a paradigm for maintaining products on a
heterogeneous network with minimal administrator intervention and
maximal automation.
.AE
.A1 Braunsdorf
.T1 Distrib

.NH
Introduction

.PP
The Purdue University Computing Center's source group
met off and on for 2 years to come to the conclusion that we wanted:
.RS
\(bu a common source for products that lived on more than one platform
.br
\(bu a list of which platforms a product lives on
.br
\(bu a simple command that compiles on all platforms
.br
\(bu something we could maintain, that is
.br
	not too fragile
.br
	well contained in few directories and/or files
.br
	easy to explain to new staff
.br
	well documented (structure and specifics)
.RE
Recently an additional criteria has been added:
.RS
\(bu distribution to off-site networks must be possible
.RE
Within the frame-work of the existing system we have been able to
construct some additional support for source distributions.

.PP
I believe this proposal meets all these goals with a flexible and
comprehensive system for multi-platform source maintainance.
.bp
.NH
How it all fits together

.PP
The meta source directory for a product is rooted under \fI/usr/msrc\fP.
The meta source is composed of the product source,
the source to a platform-specific makefile,
a control file for \fIdistrib\fP, and
a meta makefile ot drive the product distribution, compilation, and
installation.
This meta source is distributed to parallel directories rooted under
\fI/usr/src\fP on the various platforms for which the product is supported.

.PP
The sources for the product, as well as the other control files,
are kept under RCS under \fI/usr/msrc\fP.
The platform specific makefile's source should be called ``Make.host''
while under \fI/usr/msrc\fP and ``Makefile'' on each remote platform.
The control file for \fIdistrib\fP should be called ``Distfile'' and
is not sent to any remote platform.
The meta makefile has a standard format and uses some new variables.
Example files are appended.

.PP
Distributions are copies of the meta source, but they include some extra
files which allow an installed to emulate the platform distribution
process without \fIrdist\fP or \fIdistrib\fP.
An optional makefile called ``Make.remote'' (constructed via a shell script)
contains \fIm4\fP invokations which conjure configured copies of the
various source files needed to build the product remotely.
Some furthur instructions for putting these files in their correct homes
might be contained in an INSTALL file or Configure script.

.NH 2
The meta makefile

.PP
I've defined some new variables in the makefile to control the
construction, platform, placement and configuration of source.
The template makefile for this type is named ``Makefile'' in
the "master" template directory.
.in +2m
.TS
l l.
GEN	files generated for a \fIdistrib\fP
SEND	source we send to the platform source machine
SOURCE	source we look at + ${SEND}
INTO	where we put source
HOSTS	a target host list in the form of \fIdistrib\fP options
MDEFS	\fIm4\fP defines we need for file expansion
DDEFS	command line passed to \fIdistrib\fP; passes down INTO, HOSTS, MDEFS
.TE
.in -2m

.PP
Most of these are marked in the template makefile with ``%%''.
The text between %%'s should be replaced with the value required.

.NH 2
The Distfile

.PP
The distfile in the meta source directory is responsible for
transporting the appropriate source files to their remote platforms.
If some files need to be customized during transport, they are run
through \fIm4\fP.
An example distfile is appended.

.PP
In a \fIdistrib\fP(8l) distfile @\fIfile\fP@ denotes that \fIfile\fP is
run through \fIm4\fP(1) before it is sent to each platform.
The Make.host makefile is \fBalways\fP processed in this manner.

.PP
The local convention for quoting text from \fIm4\fP is to start each
file with a quote only to de-quote the text when macro replacement is
expected.
This prevents \fIm4\fP from removing some common keywords (viz. ``include'').
Any file protected in the manner ends with a ``close-quote dnl''.

.PP
Here is a short example:
.{V
	`# this file contains our hostname
	'HOST`
	'dnl
.}V

.NH 2
The platform makefile

.PP
The file ``Make.host'' is a makefile only after it is run 
though \fIm4\fP by \fIdistrib\fP.
The standard template makefiles have been edited; each has a 
version \fItype\fP.host (lib.host, genc.host, etc.)
which is the basis for a Make.host.

.NH
Converting to the meta source setup

.PP
We need to build a makefile for each platform using -Ddefines
and such to configure the source at compile time.
This is done by selecting appropriate compile time options in
the platform makefile with \fIm4\fP's \fBifelse()\fP facility.

.PP
For example on the machine ``foo'' we need to turn the ANGUISH option on
(in addition to our normal HOSTTYPE switch):
.{V
	....
	CDEFS= -D'HOSTTYPE`'ifelse(SHORTHOST,`foo',` -DANGUISH')`
.}V

.PP
In some cases we might send a different file to the remote machine,
or (more commonly) send two files and only compile one of them.
This is done by selecting appropriate files in
the distfile (once again with \fIm4\fP) or generated makefile.

.PP
We need to create a new meta makefile that pushes all the source files
for us and send the targets we like to the remote platforms.

.PP
We need to teach \fIdistrib\fP about these new files.

.NH 2
Lock and rename the makefile

.PP
Make sure no uncool changes have been made to the file.
.{V
	# rcsdiff Makefile
.}V

Get a lock on the file.
.{V
	# co -l Makefile
.}V

Rename the host level makefile.
.{V
	# mv -i Makefile Make.host && mv -i RCS/Makefile,v RCS/Make.host,v
.}V

.NH 2
Protect Make.host from \fIm4\fP

.PP
Search for /[\`\']/;  change each to double quotes ('') where possible.
Protect the text from \fIm4\fP with ``changequote'' in other cases.
For example:
.{V
	....
	'changequote(+,^)
	+....
	^changequote(`,')
	`
.}V

.PP
Quote the whole file, insert a back quote (`) on line 1, then add
.{V
	'dnl
.}V
to the end of the file (as a line by itself).

.PP
Various modifications can now be made to the file with \fIm4\fP with
closing quotes, \fIm4\fP commands, open quotes.

.PP
If the program needs to #if/#else/#endif options into the code
the preferred style is to set the CDEFS \fImake\fP macro
to define the HOSTTYPE:
.{V
	CDEFS= -D'HOSTTYPE`
.}V
(Note, as always, the reversed sense of the quotes.)
Then in a single header file translate the HOSTTYPE into various
macros which explain the property required
(HAVE_QUOTA, HAVE_UNIONWAIT, USE_LOCKF).
See the example (\fI/usr/msrc/local/bin/entomb/libtomb/libtomb.h\fP)
in the appendix.

.PP
If different files or options must be used in the target Makefile
the preferred style is to use an \fIm4\fP \fBifelse\fP spread out
over multiple source lines.  This allows the maintainer to see
the list of special hosts clearly.
Here is an example:
.{V
	all:$& ${OBJ}
		${CC} -o $@ ${OBJ} 'ifelse(
	HOSTTYPE,`HPUX7',` -lBSD',
	HOSTTYPE,`ETA10',` -lbsd',
	HOSTTYPE,`V386',` -lbsd',
	HOSTTYPE,`EPIX',` -lsocket -lbsd',
	HOSTTYPE,`S81',` -h')`
.}V

.PP
Each makefile change should have its own \fBifelse\fP.
While this is slightly more typing to enter originally, it clearly
shows locations where consideration should be given for new ports.

.PP
While you are editing the Make.host file you should add a distrib
target to push the installed product to all similar CPU types:
.{V
	distrib: FRC
		distrib -c ${BIN}/${PROG} HOST
.}V
is correct for quite a few Makefiles.

.NH 2
Build a meta makefile

.PP
Copy in the meta makefile template then edit
to set things that have %% ... %% around them:
.{V
	# cp -i /usr/pub/ug/doc/Makefiles.tmpl/meta Makefile
	# vi +/%% Makefile
.}V

.PP
The list below outlines each configuration to be made in the new makefile.
Most of them are trivial with a little practice.


.NH 3
# Meta Makefile for
.BX "%% product name %%"

.PP
Fill in the product's name.

.NH 3
OTHER= 
.BX "%% :r !echo *| fmt  %%"

.PP
Filter out RCS, Makefile, Distfile.

.NH 3
INTO= 
.BX "%% /usr/src/local/bin/foo %%"

.PP
Set the destination for the source code.

.NH 3
HOSTS= 
.BX "%% -S  or  -St VAX8800,S81,SUN3,SUN4,SUN5...  or  -m staff %%"

.PP
Fill in the types of the platforms this product supports.

.NH 3
MDEFS= 
.BX "%% -DNEW=''-o $$USER -g staff'' %%"

.PP
This is almost always blank; it is really just a command line hook.
I've used it to pass on options to \fIdistrib\fP while building special versions
of a product (versions for /usr/new/bin, for example).

.NH 3
DDEFS= -dINTO=${INTO} ${MDEFS} ${HOSTS}

.PP
This is the collection of the previous three macros, in much the
same way as CFLAGS is a collection of DEBUG, CDEFS, and INCLUDE in
a C product's makefile.
This macro is hardly ever changed; the only change might be to add
a \-\fBf\fP to force a different name for the Distfile.

.NH 3
# %% add any targets you want %%

.PP
Most makefiles do not need any other targets.
We need targets for those very old Distfiles that 
call \fImake\fP to build files to send.
Most of these old-style distfiles have been (and should be)
replaced with ``@file@'' constructions.

.NH 2
Fix the Distfile.

.PP
Make sure no uncool edits are included in the Distfile.
.{V
	# rcsdiff Distfile
	# co -l Distfile
.}V

.PP
Make sure we have a header comment: (# $\&Id: ... $)
.{V
	# vi Distfile
.}V

.PP
Change any reference to /usr/src/.... to ${INTO}.
This prevents an old-style `distrib -S' from sending the source
to all source platforms.

.PP
Make sure the ( . ) -> ( HOST ) does at least:
.{V
	except_pat ( /RCS /Makefile /Distfile /Make\e\e.host );
.}V

.PP
Add to the end of the Distfile:
.{V
	( @Make.host@ ) -> ( HOST )
		install -b ${INTO}/Makefile ;
.}V

The above is also a replacement for `` ( @Makefile@ ) -> ( HOST ) ...''.
Since the Make.host file is run through \fIm4\fP every time \fIdistrib\fP
is run it would be installed by \fIrdist\fP every time, without the \-\fBb\fP.

.NH
Platforms

.PP
Read the list of machines \fIexpound\fP outputs; edit the makefile's
HOSTS macro if you don't like this list.
.{V
	# /usr/msrc/Admin/bin/expound
.}V

.PP
Read the makefile and distfile output to see if they look OK.
NOTE: the /tmp/fdistXXXXX/Make.host substitution is the result of
the @\fIfile\fP@ substitution and is normal in the output distfile.

.NH
Source

.PP
See if the files sent look OK to you (yeah, you'll have to \fIservo\fP to
the hosts to look).
.{V
	# make source
	# /usr/msrc/Admin/bin/servo ls -aCF
.}V

.NH
Servo

.PP
Servo run remote shell to execute a command in each platform
source directory.

.PP
Use servo to remove any stray files (``Distfile'' is common).
.{V
	# servo rm -f Distfile a.out
.}V

.NH 2
Compile the product

.PP
You might have to edit the Make.host file to specify special
libraries or \-D switches.
See the example appended (/usr/msrc/local/bin/install.d/Make.host).
.{V
	# make all
.}V

.PP
If you are using mkcmd on a NEXT2 you'll have to add some lines to Make.host:
.{V
	'ifelse(HOSTTYPE,`NEXT2',`.SUFFIXES:
	.SUFFIXES: .c .h .o .s',`')`
.}V
and copy the lines which generate main.c and main.h to the Makefile,
add main.c and main.h to GEN= in the Makefile.

.PP
If the original makefile (now ``Make.host'') has a target to test the binary,
you may add that target to the meta makefile as:
.{V
	test: FRC
		${LOOP}
.}V
and then run:
.{V
	# make test
.}V

.NH
Cleanup

.PP
Check in the files; comment about the change to the new \fIdistrib\fP
system in the RCS log.
.{V
	# ci -u Distfile Make.host Makefile
.}V

.NH
Using the meta source setup

.PP
Five major tools help us in /usr/msrc:
\fImake\fP, \fIdistrib\fP, \fIrcsvg\fP, \fIservo\fP, and \fIexpound\fP.

.NH 2
make
.PP
Produce an error (for Vic Able).

.NH 2
make HOSTS=-aC\fIdept.cf\fP \fItarget\fP

.PP
Build \fItarget\fP on the hosts in department \fIdept.cf\fP.
This is the normal usuage.  Without specifying a \fBHOSTS\fP
value there might be no updates made, or all source hosts might get
an update, or some unknown subset of the hosts on the local
network might get an update.

.PP
It is good form to always provide a HOSTS= clause.

.NH 2
make HOSTS=-m\fIhost\fP \fItarget\fP

.PP
Build \fItarget\fP only on the given \fIhost\fP.
This is useful when a new host is added to the platform list to
do the first port.

.NH 2
make HOSTS=-m\fIhost\fP INTO=/tmp/\fIblob\fP \fItarget\fP

.PP
Build \fItarget\fP only on the given \fIhost\fP in the directory
/tmp/\fIblob\fP.
This is useful when you want to work as yourself in /tmp (or your
home directory).

.NH 2
rcsvg \fIversion-name\fP

.PP
The version level configuration management tool built on
top of RCS is \fIrcsvg\fP.
Nothing in the meta source layout prevents the use of symbolic
RCS revision names fomr making a complete version.  In fact the
same version should work for each and every supported platform.

.NH 2
expound

.PP
Provides a list of the hosts this product runs on, and then
pages each makefile/distfile combination.

.NH 2
expound \fIhost\fP

.PP
Show the makefile and distfile for the given host, or an error message.
This is how we check to see that \fIm4\fP is doing what we thought.

.NH 2
servo \fIcommand\fP

.PP
Run the given shell command on each of the platform hosts.
Note that the \fIcommand\fP must be quoted to pass \fImake\fP,
as it is inserted in a makefile for execution.
(This is a bug, of course.)

.PP
This is used to remove files from the platform hosts (like old
source files or ``lint.out'').

.NH 2
distrib -E -f \fIfile\fP -m \fIhost\fP | ${PAGER-less}

.PP
Expand the given \fIfile\fP as we would on \fIhost\fP.
NOTE: this doesn't always make sense \- most files aren't expanded by \fIm4\fP
in the Distfile.

.NH
Compatibility with the old system

.PP
The old
.{V
	distrib -S
.}V
system causes an error from rdist that ${INTO} is not
defined.  \fIThis is a safety feature\fP.

.NH
Summary

.PP
This makes porting software to new and existing platforms easy.
