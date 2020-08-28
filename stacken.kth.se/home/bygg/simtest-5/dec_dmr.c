/*
** Simulate a Digital DMR11 interface on a Unibus.
*/

/*
** Written 2002 by Johnny Eriksson <bygg@stacken.kth.se>
*/

/*
** Revision history:
**
** 2002-01-23    Modify for version 2.9.
** 2002-01-17    First attempt.
*/

/* bits, sel0: */

#define DMR_CMD_IN 0000017	/* Command in field. */
#  define CMI_TBI        0	/*   Transmit buffer in. */
#  define CMI_CTI        1	/*   Control in. */
#  define CMI_HLT        2	/*   Halt. */
#  define CMI_BSI        3	/*   Base in. */
#  define CMI_RBI        4	/*   Receive buffer in. */
#define DMR_RQI    0000040	/* Request in. */
#define DMR_IEI    0000100	/* Interrupt enable input (A). */
#define DMR_RDI    0000200	/* Ready for input command. */

#define DMR_IND    0020000	/* Inhibit micro-diags. */
#define DMR_MRC    0040000	/* Master clear. */
#define DMR_RUN    0100000	/* Run bit. */

/* bits, sel2: */

#define DMR_CMD_OUT 000007	/* Command out field. */
#  define CMO_TBO        0	/*   Transmit buffer out. */
#  define CMO_CTO        1	/*   Control out. */
#  define CMO_RBO	 4	/*   Receive buffer out. */
#define DMR_IEO	   0000100	/* Interrupt enable output (B). */
#define DMR_RDO    0000200	/* Output command done. */

#ifndef MAXDMR
#  define MAXDMR 1
#endif

/* local variables: */

uint32 dmr_sel0;
uint32 dmr_sel2;
uint32 dmr_sel4;
uint32 dmr_sel6;

uint32 dmr_rxi = 0;		/* rcv interrupts */
uint32 dmr_txi = 0;		/* xmt interrupts */

/* state/timing/etc: */

enum { dst_limbo,		/* Unknown state, after init. */
       dst_clearing,		/* Doing master clear. */
       dst_run_bi,		/* Run, waiting for base in. */
       dst_run_ci,		/* Run, waiting for control in. */
       dst_running,		/* Running. */
} dmr_state;

/* forward decls: */

t_stat dmr_rd(int32* data, int32 PA, int32 access);
t_stat dmr_wr(int32 data, int32 PA, int32 access);
t_stat dmr_svc (UNIT * uptr);
t_stat dmr_reset (DEVICE * dptr);
t_stat dmr_attach (UNIT * uptr, char * cptr);
t_stat dmr_detach (UNIT * uptr);

/* DMR11 data structs: */

DIB dmr_dib = { 0, IOBA_DMR, (IOLN_DMR * MAXDMR), &dmr_rd, &dmr_wr };

UNIT dmr_unit[MAXDMR] = {
  { UDATA (&dmr_svc, UNIT_ATTABLE, 0) },
};

REG dmr_reg[] = {
  { ORDATA ( SEL0, dmr_sel0, 16) },
  { ORDATA ( SEL2, dmr_sel2, 16) },
  { ORDATA ( SEL4, dmr_sel4, 16) },
  { ORDATA ( SEL6, dmr_sel6, 16) },

  { GRDATA (DEVADDR, dmr_dib.ba, DMR_RDX, 32, 0), REG_HRO },
  { FLDATA (*DEVENB, dmr_dib.enb, 0), REG_HRO },
  { NULL },
};

MTAB dmr_mod[] = {
  { MTAB_XTD|MTAB_VDV, 010, "address", "ADDRESS",
    &set_addr, &show_addr, &dmr_dib },
  { MTAB_XTD|MTAB_VDV, 1, NULL, "ENABLED",
    &set_enbdis, NULL, &dmr_dib },
  { MTAB_XTD|MTAB_VDV, 0, NULL, "DISABLED",
    &set_enbdis, NULL, &dmr_dib },
  { 0 },
};

DEVICE dmr_dev = {
  "DMR", dmr_unit, dmr_reg, dmr_mod,
  1, DMR_RDX, 13, 1, DMR_RDX, 8,
  NULL, NULL, &dmr_reset,
  NULL, &dmr_attach, &dmr_detach
};

/* Interrupt routines */

void dmr_clr_rxint (int32 dmr)
{
  dmr_rxi = dmr_rxi & ~(1 << dmr);			/* clr intf rcv int */
  if (dmr_rxi == 0) CLR_INT (DMRA);			/* all clr? */
  return;
}

void dmr_set_rxint (int32 dmr)
{
  dmr_rxi = dmr_rxi | (1 << dmr);			/* set intf rcv int */
  SET_INT (DMRA);					/* set master int */
  return;
}

int32 dmr_rxinta (void)
{
  int32 dmr;

  for (dmr = 0; dmr < MAXDMR; dmr++) {			/* find 1st intf */
    if (dmr_rxi & (1 << dmr)) return (VEC_DMRA + (dmr * 010)); 
  }
  return 0;
}

void dmr_clr_txint (int32 dmr)
{
  dmr_txi = dmr_txi & ~(1 << dmr);			/* clr intf xmt int */
  if (dmr_txi == 0) CLR_INT (DMRB);			/* all clr? */
  return;
}

void dmr_set_txint (int32 dmr)
{
  dmr_txi = dmr_txi | (1 << dmr);			/* set intf xmt int */
  SET_INT (DMRB);					/* set master int */
  return;
}

int32 dmr_txinta (void)
{
  int32 dmr;

  for (dmr = 0; dmr < MAXDMR; dmr++) {			/* find 1st intf */
    if (dmr_txi & (1 << dmr)) return (VEC_DMRB + (dmr * 010));  }
  return 0;
}

/* read registers: */

t_stat dmr_rd(int32* data, int32 PA, int32 access)
{
  switch ((PA >> 1) & 03) {
  case 00:
    *data = dmr_sel0;
    break;
  case 01:
    *data = dmr_sel2;
    break;
  case 02:
    *data = dmr_sel4;
    break;
  case 03:
    *data = dmr_sel6;
    break;
  }
  return SCPE_OK;
}

/* write registers: */

t_stat dmr_wr(int32 data, int32 PA, int32 access)
{
  switch ((PA >> 1) & 03) {
  case 00:
    if (access == WRITEB) {	/* byte? merge */
      data = (PA & 1)
	? (dmr_sel0 & 0377) | (data << 8)
	: (dmr_sel0 & ~0377) | data;
    }
    dmr_sel0 = data;
    break;
  case 01:
    if (access == WRITEB) {	/* byte? merge */
      data = (PA & 1)
	? (dmr_sel2 & 0377) | (data << 8)
	: (dmr_sel2 & ~0377) | data;
    }
    dmr_sel2 = data;
    break;
  case 02:
    if (access == WRITEB) {	/* byte? merge */
      data = (PA & 1)
	? (dmr_sel4 & 0377) | (data << 8)
	: (dmr_sel4 & ~0377) | data;
    }
    dmr_sel4 = data;
    break;
  case 03:
    if (access == WRITEB) {	/* byte? merge */
      data = (PA & 1)
	? (dmr_sel6 & 0377) | (data << 8)
	: (dmr_sel6 & ~0377) | data;
    }
    dmr_sel6 = data;
    break;
  }
  return SCPE_OK;
}

/*
** service routine:
*/

t_stat dmr_svc (UNIT* uptr)
{
  return SCPE_OK;
}

/* reset device: */

t_stat dmr_reset(DEVICE* dptr)
{
  static t_bool done = FALSE;
  int i;

  if (!done) {
    for (i = 1; i <MAXDMR; i += 1) {
      dmr_unit[i] = dmr_unit[0];
    }
    done = TRUE;
  }
  return SCPE_OK;
}

/* attach device: */

t_stat dmr_attach(UNIT* uptr, char* cptr)
{
  char* tptr;

  tptr = malloc(strlen(cptr) + 1);
  if (tptr == NULL) return SCPE_MEM;
  strcpy(tptr, cptr);

  /*
  if ((r = sync_open(&vline, cptr) != SCPE_OK) {
    free(tptr);
    return SCPE_OPENERR;
  }
  */

  uptr->filename = tptr;
  uptr->flags |= UNIT_ATT;

  dmr_state = dst_limbo;

  dmr_sel0 = 0;
  dmr_sel2 = 0;
  dmr_sel4 = 0;
  dmr_sel6 = 0;

  return SCPE_OK;
}

/* detach device: */

t_stat dmr_detach(UNIT* uptr)
{
  /*
  sync_close(...);
  */

  if (uptr->flags & UNIT_ATT) {
    free(uptr->filename);
    uptr->filename = NULL;
    uptr->flags &= ~UNIT_ATT;
  }

  return SCPE_OK;
}
