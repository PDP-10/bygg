/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Howie Kaye
*/

/*
 * cmpara
 * parse 'chunks' of text, with some nice actions defined.
 *
 * texti style data input.
 */

/*
 * This file contains the code to parse chunks of text, simlilarly to the 
 * way MM parses a message on the DEC-20
 *
 * Accepts a char *, which is:
 *  a buffer of initialized text -- so that the function can be 
 *	interrupted, and then continued.
 *  and a list of action characters.  These are only in place when inside 
 *  the actual paragraph parse.  When an action char is typed.
 * 
 * This is pretty hairy.  What it does, is to change the action routines
 * being used, to allow the default/user defined actions to work.  It also
 * has to manage it's own set of cmd buffers and work buffers, so that
 * calls to ccmd insertion routines (cmsti) do not overflow the buffer
 * space which ccmd has been given (cmbufs call).  The routines here are
 * meant to be called after a confirm has been given, and no reparses into
 * previous data will be necessary.  It can be used to parse large chunks
 * of text, in a manner similar to the TEXTI jsys under TOPS-20.
 *
 * In order to make ccmd appear to be in a normal state whenever we leave
 * the paragraph environment, we have to remember the old environment, and
 * 'context switch' the old environment in (and out again) whenever an
 * action character is typed (the action routine may make asumptions about
 * the ccmd environment, and try to parse something).  To do this, all
 * action character force a call to the break_hndlr() routine.  This then
 * saves the environment, calls the real handler, and then restores the
 * environment.  It also fixes the cmd internal buffers to reflect the new
 * state.   
 * 
 * When a user supplied (or default) action routine is called, it is
 * passed the text it is acting on (This buffer is malloc'ed to the correct
 * size, and should not be written to if the write will go past the bounds
 * of the current buffer).  The routine is also passed two flags.  If the
 * buffer is modified, then the modified flag should be set.  If the
 * parse should end (eof,abort,etc), then the ret flags should be set.  In
 * all cases, a pointer to the new text should be returned.
 */

					/* allocate cmd stuff here */
#define PARAERR

					/* system structures/defines */
#include "ccmdlib.h"
#include "cmfncs.h"
					/* incremental buffer size */
#define BSIZ 1000
					/* minimum space left before buffer */
					/* expansion */
#define MINSIZE 133

#define beep() cmputc(BELL,cmcsb._cmoj)

/* 
 * paragraph break mask
 */
static brktab parabrk = {
  {					/* all print characters */
					/* except tab and space */
    0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  },
  {
    0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  }
};
    
static brktab pipeparabrk = {
  {					/* just newline is a brk char */
					/* except tab and space */
    0x00, 0x24, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  },
  {
    0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  }
};

/*
 * The behavior of this parser is different from the standard
 * ccmd interface, so we must set up our own action table.
 */

/*
 * forward declares
 */
char *mktemp();
PASSEDSTATIC int paraparse(), paracomplete(), parahelp(); 
static char *redispact(), *begact(), *fixact(), *quoact(), *delact(),
     *wrdact(),*editact(),*dispact(),*insact(),*abortact(), *eolact(),
     *autowrapact(), *autowraptab(), *autowrapspace(), *autowrapnl();
static char **get_path();
static char *eofact();
static char *executeact();
static char *filteract();
static char *parse_cmdline();
static char *parse_cmdline1();
static int lastlinelen();
static int tablen();

/* 
 * default paragraph actions
 */
para_actions def_actions[] = {
  { '\002', insact },			/* ^B insert file */
  { '\004', eofact },			/* ^D end of file */
  { '\005', editact },			/* ^E invoke editor on this buffer */
  { '\006', filteract },		/* ^F run buffer through filter */
  { '\b', delact },			/* back space */
  { '\t', autowraptab },		/* autowrap on tab */
  { '\n', autowrapnl },			/* \n end of line wrap */
  { '\r', autowrapnl },			/* CR or LF. */
  { '\013', dispact },			/* ^K display whole buffer */
  { '\f', redispact },			/* ^L redisplay screen */
  { '\016', abortact },			/* ^N abort this insertion */
  { '\020', executeact },		/* ^P execute command and insert */
  { '\022', fixact },			/* ^R redisplay line */
  { '\025', begact },			/* ^U delete line */
  { '\026', quoact },			/* ^V quota character */
  { '\027', wrdact },			/* ^W delete backwards word */
  { ' ', autowrapspace },		/* SPC autowrap on space */
  { '\177', delact },			/* DEL delete previous char */
  { (char)NULL, NULL },			/* end the table */
};

static int (**oldact)();		/* old action table */

					/* clean paragraph action table */
static int (*(paraact[128]))() = {
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL
};

					/* clean user defined action table */
static char * (*(paraact1[128]))() = {
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   
    NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL
};

/*
 * global state variables
 */

static int eof=FALSE;
static int myabort = FALSE;
static csb savecsb;
static int *cmdbuf=NULL;
static int set=FALSE;			/* first call to parse, or an */
static jmp_buf ojmp;

					/* handler structure */
ftspec ft_para = { paraparse, parahelp, paracomplete, 0, &parabrk };

static int (*oerrh)();

parareset(code) int code; {
  bcopy(ojmp, cmerjb, sizeof (jmp_buf)); /* put back old error handler */
  null_actions();			/* restore action block */
  cmact(oldact);			/* put back the original action table*/
  set = FALSE;				/* uninitialized...for next time */
  if (cmcsb._cmwbp)
      free(cmcsb._cmwbp);
  if (cmcsb._cmbfp)
      free(cmcsb._cmwbp);
  cmcsb = savecsb;
  cmcsb._cmerh = oerrh;
  return((*cmcsb._cmerh)(code));
}

