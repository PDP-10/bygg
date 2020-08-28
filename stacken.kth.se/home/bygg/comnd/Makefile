COMND=comnd.o cmsys.o
TESTW=testw.o xwin.o buf.o terminal.o

CC_XJUNK=-I/usr/X11R6/include
LD_XJUNK=-L/usr/X11R6/lib -lXext -lX11

.c.o:
	gcc -O -Wall -g $(CC_XJUNK) -c $*.c

test:	test.o
	gcc -o test test.o -L. -l comnd

test2:	test2.o
	gcc -o test2 test2.o -L. -l comnd

testw:	$(TESTW)
	gcc -o testw $(TESTW) -L. -l comnd $(LD_XJUNK)

parseif: parseif.o
	gcc -o parseif parseif.o -L. -l comnd

fm:     fm.o
	gcc -o fm fm.o -L. -l comnd

libcomnd: $(COMND)
	ar cru libcomnd.a $(COMND)

$(COMND): comnd.h
