/*
** Simulate a Digital DEUNA interface on a Unibus.
*/

/*
** Written 2001-2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

/*
** Revision history:
**
** 2002-01-23    Modify for version 2.9.
** 2001-10-11    Second beta release.
** 2001-09-30    First beta release.
*/

/*
** Todo list:
**
** Implement Multicast handling.
** Fixup error handling (bus timeouts etc).
** Implement counters.
** Possibly implement more than one interface.
*/

/* PCSR0 register definitions: */

#define DEUNA_SERI  0100000	/* <15>    Status Error Interrupt. */
#define DEUNA_PCEI  0040000	/* <14>    Port Command Error Interrupt. */
#define DEUNA_RXI   0020000	/* <13>    Receive Interrupt. */
#define DEUNA_TXI   0010000	/* <12>    Transmit Interrupt. */
#define DEUNA_DNI   0004000	/* <11>    Done Interrupt. */
#define DEUNA_RCBI  0002000	/* <10>    Receive Buffer Unavail Interrupt. */
#define DEUNA_USCI  0000400	/* <08>    Unsolicited State Chg Interrupt. */
#define DEUNA_INTR  0000200	/* <07>    Interrupt Summary. */
#define DEUNA_INTE  0000100	/* <06>    Interrupt Enable. */
#define DEUNA_RSET  0000040	/* <05>    Reset. */
#define DEUNA_PCMD  0000017	/* <03:00> Port Command field. */
#  define CMD_NOOP        0	/*   No-op. */
#  define CMD_GETPCB      1	/*   Get PCB base. */
#  define CMD_GETCMD      2	/*   Get Command. */
#  define CMD_SELFTEST    3	/*   Self-test init. */
#  define CMD_START       4	/*   Start xmit/recv. */
#  define CMD_BOOT        5	/*   Boot. */
#  define CMD_PDMD      010	/*   Polling Demand. */
#  define CMD_STOP      017	/*   Stop. */

/* PCSR1 register definitions: */

#define DEUNA_XPWR  0100000	/* <15>    Tranceiver power failure. */
#define DEUNA_ICAB  0040000	/* <14>    Port/Link cable failure. */
#define DEUNA_ECOD  0037400	/* <13:08> Self-test error code. */
#define DEUNA_PCTO  0000200	/* <07>    Port Command Timeout. */
#define DEUNA_TYPE  0000160	/* <06:04> Interface type. */
#  define TYPE_DEUNA   (0 << 4)	/*   0 = DEUNA. */
#  define TYPE_DELUA   (1 << 4)	/*   1 = DELUA */
#define DEUNA_STATE 0000017	/* <03:00> State: */
#  define STATE_RESET     0	/*   Reset. */
#  define STATE_READY     2	/*   Ready. */
#  define STATE_RUNNING   3	/*   Running. */

/* Status register definitions: */

#define STAT_ERRS   0100000	/* <15> Error summary. */
#define STAT_MERR   0040000	/* <14> Multiple errors. */
#define STAT_CERR   0010000	/* <12> Collision test error. */
#define STAT_TMOT   0004000	/* <11> Unibus timeout during ring access. */
#define STAT_RRNG   0001000	/* <09> Receive ring error. */
#define STAT_TRNG   0000400	/* <08> Transmit ring error. */
#define STAT_PTCH   0000200	/* <07> ROM patch. */
#define STAT_RRAM   0000100	/* <06> Running from RAM. */
#define STAT_RREV   0000077	/* <05:00> ROM version. */

/* Mode register definitions: */

#define MODE_PROM   0100000	/* <15> Promiscuous Mode */
#define MODE_ENAL   0040000	/* <14> Enable All Multicasts */
#define MODE_DRDC   0020000	/* <13> Disable Data Chaining */
#define MODE_TPAD   0010000	/* <12> Transmit Msg Pad Enable */
#define MODE_ECT    0004000	/* <11> Enable Collision Test */
#define MODE_DMNT   0001000	/* <09> Disable Maint Message */
#define MODE_DTCR   0000010	/* <03> Disable Transmit CRC */
#define MODE_LOOP   0000004	/* <02> Internal Loopback Mode */
#define MODE_HDPX   0000001	/* <00> Half-Duplex Mode */

