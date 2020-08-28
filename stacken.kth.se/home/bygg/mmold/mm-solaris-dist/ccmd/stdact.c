/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Andrew Lowry ; Major Modifications: Howie Kaye
*/
/* stdact 
** 
** Standard command action routines.  These actions are modeled after 
** those found in the TOPS-20 COMND jsys.  The standard actions defined 
** here will be used unless the programmer specifically provides an 
** overriding set of actions.  
**/

/*
 * these routines were all static.  They are often useful inside applications
 * though, so...they are no longer static
 */

#include "ccmdlib.h"		/* get ccmd package user symbols */
#include "cmfncs.h"		/* and internal symbols */

/* Calling conventions for action routines:
**
** Input arguments:
**   fdblist - A pointer to the list of FDB's for this field.
**   brk - The break character that caused this action.
**   deferred - TRUE if this action is invoked in deferred mode (ie, the
**     current input resulted in an incomplete parse) or immediate mode
**     (immediately after the action character was typed).
**
** Output arguments: None.
** Returns: Standard return code.  CMxDFR will cause the action to be
**   invoked again later in deferred mode, as soon as some field results
**   in an incomplete parse.  CMxRPT will cause a reparse.  CMxGO will
**   cause a wakeup following the action, and CMxOK will indicate a
**   successful action without a subsequent wakeup.
**/

/* Forward declarations for auxilliary routines */
int *goto_end_of_line(), *goto_beginning_of_line(), goto_current_pos(),
    *go_forward_char(), *go_backward_char();

/* Forward declarations for action routines */
int cmpact(), cmpact2(), pcmact(), hlpact(), cfmact(), delact();
int begact(), wrdact(), fixact(), hstact(), bsact(), quoact();
int indiract(), prevact(), nextact();

static int cm_env_var();

int begline(), endline(), backchar(), forchar(), delchar();
int killeol(), twiddle(), cmprefix(), killword(), backword(), forword();
int cmp_pre();
#ifdef TIOCSUST
int loadav(), twiddle_or_load();
#endif /* TIOCSUST */

int pbeep();
int bupcase_word(), bdowncase_word(), bcap_word();
int fupcase_word(), fdowncase_word(), fcap_word();
int upcase_word(), downcase_word(), cap_word();


static cmacttab gmacs_actions[] = {
    { '\001', begline },		/* ^A beginning of line */
    { '\002', backchar },		/* ^B backwards character */
    { '\004', delchar },		/* ^D delete character */
    { '\005', endline },		/* ^E end of line */
    { '\006', forchar },		/* ^F forward character */
    { '\010', bsact },			/* ^H backspace or history */
    { '\011', cmpact2 },		/* TAB - completion */
    { '\012', cfmact },			/* ^J confirm */
    { '\013', killeol },		/* ^K - kill to end of line */
    { '\014', cfmact },			/* ^L - confirm */
    { '\015', cfmact },			/* ^M - confirm */
    { '\016', nextact },		/* ^N - next line (history) */
    { '\020', prevact },		/* ^P - previous line (history) */
    { '\022', fixact },			/* ^R - redisplay line */
#ifdef TIOCSUST
    { '\024', twiddle_or_load },	/* ^T twiddle chars or disp loadav */
#else
    { '\024', twiddle },		/* ^T twiddle chars */
#endif
    { '\025', begact },			/* ^U - erase line */
    { '\026', quoact },			/* ^V - quote next char */
    { '\027', wrdact },			/* ^W delete backwards word */
    { '\033', cmprefix },		/* ESC - prefix char */
    { '@',    indiract },		/* @ - indirect file */
    { '?',    hlpact },			/* ? - help */
    { '\177', delact },			/* ^? delete backward char */
    { '\000', NULL },			/* end of table */
};

static cmacttab emacs_actions[] = {
    { '\001', begline },		/* ^A beginning of line */
    { '\002', backchar },		/* ^B backwards character */
    { '\004', delchar },		/* ^D delete character */
    { '\005', endline },		/* ^E end of line */
    { '\006', forchar },		/* ^F forward character */
    { '\010', bsact },			/* ^H backspace or history */
    { '\011', cmpact2 },		/* TAB - completion */
    { '\012', cfmact },			/* ^J confirm */
    { '\013', killeol },		/* ^K - kill to end of line */
    { '\014', cfmact },			/* ^L - confirm */
    { '\015', cfmact },			/* ^M - confirm */
    { '\016', nextact },		/* ^N - next line (history) */
    { '\020', prevact },		/* ^P - previous line (history) */
    { '\022', fixact },			/* ^R - redisplay line */
#ifdef TIOCSUST
    { '\024', twiddle_or_load },	/* ^T twiddle chars or disp loadav */
#else
    { '\024', twiddle },		/* ^T twiddle chars */
#endif
    { '\025', begact },			/* ^U - erase line */
    { '\026', quoact },			/* ^V - quote next char */
    { '\027', wrdact },			/* ^W delete backwards word */
    { '\033', cmp_pre },		/* ESC - completion and prefix char */
    { '@',    indiract },		/* @ - indirect file */
    { '?',    hlpact },			/* ? - help */
    { '\177', delact },			/* ^? delete backward char */
    { '\000', NULL },			/* end of table */
};


static cmacttab emacs_prefix_actions[] = {
    { '\011', cmpact2 },		/* M-TAB - completion */
    { '\033', cmpact },			/* M-ESC - completion */
    { 'b', backword },			/* M-b - backwards word */
    { 'c', cap_word },			/* M-c capitalize word */
    { 'd', killword },			/* M-d delete forwards word */
    { 'f', forword },			/* M-f move forwards word */
    { 'l', downcase_word },		/* M-l downcase word */
    { 'u', upcase_word },		/* M-u upcase word */
    { 'B', backword },			/* M-B - backwards word */
    { 'C', cap_word },			/* M-C capitalize word */
    { 'D', killword },			/* M-D delete forwards word */
    { 'F', forword },			/* M-F move forwards word */
    { 'L', downcase_word },		/* M-L downcase word */
    { 'U', upcase_word },		/* M-U upcase word */
    { '\0', NULL },
};

