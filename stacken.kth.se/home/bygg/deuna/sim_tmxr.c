/* sim_tmxr.c: Telnet terminal multiplexor library

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
*/

#include "sim_defs.h"
#include "sim_sock.h"
#include "sim_tmxr.h"

/* Telnet protocol constants */

#define TN_IAC		0xFF				/* protocol delim */
#define TN_WILL		0xFB				/* will */
#define TN_WONT		0xFC				/* wont */
#define TN_DO		0xFD				/* do */
#define TN_DONT		0xFE				/* dont */
#define TN_BIN		0				/* bin */
#define TN_ECHO		1				/* echo */
#define TN_SGA		3				/* sga */
#define TN_LINE		34				/* line mode */
#define TN_CR		015				/* carriage return */

/* Telnet line states */

#define TNS_NORM	000				/* normal */
#define TNS_IAC		001				/* IAC seen */
#define TNS_WILL	002				/* WILL seen */
#define TNS_WONT	003				/* WONT seen */
#define TNS_SKIP	004				/* skip next */

void tmxr_rmvrc (TMLN *lp, int32 p);
extern int32 sim_switches;
extern char sim_name[];
extern FILE *sim_log;

/* Poll for new connection

   Called from unit service routine to test for new connection

   Inputs:
	*mp	=	pointer to terminal multiplexor descriptor
   Outputs:
	line number activated, -1 if none
*/

int32 tmxr_poll_conn (TMXR *mp, UNIT *uptr)
{
SOCKET newsock;
TMLN *lp;
int32 i;
static uint8 mantra[] = {
  TN_IAC, TN_WILL, TN_LINE,
  TN_IAC, TN_WILL, TN_SGA,
  TN_IAC, TN_WILL, TN_ECHO,
  TN_IAC, TN_WILL, TN_BIN,
  TN_IAC, TN_DO, TN_BIN };

newsock = sim_accept_conn (mp -> master, uptr);		/* poll connect */
if (newsock != INVALID_SOCKET) {			/* got a live one? */
	for (i = 0; i < mp -> lines; i++) {		/* find avail line */
		lp = mp -> ldsc[i];			/* ptr to ln desc */
		if (lp -> conn == 0) break;  }		/* available? */
	if (i >= mp -> lines) {				/* all busy? */
		tmxr_msg (newsock, "All connections busy... please try later\r\n");
		sim_close_sock (newsock, 0);  }
	else {	lp = mp -> ldsc[i];
		lp -> conn = newsock;			/* record connection */
		lp -> rxbpr = lp -> rxbpi = 0;		/* init buf pointers */
		lp -> txbpr = lp -> txbpi = 0;
		lp -> rxcnt = lp -> txcnt = 0;		/* init counters */
		lp -> tsta = 0;				/* init telnet state */
		lp -> xmte = 1; 			/* enable transmit */
		lp -> dstb = 0;				/* default bin mode */
		sim_write_sock (newsock, mantra, 15);
		tmxr_msg (newsock, "\n\r\nWelcome to the ");
		tmxr_msg (newsock, sim_name);
		tmxr_msg (newsock, " simulator\r\n\n");
		return i;  }
	}						/* end if newsock */
return -1;
}

/* Reset line */

void tmxr_reset_ln (TMLN *lp)
{
sim_close_sock (lp -> conn, 0);				/* reset conn */
lp -> conn = lp -> tsta = 0;				/* reset state */
lp -> rxbpr = lp -> rxbpi = 0;
lp -> txbpr = lp -> txbpi = 0;
lp -> xmte = 1;
lp -> dstb = 0;
return;
}


/* Get character, if available

   Inputs:
	*mp	=	pointer to terminal multiplexor descriptor
   Output:
	valid+line#+char, 0 if none
*/

int32 tmxr_getchar (TMXR *mp)
{
int32 i, j, val;
uint32 tmp;
TMLN *lp;

for (i = val = 0; (i < mp -> lines) && (val == 0); i++) { /* loop thru lines */
	lp = mp -> ldsc[i];				/* get line desc */
	if (!lp -> conn || !lp -> rcve) continue;	/* skip if !conn */
	j = lp -> rxbpi - lp -> rxbpr;			/* any input? */
	if (j == 0) continue;				/* enough input */
	tmp = lp -> rxb[lp -> rxbpr];			/* get char */
	lp -> rxbpr = lp -> rxbpr + 1;			/* adv pointer */
	val = TMXR_VALID | (i << TMXR_V_RLINE) | (tmp & 0377);
	}						/* end for */
for (i = 0; i < mp -> lines; i++) {			/* clean up pointers */
	lp = mp -> ldsc[i];				/* get line desc */
	if (lp -> rxbpi == lp -> rxbpr)			/* empty? zero ptrs */
		lp -> rxbpi = lp -> rxbpr = 0;
	}
return val;
}

