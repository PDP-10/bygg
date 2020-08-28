/*
** Simulate a Digital KMC11 with DUP11's on a Unibus.
*/

/*
** Written 2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

/*
** Revision history:
**
** 2002-02-15    Massive changes/cleanups.
** 2002-01-23    Modify for version 2.9.
** 2002-01-17    First attempt.
*/

/*
** Loose ends, known problems etc:
**
**   We don't handle NXM on the unibus.  At all.  In fact, we don't
**   generate control-outs.
**
**   We don't really implement the DUP registers.
**
**   We don't do anything but full-duplex DDCMP.
**
**   We don't implement buffer flushing.
*/

#define DEBUG

#ifdef DEBUG
#  define DF_CMD    0001	/* Print commands. */
#  define DF_TX     0002	/* Print tx done. */
#  define DF_RX     0004	/* Print rx done. */
#  define DF_DATA   0010	/* Print data. */
#  define DF_QUEUE  0020	/* Print rx/tx queue changes. */

uint32 kmc_debug = 0;

#endif /* DEBUG */

extern t_stat sync_open(int* channel, char* params);
extern int sync_read(int line, uint8* packet, int length);
extern void sync_write(int line, uint8* packet, int length);
extern void sync_close(int line);

/* bits, sel0: */

#define KMC_RUN    0100000	/* Run bit. */
#define KMC_MRC    0040000	/* Master clear. */
#define KMC_CWR    0020000	/* CRAM write. */
#define KMC_SLU    0010000	/* Step Line Unit. */
#define KMC_LUL    0004000	/* Line Unit Loop. */
#define KMC_RMO    0002000	/* ROM output. */
#define KMC_RMI    0001000	/* ROM input. */
#define KMC_SUP    0000400	/* Step microprocessor. */
#define KMC_RQI    0000200	/* Request input. */
#define KMC_IEO    0000020	/* Interrupt enable output. */
#define KMC_IEI    0000001	/* Interrupt enable input. */

/* bits, sel2: */

#define KMC_OVR    0100000	/* Buffer overrun. */
#define KMC_LINE   0077400	/* Line number. */
#define KMC_RDO    0000200	/* Ready for output transaction. */
#define KMC_RDI    0000020	/* Ready for input transaction. */
#define KMC_IOT    0000004	/* I/O type, 1 = rx, 0 = tx. */
#define KMC_CMD    0000003	/* Command code. */
#  define CMD_BUFFIN     0	/*   Buffer in. */
#  define CMD_CTRLIN     1	/*   Control in. */
#  define CMD_BASEIN     3	/*   Base in. */
#  define CMD_BUFFOUT    0	/*   Buffer out. */
#  define CMD_CTRLOUT    1	/*   Control out. */

/* bits, sel6: */

#define BFR_EOM    0010000	/* End of message. */
#define BFR_KIL    0010000	/* Buffer Kill. */

/* buffer descriptor list bits: */

#define BDL_LDS    0100000	/* Last descriptor in list. */
#define BDL_RSY    0010000	/* Resync transmitter. */
#define BDL_XAD    0006000	/* Buffer address bits 17 & 16. */
#define BDL_EOM    0001000	/* End of message. */
#define BDL_SOM    0000400	/* Start of message. */

#define KMC_CRAMSIZE 1024	/* Size of CRAM. */

#ifndef MAXDUP
#  define MAXDUP 2		/* Number of DUP-11's we can handle. */
#endif

#define MAXQUEUE 16		/* Number of rx bdl's we can handle. */

#define MAXMSG 2000		/* Largest message we handle. */

/* local variables: */

uint32 kmc_sel0;
uint32 kmc_sel2;
uint32 kmc_sel4;
uint32 kmc_sel6;

uint16 kmc_microcode[KMC_CRAMSIZE];

uint32 dup_rxcsr[MAXDUP];
uint32 dup_rxdbuf[MAXDUP];
uint32 dup_parcsr[MAXDUP];
uint32 dup_txcsr[MAXDUP];
uint32 dup_txdbuf[MAXDUP];

int dup_sli[MAXDUP];

struct dupblock {
  uint32 rxqueue[MAXQUEUE];	/* Queue of bd's to receive into. */
  uint32 rxcount;		/* No. bd's in above. */
  uint32 rxnext;		/* Next bd to receive into. */
  uint32 txqueue[MAXQUEUE];	/* Queue of bd's to transmit. */
  uint32 txcount;		/* No. bd's in above. */
  uint32 txnext;		/* Next bd to transmit. */
  uint32 txnow;			/* No. bd's we are transmitting now. */
};

typedef struct dupblock dupblock;

dupblock dup[MAXDUP] = { 0 };

/* state/timing/etc: */

t_bool kmc_output = FALSE;	/* Flag, need at least one output. */
int kmc_interval = 10000;	/* Polling interval. */

/* forward decls: */

t_stat kmc_rd(int32* data, int32 PA, int32 access);
t_stat kmc_wr(int32 data, int32 PA, int32 access);
t_stat kmc_svc(UNIT * uptr);
t_stat kmc_reset(DEVICE * dptr);

t_stat dup_rd(int32* data, int32 PA, int32 access);
t_stat dup_wr(int32 data, int32 PA, int32 access);
t_stat dup_svc(UNIT * uptr);
t_stat dup_reset(DEVICE * dptr);
t_stat dup_attach(UNIT * uptr, char * cptr);
t_stat dup_detach(UNIT * uptr);

/* KMC11 data structs: */

DIB kmc_dib = { 1, IOBA_KMC, IOLN_KMC, &kmc_rd, &kmc_wr };

UNIT kmc_unit = { UDATA (&kmc_svc, 0, 0) };

REG kmc_reg[] = {
  { ORDATA ( SEL0, kmc_sel0, 16) },
  { ORDATA ( SEL2, kmc_sel2, 16) },
  { ORDATA ( SEL4, kmc_sel4, 16) },
  { ORDATA ( SEL6, kmc_sel6, 16) },

  { ORDATA ( DEBUG, kmc_debug, 32) },
  { DRDATA ( INTERVAL, kmc_interval, 32) },

  { GRDATA (DEVADDR, kmc_dib.ba, KMC_RDX, 32, 0), REG_HRO },
  { FLDATA (*DEVENB, kmc_dib.enb, 0), REG_HRO },
  { NULL },
};

MTAB kmc_mod[] = {
  { MTAB_XTD|MTAB_VDV, 010, "address", "ADDRESS",
    &set_addr, &show_addr, &kmc_dib },
  { MTAB_XTD|MTAB_VDV, 1, NULL, "ENABLED",
    &set_enbdis, NULL, &kmc_dib },
  { MTAB_XTD|MTAB_VDV, 0, NULL, "DISABLED",
    &set_enbdis, NULL, &kmc_dib },
  { 0 },
};

DEVICE kmc_dev = {
  "KMC", &kmc_unit, kmc_reg, kmc_mod,
  1, KMC_RDX, 13, 1, KMC_RDX, 8,
  NULL, NULL, &kmc_reset,
  NULL, NULL, NULL,
};

/* DUP11 data structs: */

DIB dup_dib = { 1, IOBA_DUP, (IOLN_DUP * MAXDUP), &dup_rd, &dup_wr };

UNIT dup_unit[MAXDUP] = {
  { UDATA (&dup_svc, UNIT_ATTABLE, 0) },
};

REG dup_reg[] = {

  { GRDATA (DEVADDR, dup_dib.ba, DUP_RDX, 32, 0), REG_HRO },
  { FLDATA (*DEVENB, dup_dib.enb, 0), REG_HRO },
  
  { NULL },
};