static int forward_casing = TRUE;	/* case changes go forwards */
static int ignore_eof = TRUE;		/* ignore ^D's */


#define NCHAR 128			/* size of action tables */

/* Standard action table with defaults loaded */
int (*(stdact[NCHAR]))() = {
    NULL,   begline,   backchar,NULL,	  delchar,endline,forchar, NULL,
    bsact,  cmpact2,   cfmact, killeol,   cfmact, cfmact, nextact, NULL,
    prevact,   NULL,   fixact, NULL,   
#ifdef TIOCSUST
    				    twiddle_or_load,
#else
    				    twiddle,
#endif /* TIOCSUST */
					    begact, quoact, wrdact,
    NULL,   NULL,   NULL,   cmp_pre, NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   hlpact,
    indiract,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   delact
};

int (*(preact[NCHAR]))() = {
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   
    pbeep,   pbeep,   pbeep,   cmpact,   pbeep,   pbeep,   pbeep,   pbeep,
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   
    pbeep,   pbeep,   backword,cap_word,killword,pbeep,forword, pbeep,   
    pbeep,   pbeep,   pbeep,   pbeep,   downcase_word, pbeep, pbeep, pbeep,
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   upcase_word,pbeep,  pbeep,
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,
    pbeep,   pbeep,   backword,cap_word,killword,pbeep,forword, pbeep,   
    pbeep,   pbeep,   pbeep,   pbeep,   downcase_word, pbeep, pbeep, pbeep,
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   upcase_word,pbeep,  pbeep,
    pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep,   pbeep
};

/* miscellaneous global declarations */

#if unix
char cmcont = '\\';		/* continuation character (used by cfmact) */
#else
char cmcont = '-';		/* TOPS-20's continuation character */
#endif

int *disp_forward_char();



/* cmpact
** 
** Purpose:
**   Action routine for an ESCAPE character.  First, cmdflt is called
**   to attempt to stuff a default string into the command line.  If
**   that fails, cmcplt is called to attempt completion on the current
**   input.  This is a deferred action, and always causes wakeup after
**   success.
**/


int 
cmpact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  if (!deferred)
    return(CMxDFR);			/* wait for deferred mode */

  if (cmcsb._cmflg & CM_CMT) {		/* if inside a comment, just beep */
    cmputc(BELL,cmcsb._cmoj);		/* beep if they asked us to */
    cmxflsh();
    return(CMxOK);
  }

  if (cmdflt(fdblist) == CMxOK) {	/* first try filling a default */
    cmcsb._cmflg |= CM_PFE;		/* wants wakeup -activate noise words*/
    return(CMxGO);			/* wakeup on success */
  }
  else
    return(cmcplt(TRUE));		/* otherwise try full completion */
}


/* cmpact2
** 
** Purpose:
**   Action routine for a TAB character.  We only do this if input is
**   coming from a TTY.  This is so that we won't choke on tabs in 
**   Take and indirect files.  First, cmdflt is called
**   to attempt to stuff a default string into the command line.  If
**   that fails, cmcplt is called to attempt completion on the current
**   input.  This is a deferred action, and always causes wakeup after
**   success.
**/

int 
cmpact2(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  if (!(cmcsb._cmflg & CM_ITTY)) {
      cmsti1(brk,0);
      return(CMxOK);
  }
  return(cmpact(fdblist,brk,deferred));
}

/* pcmact
**
** Purpose:
**   Action to perform partial completion on the current input.
**/

int
pcmact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  if (!deferred)
    return(CMxDFR);			/* wait for deferred mode */

  if (cmcsb._cmflg & CM_CMT) {		/* if inside a comment, just beep */
    cmputc(BELL,cmcsb._cmoj);		/* beep if they asked us to */
    cmxflsh();
    return(CMxOK);
  }

  if (cmpdflt(fdblist) == CMxOK) {	/* first try filling a default */
    return(CMxGO);			/* wakeup on success */
  }
  else
    return(cmcplt(FALSE));		/* use standard utility */
}

/* hlpact
**
** Purpose:
**   Action routine for a help request.
**/

int 
hlpact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  if (!deferred)
    return(CMxDFR);			/* wait for deferred mode */

  if (cmcsb._cmflg & CM_CMT) {		/* if inside a comment */
    cmsti1(brk,0);
    return(CMxOK);
  }

  return(cmhelp(fdblist,cmcsb._cmbkc));	/* use std help utility */
}

/* cfmact
**
** Purpose:
**   Action routine for a confirmation.  Stuff a newline, and set the
**   CM_CFM flag in the CSB.  Also, a newline character is set into
**   field _cmbkc in the CSB, to match the newline that got stuffed
**   into the buffer.  If the confirmation character was formfeed,
**   clear the screen.  If the confirmation character was carriage
**   return or newline, and if there is a continuation character
**   at the end of the unparsed input buffer, no action is taken,
**   but a newline is stuffed with CC_SKP flag on, and the CC_CSK
**   flag is added to the hyphen (to make it skip conditionally on
**   the newline's presence).
**/


