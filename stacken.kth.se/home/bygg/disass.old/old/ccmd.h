/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Andrew Lowry
*/

/* ccmd.h
 *
 * Include this file if your program will make use of the CCMD
 * command parsing routines.  Included here are function and
 * flag definitions, error codes, and parsing structure declarations.
 * As much as possible, symbols are patterned after corresponding
 * TOPS-20 symbols, with non-alphameric characters (. and %) replaced
 * by underscores.
 */

#ifndef stdin
#include <stdio.h>
#endif
#ifndef _JBLEN
#include <setjmp.h>
#endif
#include "cmfnc.h"		/* get function-specific symbols */
#include "ccmdmd.h"		/* get machine dependent symbols */

/*
 * BRKTAB is a pair of 128-bit arrays specifying the break characteristics
 * of the ASCII characters.  The _br1st array specifies characters which
 * will break field input when they are typed as the first character of
 * a field.  The _brrest array specifies characters that break in other
 * than the first position.  Each array contains one bit per ASCII 
 * code, ordered according to the ASCII sequence.  The leftmost (most
 * significant) bit of the first byte corresponds to ASCII code 0, and
 * the rightmost bit of that same byte corresponds to ASCII code 7.
 * Leftmost bit of the second bit is for ASCII code 8, and so on.
 * When a bit is on, the corresponding character will act as a break
 * character, otherwise it will not.
 *
 * Routines in module cmutil can be used to construct and maintain
 * break tables.
 */

typedef struct BRKTAB {
	char _br1st[16];	/* Bit array for initial character breaks */
	char _brrest[16];	/* Bit array for subsequent breaks */
} brktab;

/*
 * FDB structures hold information required to parse specific fields of
 * a command line.
 */

typedef struct FDB {
	int	_cmfnc;		/* Function code for this field */
	int	_cmffl;		/* Function specific parse flags */
	struct FDB * _cmlst;	/* Link to alternate FDB */
	pdat	_cmdat;		/* Function specific parsing data */
	char *	_cmhlp;		/* pointer to help string */
	char *	_cmdef;		/* pointer to default string */
	brktab * _cmbrk;	/* pointer to special break table */
	char *	_cmest;		/* specialized error string */
} fdb;

/* Common flag defined for all parse functions */
#define	CM_SDH	0x8000		/* Suppress default help message */
#define CM_SDE	0x4000		/* Supress default error msg */
#define CM_NLH  0x2000		/* Extra newline help for multi fdbs */
#define CM_NEOF 0x1000		/* parser handles eof itself. */

typedef struct histbuf {
    int *buf;
    int len;
} cmhistbuf;



typedef struct hist {
    cmhistbuf *bufs;		/* array of history buffers */
    int  len;			/* how many buffers there are */
    int  next;			/* index of next buffer to write into */
    int  current;		/* index of current point in history */
    int  enabled;		/* tell if we should remember */
} cmhist;
/*
 * CSB structure holds information on the state of parsing a command
 * line, as well as pointers to required buffers.
 */

typedef struct CSB {
	int 	_cmflg;		/* flags describing parse state */
	int	_cmflg2;	/* more flags */
	FILE *  _cmij;		/* file for command input */
	FILE *	_cmoj;		/* file for command output */
	FILE *  _cmej;		/* file for error output */
	char *	_cmrty;		/* pointer to prompt string */
	int *	_cmbfp;		/* pointer to beginning of user input */
	int *	_cmptr;		/* pointer to beg of next field to parse */
	int 	_cmcnt;		/* # of chars in buffer past _cmptr */
	int	_cminc;		/* number of unparsed chars after _cmptr */
	int *	_cmhst;		/* history parse point */
	char *	_cmabp;		/* pointer to beginning of atom buffer */
	int	_cmabc;		/* size of atom buffer */
	char *	_cmwbp;		/* pointer to beginning of work buffer */
	int	_cmwbc;		/* size of work buffer */
	int (** _cmact)();	/* table of character action routines */
	int 	_cmbkc;	    	/* break character that caused deferred */
				/*  action or confirmation */
	int	_cmcmx;		/* maximum column position on terminal */
	int	_cmcol;		/* current column position on terminal */
	int	_cmerr;		/* most recent parse error */
	fdb *	_cmifd;		/* ptr to FDB giving an incomplete parse */
	int     (* _cmrph)();	/* function to call when reparse is needed */
	int     (* _cmerh)();	/* function to call on parse error */
	char *  _cmntb;		/* comment to eol string */
	char *	_cmnts;		/* delimited comment beginning */
	char *	_cmnte;		/* delimited comment end */
	int 	_cmmax;		/* maximum number of help tokens to display */
	int     (* _cmblh)();	/* function to call nonblocking and no data */
	int 	_cmwrp;		/* column to wrap at */
	cmhist *_cmhist;	/* command history  structure */
	int 	_cmrmx;		/* max number of rows */
	int *	_cmcur;		/* current position in cmd buffer */
	int (** _cmpract)();	/* table of actions after prefix character */
/* bygg... */
	int  (* _cmXrd)();	/* Read character from X handler. */
	void (* _cmXset)();	/* Tell X handler of new input stream. */
/* ...bygg */
} csb;

