COMND=comnd.o sysdep.o
TEST=test.o

.c.o:
	gcc -O -g -c $*.c

test:	$(COMND) test.o
	gcc -o test test.o $(COMND)

testxx:	$(COMND) testxx.o
	gcc -o testxx testxx.o $(COMND)

parseif: $(COMND) parseif.o
	gcc -o parseif parseif.o $(COMND)

$(COMND): comnd.h