int
cfmact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  int ret;
  int *end;				/* end of current input */

  if (!deferred)
      return(CMxDFR);

  if (cmcsb._cminc > 0) {		/* is there unparsed input? */
    end = cmcsb._cmcur - 1;		/* point to last char */
    if ((*end & CC_QCH) == cmcont) {	/* last char continuation char? */
					/*  (fails if quoted) */
      *end |= CC_CSK;			/* make it conditionally skipped */
      ret = cmsti1(NEWLINE,CC_SKP);	/* and stuff a skipped newline */
      return(ret);			/* no wakeup */
    }
  }

  if (cmcsb._cmflg & CM_CMT) {		/* if inside a comment */
      cmcsb._cmflg &= ~CM_CMT;		/* then turn off the comment */
  }
      
  if (cmcsb._cmflg2 & CM_IND) {
      cmsti1(' ',0 );
      return(CMxOK);
  }

  go_from(cmcsb._cmcur, cmcsb._cmptr + cmcsb._cminc);
  cmcsb._cmcur = cmcsb._cmptr + cmcsb._cminc;

  ret = cmsti1(NEWLINE,0);		/* stuff newline */
  if (ret != CMxOK)
    return(ret);			/* propagate problems */

  cmcsb._cmflg |= CM_CFM;		/* set confirmed flag */
  cmcsb._cmbkc = NEWLINE;		/* and set confirming char */
  if ((brk == FORMFEED) &&
      (cmcsb._cmflg2 & CM_CRT) &&
      !(cmcsb._cmflg & CM_NEC)
     )
    cmxcls();	 	  	    	/* clear the screen for a formfeed */
  remember();				/* add to cmdline history */
  return(CMxGO);			/* now wake them up */
}

/* delact
**
** Purpose:
**   Erase back to and including the last non-hidden character in the
**   command buffer.  If erasing continues into the parsed region, a
**   reparse is signalled.
**/

int
delact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  int *cp;				/* for scanning the command buffer */
  int eralen;				/* number of characters to erase */

  cp = cmcsb._cmcur;			/* point to end of buffer */
  while (cp-- != cmcsb._cmbfp) { 	/* loop over all the chars */
    if ((*cp & CC_HID) == 0)
      break;				/* found a non-hidden character */
  }
  cp++;					/* point to last nonhidden char */

  if (cp != cmcsb._cmbfp)		/* if there are nonhidden chars */
    cp--;				/* consume the last one */

  eralen = (cmcsb._cmcur - cp);		/* get # of chars erased */
  if (eralen == 0) {
    if (cmcsb._cmflg & CM_TTY)
      cmputc(BELL,cmcsb._cmoj);	/* beep if nothing */
    return(CMxOK);
  }

#ifdef undef
  if (cmcsb._cmflg2 & CM_CRT)
    cmcsb._cmcol = cmxera(eralen,FALSE); /* erase the characters */
  else if ((*cp & (CC_HID | CC_NEC)) == 0) { 
    cmechx('\\');			/* give erase marker on hardcopy */
    cmechx((char) (*cp) & CC_CHR);
  }
#endif
  return(delcurrent(eralen));
}

/* wrdact
**
** Purpose:
**   Erase the last word of command line input.  Words consist of letters
**   and digits.  All other characters are delimiters.  This action erases
**   the last nonhidden character in the input, and then continues erasing
**   until it is about to erase a delimiter.
**/

int
wrdact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  int *cp;				/* pointer to deletion site */
  int cc;				/* character under examination */
  char c;
  int eralen;				/* # of chars erased */

  cp = cmcsb._cmcur;			/* point to end of buffer */
  while (cp-- != cmcsb._cmbfp) { 	/* loop over all the chars */
    if ((*cp & CC_HID) == 0)
      break;				/* found a non-hidden character */
  }
  cp++;					/* point to last nonhidden char */

  if (cp != cmcsb._cmbfp)
    cp--;				/* erase at least 1 nonhidden char */
  while (cp-- != cmcsb._cmbfp) {	/* search for nonhidden delimiter */
    c = (cc = *cp) & CC_CHR;		/* get next char */
    if (((cc & CC_HID) == 0) &&		/* nonhidden? */
        ((c < '0') ||			/* and not a letter or digit? */
	 ((c > '9') && (c < 'A')) ||
	 ((c > 'Z') && (c < 'a')) ||
	 (c > 'z')
	)
       )
      break;				/* yup, stop looking */
  }
  cp++;					/* point to char after break */

  eralen = (cmcsb._cmcur - cp);		/* get # of chars erased */
  if (eralen == 0) {
    if (cmcsb._cmflg & CM_TTY)
      cmputc(BELL,cmcsb._cmoj); 	/* beep if nothing */
    return(CMxOK);
  }

#ifdef undef
  if (cmcsb._cmflg2 & CM_CRT)
    cmcsb._cmcol = cmxera(eralen,FALSE); /* erase the characters */
  else
    cmechx('_');			/* print underscore on hardcopy */
#endif
  return(delcurrent(eralen));
}


/* begact
**
** Purpose:
**   Erase the entire command line, back to the last unhidden newline
**   character.  If the last character is a newline, it is erased, and 
**   the prior line is erased back to the previous newline.
**   If parsed characters are deleted, a reparse is called for.
**/

int
begact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  int *cp;				/* pointer to deletion site */
  int cc;				/* character under examination */
  char c;
  int eralen;				/* # of chars erased */

  cp = cmcsb._cmptr + cmcsb._cminc; 	/* point to end of buffer */
  while (cp-- != cmcsb._cmbfp) { 	/* loop over all the chars */
    if ((*cp & CC_HID) == 0)
      break;				/* found a non-hidden character */
  }
  cp++;					/* point to last nonhidden char */

  if (cp != cmcsb._cmbfp)
    cp--;				/* erase at least 1 nonhidden char */
  while (cp-- != cmcsb._cmbfp) {	/* search for nonhidden newline */
    c = (cc = *cp) & CC_CHR;		/* get next char */
    if (((cc & CC_HID) == 0) && (c == NEWLINE)) /* nonhidden newline? */
      break;				/* yup, stop looking */
  }
  cp++;					/* point to char after break */

  eralen = (cmcsb._cmptr - cp) + cmcsb._cminc; /* get # of chars erased */

