# mkcmd command line option parser description for distrib
# $Compile(*): ${mkcmd-mkcmd} std_help.m std_version.m %f
from '<sys/param.h>'
from '<sys/file.h>'
from '"envlist.h"'
from '"distrib.h"'
from '"machine.h"'

basename "distrib" ""
require "std_help.m" "std_version.m"

augment action 'V' {
	user 'ShowVer(ALT_CONFIG_PATH);'
}

%i
static char *rcsid =
	"$Id: distrib.m,v 4.22 1999/06/20 10:38:58 ksb Dist $";

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif
%%

# The first two PATH elements are reversed by a "support.m" file
# that we include on the mkcmd command line for support tools,
# for non-support builds are omit the module. -- ksb.
key "PATH" 2 initializer {
	"/usr/local/bin" "/opt/fedex/support/bin" "/usr/local/etc"
	"/usr/custom/bin" "/opt/gnu/bin" "/opt/gnu" "/opt/hppd/bin" "/opt/hppd"
	"/usr/ccs/bin" "/usr/bin" "/usr/ucb" "/usr/bsd" "/bin"
	"/usr/sbin" "/usr/etc" "/sbin" "/etc"
	"/usr/local/X11R6/bin" "/usr/bin/X11" "/usr/openwin/bin"
}

key "utils" 1 initializer {
	"m4" "rdist"
}

init "Init(%K<utils><apply><path><apply><quote><list>);"

# rdist only options, we don't look at them
# we collect them in this buffer
string [201] variable "acRdistOpts" {
	init '"-"'
	help "none"
}

accum[","] 'o' {
	local named "pcRdistO" "pcFeeder"
	param "rdist-opts"
	help "options to be passed down to rdist"
}

char* 'p' {
	named "pcRemoteD"
	param "rdistd"
	help "pass -p down to rdist for targets with two rdist versions"
}
char* 'P' {
	named "pcRemsh"
	param "rsh"
	help "pass -P down to rdsit for remote shell path"
}

action 'R' {
	update '%roN = \"remove\";%roT'
	help "tell rdist remove extraneous files"
}

boolean 'b' {
	named "fBinary"
	user '%roN = \"compare\";%roT'
	help "tell rdist to perform binary comparisons"
}

action 'n' {
	update '(void)strcat(acRdistOpts, "%l");'
	help "do not execute commands"
}

action 'q' {
	update '%roN = \"quiet\";%roT'
	help "tell rdist to be quiet"
}


action 'v' {
	update '%roN = \"verify\";%roT'
	help "tell rdist to verify"
}

action 'y' {
	update '%roN = \"younger\";%roT'
	help "tell rdist to update only younger files"
}

function 'd' {
	update 'KeepDef("-d");KeepDef(%a);'
	param "var=value"
	help "pass definition on to rdist"
}


# our options....
boolean 'x' {
	named "fVerbose"
	help "output shell commands to approximate actions"
}

exclude "EH"
boolean 'E' {
	named "fExam"
	help "output the m4 processed distfile on stdout"
	boolean 'F' {
		named "fFilter"
		init "0"
		help "force @file@ processing in files not named Distfile"
	}
}
boolean 'H' {
	named "fHosts"
	help "output the list of hosts to update"
}

exclude "cf"
boolean 'c' {
	named "fCmdLine"
	init "0"
	help "use a command-line distfile, like rdist"
# this could get out of control fast with -e (expect_pat) -i (install)
# maybe -o special="command" would be better?
	char* 's' {
		once named "pcSpecial"
		param "cmd"
		help "insert a special command into the command line distfile"
	}
}
char* 'f' {
	named "pcDistfile"
	param "distfile"
	init "(char *)0"
	help "the file to process"
}

exclude "amtI"
boolean 'a' {
	named "fAll"
	help "update all machines in the configuration"
}
accum [","] 'm' {
	named "pcOnly"
	param "machine"
	help "update just one machine, treat params as rdist labels"
}
accum [","] 't' {
	named "pcType"
	param "type"
	help "machine type to update, if no hosts are given"
}
boolean 'I' {
	named "fMyself"
	help "include myself in the default target list"
}
char* 'G' {
	once named "pcGuard"
	param "guard"
	help "m4 clause to select target hosts"
}
boolean 'S' {
	once named "fSource"
	help "update only source machines"
}
function 'D' {
	param "var=value"
	named "KeepM4"
	help "define an m4 variable from the command line"
}
char* 'C' {
	named "pcConfig"
	param "config"
	help "the list of machines and attributes"
}

# process the data
after {
	named "Post"
	update "%n(%ron);"
}

integer variable "iExt" {
	init "0"
	help "none"
}

list {
	param ""
	update "if (fCmdLine) {exit(DoCmd(%#, %@));}"
}

every {
	param "hosts"
	named "UpHost"
	update "if ((char *)0 != %rmn) {KeepDef(%a);}else {iExt += %n(%a);}"
	help "an explicit target list"
}

zero {
	named "Zero"
	update "iExt += %n();"
}

exit {
	update "if ((char*)0 != %rmn) {iExt += UpHost(%rmn);}Cleanup(1);"
	abort "exit(iExt);"
}