/*
 * parse routine.
 * parse until eof or abort get's set.
 * returns buffer which has been typed, or NULL on abort
 */
PASSEDSTATIC int
paraparse(text,textlen,fdbp,parselen,value)
char *text;
int textlen;
fdb *fdbp;
int *parselen;
pval *value;
{
  int len,i;
  brktab *btab;				/* break table to use */
					/* action call? */
  static int flg;
  static char *buf;

  if (cmcsb._cmflg2 & CM_EOF) {
      cmcsb._cmflg2 &= ~CM_EOF;
      eof = TRUE;
  }

  if (!set) {				/* first time through, set things up */
    bcopy(cmerjb, ojmp, sizeof (jmp_buf));
    
    savecsb = cmcsb;			/* save stae for later */
    oerrh = cmcsb._cmerh;
    if (cmdbuf != NULL)			/* get rid of old buffer */
      free(cmdbuf);
    if ((cmdbuf = (int *)calloc(1,BSIZ*sizeof(*cmdbuf))) == NULL)
      return(PARAxNM);
    if ((cmcsb._cmwbp = 
	 (char *)calloc(1,BSIZ*sizeof(*cmcsb._cmwbp))) == NULL) {
      free(cmdbuf);
      cmdbuf = NULL;
      return(PARAxNM);
    }
    cmcsb._cmwbc = BSIZ;		/* set up. */
    cmcsb._cmcnt = BSIZ;
    cmcsb._cmptr = cmdbuf;
    cmcsb._cmbfp = cmdbuf;
    cmcsb._cmcur = cmdbuf;
    cmcsb._cminc = 0;
    cmcsb._cmflg &= ~CM_NAC;
    cmcsb._cmerh = parareset;

    if (fdbp->_cmdat != NULL) {		/* insert specified starting text */
      para_data *dat = (para_data *) fdbp->_cmdat;
      if (dat->buf != NULL) {
	int len = strlen(dat->buf);
	if (len >= BSIZ) {		/* do we have enough space? */
					/* no!  get more */
	  if ((cmdbuf=(int *)cmrealloc(cmdbuf,(len+BSIZ)*sizeof(int)))==NULL){
	    return(PARAxNM);
	  }
	  cmcsb._cmcur = cmcsb._cmptr = cmdbuf + (cmcsb._cmptr - cmcsb._cmbfp);
	  cmcsb._cmbfp = cmdbuf;
	  cmcsb._cmcnt = len + BSIZ;

	  if ((cmcsb._cmwbp = 
	       (char *)cmrealloc(cmcsb._cmwbp, 
				 sizeof(*cmcsb._cmwbp)*(len+BSIZ))) == NULL){
	    return(PARAxNM);
	  }
	  cmcsb._cmwbc = len + BSIZ;
	}
	cmsti(dat->buf,CC_NEC|CC_HID|CC_QUO); /* insert text, no echoing */
	for (i = 0; i < cmcsb._cminc; i++) { /* turn off the flags */
	  cmcsb._cmptr[i] &= ~(CC_HID|CC_NEC|CC_QUO);
	}
      }
      null_actions();			/* zero out the action table */
      if (cmcsb._cmflg & CM_ITTY) {
	if (dat->actions != NULL) {	/* install user/default actions */
	  if (fdbp->_cmffl & PARA_DEF)
	    install_actions(def_actions);
	  install_actions(dat->actions);
	}
	else {
	  install_actions(def_actions);
	}
      }
      else {
	install_nl_action();
      }
    }
    oldact = cmcsb._cmact;		/* save old action table */
    cmact(paraact);			/* now use our own action characters */
    eof = FALSE;			/* not done yet */
    myabort = FALSE;			/* not aborting */
    set = TRUE;				/* but we are initialized */
  }

  if (cmcsb._cmcnt < MINSIZE) {		/* running out of buffer space? */
					/* then get more space */
    if ((cmdbuf=(int *)cmrealloc(cmdbuf,(cmcsb._cmwbc + BSIZ)
				 *sizeof(int)))==NULL){
      return(PARAxNM);
    }
    cmcsb._cmptr = cmdbuf + (cmcsb._cmptr - cmcsb._cmbfp);
    cmcsb._cmcur = cmdbuf + (cmcsb._cmcur - cmcsb._cmbfp);
    cmcsb._cmbfp = cmdbuf;		/* and install it into the CSB */
    
    cmcsb._cmcnt += BSIZ;		/* count it */
    
    if ((cmcsb._cmwbp = (char *)cmrealloc(cmcsb._cmwbp,cmcsb._cmwbc+BSIZ))
	== NULL) {
      return(PARAxNM);
    }
    cmcsb._cmwbc += BSIZ;
  }

  if (eof) {				/* done! */
    if (buf != NULL)			/* free previous buffer */
      free(buf);
    
    if ((buf = (char *)malloc(textlen+1)) == NULL) { /* get space */
      return(PARAxNM);
    }
    strncpy(buf,text,textlen);		/* copy text into buffer */
    buf[textlen] = '\0';		/* NULL terminate it */
    for (i = 0; i < textlen; i++)
      buf[i] &= 0x7f;			/* strip those quota characters out. */
    value->_pvpara = buf;		/* point return value at it */
    *parselen = textlen;		/* set the length properly */
    null_actions();			/* restore action block */
    bcopy(cmerjb, ojmp, sizeof (jmp_buf)); /* put back old error handler */
    cmact(oldact);			/* put back the original action table*/
    set = FALSE;			/* initialized...for next time */
    if (cmcsb._cmcol != 0)
	cmxnl();
    free(cmcsb._cmwbp);
    free(cmcsb._cmbfp);
    cmdbuf = NULL;			/* cmdbuf = cmcsb._cmbfp */
    cmcsb = savecsb;			/* restore ccmd's buffers */
    return(CMxOK);			/* return done */
  }
  else
    if (myabort) {
      bcopy(ojmp, cmerjb, sizeof (jmp_buf)); /* put back old error handler */
      null_actions();			/* restore action block */
      cmact(oldact);			/* put back the original action table*/
      set = FALSE;			/* uninitialized...for next time */
      value->_pvpara = NULL;
      *parselen = textlen;
      if (cmcsb._cmcol != 0)
	  cmxnl();
      free(cmcsb._cmwbp);		/* restore ccmd's buffers */
      cmcsb = savecsb;
      return(CMxOK);
    }
  return(CMxINC);			/* nothing here...return incomplete */
}

