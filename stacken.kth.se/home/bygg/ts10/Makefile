CC = gcc
LD = gcc
CFLAGS = -g -O3 -c -DDEBUG #-DSVR4SIGNALS
P10FLAGS = -DKS10 #-DIDLE
#CFLAGS = -g -O3 -c
LDFLAGS = -g
INCLUDES = -I.
LIBS = 

TS10_OBJS = \
	emu/commands.o \
	emu/debug.o \
	emu/disk.o \
	emu/main.o \
	emu/socket.o \
	emu/system.o \
	emu/tape.o \
	emu/timer.o \
	emu/tty.o \
	emu/unit.o \
	emu/utils.o \
	dev/rh.o \
	dev/rm.o \
	dev/rp.o \
	dev/tm.o

KS10_OBJS = \
	pdp10/asm.o \
	pdp10/commands.o \
	pdp10/cpu_extend.o \
	pdp10/cpu_float.o \
	pdp10/cpu_integer.o \
	pdp10/cpu_ksapr.o \
	pdp10/cpu_ksio.o \
	pdp10/cpu_kspag.o \
	pdp10/cpu_kspi.o \
	pdp10/cpu_kstim.o \
	pdp10/cpu_main.o \
	pdp10/disasm.o \
	pdp10/fe.o \
	pdp10/instruction.o \
	pdp10/memory.o \
	pdp10/system.o \
	pdp10/uba.o

VAX_OBJS = \
	vax/commands.o \
	vax/cpu_branch.o \
	vax/cpu_call.o \
	vax/cpu_compare.o \
	vax/cpu_field.o \
	vax/cpu_integer.o \
	vax/cpu_main.o \
	vax/cpu_move.o \
	vax/cpu_misc.o \
	vax/disasm.o \
	vax/ka780.o \
	vax/instruction.o \
	vax/memory.o \
	vax/system.o

ts10:
	cd emu; make CFLAGS="${CFLAGS}" all
	cd dev; make CFLAGS="${CFLAGS}" all
	cd pdp10; make CFLAGS="${CFLAGS} ${P10FLAGS}" all
	cd vax; make CFLAGS="${CFLAGS}" all
	${CC} ${LDFLAGS} -o $@ ${TS10_OBJS} ${KS10_OBJS} ${VAX_OBJS} ${LIBS}

clean:
	cd emu; make clean
	cd dev; make clean
	cd pdp10; make clean
	cd vax; make clean
	@rm -f ts10
