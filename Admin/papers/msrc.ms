.\" $Id: msrc.ms,v 2.4 1997/02/24 19:56:07 ksb Exp $

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
The Meta Source System
.AI
Distributed Systems Process Development
.DA December 1996

.AB
Maintaining source and binary files on heterogenous networks has
always been a difficult and time consuming task.  Programs such as
\fIrdist\fP(1) perform adequately in homogeneous environments but they
are not robust enough for heterogenous environments.
This document describes a powerful system to
simplify source management tasks: the \fIMeta Source System\fP
more commonly "msrc" (pronounced m-source).
.AE

.NH 1
TERMINOLOGY

.PP
There are a number of terms that are used in conjunction with the Meta
Source System that need to be defined.

.LI "Product"
program, library, configuration file, document, or the like
that is under control of the meta source system.

.LI "Platform"
the unique CPU / OS combination that describes a particular machine.

.LI "Meta Source"
this is the `source' for a particular product, whether that actually
be C source code or a shell script, that has been generalized for multiple
platforms.

.LI "Platform Source"
the source for a particular product that has been specialized for a
particular platform from the generalized source.

.LI "Master Source Host"
this is the machine that holds all of the meta
sources for each of the products.
The meta sources are held under the directory hierarchy /usr/msrc.
Currently this machine is \*Qsvr6.dscm.fedex.com\*U,
but there is a name server alias \*Qmsrc.sa.fedex.com\*U
that always points to this host.

.LI "Source Host"
this is a machine for a particular platform that holds
the platform source, builds, and distributes the product to its peers.

.LI "Peers"
machines that are of the same platform.  Each gets its products
from a Platform Source host.

.LI "Clients"
machines, usually diskless, the boot off another machine

.LI "Meta-Makefile"
this is the Makefile in the meta source for a product that controls
how the source is distributed to the platform source hosts.

.LI "Make.host"
this is makefile for the platform source.  It is preprocessed by m4
before being distributed to the platform source host.

.LI "Distfile"
this is the control file for distrib which in turn controls rdist.

.LI "Revision"
(taken from RCS) is a file with labeled contents.  A revision
number can recall from a cache a known contents.

.LI "Version"
a group of files with coordinated contents that together build a
product with known attributes.  Each version of a product can be
recalled by a version name.

.NH 1
DISTRIB

.PP
Distrib is the backbone of the meta source system.  It controls how
the sources are handled when being pushed to the different platform
source hosts.  Distrib processes the Make.host file with m4 and turns it
into the Makefile for the particular platform source.  Its control
file, Distfile, specifies if and how files are processed and
transferred to the platform source host.

.NH 2
Distfile

.PP
The following is a list of m4 variables which are defined in the distrib
configuration file.

.LI "HOST"
The canonical host name of the machine which the files are being
transferred to.

.LI "HOSTTYPE"
An abbreviation used to describe a platform, it is based on the
cross product of the architecture and the operating system.

.LI "SHORTHOST"
An abbreviation of the canonical host name used to trigger the rare
host exception.

.LI "HOSTOS"
The operating system release number converted to an integer by
reserving 2 digits for each position.
For example BSD version 4.4 would be 404.

.LI "HASSRC"
If defined for machines that have a platform /usr/src (to some value
which has no meaning to the baseline system) or undefined for any
other machine.

.PP
The distrib program outputs these column headers in the order
expected under -V.

.PP
The best way to understand Distfile's is to look at an example.
.{V
msrc # pwd
/usr/msrc/usr/local/bin/rn
msrc # cat -n Distfile
     1  # $Id: msrc.ms,v 2.4 1997/02/24 19:56:07 ksb Exp $
     2  #
     3  ( . ) -> ( HOST )
     4          except_pat ( /RCS /Distfile /Makefile /Make\e.host /config\e.* /hints  );
     5          install ${INTO} ;
     6
     7  ( @Make.host@ ) -> ( HOST )
     8          install -b ${INTO}/Makefile ;
     9
    10  ( config.HOSTTYPE ) -> ( HOST )
    11          install -b ${INTO}/config.h ;
.}V

Lines 1 and 2 are, of course, comments.  Distrib ignores any line
beginning with a pound sign (#).

.PP
Line 3 says, transfer the current directory contents to HOST, which is
a m4 variable that is expanded to the appropriate host name.

.PP
Line 4 says, remove the files that match the following regular
expressions from the list of files to be transferred.

.PP
Line 5, installs those files into the directory ${INTO} which is
Makefile variable that is passed down to distrib.

.PP
The rule on lines 7-8 says, process the file `Make.host' with m4
before transferring it to HOST and install it into the directory
${INTO} as the file `Makefile'.

.PP
The rule on lines 10-11 is a little more tricky but it shows how
leverage that can be used to transfer platform specific files.  There
is a m4 variable, HOSTTYPE, that is defined in the different distrib
configuration files which specifies the platform designator.  There
several files in the directory (i.e. config.SUN4, config.SUN5,
config.S81) that are config headers specific to that particular
platform.  Distrib processes the distfile with m4 before passing it
off to rdist.  Therefore, this rule would say, for example, to install
config.SUN4 into ${INTO}/config.h on the SUN4 platform source host.

.NH 2
Peer updates

.PP
After the master source host has delivered and compiled a product on
a platform source host it can install that product and push it to
all the peer subscribers.
\fIDistrib\fP is used for this operation as well.

.NH
Configuration Management

.PP
Configuration managements is described by content
(file, product, facility, and system) in four levels
revision, version, release, and signature.
These are tracked by rational and indexed by names.
For more information on CM theory see [1] because
a full review of configuration management is well beyond the scope
of this paper.

.PP
The meta source system depends on RCS (as presented) to track
revisions to each individual source file.  Every product meta
source directory has a RCS cache directory, while \fBnone\fP of
the platform directories do.

.PP
This not only makes the ISO9000 auditor happy it assures that bugs
fixed on any system must be #if'd or at least cross checked on
each other system (because they share files).

.PP
By marking coordinated revisions with a common symbolic name (see ci(1)
for a description of symbolic names) we denote a version of a product.
The program \fIrcsvg\fP extracts such versions of a product into
a stage directory (which is then treated as if it were under /usr/msrc).

.PP
In the next paper we will cover \fIrcsvg\fP in greater depth.

.NH 1
Meta-Makefile

.PP
The Meta-makefile is the Makefile in the meta source of a product.
It has several predefined targets that are used.

.LI all
Build the product, but do not install it.

.LI clean
Remove all the temporary files used to build the product, but
not the source.

.LI deinstall
Remove the installed product and documentation.

.LI depend
Update the transitive dependencies in this Makefile, don't use -a

.LI dirs
Build the destination directories for the product.

.LI distrib
Assuming the product is installed, copy it to the machine's peers
with distrib.

.LI install
Install the product using install(1l).

.LI lint
Reports any questionable C constructs under each platform's lint.

.LI mkcat
Use mkcat(8l) to install the documentation for the product.

.LI print
Output the source code to the product to a printer.

.LI source
Checkout all the source files needed to build the product.

.LI spotless
Use rcsclean(1l) to remove unmodified products.

.LI tags
Use ctags(1) to build a database of functions in the product.

.PP
Note:  Some of these targets do not make sense for the nature of the
product, viz. it does not make sense to lint a shell script.

.NH 1
Meta-Makefile variables

.PP
The following is a description of the most commonly used Meta-Makefile
variables.  There are many defined, but these are the ones that will
most commonly be used.

.LI "HOSTS"

This macro defines the distrib configuration file as well as the 
platforms or hosts expected in that file.
This is set on the command line, i.e.
.sp 1
	make HOSTS=-aCrcd.cf
.sp 1
	make HOSTS=-mfrodo

See the manual page for distrib for example usage.

.LI "INTO"

This is the location where the sources are installed.  It usually
mirrors where the product is actually installed,
e.g. /usr/src/local/lib/news/moderators would be the appropriate
locations for the moderators file, which gets installed in
/usr/local/lib/news.

.NH 1
HINTS

.PP
If you need to use quotes in a file being processed by m4, use the m4
changequote command to something else.  A good set to change them to
is Control-B and Control-E, begin/end.  The m4 line would look
something like the following:

.{V
\'changequote(^B,^E)dnl
^B\fIshell_code\fP
^Echangequote(`,')dnl
\`
.}V

.NH 1
EXAMPLE SESSION

.PP
The following is the script output from the installation of the
weather program (/usr/msrc/local/bin/weather).

.PP
First of all, you need to install the package on the platform
source hosts.

.{V
# make HOSTS=-aCcsd.cf install
distrib -d\*QINTO=/usr/src/local/bin/weather\*Q  -Ccsd.cf -S
sage.sa.fedex.com: updating host sage.sa.fedex.com
sage.sa.fedex.com: .: installing
sage.sa.fedex.com: sage: /usr/src/local/bin/weather: mkdir
sage.sa.fedex.com: ./weather.man: installing
sage.sa.fedex.com: ./weather.sh: installing
sage.sa.fedex.com: /tmp/fdista18211/1/Make.host: installing
sage.sa.fedex.com: updating of sage.sa.fedex.com finished
staff.sa.fedex.com: updating host staff.sa.fedex.com
staff.sa.fedex.com: ./weather.sh: updating
staff.sa.fedex.com: staff: /usr/src/local/bin/weather/Makefile: updated
staff.sa.fedex.com: updating of staff.sa.fedex.com finished
b.dscm.fedex.com: updating host b.dscm.fedex.com
b.dscm.fedex.com: .: installing
b.dscm.fedex.com: b: /usr/src/local/bin/weather: mkdir
b.dscm.fedex.com: ./weather.man: installing
b.dscm.fedex.com: ./weather.sh: installing
b.dscm.fedex.com: /tmp/fdista18211/1/Make.host: installing
b.dscm.fedex.com: updating of b.dscm.fedex.com finished
for i in `distrib -H -Ccsd.cf -S` ; do  echo $i: ; rsh $i -n sh -c ' \*U.
/usr/local/lib/distrib/local.defs && cd /usr/src/local/bin/weather &&
make DESTDIR=\*Q\*Q DEBUG=\*Q\*Q install\*U ' ; done
sage.sa.fedex.com:
	cat weather.sh > weather
	chmod 755 weather
	/usr/local/bin/install -c weather /usr/local/bin
staff.sa.fedex.com:
cat weather.sh > weather
chmod 755 weather
/usr/local/bin/install -c weather /usr/local/bin
b.dscm.fedex.com:
	cat weather.sh > weather
	chmod 755 weather
	/usr/local/bin/install -c weather /usr/local/bin
.}V

.PP
Now that it has been installed on the platform source host, you must
then distrib it to the machine's peers.

.{V
# make HOSTS=-aCcsd.cf distrib
for i in `distrib -H -Ccsd.cf -S` ; do  echo $i: ; rsh $i -n sh -c '".
/usr/local/lib/distrib/local.defs && cd /usr/src/local/bin/weather &&
make DESTDIR="" DEBUG="" distrib"' ; done
sage.sa.fedex.com:
	distrib -c /usr/local/bin/weather HOST
mentor.sa.fedex.com: updating host mentor.sa.fedex.com
mentor.sa.fedex.com: /usr/local/bin/weather: updating
mentor.sa.fedex.com: updating of mentor.sa.fedex.com finished
expert.sa.fedex.com: updating host expert.sa.fedex.com
expert.sa.fedex.com: /usr/local/bin/weather: updating
expert.sa.fedex.com: updating of expert.sa.fedex.com finished
tyro.sa.fedex.com: updating host tyro.sa.fedex.com
tyro.sa.fedex.com: /usr/local/bin/weather: updating
tyro.sa.fedex.com: updating of tyro.sa.fedex.com finished
staff.sa.fedex.com:
distrib -c /usr/local/bin/weather HOST
extra.sa.fedex.com: updating host extra.sa.fedex.com
extra.sa.fedex.com: /usr/local/bin/weather: updating
extra.sa.fedex.com: updating of extra.sa.fedex.com finished
b.dscm.fedex.com:
	distrib -c /usr/local/bin/weather HOST
updating host prism.sa.fedex.com
installing: /usr/local/bin/weather
updating host oasis.sa.fedex.com
updating: /usr/local/bin/weather
updating host mm.sa.fedex.com
updating: /usr/local/bin/weather
updating host a.dscm.fedex.com
updating: /usr/local/bin/weather
.}V

.NH 1
Using the Meta Source System with self-configuring products

.PP
Many software packages, such as the GNU products, come with
configuration scripts that will create a makefile to build the
product.  At first thought this may not seem like it fits into
the Meta Source culture, but it is very easy to map these
packages into the meta source system.

.PP
Modify the configure script so that it outputs Make.build instead
of Makefile.  This way it will not clobber the Make.host file.

.PP
Modify the Make.host file to the following:
.{V
all:
        ./configure
        make -f Make.build all
.}V


.NH 1
Support files

.PP
To keep the right product compiled for the right platform up to date
on each system we must build some configuration files.

.NH 2
distrib.cf

.PP
One of the intragal parts of the Meta Source System is the
distrib configuration file.  The following is a sample configuration
file:
.{V
#
# official              Short    CPU    OS      has
# Hostname              Hostname Type   Version source
#
zoyd.sa.fedex.com      ZOYD     SUN5   203     .
solarium.sa.fedex.com  SOLARIUM SUN5   203     .
lv426.sa.fedex.com     LV426    SUN5   203     yes
.}V

\fIofficial Hostname\fP
is the full qualified hostname.

.PP
If \fBhas source\fP is not \*Q.\*U, then this machine is a
platform host machine.

.PP
In this case, lv426 is the platform source machine. The sources
for the product will be rdist'ed to lv426 and the source will be
compiled there. After being installed, lv426 will distribute
the product to its two peers, zoyd and solarium.

.PP
This file doesn't have to be the same on every host.  The
Master Source Host might not know about all the peer hosts
for each platform -- it might only know about the platform
hosts themselves.

.NH 2
local.defs

.PP
When the master source host reaches out through the network to a
platform host to run a make(1) command we need a known environment.
The default system umask and PATH are (more than likely) not exactly
what we need for our build environment.

.PP
Each meta makefile uses the shell source command (.) to incorporate
a file into the environment before it runs any other command.
This file usually augments the system PATH and sets any other
environment variables we might need to run the C compiler, make,
and the like.


.NH 2
root's .rhosts

.PP
While the meta source system doesn't require root access to build products
it does usually require root access to install them.
Enough said?


.NH 2
NFS access to /usr/msrc

.PP
The master source host usually exports /usr/msrc \fBread only\fP to
all the machines on the Intranet.  This allows unrelated departments
to keep up to date on tools used/provided by other groups without any
explicit trust relationship.

.NH 1
Summary

.PP
We've outlined a system for maintaining products across a 
heterogenous network.
Given the list of hosts and apropos trust relationships cooperating
staff are able to maintain and distribute versions of program,
libraries, documents and third party files to each and every
host on the local network.

.PP
This is not quite enough if you need to send these products outside
the local administrative domain.
In the next paper we shall examine the need to package up related
works into a more global distribution.