int break_hndlr();			/* forward declaration */

/*
 * scan through para_action table, and install actions in 
 * the real actions tables.
 * terminates in NULL function.
 */
install_actions(actions) para_actions *actions; {
  if (!cmcsb._cmflg & CM_TTY)
    return;
  for(; !(actions->actionfunc==NULL && actions->actionchar==NULL);actions++) {
    paraact[actions->actionchar] = break_hndlr;
    paraact1[actions->actionchar] = actions->actionfunc;
  }
}

/*
 * zero out the action tables
 */
null_actions() {
    bzero(paraact, sizeof(paraact));
    bzero(paraact1, sizeof(paraact));
}

/*
 * parse completion.
 * does nothing.
 */
PASSEDSTATIC int
paracomplete(text,textlen,fdbp,full,cplt,cpltlen)
char *text,**cplt;
int textlen,full,*cpltlen;
fdb *fdbp;
{
  *cplt = NULL;
  return(CMP_BEL);		/* beep at them */
}

/*
 * helpless
 */

PASSEDSTATIC int
parahelp(text,textlen,fdbp,cust,lines)
char *text;
int textlen,cust;
fdb *fdbp;
int lines;
{
  return(lines);
}

/*
 * Action routines
 */

/*
 * eof action.
 */
static char *
eofact(text,modified,ret) char *text; int *modified,*ret; {
  *ret = TRUE;
  *modified = FALSE;
  return(text);
}


/*
 * redisplay the last screenful of text
 */

static char *
redispact(text,modified,ret) char *text; int *modified, *ret;{
  int i, li, co, cols, ov=FALSE;
  char c;

  *ret = FALSE;
  *modified = FALSE;
  cmcls();
  li = cmcsb._cmrmx;			/* get number of lines */
  co = cmcsb._cmcmx;			/* and number of columns */

  for (i = strlen(text)-1, cols = 0; i >= 0; i--) { /* figure out what */
    c = text[i] & 0x7f;			/*   we can display */
    if (c == '\t')
	cols = ((cols + 8) / 8) * 8;
    else 
	cols++;				/* incr column count */
    if (iscntrl(c) && !isspace(c))	/* control char takes two chars */
      cols++;				/*   to display, count the ^ */
    if (cols > co) {		
      --li;				/* we overflowed the line */
      cols = 0;				/* reset column count */
      ov = TRUE;			/* remember */
    }
    if (c == '\n') {			/* another line */
      cols = 0;				/* reset the column count */
      li--;
      ov = FALSE;
    }
    if (li == 0)			/* can't display any more lines */
      break;
  }
  if (ov && li == 0) {			/* top line doesn't fit on display */
    int p;
    p = i;
    while ((c = text[p] & 0x7f) != '\n' &&  p > 0) /* find beginning of line */
      p--;
    p += ((i - p)/co + 1) * co;
    i = p-1;				/* skip over the \n */
  }
    
  cmcsb._cmcol = 0;
  for(i++; i < cmcsb._cminc; i++) {	/* display the screenful */
    c = text[i] & 0x7f;
    if (iscntrl(c) && !isspace(c)) {
      cmxputc('^');			/* display control chars as */
      cmxputc(c | 0100);		/*   caret and uppercased */
    }
    else cmechx(c);
  }
  return(text);
}

/* 
 * insact
 * insert a file at the end of the buffer
 */
