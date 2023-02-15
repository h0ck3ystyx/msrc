# $Id: Makefile,v 2.6 2000/06/17 22:03:21 ksb Exp $
#
#	package builder Makefile for %% product %%

MSRC=/usr/msrc
TMP=/tmp
STAGE=${TMP}/%% release_name-number %%
VGOPTS=%% options to rcsvg -- forced to "-R" on command line sometimes %%

quit:
	echo "Use \`make all' to contruct a release of this package" 1>&2
	false

all: source

${TMP}:
	echo "Cannot find ${TMP}, which we will not build."
	false

${STAGE}: ${TMP}
	rcsvg ${VGOPTS} -S ${STAGE} %% Release name %%
	rm -f ${STAGE}/Makefile
	mv ${STAGE}/Make.meta ${STAGE}/Makefile

# %% for each product "BIN/PROD" %%
${STAGE}/%%BIN/PROD%%: ${STAGE}
	cd ${MSRC}/%%local/BIN/PROD%% && rcsvg ${VGOPTS} -S $@ %% Version %%

# %% repeat above as needed ... %%

clean: FRC
	rm -rf ${STAGE}

# %% we do not pass "HOSTS=" here and we might/should -- ksb. %%
calls depend deinstall dirs install mkcat source msource rsource: stage
	cd ${STAGE} && ${MAKE} $@

stage: %% ${STAGE}/BIN/PROD ... %%

FRC:
