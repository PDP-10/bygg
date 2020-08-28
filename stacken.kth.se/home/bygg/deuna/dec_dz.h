/* dec_dz.c: DZ11 terminal multiplexor simulator

   Copyright (c) 2001, Robert M Supnik

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not
   be used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   Based on the original DZ11 simulator by Thord Nilson, as updated by
   Arthur Krewat.

   dz		DZ11 terminal multiplexor

   This file is intended to be included in a shell routine that invokes
   a simulator definition file:

   pdp11_dz.c	= pdp11_defs.h + dec_dz.h
   pdp10_dz.c	= pdp10_defs.h + dec_dz.h
   vax_dz.c	= vax_defs.h + dec_dz.h
*/

#include "sim_sock.h"
#include "sim_tmxr.h"

#define DZ_LINES	8				/* lines per DZ11 */
#define DZ_LNOMASK	(DZ_LINES - 1)			/* mask for lineno */
#define DZ_LMASK	((1 << DZ_LINES) - 1)		/* mask of lines */
#define DZ_SILO_ALM	16				/* silo alarm level */

/* DZCSR - 160100 - control/status register */

#define CSR_MAINT	0000010				/* maint - NI */
#define CSR_CLR		0000020				/* clear */
#define CSR_MSE		0000040				/* master scan enb */
#define CSR_RIE		0000100				/* rcv int enb */
#define CSR_RDONE	0000200				/* rcv done - RO */
#define CSR_V_TLINE	8				/* xmit line - RO */
#define CSR_TLINE	(DZ_LNOMASK << CSR_V_TLINE)
#define CSR_SAE		0010000				/* silo alm enb */
#define CSR_SA		0020000				/* silo alm - RO */
#define CSR_TIE		0040000				/* xmit int enb */
#define CSR_TRDY	0100000				/* xmit rdy - RO */
#define CSR_RW		(CSR_MSE | CSR_RIE | CSR_SAE | CSR_TIE)
#define CSR_MBZ		(0004003 | CSR_CLR | CSR_MAINT)

#define CSR_GETTL(x)	(((x) >> CSR_V_TLINE) & DZ_LNOMASK)
#define CSR_PUTTL(x,y)	x = ((x) & ~CSR_TLINE) | (((y) & DZ_LNOMASK) << CSR_V_TLINE)

/* DZRBUF - 160102 - receive buffer, read only */

#define RBUF_CHAR	0000377				/* rcv char */
#define RBUF_V_RLINE	8				/* rcv line */
#define RBUF_PARE	0010000				/* parity err - NI */
#define RBUF_FRME	0020000				/* frame err - NI */
#define RBUF_OVRE	0040000				/* overrun err - NI */
#define RBUF_VALID	0100000				/* rcv valid */
#define RBUF_MBZ	0004000

/* DZLPR - 160102 - line parameter register, write only, word access only */

#define LPR_V_LINE	0				/* line */
#define LPR_LPAR	0007770				/* line pars - NI */
#define LPR_RCVE	0010000				/* receive enb */
#define LPR_GETLN(x)	(((x) >> LPR_V_LINE) & DZ_LNOMASK)

/* DZTCR - 160104 - transmission control register */

#define TCR_V_XMTE	0				/* xmit enables */
#define TCR_V_DTR	7				/* DTRs */

/* DZMSR - 160106 - modem status register, read only */

#define MSR_V_RI	0				/* ring indicators */
#define MSR_V_CD	7				/* carrier detect */

/* DZTDR - 160106 - transmit data, write only */

#define TDR_CHAR	0000377				/* xmit char */
#define TDR_V_TBR	7				/* xmit break - NI */

extern int32 int_req, dev_enb;
extern int32 sim_switches;
extern FILE *sim_log;
extern int32 dz_poll;					/* calibrated delay */
int32 dz_csr = 0;					/* csr */
int32 dz_rbuf = 0;					/* rcv buffer */
int32 dz_lpr = 0;					/* line param */
int32 dz_tcr = 0;					/* xmit control */
int32 dz_msr = 0;					/* modem status */
int32 dz_tdr = 0;					/* xmit data */
int32 dz_mctl = 0;					/* modem ctrl enab */
int32 dz_sa_enb = 1;					/* silo alarm enabled */
TMLN dz_ldsc0 = { 0 };					/* line descriptors */
TMLN dz_ldsc1 = { 0 };
TMLN dz_ldsc2 = { 0 };
TMLN dz_ldsc3 = { 0 };
TMLN dz_ldsc4 = { 0 };
TMLN dz_ldsc5 = { 0 };
TMLN dz_ldsc6 = { 0 };
TMLN dz_ldsc7 = { 0 };
TMXR dz_desc = {					/* mux descriptor */
	DZ_LINES, 0,
	&dz_ldsc0, &dz_ldsc1, &dz_ldsc2, &dz_ldsc3,
	&dz_ldsc4, &dz_ldsc5, &dz_ldsc6, &dz_ldsc7 };

t_stat dz_svc (UNIT *uptr);
t_stat dz_reset (DEVICE *dptr);
t_stat dz_attach (UNIT *uptr, char *cptr);
t_stat dz_detach (UNIT *uptr);
t_stat dz_clear (t_bool flag);
void dz_update_rcvi (int32 scnt);
void dz_update_xmti (void);

/* DZ data structures

   dz_dev	DZ device descriptor
   dz_unit	DZ unit list
   dz_reg	DZ register list
*/

UNIT dz_unit = { UDATA (&dz_svc, UNIT_ATTABLE, 0) };