/* Poll for input

   Inputs:
	*mp	=	pointer to terminal multiplexor descriptor
   Outputs:
	number of characters in all input buffers
*/

int32 tmxr_poll_rx (TMXR *mp)
{
int32 i, nbytes, scnt, j, lcnt;
TMLN *lp;

for (i = scnt = 0; i < mp -> lines; i++) {
	lp = mp -> ldsc[i];				/* get line desc */
	if (!lp -> conn || !lp -> rcve) continue;	/* skip if !conn */
	if (lp -> rxbpi >= TMXR_MAXBUF)			/* full? */
		lp -> rxbpi = lp -> rxbpr = 0;		/* reset ptrs */
	if ((lp -> rxbpi == 0) || lp -> tsta) {		/* need input? */
	 	nbytes = sim_read_sock (lp -> conn,	/* yes, read */
			&(lp -> rxb[lp -> rxbpi]),
			TMXR_MAXBUF - lp -> rxbpi);
	 	if (nbytes < 0) tmxr_reset_ln (lp);	/* closed? reset ln */
		else if (nbytes > 0) {			/* if data rcvd */
			lp -> rxbpi = lp -> rxbpi + nbytes;
			lp -> rxcnt = lp -> rxcnt + nbytes;  }
		}					/* end if rxbpi */

/* Examine input, remove TELNET cruft before making input available */

	for (lcnt = 0, j = lp -> rxbpr; j < lp -> rxbpi; ) {
		uint8 tmp = lp -> rxb[j];		/* get char */
		switch (lp -> tsta) {			/* case tlnt state */
		case TNS_NORM:				/* normal */
			if (tmp == TN_IAC) {		/* IAC? */
				lp -> tsta = TNS_IAC;	/* change state */
				tmxr_rmvrc (lp, j);	/* remove char */
				break;  }
			if ((tmp == TN_CR) && lp -> dstb) /* CR, no bin */
				lp -> tsta = TNS_SKIP; /* skip next */
			lcnt = lcnt + 1;		/* count char */
			j = j + 1;			/* advance j */
			break;
		case TNS_IAC:				/* IAC prev */
			if (tmp == TN_WILL)		/* IAC + WILL? */
				lp -> tsta = TNS_WILL;
			else if (tmp == TN_WONT)	/* IAC + WONT? */
				lp -> tsta = TNS_WONT;
			else lp -> tsta = TNS_SKIP;	/* IAC + other */
			tmxr_rmvrc (lp, j);		/* remove char */
			break;
		case TNS_WILL: case TNS_WONT:		/* IAC+WILL/WONT prev */
			if (tmp == TN_BIN) {		/* BIN? */
				if (lp -> tsta == TNS_WILL)
					lp -> dstb = 0;
				else lp -> dstb = 1;  }
		case TNS_SKIP: default:			/* skip char */
			lp -> tsta = TNS_NORM;	/* next normal */
			tmxr_rmvrc (lp, j);		/* remove char */
			break;  }			/* end case state */
		}					/* end for */
	scnt = scnt + lcnt;				/* add to total */
	}						/* end if for */
for (i = 0; i < mp -> lines; i++) {			/* clean up pointers */
	if (lp -> rxbpi == lp -> rxbpr)
		lp -> rxbpi = lp -> rxbpr = 0;
	}
return scnt;
}

/* Remove character p from line l input buffer */

void tmxr_rmvrc (TMLN *lp, int32 p)
{
for ( ; p < lp -> rxbpi; p++) lp -> rxb[p] = lp -> rxb[p + 1];
lp -> rxbpi = lp -> rxbpi - 1;
return;
}

/* Store character in line buffer

   Inputs:
	*mp	=	pointer to terminal multiplexor descriptor
	l	=	line
	chr	=	characters
   Outputs:
	none
*/

void tmxr_putchar (TMXR *mp, int32 l, int32 chr)
{
TMLN *lp = mp -> ldsc[l];

if (lp -> conn == 0) return;				/* no conn? done */
if (lp -> txbpi < TMXR_MAXBUF) {			/* room for char? */
	lp -> txb[lp -> txbpi] = (char) chr;		/* buffer char */
	lp -> txbpi = lp -> txbpi + 1;			/* adv pointer */
	if (lp -> txbpi > (TMXR_MAXBUF - 10))		/* near full? */
		lp -> xmte = 0;  }			/* disable line */
else lp -> xmte = 0;					/* disable line */
return;
}