/* Transmitter Ring definitions. */

#define TXR_OWN     0100000	/* <15> we own it (1) */
#define TXR_ERRS    0040000	/* <14> error summary */
#define TXR_MTCH    0020000	/* <13> Station Match */
#define TXR_MORE    0010000	/* <12> Mult Retries Needed */
#define TXR_ONE     0004000	/* <11> One Collision */
#define TXR_DEF     0002000	/* <10> Deferred */
#define TXR_STP     0001000	/* <09> Start Of Packet */
#define TXR_ENP     0000400	/* <08> End Of Packet */
#define TXR_BUFL    0100000	/* <15> Buffer Length Error */
#define TXR_UBTO    0040000	/* <14> UNIBUS TimeOut */
#define TXR_LCOL    0020000	/* <12> Late Collision */
#define TXR_LCAR    0010000	/* <11> Lost Carrier */
#define TXR_RTRY    0004000	/* <10> Retry Failure (16x) */
#define TXR_TDR     0003777	/* <9:0> TDR value if RTRY=1 */

/* Receiver Ring definitions. */

#define RXR_OWN     0100000	/* <15> we own it (1) */
#define RXR_ERRS    0040000	/* <14> Error Summary */
#define RXR_FRAM    0020000	/* <13> Frame Error */
#define RXR_OFLO    0010000	/* <12> Message Overflow */
#define RXR_CRC     0004000	/* <11> CRC Check Error */
#define RXR_STP     0001000	/* <09> Start Of Packet */
#define RXR_ENP     0000400	/* <08> End Of Packet */
#define RXR_BUFL    0100000	/* <15> Buffer Length error */
#define RXR_UBTO    0040000	/* <14> UNIBUS TimeOut */
#define RXR_NCHN    0020000	/* <13> No Data Chaining */
#define RXR_MLEN    0037777	/* <11:0> Message Length */

/* local variables: */

uint32 una_pcsr0 = 0;
uint32 una_pcsr1 = 0;
uint32 una_pcsr2 = 0;
uint32 una_pcsr3 = 0;

uint32 una_mode = 0;		/* Mode register. */
uint32 una_pcbb = 0;		/* Port command block base. */
uint32 una_stat = 0;		/* Extended port status. */

uint32 una_tdrb = 0;		/* Transmit desc. ring base. */
uint32 una_telen = 0;		/* Transmit desc. ring entry len. */
uint32 una_trlen = 0;		/* Transmit desc. ring length. */

uint32 una_rdrb = 0;		/* Receive desc. ring base. */
uint32 una_relen = 0;		/* Receive desc. ring entry len. */
uint32 una_rrlen = 0;		/* receive desc. ring length. */

#define UDBSIZE 32		/* Max size of udb. */

int32 pcb[4];			/* Copy of Port Command Block. */
int32 udb[UDBSIZE];		/* Copy of Unibus Data Block. */

/* state/timing/etc: */

uint32 resetflag = 0;		/* Reset in progress. */
uint32 cmdflag = 0;		/* Command in progress. */
uint32 txflag = 0;		/* Transmit in progress. */
uint32 txdone = 0;		/* Transmit done. */
uint32 rxactive = 0;		/* Receive buffer ring set. */

uint32 rxnext;			/* Index of next rx ring entry to try. */
uint32 rxea;			/* Address of rx ring entry. */
uint32 rxhdr[4];		/* Content of rx ring entry, during wait. */

uint32 txnext;			/* Index of next tx ring entry to try. */
uint32 txea;			/* Address of tx ring entry. */
uint32 txhdr[4];		/* Content of tx ring entry, during xmit. */

/* forward decls: */

t_stat deuna_rd(int32* data, int32 PA, int32 access);
t_stat deuna_wr(int32 data, int32 PA, int32 access);
t_stat deuna_svc(UNIT * uptr);
t_stat una_reset (DEVICE * dptr);
t_stat una_attach (UNIT * uptr, char * cptr);
t_stat una_detach (UNIT * uptr);

/* DEUNA data structs: */

DIB deuna_dib = { 1, IOBA_DEUNA, IOLN_DEUNA, &deuna_rd, &deuna_wr };

UNIT deuna_unit = { UDATA (&deuna_svc, UNIT_ATTABLE, 0) };

REG deuna_reg[] = {
  { ORDATA ( PCSR0, una_pcsr0, 16) },
  { ORDATA ( PCSR1, una_pcsr1, 16) },
  { ORDATA ( PCSR2, una_pcsr2, 16) },
  { ORDATA ( PCSR3, una_pcsr3, 16) },

  { ORDATA ( MODE,  una_mode,  16) },
  { ORDATA ( STAT,  una_stat,  16) },

  { ORDATA ( PCBB,  una_pcbb,  18) },

  { ORDATA ( TDRB,  una_tdrb,  18) },
  { ORDATA ( TELEN, una_telen, 16) },
  { ORDATA ( TRLEN, una_trlen, 16) },

  { ORDATA ( RDRB,  una_rdrb,  18) },
  { ORDATA ( RELEN, una_relen, 16) },
  { ORDATA ( RRLEN, una_rrlen, 16) },

  { GRDATA (DEVADDR, deuna_dib.ba, DEUNA_RDX, 32, 0), REG_HRO },
  { FLDATA (*DEVENB, deuna_dib.enb, 0), REG_HRO },
  { NULL },
};

MTAB deuna_mod[] = {
  { MTAB_XTD|MTAB_VDV, 010, "address", "ADDRESS",
    &set_addr, &show_addr, &deuna_dib },
  { MTAB_XTD|MTAB_VDV, 1, NULL, "ENABLED",
    &set_enbdis, NULL, &deuna_dib },
  { MTAB_XTD|MTAB_VDV, 0, NULL, "DISABLED",
    &set_enbdis, NULL, &deuna_dib },
  { 0 },
};

DEVICE deuna_dev = {
  "DEUNA", &deuna_unit, deuna_reg, deuna_mod,
  1, DEUNA_RDX, 13, 1, DEUNA_RDX, 8,
  NULL, NULL, &una_reset,
  NULL, &una_attach, &una_detach
};

/*
** possibly activate the device:
*/

void deuna_activate(void)
{
  if (resetflag) {
    sim_activate(&deuna_unit, 10000);
  } else if (cmdflag | txflag) {
    sim_activate(&deuna_unit, 200);
  } else if ((una_pcsr1 & DEUNA_STATE) == STATE_RUNNING) {
    sim_activate(&deuna_unit, 1000);
  }
}

/* read registers: */

t_stat deuna_rd(int32* data, int32 PA, int32 access)
{
  switch ((PA >> 1) & 03) {
  case 00:
    *data = una_pcsr0;
    break;
  case 01:
    *data = una_pcsr1;
    break;
  case 02:
    *data = una_pcsr2;
    break;
  case 03:
    *data = una_pcsr3;
    break;
  }
  return SCPE_OK;
}

/*
** initiate a reset sequence.
*/

void deuna_setreset(void)
{
  resetflag = 1;
  txflag = 0;			/* Forget transmit in progress. */
  cmdflag = 0;			/* Forget command in progress. */

  una_pcsr0 = 0;
  una_pcsr1 = STATE_RESET;
  una_pcsr2 = 0;
  una_pcsr3 = 0;

  una_mode = 0;
  una_pcbb = 0;
  una_stat = 0;

  deuna_activate();
}

/*
** callback routine from ethernet layer, transmit done, ok to do more.
*/

void deuna_od(void)
{
  txdone = 1;
}

/*
** set interrupts bits in pcsr0, and propagate to summary bit and
** also to the main interrupt system.
*/

void setint(int32 bits)
{
  if (bits & 1) {		/* Virtual bit. */
    una_pcsr1 |= DEUNA_PCTO;
  } else {
    una_pcsr1 &= ~DEUNA_PCTO;
  }
  una_pcsr0 |= (bits & 0177400); /* Turn bits on. */
  if (una_pcsr0 & 0177400) {	/* If any bits on, */
    una_pcsr0 |= DEUNA_INTR;	/*   turn master bit on. */
  } else {
    una_pcsr0 &= ~DEUNA_INTR;	/*   ... or off. */
  }
  if (una_pcsr0 & DEUNA_INTE) {
    if (una_pcsr0 & DEUNA_INTR) {
      SET_INT(DEUNA);
    } else {
      CLR_INT(DEUNA);
    }
  }
}

