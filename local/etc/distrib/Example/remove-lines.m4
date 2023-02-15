# $Id: remove-lines.m4,v 2.1 1991/06/20 14:27:02 ksb Exp $
#	$Compile(bad): distrib -f %f -E boiler
#	$Compile(good): distrib -f %f -E staff
define(SEND_IT,ifelse(HOSTTYPE,`S81',`yes',HOSTTYPE,`VAX780',`yes',HOSTTYPE,`SUN3',`yes',HOSTTYPE,`SUN4',`yes',`no'))dnl
define(ECHO,ifelse(SEND_IT,`yes',`',``dnl ''))dnl
ECHO()
ECHO()define(CON_H,ifelse(HOSTTYPE,`SUN4',`cons.streams',HOSTYPE,`SUN3',`cons.streams',`cons.h'))dnl
ECHO()( . ) -> ( HOST )
ECHO()	except_pat ( /RCS /cons.\.* );
ECHO()	install /usr/src/local/etc/conserver ;
ECHO()
ECHO()( CON_H ) -> ( HOST )
ECHO()	install /usr/src/local/etc/conserver/cons.h ;