typedef struct cmacttab {	/* table of action chars */
    char actionchar;		/* action character */
    int (* actionfunc)();	/* action routine */
} cmacttab;

/*
 * major command line editing modes up to 16
 */
#define CMaNOSET 0		/* used to not set a field */

#define CMaEMACS 0x0001		/* emacs like actions */
#define CMaGMACS 0x0002		/* gnuemacs like actions */
#define CMaVI    0x0003		/* vi like actions (not implemented) */

/*
 * modifiers to cmd line editor
 */
#define CMaFCASE 0x0010		/* forward casing */
#define CMaBCASE 0x0020		/* backward casing */
#define CMaEOF   0x0040		/* handle eof */
#define CMaIEOF  0x0080		/* ignore eof */

/* Flags that can be set in individual character entries (each an int) in
** the _cmbfp buffer of the CSB.  The CC_QUO flag is the high order bit in
** the right half, all the rest are in the left half.  CC_QUO is left in
** the right half so that parse routines can see it, since the parse routines
** get their data as character strings, without the left half flags.
**/

#define	CC_CHR	0x007f		/* character data field */
#define CC_QUO	0x0080		/* character was quoted */
#define	CC_QCH	0x00ff		/* character data with quote flag */
#define	CC_NEC	0x0100		/* character was not echoed */
#define CC_HID	0x0200		/* character is hidden from user */
#define CC_SKP	0x0400		/* character is to be skipped (not */
				/*  considered input for parsing) */
#define	CC_CSK	0x0800		/* char is conditionally skipped, */
				/*  meaning that it is skipped as */
				/*  long as it is followed by a (possibly */
				/*  empty) string of characters with the */
				/*  CC_CSK flag, and eventually a character */
				/*  with the CC_SKP flag. */
#define CC_ACT	0x1000		/* char should be treated as an action char */
				/*  on reparse. */

/* Flag values for _cmflg field of CSB */

/*   Settable by user... */
#define	CM_RAI	0x0001		/* Convert to uppercase before parsing */
#define	CM_WKF	0x0002		/* Wake up as each field is terminated */
#define	CM_NEC	0x0004		/* Do not echo tty input as it is typed */

/*   Maintained by ccmd routines */
#define CM_ESC	0x0008		/* This field got completion to successful */
				/*  parse */
#define	CM_NOP	0x0010		/* Field could not be parsed */
#define	CM_RPT	0x0020		/* Reparse needed -- previously parsed */
				/*  input has been edited */
#define	CM_SWT	0x0040		/* Switch was terminated with a colon */
#define	CM_PFE	0x0080		/* Previous field got completion (for */
				/*  noise word handling) */
#define	CM_DRT	0x0100		/* New input has been typed (and possibly */
				/*  erased) since the last cmini call on */
				/*  this CSB. */
#define CM_CFM	0x0200		/* This field was terminated by a newline */
#define	CM_ACT	0x0400	    	/* Deferred action is set, waiting for a */
				/*  field to run out of input */
#define	CM_PRS	0x0800		/* Data has been parsed in this cmd line */
				/*  (used to ignore confirms following only */
				/*  white space) */
#define CM_ITTY 0x1000		/* input is a terminal */
#define CM_OTTY 0x2000		/* output is a terminal */
#define	CM_TTY	0x3000		/* Command source is a terminal */

#define	CM_NAC	0x4000		/* Do not copy parsed text to atom buffer */
#define CM_CMT	0x8000		/* Currently inside a comment */

/*
 * flags bits for the second flag word.
 */

/* 
 * user settable bits.
 */
#define CM_NIN	0x0001		/* don't do indirections */
#define CM_NHM  0x0002		/* don't use "more" code during help */
				/* a specialized help string */
/*
 * CCMD maintained bits
 */
#define CM_IND	0x0004		/* currently in an indirect parse */
#define CM_EOF  0x0008		/* flag EOF condition */
#define	CM_CRT	0x0010		/* Command source is a video terminal */

extern csb cmcsb;		/* CSB for all parses (ccmd) */

/* Forward declarations for routines that return anything other than int */

fdb *fdbchn();

/* Miscellaneous definitions */

#ifndef TRUE
#define	TRUE	-1
#define	FALSE	0
#endif

#ifndef NULL
#define NULL 0
#endif
extern char *cmrealloc();