/*
** set bits in extended status word.
*/

void setstat(int32 bits)
{
  bits &= (STAT_TMOT + STAT_RRNG + STAT_TRNG);
  if (una_stat & STAT_ERRS) {
    una_stat |= STAT_MERR;
  }
  una_stat |= bits;
  una_stat |= STAT_ERRS;
}

/* write registers: */

t_stat deuna_wr(int32 data, int32 PA, int32 access)
{
  switch ((PA >> 1) & 03) {
  case 00:
    if (access == WRITEB) {
      data &= 0377;
      if (PA & 1) {
	una_pcsr0 &= ~((data << 8) & 0177400);
	setint(0);		/* we might have changed the interrupt sys. */
	break;
      }
    }
    una_pcsr0 &= ~(data & 0177400); /* Handle write-one-to-clear. */
    if (data & DEUNA_RSET) {
      deuna_setreset();
      return SCPE_OK;
    } 
    if ((una_pcsr0 ^ data) & DEUNA_INTE) {
      una_pcsr0 ^= DEUNA_INTE;
    } else {
      una_pcsr0 &= ~DEUNA_PCMD;
      una_pcsr0 |= (data & DEUNA_PCMD);
      if (data & 017) {
	cmdflag = 1;
	deuna_activate();
      }
    }
    setint(0);			/* we might have changed the interrupt sys. */
    break;
  case 01:
    /* this is a read-only register. */
    break;
  case 02:
    if (access == WRITEB) {
      data = (PA & 1)
	? (((data & 0377) << 8) | (una_pcsr2 & 0377))
	: ((data & 0377) | (una_pcsr2 & 0177400));
    }
    una_pcsr2 = data & 0177776;	/* Store word, but not LSB. */
    break;
  case 03:
    if (access == WRITEB) {
      data = (PA & 1)
        ? (((data & 0377) << 8) | (una_pcsr3 & 0377))
        : ((data & 0377) | (una_pcsr3 & 0177400));
    }
    una_pcsr3 = data & 0000003;	/* Store two bits. */
    break;
  }
  return SCPE_OK;
}

/*
** read into the udb.
*/

t_stat udb_rd(int count)
{
  uint32 udbb = pcb[1] + ((pcb[2] << 16) & 3);
  int i;
  t_stat r;

  for (i = 0; i < count; i += 1) {
    r = unibus_read(&udb[i], udbb + 2*i);
    if (r != SCPE_OK) return r;
  }
  return SCPE_OK;
}

t_stat udb_wr(int count)
{
  uint32 udbb = pcb[1] + ((pcb[2] << 16) & 3);
  int i;
  t_stat r;

  for (i = 0; i < count; i += 1) {
    r = unibus_write(udb[i], udbb + 2*i);
    if (r != SCPE_OK) return r;
  }
  return SCPE_OK;
}

/*
** aux. function codes (octal):
**
**  0 -- No op.
**  1 -- Start microaddress.
**  2 -- Read default physical address.
**  3 -- No op.
**  4 -- Read physical address.
**  5 -- Write physical address.
**  6 -- Read multicast address list.
**  7 -- Write multicast address list.
** 10 -- Read descriptor ring format.
** 11 -- Write descriptor ring format.
** 12 -- Read counters.
** 13 -- Read and clear counters.
** 14 -- Read mode register.
** 15 -- Write mode register.
** 16 -- Read status.
** 17 -- Read and clear status.
** 20 -- Dump internal memory.
** 21 -- Load internal memory.
** 22 -- Read system ID parameters.
** 23 -- Write system ID parameters.
** 24 -- Read load server address.
** 25 -- Write load server address.
*/