static char *
insact(text, modified, ret) char *text; int *modified, *ret; {
					/* filename fdb */
  static fdb filfdb = { _CMFIL ,FIL_TYPE|FIL_NODIR, NULL, NULL, NULL, NULL,
			    NULL };
					/* confirm fdb */
  static fdb filfdb1 = { _CMCFM, CM_SDH, &filfdb, NULL, "Confirm to cancel",
			 NULL, NULL };
  static fdb cfmfdb = { _CMCFM, 0, NULL, NULL, NULL, NULL, NULL };
  char c;
  char *fname= NULL;
  pval parseval;
  fdb *used;
  int fd;
  struct stat buf;
  char *ntext = NULL;
  int len;
  static int cmdbuf[200];
  static char atmbuf[200],wrkbuf[200];

  cmbufs(cmdbuf,200,atmbuf,200,wrkbuf,200);
  cmseter();				/* errors come back to here */
  prompt("Insert file: ");		/* prmpt for a filename */
  cmsetrp();				/* reparses come back here. */
  parse(&filfdb1, &parseval, &used);	/* parse the filename */
  if (used == &filfdb1) {
    cmxputs("...No file inserted\n"); 
    *modified = FALSE;
    return(text);
  }
  if (fname != NULL)			/* free up old filename if */
    free(fname);			/* necessary */
  if ((fname = (char *)malloc(strlen(parseval._pvfil[0])+1)) == NULL) {
    cmxputs("?Out of memory\n");
    return(text);
  }
  strcpy(fname,parseval._pvfil[0]);	/* copy the data */
  parse(&cfmfdb,&parseval,&used);	/* get confirmation */

  fd = open(fname,O_RDONLY,0);		/* open up the file */
  if (fd == -1) {			/* couldn't open it... */
    cmxputs("?permission denied\n");	/* complain */
    *modified = FALSE;			/* no modification done */
    return(text);
  }
  stat(fname,&buf);			/* get length of file */
  len = strlen(text) + buf.st_size;
  if ((ntext = (char *)malloc(len+1)) == NULL) { /* make space */
    cmxputs("?Out of memory\n"); 
    return(text);
  }
  strcpy(ntext,text);			/* copy old text */
					/* and add in the file */  
  len = strlen(text) + read(fd,&ntext[strlen(text)],buf.st_size);
  close(fd);				/* close the file */
  cmxputs("[OK]\n"); 
  ntext[len] = '\0';			/* null terminate */
  *modified = TRUE;			/* we changed things */
  *ret = FALSE;
  return(ntext);			/* return new text */
}

/*
 * display the whole buffer
 */
static char *
dispact(text, modified, ret) char *text; int *modified, *ret; {
  if (cmcsb._cmcol != 0)
    cmxnl();
  cmxputs(text); 
  return(text);
}

/*
 * invoke the editor on the buffer
 */
static char *
editact(text, modified, ret) char *text; int *modified, *ret; {
  char fname[40];
  char *e,*getenv();
  int fd;
  char buf[100];
  static char *ntext=NULL;
  struct stat sbuf;
  int len,i;

  cmxputs("Invoking editor...\n");	/* announce our intentions */
  e = getenv("EDITOR");			/* check editor variable */
  if (e == NULL || *e == '\0')		/* no editor variable? */
    e = DEF_EDITOR;			/* use default editor */
  
#if unix
    strcpy(fname,"/tmp/cmdXXXXXX");	/* get a file name */
    mktemp(fname);
#else
  strcpy(fname,mktemp(getenv("TMP"),"cmd")); /* get a file name */  
#endif
  fd = open(fname,O_WRONLY|O_CREAT,0700); /* open the tmp file */
  if (fd < 0) {
    cmxputs("?Could not create temporary file.\n"); 
    perror(fname);
    return(text);
  }
  if (write(fd,text,strlen(text)) != strlen(text)) {
    cmxputs("?Could not write temporary file.\n"); 
    perror(fname);
    close(fd);
    unlink(fname);
    return(text);
  }
  close(fd);
  strcpy(buf,e);
#ifndef MSDOS
  strcat(buf," +1000000");
#endif
  strcat(buf," ");
  strcat(buf,fname);
  cmtend();				/* put the tty back */
  if (system(buf) != 0) {		/* execute it */
    cmtset();
    cmxputs("?Edit failed.\n");		/* failure? */
    perror(fname);
    unlink(fname);			/* throw up hands in disgust */
    return(text);
  }
  cmtset();
  if (stat(fname,&sbuf) == -1) {	/* get length of file */
    cmxputs("?Temporary file disappeared\n"); 
    return(text);
  }
  len = sbuf.st_size;
  fd = open(fname,O_RDONLY,0);		/* open the file again */
  if (fd < 0) {
    cmxputs("?Could not open temporary file for read.\n"); 
    perror(fname);
    unlink(fname);
    return(text);
  }
  if (ntext != NULL)			/* free up last buffer */
    free(ntext);
  if ((ntext = (char *)malloc(len+1)) == NULL) { /* make space */
    cmxputs("?Out of memory\n"); 
    return(text);
  }
#ifdef MSDOS
  if ((i = read(fd,ntext,len)) < 0)  /* read in the file */
#else
  if ((i = read(fd,ntext,len)) != len) /* read in the file */
#endif
  {
    cmxputs("?Could not read temporary file.\n"); 
    perror(fname);
    unlink(fname);
    return(text);
  }
  close(fd);				/* close the file */
  cmcls();
  ntext[len] = '\0';			/* null terminate */
  *modified = TRUE;
  unlink(fname);
  cmxputs("...Back again\n"); 
  return(ntext);
}

/*
 * abort the current buffer
 */
static char *
abortact(text, modified, ret) char *text; int *modified, *ret; {
  static fdb cfmfdb = { _CMCFM, CM_SDH, NULL, NULL, "Carriage Return to abort",
			NULL, NULL };
  char cmdbuf[200],atmbuf[200],wrkbuf[200];
  pval parseval;
  fdb *used;

  cmcsb._cmerr = CMxOK;
  cmbufs(cmdbuf,200,atmbuf,200,wrkbuf,200);
  cmseter();				/* errors come back to here */
  if (cmcsb._cmerr == CFMxNOC) {
    cmcsb._cmflg &= ~CM_ACT;		/* no pending actions now */
    return(text);
  }
  prompt("[Confirm to Abort]");		/* prompt for a confirm */
  cmsetrp();				/* reparses come back here. */
  parse(&cfmfdb, &parseval, &used);	/* parse the confirm */
  myabort = TRUE;
  return(text);
}