MTAB dup_mod[] = {
  { MTAB_XTD|MTAB_VDV, 010, "address", "ADDRESS",
    &set_addr, &show_addr, &dup_dib },
  { MTAB_XTD|MTAB_VDV, 1, NULL, "ENABLED",
    &set_enbdis, NULL, &dup_dib },
  { MTAB_XTD|MTAB_VDV, 0, NULL, "DISABLED",
    &set_enbdis, NULL, &dup_dib },
  { 0 },
};

DEVICE dup_dev = {
  "DUP", dup_unit, dup_reg, dup_mod,
  2, DUP_RDX, 13, 1, DUP_RDX, 8,
  NULL, NULL, &dup_reset,
  NULL, &dup_attach, &dup_detach,
};

/*
** Update interrupt status:
*/

void kmc_updints(void)
{
  if (kmc_sel0 & KMC_IEI) {
    if (kmc_sel2 & KMC_RDI) {
      SET_INT(KMCA);
    } else {
      CLR_INT(KMCA);
    }
  }

  if (kmc_sel0 & KMC_IEO) {
    if (kmc_sel2 & KMC_RDO) {
      SET_INT(KMCB);
    } else {
      CLR_INT(KMCB);
    }
  }
}

/*
** Try to set the RDO bit.  If it can be set, set it and return true,
** else return false.
*/

t_bool kmc_getrdo(void)
{
  if (kmc_sel2 & KMC_RDO)	/* Already on? */
    return FALSE;
  if (kmc_sel2 & KMC_RDI)	/* Busy doing input? */
    return FALSE;
  kmc_sel2 |= KMC_RDO;
  return TRUE;
}

/*
** Try to do an output command.
*/

void kmc_tryoutput(void)
{
  int i, j;
  dupblock* d;
  uint32 ba;

  if (kmc_output) {
    kmc_output = FALSE;
    for (i = 0; i < MAXDUP; i += 1) {
      d = &dup[i];
      if (d->rxnext > 0) {
	kmc_output = TRUE;	/* At least one, need more scanning. */
	if (kmc_getrdo()) {
	  ba = d->rxqueue[0];
	  kmc_sel2 &= ~KMC_LINE;
	  kmc_sel2 |= (i << 8);
	  kmc_sel2 &= ~KMC_CMD;
	  kmc_sel2 |= CMD_BUFFOUT;
	  kmc_sel2 |= KMC_IOT;	/* Buffer type. */
	  kmc_sel4 = ba & 0177777;
	  kmc_sel6 = (ba >> 2) & 0140000;
	  kmc_sel6 |= BFR_EOM;

	  for (j = 1; j < d->rxcount; j += 1) {
	    d->rxqueue[j-1] = d->rxqueue[j];
	  }
	  d->rxcount -= 1;
	  d->rxnext -= 1;

	  if (kmc_debug & DF_QUEUE) {
	    printf("DUP%d: (tryout) ba = %6o, rxcount = %d, rxnext = %d\r\n",
		   i, ba, d->rxcount, d->rxnext);
	  }
	  kmc_updints();
	}
	return;
      } 
      if (d->txnext > 0) {
	kmc_output = TRUE;	/* At least one, need more scanning. */
	if (kmc_getrdo()) {
	  ba = d->txqueue[0];
	  kmc_sel2 &= ~KMC_LINE;
	  kmc_sel2 |= (i << 8);
	  kmc_sel2 &= ~KMC_CMD;
	  kmc_sel2 |= CMD_BUFFOUT;
	  kmc_sel2 &= ~KMC_IOT;	/* Buffer type. */
	  kmc_sel4 = ba & 0177777;
	  kmc_sel6 = (ba >> 2) & 0140000;

	  for (j = 1; j < d->txcount; j += 1) {
	    d->txqueue[j-1] = d->txqueue[j];
	  }
	  d->txcount -= 1;
	  d->txnext -= 1;

	  if (kmc_debug & DF_QUEUE) {
	    printf("DUP%d: (tryout) ba = %6o, txcount = %d, txnext = %d\r\n",
		   i, ba, d->txcount, d->txnext);
	  }
	  kmc_updints();
	}
	return;
      }
    }
  }
}

