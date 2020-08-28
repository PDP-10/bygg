HEXSIM= hexsim.o \
	parser.o \
	buf.o \
	xwin.o \
	term.o \
	cpu.o \
	math.o \
	bg.o \
	memory.o \

CC_XJUNK=-I/usr/X11R6/include
LD_XJUNK=-L/usr/X11R6/lib -lXext -lX11

.c.o:
	gcc -O -g $(CC_XJUNK) -c $*.c

hexsim:	$(HEXSIM)
	gcc -o hexsim $(HEXSIM) -l pthread -L. -l comnd $(LD_XJUNK)

hexasm: hexasm.o
	gcc -o hexasm hexasm.o

tar:
	tar -cf hexsim.tar Makefile *.c *.h \
	2do.txt hexarch.txt paging.txt pedant.cmd .hg

$(HEXSIM): hexsim.h
