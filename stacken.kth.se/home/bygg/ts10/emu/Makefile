CC = gcc
LD = gcc
CFLAGS = -g -O3 -c -DDEBUG
LDFLAGS = -g
INCLUDES = -I..
LIBS = 

OBJS = \
	commands.o \
	debug.o \
	disk.o \
	main.o \
	socket.o \
	system.o \
	tape.o \
	timer.o \
	tty.o \
	unit.o \
	utils.o

all: ${OBJS}

.c.o:
	${CC} ${CFLAGS} ${INCLUDES} $<

clean:
	@rm -f *.o
