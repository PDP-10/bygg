CC = gcc
LD = gcc
CFLAGS = -g -O3 -c -DDEBUG
LDFLAGS = -g
INCLUDES = -I..
LIBS = 

OBJS = \
	asm.o \
	commands.o \
	cpu_extend.o \
	cpu_float.o \
	cpu_integer.o \
	cpu_ksapr.o \
	cpu_ksio.o \
	cpu_kspag.o \
	cpu_kspi.o \
	cpu_kstim.o \
	cpu_main.o \
	disasm.o \
	fe.o \
	instruction.o \
	memory.o \
	system.o \
	uba.o

all: ${OBJS}

.c.o:
	${CC} ${CFLAGS} ${INCLUDES} $<

clean:
	@rm -f *.o