/*
 * just make us wake up on eol, to check on buffer space...
 */

static char *
eolact(text, modified, ret) char *text; int *modified, *ret; {
  static char *ntext=NULL;
  
  cmechx('\n');				/* do it again */
  *modified = TRUE;			/* we modified it */
  *ret = FALSE;				/* don't return yet */
  strcat(text,"\n");
  return(ntext);
}  


static char *
executeact(text, modified, ret) 
char *text; 
int *modified, *ret; 
{
  char fname[40];
  char *getenv();
  char *output;
  char *cmd;
  struct stat sbuf;
  int fd;
  int len;
  int total;

  if ((cmd = parse_cmdline("Command: ")) == NULL) {
    *modified = FALSE;
    return (text);
  }

#if unix
  strcpy(fname,"/tmp/cmdXXXXXX");	/* get a file name */
  mktemp(fname);
#else
  strcpy(fname,mktemp(getenv("TMP"),"cmd")); /* get a file name */  
#endif

  cmd = (char *) cmrealloc (cmd, strlen(cmd)+strlen(fname)+strlen(" > ")+1);
  strcat (cmd, " > ");
  strcat (cmd, fname);
  
  cmsystem (cmd);
  free (cmd);

  if (stat(fname, &sbuf) == -1) {
    cmxputs ("?Temporary file missing\n");
    return (text);
  }
  len = sbuf.st_size;
  total = strlen(text)+len;
  if ((output = (char *) malloc (total+1)) == NULL) {
    cmxputs ("?Out of memory\n");
    *modified = FALSE;
    return (text);
  }
  strcpy (output, text);
  if ((fd = open(fname, O_RDONLY,0)) < 0) {
    cmxputs("?Could not open temporary file for read.\n"); 
    perror(fname);
    unlink(fname);
    free (output);
    return(text);
  }
#ifdef MSDOS
  if (read(fd,output+strlen(text),len) < 0)
#else
  if (read(fd,output+strlen(text),len) != len)
#endif
  {
    cmxputs("?Could not read temporary file.\n"); 
    perror(fname);
    unlink(fname);
    free (output);
    return(text);
  }
  close(fd);				/* close the file */
  output[total] = '\0';
  unlink (fname);
  *modified = TRUE;
  *ret = FALSE;
  cmxputs("[Done]\n");
  return (output);
}


static brktab fieldbrk = {
  {					/* print chars except ?, space, tab */
    0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
  },
  {					/* print chars except ?, space, tab */
    0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
  }
};
static filblk efilblk = { NULL, NULL, NULL };
static fdb efilefdb = { _CMFIL, FIL_OLD|FIL_EXEC|CM_SDH, NULL, 
			  (pdat) &efilblk, NULL, NULL, NULL };
static fdb filefdb = { _CMFIL, CM_SDH, NULL, 
			 (pdat) NULL, NULL, NULL, NULL };
static fdb fieldfdb = { _CMFLD, CM_SDH, NULL, NULL, NULL, NULL, &fieldbrk };
static fdb conffdb = { _CMCFM, CM_SDH, NULL, NULL, 
			 "command line", NULL, NULL };
static char **path = NULL;

static char *
parse_cmdline (p) char *p; {
  char *cmd;
  char *cmdend;
  pval parseval;
  fdb *used;
  static int cmdbuf[200];
  static char atmbuf[200],wrkbuf[200];

  cmbufs(cmdbuf,200,atmbuf,200,wrkbuf,200);

  path = get_path();
  efilblk.pathv = path;

  cmseter();
  prompt (p);
  cmsetrp();
  parse (fdbchn(&conffdb, &efilefdb, &filefdb, &fieldfdb, NULL), 
	 &parseval, &used);

  if (used == &conffdb) {
    cmxputs ("Aborting\n");
    return (NULL);
  }

  if (used == &efilefdb || used == &filefdb) {
    cmd = (char *) malloc (strlen (parseval._pvfil[0]) + 1);
    strcpy (cmd, parseval._pvfil[0]);
  }
  else if (used == &fieldfdb) {
    cmd = (char *) malloc (strlen (parseval._pvstr) + 1);
    strcpy (cmd, parseval._pvstr);
  }
  
  if (cmdend = parse_cmdline1()) {
      cmd = (char *) realloc (cmd, strlen (cmd)+strlen(cmdend)+2);
      strcat (cmd, " ");
      strcat (cmd, cmdend);
      free (cmdend);
  }
  return (cmd);
}

static char *
parse_cmdline1 () {
  char *cmd;
  char *cmdend;
  pval parseval;
  fdb *used;
  
  parse (fdbchn(&conffdb, &filefdb, &fieldfdb, NULL), 
	 &parseval, &used);

  if (used == &conffdb)
    return (NULL);

  if (used == &filefdb) {
    cmd = (char *) malloc (strlen (parseval._pvfil[0]) + 1);
    strcpy (cmd, parseval._pvfil[0]);
  }
  else if (used == &fieldfdb) {
    cmd = (char *) malloc (strlen (parseval._pvstr) + 1);
    strcpy (cmd, parseval._pvstr);
  }
  
  if (cmdend = parse_cmdline1()) {
      cmd = (char *) realloc (cmd, strlen (cmd)+strlen(cmdend)+2);
      strcat (cmd, " ");
      strcat (cmd, cmdend);
      free (cmdend);
  }
  return (cmd);
}


