COMND=comnd.o sysdep.o
TESTW=testw.o xwin.o buf.o terminal.o

CC_XJUNK=-I/usr/X11R6/include
LD_XJUNK=-L/usr/X11R6/lib -lXext -lX11

.c.o:
	gcc -O -g $(CC_XJUNK) -c $*.c

test:	$(COMND) test.o
	gcc -o test test.o $(COMND)

testxx:	$(COMND) testxx.o
	gcc -o testxx testxx.o $(COMND)

testw:	$(COMND) $(TESTW)
	gcc -o testw $(TESTW) $(COMND) $(LD_XJUNK)

kaka:	$(COMND) kaka.o
	gcc -o kaka kaka.o $(COMND)

$(COMND): comnd.h