int32 auxcommand(void)
{
  unsigned char mac[6];
  int fnc;

  if ((unibus_read(&pcb[0], una_pcbb) != SCPE_OK) ||
      (unibus_read(&pcb[1], una_pcbb+2) != SCPE_OK) ||
      (unibus_read(&pcb[2], una_pcbb+4) != SCPE_OK) ||
      (unibus_read(&pcb[3], una_pcbb+6) != SCPE_OK)) {
    return DEUNA_PCEI + 1;
  }
  if (pcb[0] & 0177400) return DEUNA_PCEI;

  fnc = pcb[0] & 0377;
  switch (fnc) {
  case 0:			/* No-op. */
  case 3:
    break;
  case 1:			/* Start Microaddress. */
    return DEUNA_PCEI;
  case 2:			/* Read default physical address. */
  case 4:			/* Read current physical address. */
    if (fnc == 2) {
      (void) eth_getbia(mac);
    } else {
      (void) eth_getmac(mac);
    }
    if ((unibus_write(mac[0] + (mac[1] << 8), una_pcbb+2) != SCPE_OK) ||
	(unibus_write(mac[2] + (mac[3] << 8), una_pcbb+4) != SCPE_OK) ||
	(unibus_write(mac[4] + (mac[5] << 8), una_pcbb+6) != SCPE_OK)) {
      return DEUNA_PCEI + 1;
    }
    break;
  case 5:			/* Write current physical address. */
    if (pcb[1] & 1) return DEUNA_PCEI;
    mac[0] = pcb[1] & 0377;
    mac[1] = pcb[1] >> 8;
    mac[2] = pcb[2] & 0377;
    mac[3] = pcb[2] >> 8;
    mac[4] = pcb[3] & 0377;
    mac[5] = pcb[3] >> 8;
    if (eth_setmac(mac) != SCPE_OK) return DEUNA_PCEI;
    break;
  case 010:			/* Read ring format. */
    if ((pcb[1] & 1) || (pcb[2] & 0374)) 
      return DEUNA_PCEI;
    udb[0] = una_tdrb & 0177776;
    udb[1] = (una_telen << 8) + ((una_tdrb >> 16) & 3);
    udb[2] = una_trlen;
    udb[3] = una_rdrb & 0177776;
    udb[4] = (una_relen << 8) + ((una_rdrb >> 16) & 3);
    udb[5] = una_rrlen;
    if (udb_wr(6) != SCPE_OK) return DEUNA_PCEI+1;
    break;
  case 011:			/* Write ring format. */
    if ((pcb[1] & 1) || (pcb[2] & 0374)) 
      return DEUNA_PCEI;
    /*
    ** The hardware docs are unclear on what happens if this command
    ** is executed in running state.  However, I think that a reason-
    ** able thing to do is to prohibit it.  (The alternative is to
    ** execute it as a no-op.)
    */
    if ((una_pcsr1 & DEUNA_STATE) == STATE_RUNNING)
      return DEUNA_PCEI;
    if (udb_rd(6) != SCPE_OK) return DEUNA_PCEI+1;
    if ((udb[0] & 1) || (udb[1] & 0374) ||
	(udb[3] & 1) || (udb[4] & 0374) ||
	(udb[5] < 2)) {
      return DEUNA_PCEI;
    }
    una_tdrb = ((udb[1] & 3) << 16) + (udb[0] & 0177776);
    una_telen = (udb[1] >> 8) & 0377;
    una_trlen = udb[2];
    una_rdrb = ((udb[4] & 3) << 16) + (udb[3] & 0177776);
    una_relen = (udb[4] >> 8) & 0377;
    una_rrlen = udb[5];
    break;
  case 014:			/* Read mode register. */
    if (unibus_write(una_mode, una_pcbb+2) != SCPE_OK) {
      return DEUNA_PCEI + 1;
    }
    break;
  case 015:			/* Write mode register. */
    if (pcb[1] & 0762) return DEUNA_PCEI;
    una_mode = pcb[1];
    break;
  case 016:			/* Read extended status. */
  case 017:			/* Read and clear extended status. */
    if ((unibus_write(una_stat, una_pcbb+2) != SCPE_OK) ||
	(unibus_write(10, una_pcbb+4) != SCPE_OK) ||
	(unibus_write(32, una_pcbb+6) != SCPE_OK)) {
      return DEUNA_PCEI + 1;
    }
    if (fnc = 017) {
      una_stat &= 0377;		/* Clear high byte. */
    }
    break;
  default:			/* Unknown (unimplemented) command. */
    return DEUNA_PCEI;
    break;
  }
  return DEUNA_DNI;
}

/*
** handle polling demand:
*/

int32 deuna_poll(void)
{
  static uint8 txbuf[1514];	/* Transmit buffer. */
  static uint32 txlen;		/* Amount of data in txbuf. */

  if (!txflag) {
    txea = una_tdrb + txnext * una_telen * 2;
    if ((unibus_read(&txhdr[0], txea+0) != SCPE_OK) ||
	(unibus_read(&txhdr[1], txea+2) != SCPE_OK) ||
	(unibus_read(&txhdr[2], txea+4) != SCPE_OK) ||
	(unibus_read(&txhdr[3], txea+6) != SCPE_OK)) {
      setstat(STAT_TMOT);
      return DEUNA_SERI;
    }
    if (txhdr[2] & 0374) {
      setstat(STAT_TRNG);
      return DEUNA_SERI;
    }
    if (txhdr[2] & TXR_OWN) {
      uint32 ba;
      uint32 wd;

      txlen = txhdr[0];

      if ((txlen > 1514) || (txlen < 14)) {
	txhdr[2] |= TXR_ERRS;
	txhdr[2] &= ~(TXR_OWN | TXR_MTCH | TXR_MORE | TXR_ONE | TXR_DEF);
	txhdr[3] = TXR_BUFL;
	txnext += 1;
	if (txnext >= una_trlen)
	  txnext = 0;
	if ((unibus_write(txhdr[2], txea+4) != SCPE_OK) ||
	    (unibus_write(txhdr[3], txea+6) != SCPE_OK)) {
	  setstat(STAT_TMOT);
	  return DEUNA_SERI;
	}
	return DEUNA_DNI + DEUNA_TXI;
      }

      ba = txhdr[1] + ((txhdr[2] & 3) << 16);

      if (dma_read(ba, txbuf, txlen) != SCPE_OK) {
	setstat(STAT_TMOT);
	return DEUNA_SERI;
      }
      txhdr[3] = 0;		/* Write zero into BUFL bit. */

      /* check and set MTCH bit. */

      eth_write(txlen, txbuf);

      txflag = 1;
    }
  }

  if (!rxactive) {
    rxea = una_rdrb + rxnext * una_relen * 2;
    if ((unibus_read(&rxhdr[0], rxea+0) != SCPE_OK) ||
	(unibus_read(&rxhdr[1], rxea+2) != SCPE_OK) ||
	(unibus_read(&rxhdr[2], rxea+4) != SCPE_OK) ||
	(unibus_read(&rxhdr[3], rxea+6) != SCPE_OK)) {
      /* XXX figure out what to do. */
    }
    rxactive = 1;
  }
  return DEUNA_DNI;
}

/*
** store new packet into receive ring.
*/

void rxstore(uint8* packet, int length)
{
  /* XXX does not handle timeout etc. */

  uint32 segb;
  uint32 slen;

  slen = rxhdr[0];
  segb = rxhdr[1] + ((rxhdr[2] & 3) << 16);

  if (length > 1514) {
    length = 1514;		/* Trim length down. */
    rxhdr[2] |= RXR_OFLO;	/* Set overflow bit. */
  }

  if (length > slen) {
    length = slen;		/* XXX do data chaining. */
  }

  if (dma_write(segb, packet, length) != SCPE_OK) {
    rxhdr[2] |= RXR_ERRS;
    rxhdr[3] |= RXR_UBTO;
  }
  rxhdr[2] &= ~RXR_OWN;
  rxhdr[2] |= (RXR_STP | RXR_ENP);
  rxhdr[3] &= ~RXR_MLEN;
  if (length < 60) {
    length = 60;
  }
  rxhdr[3] |= (length + 4);	/* Include CRC bytes in length. */

  if ((unibus_write(rxhdr[2], rxea+4) != SCPE_OK) ||
      (unibus_write(rxhdr[3], rxea+6) != SCPE_OK)) {
    /* XXX figure out what to do. */
  }

  /* Get next receive entry from ring. */

  rxnext += 1;
  if (rxnext >= una_rrlen) {
    rxnext = 0;
  }
  rxea = una_rdrb + rxnext * una_relen * 2;
  if ((unibus_read(&rxhdr[0], rxea+0) != SCPE_OK) ||
      (unibus_read(&rxhdr[1], rxea+2) != SCPE_OK) ||
      (unibus_read(&rxhdr[2], rxea+4) != SCPE_OK) ||
      (unibus_read(&rxhdr[3], rxea+6) != SCPE_OK)) {
    /* XXX figure out what to do. */
  }
}