/*
** Try to start output.  Does nothing if output is already in progress,
** or if there are no packets in the output queue.
*/

void dup_tryxmit(int dupindex)
{
  dupblock* d;

  uint8 buffer[MAXMSG];		/* Data buffer. */
  int pos;			/* Offset into above. */

  uint32 bda;			/* Buffer Descriptor Address. */
  uint32 bd[3];			/* Buffer Descriptor. */
  uint32 bufaddr;		/* Buffer Address. */
  uint32 buflen;		/* Buffer Length. */

  int msglen;			/* Message length. */
  int dcount;			/* Number of descriptors to use. */
  t_bool lds;			/* Found last descriptor. */

  int i;			/* Random loop var. */
  int delay;			/* Estimated transmit time. */

  extern int32 tmxr_poll;	/* calibrated delay */

  d = &dup[dupindex];

  if (d->txnow > 0) return;	/* If xmit in progress, quit. */
  if (d->txcount <= d->txnext) return;

  /*
  ** check the transmit buffers we have queued up and find out if
  ** we have a full DDCMP frame.
  */

  lds = FALSE;			/* No last descriptor yet. */
  dcount = msglen = 0;		/* No data yet. */

  /* accumulate length, scan for LDS flag */

  for (i = d->txnext; i < d->txcount; i += 1) {
    bda = d->txqueue[i];
    (void) unibus_read(&bd[0], bda);
    (void) unibus_read(&bd[1], bda + 2);
    (void) unibus_read(&bd[2], bda + 4);

    dcount += 1;		/* Count one more descriptor. */
    msglen += bd[1];		/* Count some more bytes. */
    if (bd[2] & BDL_LDS) {
      lds = TRUE;
      break;
    }
  }
  
  if (!lds) return;		/* If no end of message, give up. */

  d->txnow = dcount;		/* Got a full frame, will send or ignore it. */

  if (msglen <= MAXMSG) {	/* If message fits in buffer, - */
    pos = 0;

    for (i = d->txnext; i < (d->txnext + dcount); i += 1) {
      bda = d->txqueue[i];
      (void) unibus_read(&bd[0], bda);
      (void) unibus_read(&bd[1], bda + 2);
      (void) unibus_read(&bd[2], bda + 4);

      bufaddr = bd[0] + ((bd[2] & 06000) << 6);
      buflen = bd[1];

      (void) dma_read(bufaddr, &buffer[pos], buflen);
      pos += buflen;
    }

    if (dup_sli[dupindex] >= 0) {
      sync_write(dup_sli[dupindex], buffer, msglen);
    }
  }

#define IPS() (tmxr_poll * 50)	/* UGH! */

  /*
  ** Delay calculation:
  ** delay (instructions) = bytes * IPS * 8 / speed;
  ** either do this in floating point, or be very careful about
  ** overflows...
  */

  delay = IPS() / (19200 >> 10);
  delay *= msglen;
  delay >>= 7;

  sim_activate(&dup_unit[dupindex], delay);
}

/*
** Here with a bdl for some new receive buffers.  Set them up.
*/

void dup_newrxbuf(int line, int32 ba)
{
  dupblock* d;
  int32 w3;

  d = &dup[line];

  for (;;) {
    if (d->rxcount < MAXQUEUE) {
      d->rxqueue[d->rxcount] = ba;
      d->rxcount += 1;
    }

    (void) unibus_read(&w3, ba + 4);
    if (w3 & BDL_LDS)
      break;

    ba += 6;
  }

#ifdef DEBUG
  if (kmc_debug & DF_QUEUE) {
    printf("DUP%d: (newrxb) rxcount = %d, rxnext = %d\r\n",
	   line, d->rxcount, d->rxnext);
  }
#endif

}

/*
** Here with a bdl for some new transmit buffers.  Set them up and then
** try to start output if not already active.
*/