REG dz_reg[] = {
	{ ORDATA (CSR, dz_csr, 16) },
	{ ORDATA (RBUF, dz_rbuf, 16) },
	{ ORDATA (LPR, dz_lpr, 16) },
	{ ORDATA (TCR, dz_tcr, 16) },
	{ ORDATA (MSR, dz_msr, 16) },
	{ ORDATA (TDR, dz_tdr, 16) },
	{ FLDATA (SAENB, dz_sa_enb, 0) },
	{ FLDATA (MDMCTL, dz_mctl, 0) },
	{ DRDATA (RPOS0, dz_ldsc0.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS0, dz_ldsc0.txcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (RPOS1, dz_ldsc1.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS1, dz_ldsc1.txcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (RPOS2, dz_ldsc2.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS2, dz_ldsc2.txcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (RPOS3, dz_ldsc3.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS3, dz_ldsc3.txcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (RPOS4, dz_ldsc4.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS4, dz_ldsc4.txcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (RPOS5, dz_ldsc5.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS5, dz_ldsc5.txcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (RPOS6, dz_ldsc6.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS6, dz_ldsc6.txcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (RPOS7, dz_ldsc7.rxcnt, 32), PV_LEFT+REG_RO },
	{ DRDATA (TPOS7, dz_ldsc7.txcnt, 32), PV_LEFT+REG_RO },
	{ FLDATA (*DEVENB, dev_enb, INT_V_DZ0RX), REG_HRO },
	{ NULL }  };

DEVICE dz_dev = {
	"DZ", &dz_unit, dz_reg, NULL,
	1, 8, 13, 1, 8, 8,
	&tmxr_ex, &tmxr_dep, &dz_reset,
	NULL, &dz_attach, &dz_detach };

/* IO dispatch routines, I/O addresses 17760100 - 17760107 */

t_stat dz0_rd (int32 *data, int32 PA, int32 access)
{
int32 cnt;

switch ((PA >> 1) & 03) {				/* case on PA<2:1> */
case 00:						/* CSR */
	*data = dz_csr = dz_csr & ~CSR_MBZ;
	break;
case 01:						/* RBUF */
	dz_csr = dz_csr & ~CSR_SA;			/* clr silo alarm */
	if (dz_csr & CSR_MSE) {				/* scanner on? */
		dz_rbuf = tmxr_getchar (&dz_desc);	/* get top of silo */
		if (!dz_rbuf) dz_sa_enb = 1;		/* empty? re-enable */
		cnt = tmxr_poll_rx (&dz_desc);		/* poll input */
		dz_update_rcvi (cnt);  }		/* update rx intr */
	else {	dz_rbuf = 0;				/* no data */
		dz_update_rcvi (0);  }			/* no rx intr */
	*data = dz_rbuf;
	break;
case 02:						/* TCR */
	*data = dz_tcr;
	break;
case 03:						/* MSR */
	*data = dz_msr;
	break;  }
return SCPE_OK;
}

t_stat dz0_wr (int32 data, int32 PA, int32 access)
{
int32 i, cnt, line;
TMLN *lp;

switch ((PA >> 1) & 03) {				/* case on PA<2:1> */
case 00:						/* CSR */
	if (access == WRITEB) data = (PA & 1)?
		(dz_csr & 0377) | (data << 8): (dz_csr & ~0377) | data;
	if (data & CSR_CLR) dz_clear (FALSE);
	if (data & CSR_MSE) sim_activate (&dz_unit, dz_poll);
	else {	sim_cancel (&dz_unit);
		dz_csr = dz_csr & ~(CSR_SA | CSR_RDONE | CSR_TRDY);  }
	if ((data & CSR_RIE) == 0) int_req = int_req & ~INT_DZ0RX;
	else if (((dz_csr & CSR_IE) == 0) &&		/* RIE 0->1? */
	         ((dz_csr & CSR_SAE)? (dz_csr & CSR_SA): (dz_csr & CSR_RDONE)))
		int_req = int_req | INT_DZ0RX;
	if ((data & CSR_TIE) == 0) int_req = int_req & ~INT_DZ0TX;
	else if (((dz_csr & CSR_TIE) == 0) && (dz_csr & CSR_TRDY))
		int_req = int_req | INT_DZ0TX;
	dz_csr = (dz_csr & ~CSR_RW) | (data & CSR_RW);
	break;
case 01:						/* LPR */
	dz_lpr = data;
	line = LPR_GETLN (dz_lpr);			/* get line */
	lp = dz_desc.ldsc[line];			/* get line desc */
	if (dz_lpr & LPR_RCVE) lp -> rcve = 1;		/* rcv enb? on*/
	else lp -> rcve = 0;				/* else line off */
	cnt = tmxr_poll_rx (&dz_desc);			/* poll input */
	dz_update_rcvi (cnt);				/* update rx intr */
	break;
case 02:						/* TCR */
	if (access == WRITEB) data = (PA & 1)?
		(dz_tcr & 0377) | (data << 8): (dz_tcr & ~0377) | data;
	if (dz_mctl) {					/* modem ctl? */
		int32 cdet, ring, drop;
		cdet = (data & 0177400) &		/* car det = dtr & ring */
			((dz_msr & DZ_LMASK) << MSR_V_CD); /* ring = ring & ~dtr */
		ring = (dz_msr & DZ_LMASK) & ~(data >> TCR_V_DTR);
		drop = (dz_tcr & ~data) >> TCR_V_DTR;	/* drop = dtr & ~data */
		dz_msr = cdet | ring;			/* update msr */
		for (i = 0; i < DZ_LINES; i++) {	/* drop hangups */
		    lp = dz_desc.ldsc[i];		/* get line desc */
		    if (lp -> conn && (drop & (1 << i))) {
			tmxr_msg (lp -> conn, "\r\nLine hangup\r\n");
			tmxr_reset_ln (lp);		/* reset line */
			dz_msr = dz_msr & ~(1 << (i + MSR_V_CD)); /* reset car det */
			}				/* end if */
		    }					/* end for */
		}					/* end if modem */
	dz_tcr = data;
	tmxr_poll_tx (&dz_desc);			/* poll output */
	dz_update_xmti ();				/* update int */
	break;
case 03:						/* TDR */
	if (PA & 1) {					/* odd byte? */
		dz_tdr = (dz_tdr & 0377) | (data << 8);	/* just save */
		break;  }
	dz_tdr = data;
	line = CSR_GETTL (dz_csr);			/* get xmit line */
	if (dz_csr & CSR_MSE) {				/* enabled? */
		tmxr_putchar (&dz_desc, line, dz_tdr & 0177);	/* store char */
		tmxr_poll_tx (&dz_desc);		/* poll output */
		dz_update_xmti ();  }			/* update int */
	break;  }
return SCPE_OK;
}

/* Unit service routine

   The DZ11 polls to see if asynchronous activity has occurred and now
   needs to be processed.  The polling interval is controlled by the clock
   simulator, so for most environments, it is calibrated to real time.
   Typical polling intervals are 50-60 times per second.
*/

t_stat dz_svc (UNIT *uptr)
{
int32 cnt, newln;

if (dz_csr & CSR_MSE) {					/* enabled? */
	newln = tmxr_poll_conn (&dz_desc, uptr);	/* poll connect */
	if ((newln >= 0) && dz_mctl)			/* got a live one? */
		dz_msr = dz_msr | (1 << newln);		/* set ring */
	cnt = tmxr_poll_rx (&dz_desc);			/* poll input */
	dz_update_rcvi (cnt);				/* upd rcv intr */
	tmxr_poll_tx (&dz_desc);			/* poll output */
	dz_update_xmti ();				/* upd xmt intr */
	sim_activate (uptr, dz_poll);  }		/* reactivate */
return SCPE_OK;
}

/* Update receive interrupts */

void dz_update_rcvi (int32 scnt)
{
int32 i;

for (i = 0; dz_mctl && (i < DZ_LINES); i++) {		/* poll for drops */
	if (!dz_desc.ldsc[i] -> conn)			/* if disconn */
		dz_msr = dz_msr & ~(1 << (i + MSR_V_CD)); /* reset car det */
	}
if (scnt) {						/* any chars? */
	dz_csr = dz_csr | CSR_RDONE;			/* set done */
	if (dz_sa_enb && (scnt >= DZ_SILO_ALM)) {	/* alm enb & cnt hi? */
		dz_csr = dz_csr | CSR_SA;		/* set status */
		dz_sa_enb = 0;  }  }			/* disable alarm */
else dz_csr = dz_csr & ~CSR_RDONE;			/* no, clear done */
if ((dz_csr & CSR_RIE) &&				/* int enable */
    ((dz_csr & CSR_SAE)? (dz_csr & CSR_SA): (dz_csr & CSR_RDONE)))
	int_req = int_req | INT_DZ0RX;			/* and alm/done? */
else int_req = int_req & ~INT_DZ0RX;			/* no, clear int */
return;
}

void dz_update_xmti (void)
{
int32 linemask, i, j;

linemask = dz_tcr & DZ_LMASK;				/* enabled lines */
dz_csr = dz_csr & ~CSR_TRDY;				/* assume not rdy */
for (i = 0, j = CSR_GETTL (dz_csr); i < DZ_LINES; i++) {
	j = (j + 1) & DZ_LNOMASK;
	if ((linemask & (1 << j)) && dz_desc.ldsc[j] -> xmte) {
		CSR_PUTTL (dz_csr, j);			/* update CSR */
		dz_csr = dz_csr | CSR_TRDY;		/* set xmt rdy */
		break;  }  }
if ((dz_csr & CSR_TIE) && (dz_csr & CSR_TRDY))		/* ready plus int? */
	 int_req = int_req | INT_DZ0TX;
else int_req = int_req & ~INT_DZ0TX;			/* no int req */
return;
}

/* Device reset */

t_stat dz_clear (t_bool flag)
{
int32 i;

dz_csr = 0;						/* clear CSR */
dz_rbuf = 0;						/* silo empty */
dz_lpr = 0;						/* no params */
if (flag) dz_tcr = 0;					/* INIT? clr all */
else dz_tcr = dz_tcr & ~0377;				/* else save dtr */
dz_tdr = 0;
dz_sa_enb = 1;
int_req = int_req & ~(INT_DZ0RX | INT_DZ0TX);		/* clear int */
sim_cancel (&dz_unit);					/* no polling */
for (i = 0; i < DZ_LINES; i++)				/* clr rcv enb */
	dz_desc.ldsc[i] -> rcve = 0;
return SCPE_OK;
}

t_stat dz_reset (DEVICE *dptr)
{
return dz_clear (TRUE);
}

/* Attach */

t_stat dz_attach (UNIT *uptr, char *cptr)
{
t_stat r;
extern int32 sim_switches;

r = tmxr_attach (&dz_desc, uptr, cptr);			/* attach mux */
if (r != SCPE_OK) return r;				/* error? */
dz_mctl = (sim_switches & SWMASK ('M'))? 1: 0;		/* modem control? */
if (dz_mctl) {
	printf ("Modem control activated\n");
	if (sim_log) fprintf (sim_log, "Modem control activated\n");  }
return SCPE_OK;
}

/* Detach */

t_stat dz_detach (UNIT *uptr)
{
return tmxr_detach (&dz_desc, uptr);
}