static char **
get_path() {
  char **path = NULL;
  int num = 0;
  char *PATH, *cp, *getenv();
  char *p, *index();

  if ((PATH = getenv ("PATH")) == NULL)
    return (NULL);
  else {
    cp = (char *) malloc (strlen(PATH)+1); /* space for private copy */
    strcpy (cp, PATH);
  }

  while ((p = index (cp, ':')) != NULL) { /* while we have dirs */
    *p = '\0';				/* tie of with a NULL */
    if (strlen(cp) != 0) {
      if (num == 0) 			/* first path */
	path = (char **) malloc (sizeof (char *)); /* space for 1 */
      else				/* need more space */
	path = (char **) realloc (path, (num+1)*sizeof(char *));
      path[num] = (char *) malloc (strlen(cp)+1); /* space for path */
      strcpy (path[num], cp);		/* copy path into array */
      num++;				/* increment count */
    }
    cp = ++p;				/* move on */
  }
  /* the last path element */
  path = (char **) realloc (path, (num+2)*sizeof(char *));
  path[num] = (char *) malloc (strlen(cp)+1);
  strcpy (path[num], cp);
  num++;
  path[num] = NULL;			/* tie off with null path */
  return (path);
}

static char *
autowrapspace (text, modified, ret)
char *text;
int *modified, *ret;
{
  return (autowrapact (text, modified, ret, ' '));
}

static char *
autowrapnl (text, modified, ret)
char *text;
int *modified, *ret;
{
  return (autowrapact (text, modified, ret, '\n'));
}


static char *
autowraptab (text, modified, ret)
char *text;
int *modified, *ret;
{
  return (autowrapact (text, modified, ret, '\t'));
}

static char *
autowrapact(text, modified, ret, c)
char *text;
int *modified, *ret;
char c;
{
    int i,j;
    extern csb cmcsb;
    int wordbeg, spacebeg;
    int maxcol;

    maxcol = cmcsb._cmwrp >= 0 ? cmcsb._cmwrp : cmcsb._cmcmx + cmcsb._cmwrp;
    if (cmcsb._cmwrp != 0 && lastlinelen(text) >= maxcol) {
	for(i = strlen(text)-1; i >= 0; i--) {
	    if (text[i] == ' ' || text[i] == '\t') {
		wordbeg = i+1;
		i--;
#ifdef undef
		while((text[i] == ' ' || text[i] == '\t') && i >= 0) i--;
#endif
		spacebeg = i+1;
		break;
	    }
	    else {
		if (text[i] == '\n' || i == 0 ||
		    strlen(text) - i >= maxcol) {
		    strcat(text,"\n");
		    cmechx('\n');
		    *ret = FALSE;
		    *modified = TRUE;
		    return(text);
		}
	    }
	}
	if (cmcsb._cmcol != strlen(text) - wordbeg) {
	    delcurrent(strlen(text) - wordbeg);
	    cmechx('\n');
	    cmxputs(&text[wordbeg]);
	}
	cmechx(c);
	text[spacebeg] = '\n';
	text[strlen(text)+1] = '\0';
	text[strlen(text)] = c;
	*ret = FALSE;
	*modified = TRUE;
	return(text);
    }
    text[strlen(text)+1] = '\0';
    text[strlen(text)] = c;
    cmechx(c);
    *ret = FALSE;
    *modified = TRUE;
    return(text);
}

static int
lastlinelen(str) 
char *str;
{
    register int i,j=strlen(str);
    for(i = j - 1; i >= 0; i--)
	if (str[i] == '\n')
	    return(tablen(str + i + 1));
    return(tablen(str));
}

/* 
 * calculate length of a line, even if there are tabs in it.
 */

static int
tablen(str) 
char *str;
{
    register char *cp = str;
    register int len=0;
    while(*cp) {
	if (*cp == '\t') {
	    len = ((len + 8) / 8) * 8;
	}
	else
	    len++;
	cp++;
    }
    return(len);
}
	    

static char *
filteract (text, modified, ret) 
char *text; 
int *modified, *ret; 
{
  char fname1[40];
  char fname2[40];
  char *getenv();
  char *output;
  char *cmd, *parse_cmdline();
  struct stat sbuf;
  int fd;
  int len;
  int total;
  FILE *pp;

  if ((cmd = parse_cmdline("Filter: ")) == NULL) {
    *modified = FALSE;
    return (text);
  }

#if unix
  strcpy(fname1,"/tmp/cmd1XXXXXX");	/* get a file name */
  mktemp(fname1);
  strcpy(fname2,"/tmp/cmd2XXXXXX");	/* get a file name */
  mktemp(fname2);
#else
  strcpy(fname1,mktemp(getenv("TMP"),"cmd1")); /* get a file name */  
  strcpy(fname2,mktemp(getenv("TMP"),"cmd2")); /* get a file name */  
#endif

  cmd = (char *) realloc (cmd, strlen(cmd)+strlen(fname1)+strlen(" < ")+
			  strlen(" > ") + strlen(fname2) +1);
  strcat (cmd, " < ");
  strcat (cmd, fname1);
  strcat (cmd, " > ");
  strcat (cmd, fname2);
  if ((pp = fopen (fname1, "w")) == NULL) {
    cmxputs ("Could not open temp file to execute command\n");
    *modified = FALSE;
    free (cmd);
    return (text);
  }
  fprintf (pp, "%s", text);
  fclose (pp);
  system(cmd);
  free (cmd);
  unlink(fname1);

  if (stat(fname2, &sbuf) == -1) {
    cmxputs ("?Temporary file missing\n");
    return (text);
  }
  len = sbuf.st_size;
  if ((output = (char *) malloc (len+1)) == NULL) {
    cmxputs ("?Out of memory\n");
    *modified = FALSE;
    unlink(fname2);
    return (text);
  }
  if ((fd = open(fname2, O_RDONLY,0)) < 0) {
    cmxputs("?Could not open temporary file for read.\n"); 
    perror(fname2);
    unlink(fname2);
    free (output);
    return(text);
  }
#ifdef MSDOS
  if (read(fd,output,len) < 0)
#else
  if (read(fd,output,len) != len)
#endif
  {
    cmxputs("?Could not read temporary file.\n"); 
    perror(fname2);
    unlink(fname2);
    free (output);
    return(text);
  }
  close(fd);				/* close the file */
  output[len] = '\0';
  unlink (fname2);
  *modified = TRUE;
  *ret = FALSE;
  cmxputs("[Done]\n");
  return (output);
}


   
#ifdef MSDOS
static char *
mktemp(dir,prefix) char *dir, *prefix; {
  static int inited=FALSE;
  int x;
  char name[100];
  struct stat sbuf;
  if (!inited++)
    srand(1);
  x = rand();
  if (dir == NULL)
    dir = "./";
  strcpy(name,dir);
  strcat(name,"\\");
  strcat(name,prefix);
  strcat(name,atoi(x));
  if (stat(name,&sbuf) == 0)
    return(mktemp(dir,prefix));
  return(name);
}
#endif /*  MSDOS */