void dup_newtxbuf(int line, int32 ba)
{
  dupblock* d;
  int32 w3;

  d = &dup[line];

  for (;;) {
    if (d->txcount < MAXQUEUE) {
      d->txqueue[d->txcount] = ba;
      d->txcount += 1;
    }
    (void) unibus_read(&w3, ba + 4);
    if (w3 & BDL_LDS)
      break;

    ba += 6;
  }

#ifdef DEBUG
  if (kmc_debug & DF_QUEUE) {
    printf("DUP%d: (newtxb) txcount = %d, txnext = %d\r\n",
	   line, d->txcount, d->txnext);
  }
#endif

  dup_tryxmit(line);		/* Try to start output. */
}

/*
** Here to store a block of data into a receive buffer.
*/

void dup_receive(int line, uint8* data, int count)
{
  int i;
  dupblock* d;
  uint32 bda;
  uint32 bd[3];
  uint32 ba;
  uint32 bl;

  d = &dup[line];

  if (d->rxcount > d->rxnext) {
    bda = d->rxqueue[d->rxnext];
    (void) unibus_read(&bd[0], bda);
    (void) unibus_read(&bd[1], bda + 2);
    (void) unibus_read(&bd[2], bda + 4);

    ba = bd[0] + ((bd[2] & 06000) << 6);
    bl = bd[1];

    if (count > bl) count = bl;	/* XXX */

    (void) dma_write(ba, data, count);

    bd[2] |= (BDL_SOM | BDL_EOM);

    (void) unibus_write(bd[2], bda + 4);

    d->rxnext += 1;
  }
}

/*
** Try to receive data for a given line:
*/

void dup_tryreceive(int dupindex)
{
  int length;
  uint8 buffer[MAXMSG];

  if (dup_sli[dupindex] >= 0) {	/* Got a sync line? */
    length = sync_read(dup_sli[dupindex], buffer, MAXMSG);
    if (length > 0) {		/* Got data? */
      if (kmc_debug & DF_RX) {
	printf("DUP%d: receiving %d bytes\r\n", dupindex, length);
      }
      dup_receive(dupindex, buffer, length);
      kmc_output = TRUE;	/* Flag this. */
    }
  }
}

/*
** testing testing
*/

#ifdef DEBUG

void prbdl(int32 ba, int prbuf)
{
  int32 w1, w2, w3;
  int32 dp;

  for (;;) {
    (void) unibus_read(&w1, ba);
    (void) unibus_read(&w2, ba + 2);
    (void) unibus_read(&w3, ba + 4);

    printf("  bd = %6o/%6o/%6o\r\n", w1, w2, w3);

    if (prbuf) {
      if (w2 > 20) w2 = 20;
      dp = w1 + ((w3 & 06000) << 6);

      while (w2 > 0) {
	(void) unibus_read(&w1, dp);
	dp += 2;
	w2 -= 2;

	printf(" %2x %2x", w1 & 0xff, w1 >> 8);
      }
      printf("\r\n");
    }
    if (w3 & BDL_LDS) break;
    ba += 6;
  }
}

#endif

/*
** Here to perform an input command:
*/

void kmc_doinput(void)
{
  int line;
  int32 ba;

  line = (kmc_sel2 & 077400) >> 8;
  ba = ((kmc_sel6 & 0140000) << 2) + kmc_sel4;

#ifdef DEBUG
  if (kmc_debug & DF_CMD) {
    printf("KMC: input command: %6o/%6o/%6o",
	   kmc_sel2, kmc_sel4, kmc_sel6);
    switch (kmc_sel2 & 7) {
    case 0:
      printf(" (bufr in)");
      prbdl(ba, 1);
      break;
    case 4:
      printf(" (bufr out)");
      prbdl(ba, 0);
    }
    printf("\r\n");
  }
#endif

  switch (kmc_sel2 & 7) {
  case 0:			/* Buffer in, data to send: */
    dup_newtxbuf(line, ba);
    break;
  case 1:			/* Control in. */
    /*
    ** The only thing this does is tell us to run DDCMP, in full duplex,
    ** but that is the only thing we know how to do anyway...
    */
    break;
  case 3:			/* Base in. */
    /*
    ** The only thing this does is tell the KMC what unibus address
    ** the dup is at.  But we already know...
    */
    break;
  case 4:			/* Buffer in, receive buffer for us... */
    dup_newrxbuf(line, ba);
    break;
  }
}

