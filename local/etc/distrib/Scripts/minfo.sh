#!/bin/ksh
# $Id: minfo.sh,v 4.2 1996/10/10 18:32:58 kb207252 Exp $
# tell me about a machine
#	minfo: usage [-S | -t type | -a | -m machine | machine]
#
distrib -E -f - <<\! "$@"
Machine:  HOST      (SHORTHOST)
Type:     HOSTTYPE/HOSTOS  ifelse(HOSTTYPE,MYTYPE,ifelse(MYOS,HOSTOS,`(same)',`(different os)'),`(different)')
ifdef(`RDIST_PATH',`	special rdist to push (RDIST_PATH)
')dnl
ifdef(`RDISTD_PATH',`	special rdistd on target (RDISTD_PATH)
')dnl
ifdef(`HASSRC',`	platform source machine (HASSRC)
')dnl
ifdef(`NISDOMAIN',`	yp server for NISDOMAIN
')dnl
ifdef(`NETINFO',`	netinfo server for NETINFO
')dnl
!