cm_para_abort() {
  myabort = TRUE;
}

cm_para_eof() {
  eof = TRUE;
}


/*
 * delete to beginning of line.   mostly taken from begact() in stdact.c
 */
static char *
begact(text,modified,ret) char *text; int *modified, *ret; {
  char *cp;				/* pointer to deletion site */
  int cc;				/* character under examination */
  char c;
  int eralen;				/* # of chars erased */

  *modified = FALSE;
  *ret = FALSE;
  if (strlen(text) == 0) return(text);
  cp = text + strlen(text) - 1; 	/* point to end of buffer */
  while (cp-- != text) {		/* search for nonhidden newline */
    if (*cp == NEWLINE)			/* newline? */
      break;				/* yup, stop looking */
  }
  cp++;					/* point to char after break */
  eralen = (text - cp) + strlen(text); /* get # of chars erased */
  if (eralen <= 0) return(text);
  if (cmcsb._cmflg2 & CM_CRT)
    delcurrent(eralen);			/* erase the characters */
  else {
    cmxputs("^U");			/* signal line kill on hardcopy */
    cmxnl();				/* move to a new line */
  }
  *modified = TRUE;
  text[strlen(text)-eralen] = '\0';
  return(text);
}

/*
 * wrdact:
 * delete backwards one word.  If at the beginning of a line, 
 * and previous line is empty, just deletes to beginning of
 * previous line.
 */
static char *
wrdact(text,modified,ret) char *text; int *modified, *ret; {
  char *cp;				/* pointer to deletion site */
  int cc;				/* character under examination */
  char c;
  int eralen;				/* # of chars erased */

  *modified = FALSE;
  *ret = FALSE;
  if (strlen(text) == 0) {
    beep();
    return(text);
  }
  cp = text + strlen(text) - 1; 	/* point to end of buffer */
  while (cp-- != text) {		/* search for a character which is */
    c = *cp;
    if ((c < '0') ||			/* not a letter or digit? */
	 ((c > '9') && (c < 'A')) ||
	 ((c > 'Z') && (c < 'a')) ||
	 (c > 'z')
	)
      break;				/* yup, stop looking */
  }
  cp++;					/* point to char after break */
  eralen = (text - cp) + cmcsb._cminc; /* get # of chars erased */
  if (eralen <= 0) return(text);
  if (cmcsb._cmflg2 & CM_CRT)
    delcurrent(eralen);			/* erase the characters */
  else {
    cmxputs("^W");			/* signal line kill on hardcopy */
  }
  *modified = TRUE;
  text[strlen(text)-eralen] = '\0';	/* modify in place */
  return(text);
}

/* fixact
**
** Purpose:
**   Refresh the display of the current line of text, back to the
**   last unhidden newline character.  If the last character in the
**   buffer is a newline, the previous line is refreshed.
**/

static char *
fixact(text,modified,ret) char *text; int *modified, *ret; {
  char *cp;				/* pointer into buffer */
  int cc;				/* character under examination */
  char c;
  int reflen;				/* # of chars refreshed */

  *ret = FALSE;
  *modified = FALSE;
  if (strlen(text) == 0) return(text);
  cp = text + strlen(text) -1;		/* point to end of buffer */

  while (cp-- != text) {		/* search for nonhidden newline */
    if (*cp == NEWLINE)			/* newline? */
      break;				/* yup, stop looking */
  }
  cp++;					/* point to char after break */

  reflen = (text - cp) + strlen(text); /* get # of chars to refresh */

  if (cmcsb._cmflg2 & CM_CRT)
    delcurrent(reflen);			/* erase the characters from screen */
  else {
    cmxputs("^R");			/* signal line kill on hardcopy */
    cmxnl();				/* move to a new line */
  }
  while (reflen-- > 0) {		/* retype buffer contents */
    c = (cc = *cp++) & CC_CHR;		/* get next char */
    cmechx(c);				/* do it again */
  }
  return(text);
}