#ifdef undef
  if (cmcsb._cmflg2 & CM_CRT)
    cmcsb._cmcol = cmxera(eralen,TRUE); /* erase the characters */
  else {
    cmxputs("^U");			/* signal line kill on hardcopy */
    cmxnl();				/* move to a new line */
    if (cp == cmcsb._cmbfp)		/* killed prompt line? */
      cmxputs(cmcsb._cmrty);		/* then reprompt */
  }
#endif
  cmcsb._cmcur = goto_end_of_line();
  return(delcurrent(eralen));
}

/* fixact
**
** Purpose:
**   Refresh the display of the current line of text, back to the
**   last unhidden newline character.  If there is no newline, refresh the
**   prompt and all the current text.  If the last character in the
**   buffer is a newline, the previous line is refreshed.
**/

int
fixact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  int *cp1, *cp2, *cp;			/* pointers into buffer */

  cp = cp2 = cp1 = cmcsb._cmcur;	/* point to end of buffer */
  while(cp1 > cmcsb._cmbfp && (*(cp1-1) & CC_CHR) != '\n') cp1--;
  while((cp2 < cmcsb._cmptr + cmcsb._cminc) && (*cp2 & CC_CHR) != '\n') cp2++;
  

  if (cmcsb._cmflg2 & CM_CRT) {
      cmputc('\r', cmcsb._cmoj);
      cmceol();
  }
  else {
    cmxputs("^R");			/* signal line kill on hardcopy */
    cmxnl();				/* move to a new line */
  }
  cmcsb._cmcol = 0;
  if (cp1 == cmcsb._cmbfp) {		/* killed prompt line? */
      cmxputs(cmcsb._cmrty);		/* then reprompt */
  }
  
  cmcsb._cmcur = cp1;
  cmcsb._cmcur = disp_forward_char(cp2 - cp1);
  cmcsb._cmcur = go_backward_char(cp2 - cp);
  return(CMxOK);
}



/* hstact
**
** Purpose:
**   Action routine to reinstate the command buffer from a previous
**   failed parse.  If the CM_DRT flag is off in the CSB, then the
**   prior input text is still intact.  That text is redisplayed, and
**   the CSB pointers are set so that all of the text is considered
**   unparsed.  Then a no-wakeup return is made (a wakeup would probably
**   just cause the same parse failure again).
**/

int
hstact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  int *cp;				/* pointer into command buffer */
  int count,i;				/* number of chars to reinstate */

  if (!deferred)
      return(CMxDFR);
  if (cmcsb._cmflg & CM_DRT || cmcsb._cminc > 0)
    return(CMxOK);			/* nothing to do if buffer is dirty */
  count = cmcsb._cmhst - cmcsb._cmbfp;	/* count buffered chars */
  cp = cmcsb._cmbfp;			/* point to beginning of buffer */
  for (i = 0; i < count; i++) {
      if ((cp[i] & CC_CHR) == NEWLINE) {
	  count = i;
	  break;
      }
  }
  cmcsb._cminc = count;			/* this many chars now to parse */
  cmcsb._cmcnt -= count;		/* count their presence */
  cmcsb._cmflg |= CM_DRT;		/* now the buffer is dirty */
  cmcsb._cmcur = cmcsb._cmbfp + count;	/* point at end of line */
  while (count-- > 0) 			/* step through the buffer */
    if (((*cp) & CC_NEC) == 0)		/* originally echoed? */
      cmechx((char) (*cp++) & CC_CHR);	/* then echo it now */
    else
      cp++;				/* else just move on */
  return(CMxOK);
}



/* bsact
**
** Purpose:
**   Action routine for a backspace.  If the buffer is dirty, invoke the
**   single character deletion action.  Otherwise, invoke the history
**   action.
**/

int
bsact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  if (cmcsb._cmflg & CM_DRT)		/* buffer dirty? */
    return(delact(fdblist,brk,deferred)); /* delete one char */
  else
    return(hstact(fdblist,brk,deferred)); /* otherwise try history */
}


/* quoact
**
** Purpose:
**   Enter the next character into the buffer with its quote flag
**   turned on, so it will be treated as a normal character regardless
**   of any handling it would normally receive.
**/

int
quoact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
  int c;			/* quoted character */
  int ret;			/* result code from input operation */
  
  ret = cmgetc(&c,cmcsb._cmij); /* get another character */
  if (ret != CMxOK)
    return(ret);		/* propagate problems */
  ret = cmsti1(c,CC_QUO);	/* enter the charcter, quoted */
  return(ret);			/* CMxOK normally -- no wakeup */
}

#define MAXINDIRECTIONS 25
struct indir_stack_ent {
    FILE *oldij, *oldoj;
    int (*olderr)();
};

struct indir_stack {
    struct indir_stack_ent data[MAXINDIRECTIONS];
    int sptr;
};

static struct indir_stack indstack;

ind_oldfds() {
    static int first = TRUE;
    if (first) {
	indstack.sptr = -1;
	first = FALSE;
    }
    if (indstack.sptr >= MAXINDIRECTIONS) {
	cmerjmp(CMxSOF, NULL);
    }
    indstack.sptr++;
    indstack.data[indstack.sptr].oldij = cmcsb._cmij;
    indstack.data[indstack.sptr].oldoj = cmcsb._cmoj;
    indstack.data[indstack.sptr].olderr = cmcsb._cmerh;
}

cmindend() {
    if (indstack.sptr < 0)  {
	cmcsb._cmflg2 &= ~CM_IND;
	cmerjmp(CMxSUF, NULL);
    }
    fclose(cmcsb._cmij);
    if (cmcsb._cmoj) 
	fclose(cmcsb._cmoj);
    cmseti(indstack.data[indstack.sptr].oldij,
	   indstack.data[indstack.sptr].oldoj, cmcsb._cmej);
    cmcsb._cmerh = indstack.data[indstack.sptr].olderr;
    indstack.sptr--;
    if (indstack.sptr < 0)
	cmcsb._cmflg2 &= ~CM_IND;
}