/*
** try to receive frames.
*/

void doreceive(void)
{
  uint32 length;
  uint8* packet;

  length = eth_read(&packet);
  if (length > 0) {
    if (!rxactive) {
      while (length > 0) {
	length = eth_read(&packet);
      }
    } else if (!(rxhdr[2] & 0100000)) {
      setint(DEUNA_RCBI);
      rxactive = 0;
    } else {
      rxstore(packet, length);
      while ((rxhdr[2] & 0100000) &&
	     ((length = eth_read(&packet)) > 0)) {
	rxstore(packet, length);
      }
      setint(DEUNA_RXI);
    }
  }
}

/*
** step the tx ring after updating this entry.
*/

void steptx(void)
{
  txhdr[2] &= ~0156000;	/* OWN, ERRS, MORE, ONE, DEF */
  txhdr[3] = 0;

  if ((unibus_write(txhdr[2], txea+4) != SCPE_OK) ||
      (unibus_write(txhdr[3], txea+6) != SCPE_OK)) {
    setstat(STAT_TMOT);
  }

  txnext += 1;
  if (txnext >= una_trlen) {
    txnext = 0;
  }
}

/*
** service routine:
*/

t_stat deuna_svc(UNIT* uptr)
{
  if (resetflag) {
    resetflag = 0;
    una_pcsr1 = STATE_READY;
    setint(DEUNA_DNI);
  } else if (cmdflag) {
    int32 bits = DEUNA_DNI;	/* Default is to set DONE. */
    cmdflag = 0;
    switch (una_pcsr0 & 017) {
    case CMD_GETPCB:		/* GET PCB-BASE */
      una_pcbb = (una_pcsr3 << 16) | una_pcsr2;
      break;
    case CMD_GETCMD:		/* GET COMMAND */
      bits = auxcommand();
      break;
    case CMD_SELFTEST:		/* SELFTEST */
      deuna_setreset();
      bits = 0;
      break;
    case CMD_START:		/* START */
      if ((una_pcsr1 & DEUNA_STATE) != STATE_RUNNING) {
	eth_start();
	una_pcsr1 &= ~DEUNA_STATE;
	una_pcsr1 |= STATE_RUNNING;
	/* should reset the ring pointers, according to documentation. */
	rxnext = 0;
	txnext = 0;
      }
      break;
    case CMD_PDMD:		/* POLLING DEMAND */
      bits = deuna_poll();
      break;
    case CMD_STOP:		/* STOP */
      if ((una_pcsr1 & DEUNA_STATE) != STATE_READY) {
	eth_stop();
	una_pcsr1 &= ~DEUNA_STATE;
	una_pcsr1 |= STATE_READY;
      }
      break;
    default:			/* NO-OP */
      /* well? */
      break;
    }
    setint(bits);
  } else if (txflag) {
    if (txdone) {
      txflag = txdone = 0;
      steptx();
      setint(DEUNA_TXI);
    }
  } else {
    doreceive();
  }
  deuna_activate();
  return SCPE_OK;
}

/* reset device: */

t_stat una_reset(DEVICE* dptr)
{
  (void) eth_reset();
  deuna_setreset();
  return SCPE_OK;
}

/* attach device: */

t_stat una_attach(UNIT* uptr, char* cptr)
{
  t_stat r;
  char* tptr;

  tptr = malloc(strlen(cptr) + 1);
  if (tptr == NULL) return SCPE_MEM;
  strcpy(tptr, cptr);

  r = eth_open(cptr, deuna_od);
  if (r != SCPE_OK) {
    free(tptr);
    return r;
  }
  uptr->filename = tptr;
  uptr->flags |= UNIT_ATT;

  return SCPE_OK;
}

/* detach device: */

t_stat una_detach(UNIT* uptr)
{
  /* detach from bpf filter. */

  if (uptr->flags & UNIT_ATT) {
    (void) eth_stop();
    free(uptr->filename);
    uptr->filename = NULL;
    uptr->flags &= ~UNIT_ATT;
  }

  return SCPE_OK;
}
