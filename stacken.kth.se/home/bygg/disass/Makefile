DISASS=	disass.o sysdep.o parser.o addr.o buffer.o filio.o memory.o \
	loader.o rpn.o xwin.o\
	common.o\
	example.o i8051.o i8086.o\
	hexa.o \
	m6502.o \
	m6800.o m6805.o m6809.o m68k.o mips.o\
	pdp10.o pdp11.o ppc.o vax.o z8.o z80.o

CC_XJUNK=-I/usr/X11R6/include
LD_XJUNK=-L/usr/X11R6/lib -lXext -lX11

.c.o:
	gcc -O -g $(CC_XJUNK) -c $*.c

disass:	$(DISASS)
	gcc -o disass $(DISASS) -L. -l comnd $(LD_XJUNK)

tar:
	tar -cf disass.tar Makefile *.c *.h pedant.cmd .hg

$(DISASS): disass.h