cminderr(code) int code; {
    cmindend();				/* turn off indirection */
    cmerjmp(code,NULL);			/* call old error handler */
}


int indiract(fdblist, brk, deferred, flags)
fdb *fdblist;
char brk;
int deferred,flags;
{
  char c;			/* quoted character */
  int ret;			/* result code from input operation */
  FILE *f;
  
  static fdb filfdb = { _CMFIL, CM_SDH, NULL, NULL,
			    "Filename for indirect file", NULL, NULL };
  static fdb cfmfdb = { _CMCFM, 0, NULL, NULL, NULL, NULL, NULL };
  static brktab chrbrk = {
      {
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      },
      {
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      },
  };
  static fdb chrfdb = { _CMCHAR, CM_SDH, NULL, NULL,
 			    "Filename for indirect file", NULL, &chrbrk };
  static int entered = FALSE;
  fdb *used;
  pval pv;
  static char *fname = NULL;
  char *malloc();
  int i;

  if (cmcsb._cmflg & CM_CMT) {		/* if inside a comment,  */
      cmsti1(brk,0);			/* just insert the break char */
      return(CMxOK);
  }

  if (fname != NULL)			/* free up old filename */
      free(fname);

  if (cmcsb._cmflg2 & CM_NIN) { /* no indirections allowed  */
      cmsti1(brk,0);			/* just insert the break char */
      return(CMxOK);			/* and go home */
  }

  if (!deferred) {
    return(CMxDFR);			/* wait for deferred mode */
  }

  if (!(flags & CC_ACT))
      cmsti1(brk,CC_ACT);			/* make it show up. */
  cmcsb._cmflg &= ~CM_ACT;

  for(i = 0; i < 16; i++) {
      chrbrk._br1st[i] = 0xff;
      chrbrk._brrest[i] = 0xff;
  }
  chrbrk._br1st[brk/8] &= ~(1<<(7-(brk%8))); /* turn off the bit */
  chrbrk._brrest[brk/8] &= ~(1<<(7-(brk%8))); /* turn off the bit */
  parse(&chrfdb, &pv, &used);
  parse(&filfdb, &pv, &used);		/* parse filename */
  fname = malloc(strlen(pv._pvfil[0]) + 1); /* copy it */
  strcpy(fname,pv._pvfil[0]);
  parse(&cfmfdb, &pv, &used);		/* parse a confirm. */
  
  ind_oldfds();
  f = fopen(fname,"r");
  if (f == NULL) {
      cmcsb._cmerr = 0;
      indstack.sptr--;
      cmerjmp(0, NULL);
  }
  cmseti(f,NULL,cmcsb._cmej);
  cmcsb._cmerh = cminderr;
  cmcsb._cmflg2 |= CM_IND;		/* turn on indirection flag */
  return(CMxOK);
}

#ifdef TIOCSUST
int
loadav(fdblist,brk,deferred)
{
    int x,y,z;
    int ret;
    int i;

    ioctl(fileno(cmcsb._cmij),TIOCGETD,&y); /* get line discipline */
    ioctl(fileno(cmcsb._cmij),TIOCSETD,&x); /* set to new line discipline */
    if (!ioctl(fileno(cmcsb._cmij),TIOCSUST,&x)) { /* show the load */
	fixact(fdblist, brk, deferred);
    }
    ioctl(fileno(cmcsb._cmij),TIOCSETD,&y); /* restore line disc. */
    return(CMxOK);			/* all done. */
}

#endif /* TIOCSUST */


/* 
 * routines to implement command line history.
 */


/* 
 * add a line to the command line history (if we have one).
 */
remember() {
    cmhist *h = cmcsb._cmhist;
    int i,len,j;
    if (h == NULL) {
	cmhst(10);
	h = cmcsb._cmhist;
    }
    if (h->enabled == FALSE || h->len == 0)
	return;
    i =  h->next % h->len;
    len = cmcsb._cmptr - cmcsb._cmbfp + cmcsb._cminc;
    if (cmcsb._cmbfp[len-1] == '\n')
	len--;
    if (len == 0) return;
    h->bufs[i].buf = (int *)cmrealloc(h->bufs[i].buf, len * sizeof(int));
    for(j = 0; j < len; j++) {
	h->bufs[i].buf[j] = cmcsb._cmbfp[j];
    }
    h->bufs[i].len = len;
    h->next++;
    h->next %= h->len;
    h->current = h->next;
    if (h->bufs[h->next].buf) {
	free(h->bufs[h->next].buf);
	h->bufs[h->next].buf = NULL;
	h->bufs[h->next].len = 0;
    }
}

int 
nextact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    cmhist *h = cmcsb._cmhist;
    int i;
    int next;
    if (h == NULL || h->len == 0) {
	cmputc(BELL,cmcsb._cmoj);
	cmxflsh();
	return(CMxOK);
    }
    next = (h->current + h->len + 1) % h->len;
    if (h->bufs[next].buf == NULL || h->next == h->current) {
	cmputc(BELL,cmcsb._cmoj);
	cmxflsh();
	return(CMxOK);
    }
    h->current = next;
    begact(fdblist,brk,deferred);
    for( i= 0; i < h->bufs[next].len; i++)
	cmsti1(h->bufs[next].buf[i] & CC_CHR, h->bufs[next].buf[i] & ~CC_CHR);
    cmcsb._cmflg &= ~CM_DRT;
    return(force_reparse());
}