/*
** master clear the KMC:
*/

void kmc_mclear(void)
{
  int i;
  dupblock* d;

  kmc_sel0 = KMC_MRC;
  kmc_sel2 = 0;
  kmc_sel4 = 0;
  kmc_sel6 = 0;

  /* clear out the dup's as well. */

  for (i = 0; i < MAXDUP; i += 1) {
    d = &dup[i];
    d->rxcount = 0;
    d->rxnext = 0;
    d->txcount = 0;
    d->txnext = 0;
    d->txnow = 0;
    sim_cancel(&dup_unit[i]);	/* Stop xmit wait. */
  }
  sim_cancel(&kmc_unit);	/* Stop the clock. */
}

/*
** DUP11, read registers:
*/

t_stat dup_rd(int32* data, int32 PA, int32 access)
{
  int dupno;

  dupno = ((PA - dup_dib.ba) >> 3) & (MAXDUP - 1);

  switch ((PA >> 1) & 03) {
  case 00:
    *data = dup_rxcsr[dupno];
    break;
  case 01:
    *data = dup_rxdbuf[dupno];
    break;
  case 02:
    *data = dup_txcsr[dupno];
    break;
  case 03:
    *data = dup_txdbuf[dupno];
    break;
  }
  return SCPE_OK;
}

/*
** KMC11, read registers:
*/

t_stat kmc_rd(int32* data, int32 PA, int32 access)
{
  switch ((PA >> 1) & 03) {
  case 00:
    *data = kmc_sel0;
    break;
  case 01:
    *data = kmc_sel2;
    break;
  case 02:
    *data = kmc_sel4;
    break;
  case 03:
    *data = kmc_sel6;
    break;
  }
  return SCPE_OK;
}

/*
** DUP11, write registers:
*/

t_stat dup_wr(int32 data, int32 PA, int32 access)
{
  int dupno;

  dupno = ((PA - dup_dib.ba) >> 3) & (MAXDUP - 1);

  switch ((PA >> 1) & 03) {
  case 00:
    dup_rxcsr[dupno] = data;
    break;
  case 01:
    dup_parcsr[dupno] = data;
    break;
  case 02:
    dup_txcsr[dupno] = data;
    break;
  case 03:
    dup_txdbuf[dupno] = data;
    break;
  }
  return SCPE_OK;
}

/*
** KMC11, write registers:
*/

t_stat kmc_wr(int32 data, int32 PA, int32 access)
{
  uint32 toggle;

  switch ((PA >> 1) & 03) {
  case 00:
    if (access == WRITEB) {
      data = (PA & 1)
	? (((data & 0377) << 8) | (kmc_sel0 & 0377))
	: ((data & 0377) | (kmc_sel0 & 0177400));
    }
    toggle = kmc_sel0 ^ data;
    kmc_sel0 = data;
    if (kmc_sel0 & KMC_MRC) {
      kmc_mclear();
      break;
    }
    if ((kmc_sel0 & KMC_CWR) && (kmc_sel0 & KMC_RMO)) {
      kmc_microcode[kmc_sel4 & (KMC_CRAMSIZE - 1)] = kmc_sel6;
    }
    if (toggle & KMC_RUN) {	/* Changing the run bit? */
      if (kmc_sel0 & KMC_RUN) {
	sim_activate(&kmc_unit, kmc_interval);
      } else {
	sim_cancel(&kmc_unit);
      }
    }
    break;
  case 01:
    if (access == WRITEB) {
      data = (PA & 1)
	? (((data & 0377) << 8) | (kmc_sel2 & 0377))
	: ((data & 0377) | (kmc_sel2 & 0177400));
    }
    if ((kmc_sel2 & KMC_RDI) && (!(data & KMC_RDI))) {
      kmc_sel2 = data;
      kmc_doinput();
    } else if ((kmc_sel2 & KMC_RDO) && (!(data & KMC_RDO))) {
      kmc_sel2 = data;
      kmc_tryoutput();
    } else {
      kmc_sel2 = data;
    }
    break;
  case 02:
    if (kmc_sel0 & KMC_RMO) {
      kmc_sel6 = kmc_microcode[data & (KMC_CRAMSIZE - 1)];
    }
    kmc_sel4 = data;
    break;
  case 03:
    kmc_sel6 = data;
    break;
  }
  
  if (kmc_output) {
    kmc_tryoutput();
  }
  if (kmc_sel0 & KMC_RQI) {
    if (!(kmc_sel2 & KMC_RDO)) {
      kmc_sel2 |= KMC_RDI;
    }
  }

  kmc_updints();

  return SCPE_OK;
}

