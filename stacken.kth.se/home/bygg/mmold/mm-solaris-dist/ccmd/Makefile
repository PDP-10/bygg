#
# Copyright (c) 1986, 1990 by The Trustees of Columbia University in
# the City of New York.  Permission is granted to any individual or
# institution to use, copy, or redistribute this software so long as it
# is not sold for profit, provided this copyright notice is retained.
#
# Be sure to run "make depend" before building on a new system.
#
# Reasonable invocations are:
#
#	make			4.3bsd, Ultrix, Umax
#	make hpux		for HP-UX systems
#	make aix		for AIX systems
#	make bsd		for 4.2bsd
#	make sun		for SunOS
#	make ultrix40		for Ultrix 4.0	
#	make next		for NeXT
#	make SVR3		for SVR3
#	make SVR2		for SVR2
#	make SYSV		for older versions of System V 
#	make MSDOS		for MSDOS
#	make solaris		for Solaris

CC	      = ${CC}

OS	      =

DEBUGOPT      = -g

MAKEFILE      = Makefile

CFLAGS        = $(DEBUGOPT) $(OS)

OSLIBS	      =

DEST	      = /usr/local/lib

CPP	      =	/lib/cpp $(DEFINES)

RANLIB	      =	ranlib

CO	      =	co

CI	      =	ci

CP	      = cp

INSTALL	      = install -c

SHELL	      = /bin/sh

LOCALINCLUDE  = ccmd.h \
		cmfnc.h \
		cmfncs.h \
		ccmdmd.h \
		datime.h

INCLUDEDIR    = /usr/include/local

M4FILES       = cmfnc.h4 \
		cmconf.h4 \
		cmfnc.top \
		cmfncs.top \
		cmcfm.cnf \
		cmchar.cnf \
		cmfil.cnf \
		cmfld.cnf \
		cmgnrc.cnf \
		cmgrp.cnf \
		cmkey.cnf \
		cmnoi.cnf \
		cmnum.cnf \
		cmpara.cnf \
		cmqst.cnf \
		cmswi.cnf \
		cmtad.cnf \
		cmtok.cnf \
		cmtxt.cnf \
		cmusr.cnf

HDRS	      =	ccmdlib.h \
		ccmd.h \
		ccmdmd.h \
		cmfil.h \
		cmfnc.h \
		cmfncs.h \
		cmgrp.h \
		cmusr.h \
		cmver.h \
		datime.h \
		dtpat.h \
		filelist.h \
		machdep.h \
		site.h \
		tzone.h

OBJS	      =	ccmd.o \
		ccmdio.o \
		ccmdmd.o \
		ccmdst.o \
		ccmdut.o \
		cmcfm.o \
		cmchar.o \
		cmfil.o \
		cmfld.o \
		cmgrp.o \
		cmkey.o \
		cmmisc.o \
		cmnoi.o \
		cmnum.o \
		cmpara.o \
		cmqst.o \
		cmswi.o \
		cmtad.o \
		cmtok.o \
		cmtxt.o \
		cmusr.o \
		cmver.o \
		cursor.o \
		datime.o \
		dir.o \
		filelist.o \
		getenv.o \
		setenv.o \
		stdact.o \
		wild.o

SRCS	      = ccmd.c \
		ccmdio.c \
		ccmdmd.unx \
		ccmdst.c \
		ccmdut.c \
		cmcfm.c \
		cmchar.c \
		cmfil.c \
		cmfld.c \
		cmgrp.c \
		cmkey.c \
		cmmisc.c \
		cmnoi.c \
		cmnum.c \
		cmpara.c \
		cmqst.c \
		cmswi.c \
		cmtad.c \
		cmtok.c \
		cmtxt.c \
		cmusr.c \
		cmver.c \
		cursor.c \
		datime.c \
		dir.c \
		filelist.c \
		getenv.c \
		setenv.c \
		stdact.c \
		wild.c \
		incversion.c \
		test.c \
		skel.c

LIBRARY	      = ccmd.a

PROGS	      = test \
		skel

ALL	      = $(LIBRARY) $(PROGS)

all:		$(ALL)

debug:
		$(MAKE) $(MFLAGS) DEBUGOPT=-g all

SVR2 SVR3 SYSV:
		$(MAKE) OS="-D$@ -DDIRENTLIB -DSOLARIS" OSLIBS= RANLIB=: all

aix AIX:
		$(MAKE) OS=-DSVR2 OSLIBS= RANLIB=: all

bsd BSD:
		$(MAKE) $(MFLAGS) OS=-DBSD all

sun sunos SUN SUNOS next NeXT NEXT ultrix40:
		$(MAKE) $(MFLAGS) OS="-DBSD -DHAVE_VOIDSIG" all

solaris:
		$(MAKE) $(MFLAGS) OS="-DSOLARIS" CC=gcc all
hpux:
		$(MAKE) OS= OSLIBS= RANLIB=: all

msdos:
		$(MAKE) $(MFLAGS) -f makefile.dos

