# Common make file contents.

.c.o:
	gcc -O -Wall -g -c $*.c

all:	test module

test:	test.o
	gcc -rdynamic -o test test.o -L. -l comnd ${LDADD}

module: module.o
	gcc -shared -nostartfiles -o module module.o

tar:
	tar -cf dltest.tar BSDmakefile GNUmakefile Makefile *.c *.h