/*
** DUP11 service routine:
*/

t_stat dup_svc(UNIT* uptr)
{
  int dupindex;
  dupblock* d;

  dupindex = uptr->u3;
  d = &dup[dupindex];
  
  if (d->txnow > 0) {
    d->txnext += d->txnow;
    d->txnow = 0;
    kmc_output = TRUE;
  }

  if (d->txcount > d->txnext) {
    dup_tryxmit(dupindex);
  }

  return SCPE_OK;
}

/*
** KMC11 service routine:
*/

t_stat kmc_svc (UNIT* uptr)
{
  int i;

  for (i = 0; i < MAXDUP; i += 1) {
    dup_tryreceive(i);
  }
  if (kmc_output) {
    kmc_tryoutput();		/* Try to do an output transaction. */
  }  
  sim_activate(&kmc_unit, kmc_interval);
  return SCPE_OK;
}

/*
** DUP11, reset device:
*/

t_stat dup_reset(DEVICE* dptr)
{
  static t_bool firsttime = TRUE;
  int i;

  if (firsttime) {
    for (i = 1; i < MAXDUP; i += 1) {
      dup_unit[i] = dup_unit[0]; /* Copy all the units. */
    }
    for (i = 0; i < MAXDUP; i += 1) {
      dup_sli[i] = -1;		/* No line index. */
      dup_unit[i].u3 = i;	/* Link dupblock to unit. */
    }
    firsttime = FALSE;		/* Once-only init done now. */
  }
  return SCPE_OK;
}

/*
** KMC11, reset device:
*/

t_stat kmc_reset(DEVICE* dptr)
{
  kmc_sel0 = 0;
  kmc_sel2 = 0;
  kmc_sel4 = 0;
  kmc_sel6 = 0;

  return SCPE_OK;
}

/*
** DUP11, attach device:
*/

t_stat dup_attach(UNIT* uptr, char* cptr)
{
  int dupno;
  t_stat r;
  char* tptr;

  dupno = uptr->u3;

  tptr = malloc(strlen(cptr) + 1);
  if (tptr == NULL) return SCPE_MEM;
  strcpy(tptr, cptr);

  if ((r = sync_open(&dup_sli[dupno], cptr)) != SCPE_OK) {
    free(tptr);
    return r;
  }

  uptr->filename = tptr;
  uptr->flags |= UNIT_ATT;

  return SCPE_OK;
}

/*
** DUP11, detach device:
*/

t_stat dup_detach(UNIT* uptr)
{
  int dupno;

  dupno = uptr->u3;

  sync_close(dup_sli[dupno]);
  dup_sli[dupno] = -1;

  if (uptr->flags & UNIT_ATT) {
    free(uptr->filename);
    uptr->filename = NULL;
    uptr->flags &= ~UNIT_ATT;
  }

  return SCPE_OK;
}
