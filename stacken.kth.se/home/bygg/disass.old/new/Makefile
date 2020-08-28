DISASS=disass.o sysdep.o parser.o addr.o buffer.o filio.o memory.o rpn.o\
	xwin.o\
	common.o\
	example.o pdp10.o\
	i8086.o m6800.o m6809.o m68k.o pdp11.o vax.o z80.o

CC_XJUNK=-I/usr/X11R6/include
LD_XJUNK=-L/usr/X11R6/lib -lXext -lX11

.c.o:
	gcc -O -g $(CC_XJUNK) -c $*.c

disass: $(DISASS)
	gcc -o disass $(DISASS) ccmd.a -ltermlib $(LD_XJUNK)

$(DISASS): disass.h