int 
prevact(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    cmhist *h = cmcsb._cmhist;
    int i;
    int prev;

    if (h == NULL || h->len == 0) {
	cmputc(BELL,cmcsb._cmoj);
	cmxflsh();
	return(CMxOK);
    }
    prev = (h->current + h->len - 1) % h->len;

    if (h->bufs[prev].buf == NULL) {
	cmputc(BELL,cmcsb._cmoj);
	cmxflsh();
	return(CMxOK);
    }
    h->current = prev;
    cmcsb._cmcur = (int *)go_from(cmcsb._cmcur, cmcsb._cmptr + cmcsb._cminc);
    delcurrent(cmcsb._cmcur - cmcsb._cmbfp);
    for( i= 0; i < h->bufs[prev].len; i++)
	cmsti1(h->bufs[prev].buf[i] & CC_CHR, h->bufs[prev].buf[i] & ~CC_CHR);
    cmcsb._cmflg &= ~CM_DRT;
    return(force_reparse());
}


int
delcurrent(eralen)
int eralen;
{
  int *p,*p1, i= 0;
  int l,c,len;
  int lines,col;

					/* get to end of line */
  p = cmcsb._cmptr + cmcsb._cminc;
  relcharpos(&l, &c, cmcsb._cmcur-eralen, p); /* how far to go? */

  bcopy(cmcsb._cmcur, cmcsb._cmcur-eralen, /* update the buffers */
	(cmcsb._cmptr + cmcsb._cminc - cmcsb._cmcur)*sizeof(*cmcsb._cmbfp));

  if (l > 0) {
    cmcsb._cmcnt += eralen;
    cmcsb._cmcur -= eralen;
    p -= eralen;
    lines = l;				/* how many lines to clear */
    col = cmcsb._cmcol - c;		/* column to returtn to */
    cmxputc('\r', cmcsb._cmoj);
    cmceol();
    for(; lines > 1; lines--) {
      cmxnl(cmcsb._cmoj);
      cmceol();
    }
    cmxputc('\r',cmcsb._cmoj);
    go_up_line(l);
    go_forward(col);
    cmceol();				/* clear to end of line */
    lines = l;
    for(p1 = cmcsb._cmcur; p1 < p; p1++) {
      if (!(*p1 & (CC_NEC|CC_HID))) {
        cmechx(*p1);
        if (((*p1) & CC_CHR) == '\n') {
	  cmceol();
	  lines--;
        }
        else {
	  if (cmcsb._cmcol == 0)
	    lines--;
        }
      }
    }
    if (l != lines) {
      go_up_line(l-lines);
      go_forward(col);
    }
  }
  else {
    go_backward_char(eralen);		/* back up past stuff to be deleted */
    cmceol();				/* clear to end of line */
    col = cmcsb._cmcol;
    lines = 0;
    cmcsb._cmcnt += eralen;
    cmcsb._cmcur -= eralen;
    p -= eralen;
    for(p1 = cmcsb._cmcur; p1 < p; p1++) {
      if (!(*p1 & (CC_NEC|CC_HID))) {
        cmechx(*p1);
        if (((*p1) & CC_CHR) == '\n') {
	  cmceol();
	  lines--;
        }
        else {
	  if (cmcsb._cmcol == 0)
	    lines--;
        }
      }
    }
    go_from(p, cmcsb._cmcur);
  }
  cmcsb._cmcol = col;

  if (cmcsb._cmcur <= cmcsb._cmptr) {	/* erased back to parsed data? */
    cmcsb._cminc = cmcsb._cminc + cmcsb._cmptr - cmcsb._cmbfp - eralen;
    cmcsb._cmptr = cmcsb._cmbfp;	/* yup, backup parsed pointer */
    return(CMxRPT);			/* and call for reparse */
  }
  else {
    cmcsb._cminc -= eralen;
    return(CMxOK);
  }
}

int
begline(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    cmcsb._cmcur = goto_beginning_of_line();
    return(maybe_reparse());
}

int
endline(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    cmcsb._cmcur = goto_end_of_line();
    return(maybe_reparse());
}



int
backchar(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    if (cmcsb._cmcur > cmcsb._cmbfp) {
	cmcsb._cmcur = go_backward_char(1);
	return(maybe_reparse());
    }
    else {
      cmputc(BELL,cmcsb._cmoj);	/* beep if nothing */
      return(CMxOK);
    }
}



int
forchar(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    if (cmcsb._cmcur < cmcsb._cmptr + cmcsb._cminc) {
	cmcsb._cmcur = go_forward_char(1);
	cmcsb._cmflg &= ~CM_RPT;
	return(maybe_reparse());
    }
    else {
      cmputc(BELL,cmcsb._cmoj);	/* beep if nothing */
      return(CMxOK);
    }
}



int
delchar(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    int *p,i=0;
    
    if (cmcsb._cmcur < cmcsb._cmptr + cmcsb._cminc) {
	cmcsb._cmcur = go_forward_char(1);
	return(delcurrent(1));		/* deletes backwards */
    }
    else {
      if (cmcsb._cmcur == cmcsb._cmbfp && /* empty line */
	  cmcsb._cmcur == cmcsb._cmptr + cmcsb._cminc
	  && !ignore_eof)
	  return(CMxEOF);
      cmputc(BELL,cmcsb._cmoj);	/* beep if nothing */
      return(CMxOK);
    }
}



maybe_reparse()
{
  if (cmcsb._cmcur <= cmcsb._cmptr) {	/* erased back to parsed data? */
    return(force_reparse());
  }
  return(CMxOK);
}

force_reparse() 
{
    cmcsb._cminc = cmcsb._cminc + cmcsb._cmptr - cmcsb._cmbfp;
    cmcsb._cmptr = cmcsb._cmbfp;	/* yup, backup parsed pointer */
    return(CMxRPT);			/* and call for reparse */
}

int
killeol(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    int *p = cmcsb._cmcur;
    cmcsb._cmcur = goto_end_of_line();
    delcurrent(cmcsb._cmcur - p);
    return(maybe_reparse());
}


#ifdef TIOCSUST
int
twiddle_or_load(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    if (cmcsb._cmcur == cmcsb._cmbfp) 
	return(loadav(fdblist, brk, deferred));
    return(twiddle(fdblist, brk, deferred));
}
#endif