$(LIBRARY):	$(OBJS)
		ar cru $(LIBRARY) $?
		@$(RANLIB) $(LIBRARY)

test:		test.o $(LIBRARY)
		$(CC) $(CFLAGS) -o $@ $@.o $(LIBRARY) -ltermlib $(OSLIBS)

skel:		skel.o $(LIBRARY)
		$(CC) $(CFLAGS) -o $@ $@.o $(LIBRARY) -ltermlib $(OSLIBS)

cmfncs.h cmfnc.h: $(M4FILES) split.awk
		m4 cmfnc.h4 | awk -f split.awk

ccmdmd.c:	ccmdmd.unx
		rm -f ccmdmd.c
		cp ccmdmd.unx ccmdmd.c
		chmod u+w ccmdmd.c

clean:;		rm -f $(OBJS) incversion

realclean:;	rm -f $(OBJS) $(LIBRARY) cmfncs.h cmfnc.h split.awk \
			dependencies incversion

checkout:;	$(CO) $(HDRS) $(SRCS) $(M4FILES) README Makefile

newversion:	incversion
		PATH=/bin:/usr/bin test -t 0
		$(CO) -l cmver.h
		./incversion < cmver.h > cmver.h.new
		mv cmver.h.new cmver.h
		$(CI) -u cmver.h

incversion:	incversion.c	
		$(CC) $(CFLAGS) -o $@ $@.c

depend:		dependencies
		rm -f junk
		sed '/^# DO NOT DELETE THIS LINE/,$$d' < $(MAKEFILE) > junk
		(echo '# DO NOT DELETE THIS LINE'; cat dependencies) >> junk
		mv junk $(MAKEFILE)
		rm dependencies

localdepend:;	rm -f junk
		sed -e '1,/^# DO NOT DELETE/!{ /:[ 	]*\//d; }' \
			< $(MAKEFILE) > junk
		mv junk $(MAKEFILE)

dependencies:	$(HDRS) site.h
		rm -f junk
		for f in $(SRCS); do $(CPP) < $$f | \
		    sed -n '/^#[ 0-9line]*"\(..*\)\".*$$/ '"s||$$f: \1|p" | \
		    sort -u ; \
		done | sed -e 's|\.c: |.o: |' -e 's|: *\./|: |' > junk
		mv junk dependencies

$(INCLUDEDIR):;
		-mkdir $(INCLUDEDIR)

install:	$(LIBRARY) $(INCLUDEDIR)
		$(INSTALL) $(LIBRARY) $(DEST)/lib$(LIBRARY)
		$(RANLIB) $(DEST)/lib$(LIBRARY)
		$(CP) $(LOCALINCLUDE) $(INCLUDEDIR)

tags:           $(HDRS) $(SRCS); ctags $(HDRS) $(SRCS)

TAGS:		$(HDRS) $(SRCS); etags $(HDRS) $(SRCS)

ccmd.tar:	$(HDRS) $(SRCS) $(M4FILES) README Makefile
		@echo 'Did you run "make localdepend"?'
		tar cf $@ README Makefile $(HDRS) $(SRCS) $(M4FILES)

update:         $(DEST)/$(LIBRARY)

$(DEST)/$(LIBRARY): $(SRCS) $(HDRS) $(EXTHDRS)
		@-ar xo $(DEST)/$(LIBRARY)
		@make -f $(MAKEFILE) DEST=$(DEST) install clean

# this line to make sure all of the headers get co'ed.
ccmd.o:		$(HDRS)