/* quoact
**
** Purpose:
**   Enter the next character into the buffer with its quote flag
**   turned on, so it will be treated as a normal character regardless
**   of any handling it would normally receive.
**/
static char *
quoact(text,modified,ret) char *text; int *modified, *ret; {
  int c;				/* quoted character */
  
  cmgetc(&c,cmcsb._cmij);		/* get another character */
  cmechx(c);				/* do it again */
  *modified = TRUE;			/* we modified it */
  *ret = FALSE;				/* don't return yet */
  text[strlen(text)+1] = '\0';  
  text[strlen(text)] = c;   /* | CC_QUO; */
  return(text);
}


/* delact
**
** Purpose:
**   Erase back to and including the last non-hidden character in the
**   command buffer.
**/
static char *
delact(text,modified,ret) char *text; int *modified, *ret; {
  char *cp;				/* pointer to deletion site */
  int cc;				/* character under examination */
  char c;
  int eralen;				/* # of chars erased */

  *modified = FALSE;
  *ret = FALSE;
  if (strlen(text) == 0) {
    beep();
    return(text);
  }
  eralen = 1;
  if (cmcsb._cmflg2 & CM_CRT)
    delcurrent(eralen);			/* erase the characters */
  else {
    cmxputs("\\");			/* signal line kill on hardcopy */
  }
  *modified = TRUE;
  text[strlen(text)-eralen] = '\0';	/* modify in place */
  return(text);
}



/*
 * paragraph action char handler.
 * calls user defined handlers, fixes up text, and returns
 * CMxGO
 * Must perform context switch on ccmd's buffers, and maintain enough
 * buffer space for the current buffer
 */
break_hndlr(fdblist,brk,deferred) 
fdb *fdblist;
char brk;
int deferred;
{
  csb savecsb;
  jmp_buf rpbuf, erbuf;
  int col;
  int len, ret, modified;
  char *text, *ntext;
  int i;

  if (paraact[brk] == NULL) {
    cmsti1(brk,0);
    return(CMxGO);
  }

  if ((text = (char *)malloc(cmcsb._cminc+3)) == NULL) { /* space for buffer */
    return(PARAxNM);			/* and a few added chars */
  }

  for (i = 0; i < cmcsb._cminc; i++) {
    text[i] = cmcsb._cmptr[i] & 0x7f;
    if (text[i] == '\0') text[i] |= 0x80;
  }
  text[cmcsb._cminc] = '\0';

  savecsb = cmcsb;			/* save old state */
  cmcsb._cmrty = "";			/* no prompt here. */
  cmact(oldact);
  bcopy(cmrpjb,rpbuf,sizeof(jmp_buf));
  bcopy(cmerjb,erbuf,sizeof(jmp_buf));

  modified = ret = FALSE;		/* default values */

  if (paraact1[brk] != NULL) {		/* call handler with text */
    ntext = (*paraact1[brk])(text,&modified,&ret);
  }
  else
      ntext = NULL;
  col = cmcsb._cmcol;			/* need to remember where we were on */
					/* the screen.   this is independent */
					/* of the rest of the csb state. */

  cmcsb = savecsb;			/* restore old state */
  cmcsb._cmcol = col;
  bcopy(rpbuf,cmrpjb,sizeof(jmp_buf));
  bcopy(erbuf,cmerjb,sizeof(jmp_buf));
  cmact(paraact);

  if (text != ntext) {			/* new buffer. */
    free(text);				/* get rid of old one */
    text = NULL;
  }

  if (modified) {
    if (strlen(ntext) >= 
	cmcsb._cmcnt + cmcsb._cminc) { /* out of space? */
					/* then get some more space */
      cmcsb._cmcnt = strlen(ntext);
      cmcsb._cminc = 0;
      if ((cmdbuf = 
	(int *)cmrealloc(cmdbuf, (cmcsb._cmcnt + BSIZ) * sizeof(int)))==NULL) {
	return(PARAxNM);
      }
      cmcsb._cmcnt += BSIZ;		/* count it */
      if ((cmcsb._cmwbp = 
	   (char *)cmrealloc(cmcsb._cmwbp,
			     sizeof(*cmcsb._cmwbp)*cmcsb._cmcnt))==NULL){
	return(PARAxNM);
      }
      cmcsb._cmwbc = cmcsb._cmcnt;	/* and install it into the CSB */
      cmcsb._cmptr = cmdbuf;
      cmcsb._cmbfp = cmdbuf;
      cmcsb._cmcur = cmdbuf;
    }
    else {
      cmcsb._cmptr = cmcsb._cmcur = cmcsb._cmbfp;
      cmcsb._cmcnt += cmcsb._cminc;
      cmcsb._cminc = 0;
      cmcsb._cmwbc = cmcsb._cmcnt;
    }
    len = strlen(ntext);
    for (i = 0; i < len; i++)		/* unquote everything */
      ntext[i] &= 0x7f;
    cmsti(ntext,CC_NEC|CC_HID|CC_QUO);		/* insert text into buffer */
    for (i = 0; i < cmcsb._cminc; i++)
      cmcsb._cmptr[i] &= ~(CC_HID|CC_NEC); /* mark it unhidden */
  }
  eof = ret;
  if (ntext)
    free(ntext);
  return(CMxGO);
}

char *
self_ins_nl(text, modified, ret)
char *text;
int *modified, *ret;
{
  strcat(text,"\n");
  cmechx('\n');
  *modified = TRUE;
  return(text);
}

install_nl_action()
{
  paraact['\n'] = break_hndlr;
  paraact1['\n'] = self_ins_nl;
}