int
twiddle(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    int tmp;
    int *cp;

    if (cmcsb._cmptr + cmcsb._cminc - cmcsb._cmbfp < 2) {
	cmputc(BELL,cmcsb._cmoj);	/* beep if nothing to twiddle */
	return(CMxOK);
    }
    if (cmcsb._cmcur == cmcsb._cmptr + cmcsb._cminc) { /* end of line? */
	cp = cmcsb._cmcur = go_backward_char(2);
	tmp = *cp;
	*cp = *(cp+1);
	*(cp+1) = tmp;
	cmcsb._cmcur = disp_forward_char(2);
	if (cmcsb._cminc < 2) {
	    return(force_reparse());
	}
	return(CMxOK);
    }
    if (cmcsb._cmcur == cmcsb._cmbfp) {	/* beginning of line? */
	cp = cmcsb._cmcur;
	tmp = *cp;
	*cp = *(cp+1);
	*(cp+1) = tmp;
	cmcsb._cmcur = disp_forward_char(2);
	cmcsb._cmcur = go_backward_char(2);
	return(CMxOK);
    }
    cp = cmcsb._cmcur = go_backward_char(1);
    tmp = *cp;
    *cp = *(cp+1);
    *(cp+1) = tmp;
    cmcsb._cmcur = disp_forward_char(2);
    if (cmcsb._cminc < 2)
	return(force_reparse());
    return(maybe_reparse());
}

int
cmprefix(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    int c;
    static int prebrk;
    int ret;

    cmcsb._cmflg &= ~CM_ACT;
    if (!deferred &&  !(cmcsb._cmflg & CM_ESC)) {
	c = cmgetc(&prebrk, cmcsb._cmij);
	if (c == CMxEOF)
	    return(CMxEOF);
    }
    if (cmcsb._cmpract[prebrk] == NULL) {
	return(CMxPRE);
    }
    ret = (*cmcsb._cmpract[prebrk])(fdblist,prebrk,deferred);
    if (ret == CMxDFR) 
	cmcsb._cmflg |= CM_ESC;
    return(ret);
}



int
killword(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    int i;

    for(i = 0; i < cmcsb._cmptr + cmcsb._cminc - cmcsb._cmcur; i++) {
	if (iswordchar(cmcsb._cmcur[i] & CC_CHR)) 
	    break;
    }
    for(; i < cmcsb._cmptr + cmcsb._cminc - cmcsb._cmcur; i++) {
	if (!iswordchar(cmcsb._cmcur[i] & CC_CHR)) {
	    break;
	}
    }
    cmcsb._cmcur = go_forward_char(i);
    return(delcurrent(i));
}


int
backword(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    int *p;

    for(p = cmcsb._cmcur - 1 ; p >= cmcsb._cmbfp; p--) {
	if (iswordchar(*p & CC_CHR)) 
	    break;
    }
    for(; p >= cmcsb._cmbfp; p--) {
	if (!iswordchar(*p & CC_CHR)) {
	    p++;
	    break;
	}
    }
    if (p < cmcsb._cmbfp) p = cmcsb._cmbfp;
    cmcsb._cmcur = go_backward_char(cmcsb._cmcur-p);
    return(maybe_reparse());
}


int
forword(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    int i;

    for(i = 0; i < cmcsb._cmptr + cmcsb._cminc - cmcsb._cmcur; i++) {
	if (iswordchar(cmcsb._cmcur[i] & CC_CHR)) 
	    break;
    }
    for(; i < cmcsb._cmptr + cmcsb._cminc - cmcsb._cmcur; i++) {
	if (!iswordchar(cmcsb._cmcur[i] & CC_CHR)) {
	    break;
	}
    }
    cmcsb._cmcur = go_forward_char(i);
    return(CMxOK);
}

int
cmp_pre(fdblist,brk,deferred)
fdb *fdblist;
char brk;
int deferred;
{
    if (cmcsb._cmcur == cmcsb._cmptr + cmcsb._cminc) {
	return(cmpact(fdblist, brk, deferred));
    }
    else {
	return(cmprefix(fdblist, brk, deferred));
    }
}

int
pbeep(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    cmputc(BELL,cmcsb._cmoj);		/* beep */
    return(CMxOK);
}

iswordchar(c)
char c;
{
    return(isalnum(c) || c == '.' || c == '_' || c == '-');
}




/*
 *  case changing 
 */
int
upcase_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    if (forward_casing)
	return(fupcase_word(fdblist, brk, deferred));
    else
	return(bupcase_word(fdblist, brk, deferred));
}

int
downcase_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    if (forward_casing)
	return(fdowncase_word(fdblist, brk, deferred));
    else
	return(bdowncase_word(fdblist, brk, deferred));
}

int
cap_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    if (forward_casing)
	return(fcap_word(fdblist, brk, deferred));
    else
	return(bcap_word(fdblist, brk, deferred));
}

/*
 * backward case changing
 */

int
bupcase_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    return(bcase_word(fdblist, brk, deferred, 'U'));
}

int
bdowncase_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    return(bcase_word(fdblist, brk, deferred, 'D'));
}

int
bcap_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    return(bcase_word(fdblist, brk, deferred, 'C'));
}

