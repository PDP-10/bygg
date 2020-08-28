CC = gcc
LD = gcc
CFLAGS = -g -O3 -c -DDEBUG
LDFLAGS = -g
INCLUDES = -I..
LIBS = 

OBJS = \
	commands.o \
	cpu_branch.o \
	cpu_call.o \
	cpu_compare.o \
	cpu_field.o \
	cpu_integer.o \
	cpu_main.o \
	cpu_move.o \
	cpu_misc.o \
	disasm.o \
	ka780.o \
	instruction.o \
	memory.o \
	system.o

all: ${OBJS}

vax:
	${LD} ${LDFLAGS} -o $@ ${OBJS}

.c.o:
	${CC} ${CFLAGS} ${INCLUDES} $<

clean:
	@rm -f *.o
	@rm -f vax
