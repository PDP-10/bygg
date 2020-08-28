PDP10=pdp10_cpu.o pdp10_deuna.o pdp10_dz.o pdp10_fe.o pdp10_ksio.o\
	pdp10_lp20.o pdp10_mdfp.o pdp10_pag.o pdp10_pt.o pdp10_rp.o\
	pdp10_sys.o pdp10_tim.o pdp10_tu.o pdp10_xtnd.o\
	scp.o scp_tty.o\
	sim_sock.o sim_tmxr.o

.c.o:
	gcc -DUSE_INT64 -O -g -c $*.c

test:	$(PDP10) eth_dummy.o
	gcc -o test $(PDP10) eth_dummy.o -lm

testb:	$(PDP10) eth_bpf.o
	gcc -o testb $(PDP10) eth_bpf.o -lm

pdp10_deuna.o: dec_deuna.c