int
bcase_word(fdblist, brk, deferred, action)
fdb *fdblist;
char brk;
int deferred;
int action;
{
    int *cp = cmcsb._cmcur;
    int *p,*bp;

    for(p = cmcsb._cmcur - 1 ; p >= cmcsb._cmbfp; p--) { /* skip whitespace */
	if (iswordchar(*p & CC_CHR)) 
	    break;
    }
    for(; p >= cmcsb._cmbfp; p--) {
	if (!iswordchar(*p & CC_CHR)) {
	    p++;
	    break;
	}
    }
    if (p < cmcsb._cmbfp) p = cmcsb._cmbfp;
    cmcsb._cmcur = go_backward_char(cp - p); /* back up */
    for(bp = p; bp < cp; bp++) {
	switch(action) {
	case 'C':
	    if (bp == p) {
		if (islower(*bp)) {
		    *bp = (*bp & 0xff80) | toupper(*bp & CC_CHR);
		}
	    }
	    else if (isupper(*bp)) {
		*bp = (*bp & 0xff80) | tolower(*bp & CC_CHR);
	    }
	    break;
	case 'U':
	    if (islower(*bp)) {
		*bp = (*bp & 0xff80) | toupper(*bp & CC_CHR);
	    }
	    break;
	case 'D':
	    if (isupper(*bp)) {
		*bp = (*bp & 0xff80) | tolower(*bp & CC_CHR);
	    }
	    break;
	}
    }

    cmcsb._cmcur = disp_forward_char(cp - p);
    if (p < cmcsb._cmptr)
	return(CMxRPT);
    return(CMxOK);
}

/*
 * forward case changing
 */
int
fupcase_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    return(fcase_word(fdblist, brk, deferred, 'U'));
}

int
fdowncase_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    return(fcase_word(fdblist, brk, deferred, 'D'));
}

int
fcap_word(fdblist, brk, deferred)
fdb *fdblist;
char brk;
int deferred;
{
    return(fcase_word(fdblist, brk, deferred, 'C'));
}


int
fcase_word(fdblist, brk, deferred, action)
fdb *fdblist;
char brk;
int deferred;
int action;
{
    int *cp = cmcsb._cmcur;
    int *p,*bp;
    int *wp = cp;

					/* skip whitespace */
    for(p = cmcsb._cmcur ; p < cmcsb._cmptr + cmcsb._cminc; p++) {
	if (iswordchar(*p & CC_CHR)) 
	    break;
    }
    wp = p;
    for(; p < cmcsb._cmptr + cmcsb._cminc; p++) {
	if (!iswordchar(*p & CC_CHR)) {
	    break;
	}
    }
    for(bp = cp; bp < p; bp++) {
	switch(action) {
	case 'C':
	    if (bp == wp) {
		if (islower(*bp)) {
		    *bp = (*bp & 0xff80) | toupper(*bp & CC_CHR);
		}
	    }
	    else if (isupper(*bp)) {
		*bp = (*bp & 0xff80) | tolower(*bp & CC_CHR);
	    }
	    break;
	case 'U':
	    if (islower(*bp)) {
		*bp = (*bp & 0xff80) | toupper(*bp & CC_CHR);
	    }
	    break;
	case 'D':
	    if (isupper(*bp)) {
		*bp = (*bp & 0xff80) | tolower(*bp & CC_CHR);
	    }
	    break;
	}
    }
    cmcsb._cmcur = disp_forward_char(p - cp);
    if (cmcsb._cmcur > cmcsb._cmptr + cmcsb._cminc)
	cmcsb._cmcur = cmcsb._cmptr + cmcsb._cminc;
    return(CMxOK);
}

/*
 * install a builtin action table
 * accepts a flagword containing the types to install.
 * see also cmsetact
 */
cmbuiltin_act(actiontype)
int actiontype;
{
    switch (actiontype & 0x000f) {
    case CMaNOSET:			/* not specified in this flagword */
	break;
    case CMaEMACS:
	cmsetact(emacs_actions, cmcsb._cmact);
	cmsetact(emacs_prefix_actions, cmcsb._cmpract);
	break;
    case CMaGMACS:
	cmsetact(gmacs_actions, cmcsb._cmact);
	cmsetact(emacs_prefix_actions, cmcsb._cmpract);
	break;
    case CMaVI:
	fprintf(stderr,"?VI command line editting is not implemented\n");
	break;
    default:
	fprintf(stderr,"?Invalid Command line editor specified\n");
	break;
    }
    if (actiontype & CMaBCASE)
	forward_casing = FALSE;
    if (actiontype & CMaFCASE)
	forward_casing = TRUE;
    if (actiontype & CMaEOF)
	ignore_eof = FALSE;
    if (actiontype & CMaIEOF)
	ignore_eof = TRUE;
}

/*
 * install an action table in a CSB action vector
 */
cmsetact(acttab, actvec)
cmacttab *acttab;
int (*(actvec[NCHAR]))();
{
    cmacttab *a;

    bzero(actvec, sizeof(NCHAR * sizeof(*actvec)));
    for(a = acttab; a->actionchar && a->actionfunc; a++)
	actvec[a->actionchar] = a->actionfunc;
}


/*
 * handle action related CCMD environment variables
 */
cm_env_actions(env)
char *env;
{
    char *cp=env, *cp1, *index();
    int flags=0;
    
    while(cp) {
	cp1 = index(cp, ':');
	if (cp1)
	    *cp1 = '\0';
	flags |= cm_env_var(cp);
	if (cp1) 
	    *cp1++ = ':';
	cp = cp1;
    }
    cmbuiltin_act(flags);
}

/*
 * interpret a ccmd environment variable (if possible)
 */

static struct {
    char *varname;
    int  value;
} envvars[] = {
    { "emacs", CMaEMACS },		/* emacs cmd line editing */
    { "gmacs", CMaGMACS },		/* gnuemac "   "     " */
    { "vi",    CMaVI },			/* vi      "   "     " */
    { "fcase", CMaFCASE },		/* forward up/downcasing */
    { "bcase", CMaBCASE },		/* backward up/downcasing */
    { "ignoreeof", CMaIEOF },		/* ignore eof */
    { "noignoreeof", CMaEOF },		/* return eof */
    { NULL, 0 }
};

static int
cm_env_var(var)
char *var;
{
    int i;

    for(i = 0; envvars[i].varname; i++)
	if (ustrcmp(var, envvars[i].varname) == 0)
	    return(envvars[i].value);
    return(0);
}


