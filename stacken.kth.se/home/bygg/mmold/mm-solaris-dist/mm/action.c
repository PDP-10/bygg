/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#ifndef lint
static char *rcsid = "$Header: /w/src1/sun4.bin/cucca/mm/RCS/action.c,v 2.1 90/10/04 18:23:16 melissa Exp $";
#endif

#include "mm.h"
#include "parse.h"
#include "cmfncs.h"

int (*(mmact[128]))();
extern int (*(stdact[128]))(), fixact(), begact(), nextact();
int mm_clsact(), fixact(), mm_abortact();
extern jmp_buf abortbuf;
int allow_aborts;

/*
 * set up special action characters for MM.
 */

typedef int (*(*acttab))();

acttab
mmactini()
{
    bcopy(stdact, mmact, sizeof(mmact));
    mmact[(int) '\f'] = mm_clsact;
    mmact[(int) '\016'] = mm_abortact; 
    return (mmact);
}

mm_clsact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  extern int control_l_confirm;

  if (control_l_confirm) {
      return ((*stdact[brk])(fdblist, brk, deferred));
  }
  cmxcls();	 	  	    	/* clear the screen for a formfeed */
  return(fixact(fdblist,brk,deferred));	/* refresh the line */
}
 
int
mm_abortact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    pval parseval;
    fdb *used;
    csb holdcsb;
    int cmdbuf[200];
    char atmbuf[200], wrkbuf[200];
    jmp_buf erbuf, rpbuf;
    extern int user_aborted;

    if (deferred)
	return(CMxDFR);

    if (!allow_aborts) {
	return(nextact(fdblist,brk,deferred));
    }

    if (control_n_abort == SET_ALWAYS) {
	user_aborted = true;
	longjmp (abortbuf, 1);
    }
    if (control_n_abort == SET_NEVER) {
	cmsti1 (brk, 0);
	return CMxOK;
    }
    else {				/* set control-n-abort ask */
	extern int user_aborted;
	csb oldcsb;
	save_parse_context();
	oldcsb = cmcsb;
	cmbufs (cmdbuf, sizeof cmdbuf, atmbuf, sizeof atmbuf,
		wrkbuf, sizeof wrkbuf);
	cmact (nil);
	user_aborted = yesno("Abort? ", "yes");
	cmcsb = oldcsb;
	cmcsb._cmcol = 0;
	restore_parse_context();
	if (user_aborted)
	    longjmp (abortbuf, 1);
	fixact (fdblist, brk, 0);
	return (CMxOK);
    }
}

/*
 * redisplay_line:
 * simulate a ^R
 */
redisplay_line ()
{
    fixact (NULL, NULL, NULL);		/* doesn't use any args */
}

