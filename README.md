# $Id: README,v 1.3 1997/10/26 18:21:16 ksb Exp $

This is the third kit (from ksb) you need to install all his tools.
You should have "msrc0" and "mkcmd" installed at this point.  If
you've installed "install" you might be even better off.

[N.B. this kit is rooted at "msrc" and builds msrc/local and such, most
 other kits are rooted at local and build bin, lib, etc and the like.
 This is only slightly confusing.  Follow the steps outlined.]

The kit contains the templates for building your own meta source products,
the code to the "distrib" front end for rdist, and the RCS add-on "rcsvg"
(RCS version get).  These are the parts of the meta source system you
have to get from ksb, the rest (rdist, m4, make, compilers) you can get
from FreeBSD, the GNU Project, or your platform vendor.

The HTML docs on the meta source layout are under "doc/msrc" and are scant
but will do for anyone who has done multiplatform UNIX products.

After you have rdist (version 6 would be best), m4, and make installed on
your key development machine (source repository) you should follow the
steps below to construct the environment which lets you build the rest
of my tools.  The meta source for a product lives some place OTHER than
/usr/src.  Only platform translated source is ever put under /usr/src
(which follow the purpose of /usr/src).

This package is kept under /usr/msrc on my hosts, you can pick anyplace
you like because the meta source is all relative path'd.

distrib, rcsvg, docs, notes on where to get rdist-6.1, GNU m4

Install all these in /usr/local.

Get the latest rdist from about:
	ftp://usc.edu/pub/rdist/rdist.tar.gz

If your machine doesn't come with an m4 get one from
	ftp://prep.ai.mit.edu/pub/gnu/m4-*

If you don't have a "make" you can use the GNU one, but it is _way_
non-standard and you'll have trouble porting to any other make.


Then follow the INSTALL notes.  Read the papers (Admin/papers).
Some of the INSTALL notes are duplicated in lib/distrib/README; look
there for more help if INSTALL doesn't work well for you.


--
ksb@fedex.com, 10 July 1997