/* Poll for output

   Inputs:
	*mp	=	pointer to terminal multiplexor descriptor
   Outputs:
	none
*/

void tmxr_poll_tx (TMXR *mp)
{
int32 i, nbytes, sbytes;
TMLN *lp;

for (i = 0; i < mp -> lines; i++) {			/* loop thru lines */
	lp = mp -> ldsc[i];				/* get line desc */
	if (lp -> conn == 0) continue;			/* skip if !conn */
	nbytes = lp -> txbpi - lp -> txbpr;		/* avail bytes */
	if (nbytes) {					/* >0? write */
		sbytes = sim_write_sock (lp -> conn,
			 &(lp -> txb[lp -> txbpr]), nbytes);
		if (sbytes != SOCKET_ERROR) {		/* update ptrs */
			lp -> txbpr = lp -> txbpr + sbytes;
			lp -> txcnt = lp -> txcnt + sbytes;
			nbytes = nbytes - sbytes;  }
		}
	if (nbytes == 0) {				/* buf empty? */
    		lp -> xmte = 1;				/* enable this line */
		lp -> txbpr = lp -> txbpi = 0;  }
	}						/* end for */
return;
}

/* Attach */

t_stat tmxr_attach (TMXR *mp, UNIT *uptr, char *cptr)
{
char* tptr;
int32 i, port;
SOCKET sock;
TMLN *lp;
t_stat r;
extern int32 sim_switches;

port = (int32) get_uint (cptr, 10, 65535, &r);		/* get port */
if ((r != SCPE_OK) || (port == 0)) return SCPE_ARG;
tptr = malloc (strlen (cptr) + 1);			/* get string buf */
if (tptr == NULL) return SCPE_MEM;			/* no more mem? */

sock = sim_master_sock (port);				/* make master socket */
if (sock == INVALID_SOCKET) {				/* open error */
	free (tptr);					/* release buf */
	return SCPE_OPENERR;  }
printf ("Listening on socket %d\n", sock);
if (sim_log) fprintf (sim_log, "Listening on socket %d\n", sock);
mp -> master = sock;					/* save master socket */
strcpy (tptr, cptr);					/* copy port */
uptr -> filename = tptr;				/* save */
uptr -> flags = uptr -> flags | UNIT_ATT;		/* no more errors */

for (i = 0; i < mp -> lines; i++) {			/* initialize lines */
	lp = mp -> ldsc[i];
	lp -> conn = lp -> tsta = 0;
	lp -> rxbpi = lp -> rxbpr = 0;
	lp -> txbpi = lp -> txbpr = 0;
	lp -> rxcnt = lp -> txcnt = 0;
	lp -> xmte = 1;
	lp -> dstb = 0;  }
return SCPE_OK;
}

/* Detach */

t_stat tmxr_detach (TMXR *mp, UNIT *uptr)
{
int32 i;
TMLN *lp;

if ((uptr -> flags & UNIT_ATT) == 0) return SCPE_OK;	/* attached? */
for (i = 0; i < mp -> lines; i++) {			/* loop thru conn */
	lp = mp -> ldsc[i];
	if (lp -> conn) {
		tmxr_msg (lp -> conn, "\r\n");
		tmxr_msg (lp -> conn, sim_name);
		tmxr_msg (lp -> conn, " simulator shutting down... please come back later\r\n\n");
		tmxr_reset_ln (lp);  }		/* end if conn */
	}						/* end for */
sim_close_sock (mp -> master, 1);			/* close master socket */
mp -> master = 0;
free (uptr -> filename);				/* free port string */
uptr -> filename = NULL;
uptr -> flags = uptr -> flags & ~UNIT_ATT;		/* not attached */
return SCPE_OK;
}

/* Stub examine and deposit */

t_stat tmxr_ex (t_value *vptr, t_addr addr, UNIT *uptr, int32 sw)
{
return SCPE_NOFNC;
}

t_stat tmxr_dep (t_value val, t_addr addr, UNIT *uptr, int32 sw)
{
return SCPE_NOFNC;
}

/* Utility routines */

void tmxr_msg (SOCKET sock, char *msg)
{
if (sock) sim_write_sock (sock, msg, strlen (msg));
return;
}