# The rest of this file contains dependencies generated with
# "make depend" -- don't add anything below or it will go away.
# DO NOT DELETE THIS LINE
ccmd.o: ccmd.h
ccmd.o: ccmdlib.h
ccmd.o: ccmdmd.h
ccmd.o: cmfnc.h
ccmd.o: cmfncs.h
ccmd.o: datime.h
ccmd.o: site.h
ccmdio.o: ccmd.h
ccmdio.o: ccmdlib.h
ccmdio.o: ccmdmd.h
ccmdio.o: cmfnc.h
ccmdio.o: cmfncs.h
ccmdio.o: datime.h
ccmdio.o: site.h
ccmdmd.o: ccmd.h
ccmdmd.o: ccmdlib.h
ccmdmd.o: ccmdmd.h
ccmdmd.o: cmfnc.h
ccmdmd.o: cmfncs.h
ccmdmd.o: datime.h
ccmdmd.o: site.h
ccmdst.o: ccmd.h
ccmdst.o: ccmdmd.h
ccmdst.o: cmfnc.h
ccmdst.o: cmfncs.h
ccmdst.o: datime.h
ccmdut.o: ccmd.h
ccmdut.o: ccmdlib.h
ccmdut.o: ccmdmd.h
ccmdut.o: cmfnc.h
ccmdut.o: cmfncs.h
ccmdut.o: datime.h
ccmdut.o: site.h
cmcfm.o: ccmd.h
cmcfm.o: ccmdlib.h
cmcfm.o: ccmdmd.h
cmcfm.o: cmfnc.h
cmcfm.o: cmfncs.h
cmcfm.o: datime.h
cmcfm.o: site.h
cmchar.o: ccmd.h
cmchar.o: ccmdlib.h
cmchar.o: ccmdmd.h
cmchar.o: cmfnc.h
cmchar.o: cmfncs.h
cmchar.o: datime.h
cmchar.o: site.h
cmfil.o: ccmd.h
cmfil.o: ccmdlib.h
cmfil.o: ccmdmd.h
cmfil.o: cmfil.h
cmfil.o: cmfnc.h
cmfil.o: cmfncs.h
cmfil.o: datime.h
cmfil.o: filelist.h
cmfil.o: site.h
cmfld.o: ccmd.h
cmfld.o: ccmdmd.h
cmfld.o: cmfnc.h
cmfld.o: cmfncs.h
cmfld.o: datime.h
cmgrp.o: ccmd.h
cmgrp.o: ccmdlib.h
cmgrp.o: ccmdmd.h
cmgrp.o: cmfnc.h
cmgrp.o: cmfncs.h
cmgrp.o: cmgrp.h
cmgrp.o: datime.h
cmgrp.o: site.h
cmkey.o: ccmd.h
cmkey.o: ccmdlib.h
cmkey.o: ccmdmd.h
cmkey.o: cmfnc.h
cmkey.o: cmfncs.h
cmkey.o: datime.h
cmkey.o: site.h
cmmisc.o: ccmd.h
cmmisc.o: ccmdlib.h
cmmisc.o: ccmdmd.h
cmmisc.o: cmfnc.h
cmmisc.o: cmfncs.h
cmmisc.o: datime.h
cmmisc.o: site.h
cmnoi.o: ccmd.h
cmnoi.o: ccmdlib.h
cmnoi.o: ccmdmd.h
cmnoi.o: cmfnc.h
cmnoi.o: cmfncs.h
cmnoi.o: datime.h
cmnoi.o: site.h
cmnum.o: ccmd.h
cmnum.o: ccmdlib.h
cmnum.o: ccmdmd.h
cmnum.o: cmfnc.h
cmnum.o: cmfncs.h
cmnum.o: datime.h
cmnum.o: site.h
cmpara.o: ccmd.h
cmpara.o: ccmdlib.h
cmpara.o: ccmdmd.h
cmpara.o: cmfnc.h
cmpara.o: cmfncs.h
cmpara.o: datime.h
cmpara.o: site.h
cmqst.o: ccmd.h
cmqst.o: ccmdlib.h
cmqst.o: ccmdmd.h
cmqst.o: cmfnc.h
cmqst.o: cmfncs.h
cmqst.o: datime.h
cmqst.o: site.h
cmswi.o: ccmd.h
cmswi.o: ccmdlib.h
cmswi.o: ccmdmd.h
cmswi.o: cmfnc.h
cmswi.o: cmfncs.h
cmswi.o: datime.h
cmswi.o: site.h
cmtad.o: ccmd.h
cmtad.o: ccmdlib.h
cmtad.o: ccmdmd.h
cmtad.o: cmfnc.h
cmtad.o: cmfncs.h
cmtad.o: datime.h
cmtad.o: site.h
cmtok.o: ccmd.h
cmtok.o: ccmdlib.h
cmtok.o: ccmdmd.h
cmtok.o: cmfnc.h
cmtok.o: cmfncs.h
cmtok.o: datime.h
cmtok.o: site.h
cmtxt.o: ccmd.h
cmtxt.o: ccmdlib.h
cmtxt.o: ccmdmd.h
cmtxt.o: cmfnc.h
cmtxt.o: cmfncs.h
cmtxt.o: datime.h
cmtxt.o: site.h
cmusr.o: ccmd.h
cmusr.o: ccmdlib.h
cmusr.o: ccmdmd.h
cmusr.o: cmfnc.h
cmusr.o: cmfncs.h
cmusr.o: cmusr.h
cmusr.o: datime.h
cmusr.o: site.h
cmver.o: cmver.h
datime.o: ccmd.h
datime.o: ccmdlib.h
datime.o: ccmdmd.h
datime.o: cmfnc.h
datime.o: datime.h
datime.o: dtpat.h
datime.o: site.h
datime.o: tzone.h
filelist.o: ccmd.h
filelist.o: ccmdlib.h
filelist.o: ccmdmd.h
filelist.o: cmfnc.h
filelist.o: datime.h
filelist.o: filelist.h
filelist.o: site.h
stdact.o: ccmd.h
stdact.o: ccmdlib.h
stdact.o: ccmdmd.h
stdact.o: cmfnc.h
stdact.o: cmfncs.h
stdact.o: datime.h
stdact.o: site.h
test.o: ccmd.h
test.o: ccmdmd.h
test.o: cmfnc.h
test.o: datime.h
skel.o: ccmd.h
skel.o: ccmdmd.h
skel.o: cmfnc.h
skel.o: datime.h
