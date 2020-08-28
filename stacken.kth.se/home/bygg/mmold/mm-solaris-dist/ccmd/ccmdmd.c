/*
 Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 the City of New York.  Permission is granted to any individual or
 institution to use, copy, or redistribute this software so long as it
 is not sold for profit, provided this copyright notice is retained.

 Author: Andrew Lowry
*/

/* Machine dependent code for Unix systems -- Use preprocessor
** conditionals for peculiarities of particular systems, but
** PLEASE -- Avoid nesting preprocessor conditionals!
**/

#include "ccmdlib.h"
#include "cmfncs.h"			/* and internal symbols */

extern int errno;


/* cmrpjmp
**
** Purpose:
**   Automatic reparse handler, installed via the cmsetrp macro from
**   ccmd.h.  If this handler is installed in a CSB for which a reparse
**   is needed, it will perform a longjmp to restart execution at the
**   point following the installing cmsetrp invocation.  This point
**   should be following the call to cmini that set up parsing for
**   the current command line, and before the comnd call for the first
**   field in the command.
**
** Input arguments: None
** Output arguments: None
** Returns: Nothing
**/

jmp_buf cmrpjb;			/* global jump buffer for autoreparse */

cmrpjmp()
{
  longjmp(cmrpjb,1);		/* do the jump */
  return(CMxNOAR);		/* if it returns, it failed */
}



/* cmerjmp, cmerjnp
**
** Purpose:
**   Automatic parse error handler, much like the automatic reparse
**   handler described above.  The cmseter macro should be invoked
**   just prior to issuing a prompt.  When a parsing error
**   subsequently occurs (that is, the parse function is about to
**   return anything but CMxOK), cmperr will be called to print the
**   error, and then execution will jump back to the site of the macro
**   invocation.  When the automatic error handler is installed, the
**   user program can ignore the codes returned by parse, since they
**   will always be CMxOK.  CSB field _cmerr may be examined to see
**   whether the the prior command line was terminated by an error,
**   and if so, which error.
**
**   Note: Reparse situations will be handled by the error handler if
**   no reparse handler has been installed.
**
**   Cmerjnp is the same as cmerjmp, except that the error message is
**   not printed.
**
** Input arguments: None.
** Output arguments: None.
** Returns: Nothing.
**/

jmp_buf cmerjb;				/* global jump buffer */

cmerjmp(ret,str,flags)
int ret;				/* code that triggered the handler */
char *str;
int flags;
{
  if (str) return(cmermsg(str,flags));
  cmperr(ret,flags);				/* issue error message */
  longjmp(cmerjb,1);			/* take the jump */
  return(CMxNOAE);			/* failed */
}

cmermsg(str,flags)
char *str;				/* error msg */
int flags;
{
  cmpemsg(str,flags);			/* issue error message */
  longjmp(cmerjb,1);			/* take the jump */
  return(CMxNOAE);			/* failed */
}

cmerjnp(ret)
int ret;				/* code that triggered the handler */
{
  longjmp(cmerjb,1);			/* take the jump */
  return(CMxNOAE);			/* failed */
}


#if BSD
/*
 * if handling nonblocking I/O, and a EWOULDBLOCK error comes up
 * Then jump to the point set with a cmsetbl() call.
 * The user then could wait for input.
 */

jmp_buf cmbljb;				/* global jump buffer */

cmbljmp(ret)
int ret;				/* code that triggered the handler */
{
  if (errno != EWOULDBLOCK)
      return(ret);
  longjmp(cmbljb,1);			/* take the jump */
  return(CMxNOAE);			/* failed */
}
#endif


/*
** Machine-dependent IO routines...  Generally, a file descriptor is
** supplied as an argument to the calls.
**/

static int autowr;			/* TRUE if automatic wrap at eol */
static int li;				/* lines on screen */
static char tcapent[1024];		/* complete termcap entry */
static char tcarea[100];		/* decoded termcap entries */
static char *nl = "\n";
static char *cr = "\r";
static char *cl,*ce,*up;		/* pointers to decoded entries */
static char *le,*nd,*down;
int outc();				/* output routine for tputs */

char *tgetstr();			/* termlib routine returns string */

/* cmgetc - get a character from the input source.  Return standard return
 * code. 
*/

int cmgetc(c,fd)
int *c;					/* pointer where char is placed */
FILE * fd;				/* input file descriptor */
{
  int cc;				/* value from read */

  if (cmcsb._cmoj != NULL)
    cmflsh(cmcsb._cmoj);		/* flush pending output */
  if (fd == NULL)			/* no file descriptor.  EOF */
    return(CMxEOF);
  if (fd->_cnt == 0 && (cmcsb._cmflg & CM_ITTY))
    cmselect(fileno(fd));
  *c = cc = getc(fd);
  if (cc == EOF)
    return(CMxEOF);			/* end of file */
  else
    return(CMxOK);			/* good read */
}

/* cmputc - Output a single character to the terminal */

cmputc(c,fd)
char c;					/* char to output */
FILE * fd;				/* output filedesc */
{

  if (fd != NULL) {
    putc(c,fd);
    if (c == '\n') 
      cmflsh(fd);
  }
}

/* cmputs - Output null-terminated string to the terminal */

cmputs(s,fd)
char *s;				/* output string */
FILE *fd;				/* output filedesc */
{
  while(*s != '\0')
    cmputc(*s++,fd);
}

/* cmcr - Move to the beginning of the current line */

cmcr(fd)
FILE * fd;				/* output filedesc */
{
  cmputs(cr,fd);			/* use term specific sequence */
}

/* cmnl - Output a newline sequence to the comman stream */

cmnl(fd)
FILE *fd;				/* output filedesc */
{
  cmputs(nl,fd);			/* use term-specific sequence */
}

/* cmflsh - flush output on fd */
cmflsh(fd)
FILE *fd;
{
  if (fd != NULL)
    fflush(fd);
}

/* cmwrap - Make sure that cursor wraps when it is required */

cmwrap(fd)
FILE * fd;				/* output filedesc */
{
  if (!autowr)
    cmnl(fd);				/* newline if not automatic */
}

/* cmcls - Clear the screen.  Current IOJ value in the CSB is used for
** character output.  Only invoked if that IOJ is for a CRT terminal.
** Returns TRUE iff the operation succeeds.
**/

int
cmcls()
{
  if (cl == NULL)
    return(FALSE);			/* no clear screen sequence */
  else {
    tputs(cl,li,outc);			/* output the clear sequence */
    if (cmcsb._cmoj)
	fflush(cmcsb._cmoj);
    return(TRUE);
  }
}

/* cmceol - Clear to end of line.  Current IOJ value in the CSB is
** used for character output.  Only invoked if that IOJ is for a CRT
** terminal.  Returns TRUE iff the operation succeeds.
**/

cmceol()
{
  if (ce == NULL)
    return(FALSE);			/* no ceol sequence */
  else {
    tputs(ce,1,outc);			/* else do the operation */
    return(TRUE);
  }
}

/* cmupl - Moves up on line in the display without changing column
** position.  Should not wrap to bottom of screen or cause destructive
** downward scrolling.  Current IOJ value in the CSB is used for
** character output.  Only invoked if that IOJ is for a CRT terminal.
** Returns TRUE if the operation succeeds.
*/

cmupl()
{
  if (up == NULL)
    return(FALSE);			/* no upline sequence */
  else {
    tputs(up,1,outc);			/* else do the operation */
    return(TRUE);
  }
}

cmdownl()
{
  int oldcrmod;
  if (down == NULL)
    return(FALSE);			/* no upline sequence */
  else {
    if (cmcsb._cmoj) {
	oldcrmod = crmod(FALSE);    
	tputs(down,1,outc);			/* else do the operation */
	if (cmcsb._cmoj)
	    fflush(cmcsb._cmoj);
	crmod(oldcrmod);
	return(TRUE);
    }
  }
}

cmleft()
{
  if (le == NULL)
    return(FALSE);			/* no move left sequence */
  else {
    tputs(le,1,outc);			/* else do the operation */
    cmcsb._cmcol--;
    return(TRUE);
  }
}

cmright()
{
  if (nd == NULL)
    return(FALSE);			/* no move right sequence */
  else {
    tputs(nd,1,outc);			/* else do the operation */
    cmcsb._cmcol++;
    return(TRUE);
  }
}

/* cmcpos - Returns the current column position on the display.
** We just assume the ccmd package is correct, since there's
** no facility in termlib for extracting column position, and
** the price for never knowing the cursor position is too high
** (really ugly user interface due to many blank lines).
**/

int
cmcpos()
{
  return(cmcsb._cmcol);
}

/* cmflush - Flush all pending input on the input source */

cmflush(fd)
FILE *fd;
{
  if (fd != NULL) {
    if (isatty(fileno(fd)))		/* if it's a terminal */
#if defined(SYSV)
#define TIOCFLUSH TCFLSH
#endif
      ioctl(fileno(fd),TIOCFLUSH,NULL);	/* flush input and output */
    fd->_cnt = 0;			/* flush stdio IOBUF. */
  }
}

/* cmtset - Initialize the source terminal, as currently set in the
** CSB.  If the file is a terminal, a termcap entry is obtained and
** examined to see whether or not it is a hardcopy terminal.  If not,
** various control strings are read from the termcap entry and saved
** for screen operations.  In any case, terminals are placed in cbreak
** mode without echoing, and INT and TSTP signals are caught to
** prevent the terminal remaining in a funny state upon exit, and to
** place it back into the required state upon continuation.
**/

cmtset()
{
  int ofd, ifd;
  char *areap = tcarea;			/* pointer to termcap decoding area */
  char *ttype;				/* terminal type */
  char *gttype();
  int tret;
 
  if (cmcsb._cmoj != NULL)
    ofd = fileno(cmcsb._cmoj);		/* input file descriptor */
  if (cmcsb._cmij != NULL)
    ifd = fileno(cmcsb._cmij);		/* output file designator */

  cmcsb._cmflg &= ~CM_TTY;
  if (cmcsb._cmoj != NULL && isatty(ofd)) { /* check if it is a terminal */
    cmcsb._cmflg |= CM_OTTY;		/* yup */
    ttype = gttype(ofd);		/* get the terminal type name */
    if (tcapent[0] == '\0')
      tret = tgetent(tcapent,ttype);	/* get termcap entry */
    else
      tret = 1;
    if (tret != 1) {
      cmcsb._cmflg2 &= ~CM_CRT;	/* no luck... assume hardcopy */
      cmcsb._cmcmx = 79;		/* use default max column */
      cmcsb._cmwrp = 79;		/* and wrap column */
      nl = "\n";			/* and set default newline */
      cr = "\r";			/* and return sequences */
    }
    else if (cmcsb._cmoj != NULL) {
      if (tgetflag("hc")) 		/* hardcopy indicated? */
	cmcsb._cmflg2 &= ~CM_CRT;	/* yup, note it */
      else {
	cmcsb._cmflg2 |= CM_CRT;	/* else flag a crt */
      }
      cmtsize();			/* setup term size & termcap vars */
      cmcsb._cmwrp = cmcsb._cmcmx;	/* set up autowrap column */
    }
    if (cmcsb._cmij != NULL && isatty(ifd)) {
      cmcsb._cmflg |= CM_ITTY;		/* yup */
      raw(ifd);				/* set up the terminal properly */
      intson();				/* install our interrupt handlers */
    }
  }
  else {
    if (cmcsb._cmij != NULL && isatty(ifd)) {
      raw(ifd);				/* set up the terminal properly */
      intson();				/* install our interrupt handlers */
    }
    else {
      cmcsb._cmflg &= ~CM_TTY;		/* not a tty, so not a crt either */
      cmcsb._cmflg2 &= ~CM_CRT;
      cmcsb._cmcmx = 79;		/* and just use default width */
    }
  }
}

/* cmtend - Clean up after prior input source */

cmtend()
{
  int fd;

  if (cmcsb._cmij != NULL) {
    fd = fileno(cmcsb._cmij);		/* file desc to shut down */

    if (cmcsb._cmflg & CM_TTY) {
      unraw(fd);			/* reset former tty params */
      intsoff();			/* remove our interrupt handlers */
    }
  }
}

/*
 * cmtsize()
 * set up terminal size.  try using TIOCGWINSZ if possible.  If not set,
 * use termcap entry.  If ioctl() succeeds, fix up termcap entry, so inferior
 * processes inherit correctly.
 */
cmtsize()
{
  int li = -1, co = -1; 
#ifdef TIOCGWINSZ
  struct winsize w;

  if (ioctl(fileno(cmcsb._cmoj), TIOCGWINSZ, &w) == 0) {
    if (w.ws_col > 0 && cmcsb._cmcmx != w.ws_col) {
      co = cmcsb._cmcmx = w.ws_col;
    }
    if (w.ws_row > 0 && cmcsb._cmrmx != w.ws_row) {
      li = cmcsb._cmrmx = w.ws_row;
    }
  }
#else /* TIOCGWINSZ */
#ifdef TIOCGSIZE
  struct ttysize t;

  if (ioctl(fileno(cmcsb._cmoj), TIOCGSIZE, &t) == 0) {
    if (t.ts_col > 0 && cmcsb._cmcmx != t.ts_col) {
      co = cmcsb._cmcmx = t.ts_col;
    }
    if (t.ts_row > 0 && cmcsb._cmrmx != t.ts_row) {
      li = cmcsb._cmrmx = t.ts_row;
    }
  }
#endif /* TIOCGSIZE */
#endif /* !TIOCGWINSZ */
  if (co <= 0)
    cmcsb._cmcmx = tgetnum("co");	/* get col count */
  if (cmcsb._cmcmx <= 0)
    cmcsb._cmcmx = 79;			/* use default if not given */
  else
    cmcsb._cmcmx--;			/* else drop to max col position */
  if (li <= 0)				/*  */
    cmcsb._cmrmx = tgetnum("li");	/* get row count */
  if (cmcsb._cmrmx == -1)
    cmcsb._cmrmx = 24;			/* or use default if necesary */
  li = --cmcsb._cmrmx;
  tc_setsize(cmcsb._cmrmx+1,cmcsb._cmcmx+1);
}

/*
 * set the "li" and "co" entries in the termcap entry
 */
tc_setsize(li, co) {
  char buf[1024], *p1 = NULL, *p2=NULL, *p3=NULL, *p4=NULL, *strindex();
  char *index();

  p1 = strindex(tcapent,"co#");		/* find num cols */
  if (p1) {
    p2 = index(p1+1,':');		/* and the end of it. */
  }
  p3 = strindex(tcapent,"li#");		/* find num lines */
  if (p3) {
    p4 = index(p3+1,':');		/* and the end of it */
  }
  if (p3)
    *p3 = '\0';
  if (p1) 
    *p1 = '\0';
  if (p2) 
    *(p2++) == '\0';
  else p2 = "";
  if (p4) 
    *(p4++) == '\0';
  else
    p4 = "";
					/* build new termcap string */
  sprintf(buf,"%sco#%d:%sli#%d:%s", tcapent, co, p2, li, p4);
  strcpy(tcapent, buf);
  setenv("TERMCAP", tcapent, TRUE);	/* put it in the environment. */
  tc_setents(tcarea);
}  

tc_setents(areap) 
char *areap;
{
  if (cmcsb._cmflg2 & CM_CRT) {
    cl = tgetstr("cl",&areap);		/* get clear screen sequence */
    ce = tgetstr("ce",&areap);		/* and clear end-of-line */
    up = tgetstr("up",&areap);		/* and upline sequence */
    le = tgetstr("le",&areap);		/* move left */
    nd = tgetstr("nd",&areap);		/* move right */
    down = tgetstr("do",&areap);	/* move down */
    if (le == NULL)
        le = "\b";			/* default if not specified */
  }
  nl = tgetstr("nl",&areap);		/* alwasy get newline sequence */
  if (nl == NULL)
    nl = "\n";				/* default if not specified */
  cr = tgetstr("cr",&areap);		/* get return sequence */
  if (cr == NULL)
    cr = "\r";				/* or set default */
  autowr = tgetflag("am");		/* check for autowrap */
  li = cmcsb._cmrmx;
}

/*
 * search for an embedded string
 */
char *
strindex(src, str) 
char *src, *str;
{
  char *cp = src, *cp1;

  while(cp1 = index(cp, *str)) {
    if (strncmp(cp1, str, strlen(str)) == 0) {
      return(cp1);
    }
    cp = cp1+1;
  }
  return(NULL);
}

/* gttype - Return terminal type name for given tty file descriptor 
** Auxiliary routine for cmtset
**/

static char *
gttype(fd)
int fd;
{
  char *type;				/* type name of terminal */
  char *name;				/* terminal name */
  char *cname;				/* controlling terminal name */
  int cttyfd;				/* controlling terminal file desc */
  int ctty;				/* TRUE if fd is controlling tty */
  FILE *typedb;				/* stream for ttytype database */
  char typelin[80];			/* line from ttytype database */
  char *typecp;				/* pointer into type db entry */
  extern char *ttyname(), *getenv();

  cttyfd = open("/dev/tty",O_RDWR,0);	/* open the controlling tty */
  if (cttyfd < 0)
    ctty = FALSE;			/* bad open - assume not ctty */
  else {
    name = ttyname(fd);			/* get the terminal name */
    cname = ttyname(cttyfd);		/* and controlling tty name */
    if (strcmp(name,cname) == 0)		/* same? */
      ctty = TRUE;			/* yup, it is ctrl tty */
    else
      ctty = FALSE;			/* nope, some other tty */
    close(cttyfd);			/* no more use for this */
  }
					/*  */
  if (ctty) {				/* controlling terminal? */
    type = getenv("TERM");		/* yup, use environment var */
    if (type != NULL)
      return(type);			/* give it back if successful */
  }

  name += 5;				/* skip the "/dev/" prefix */
  typedb = fopen("/etc/ttytype","r"); 	/* open type database */
  if (typedb == NULL)
    return("unknown");			/* give up if bad open */
  while (fgets(typelin,80,typedb) != NULL) { /* scan the database */
    typecp = typelin;
    while ((*typecp++) != SPACE);	/* scan for space in entry */
    *(typecp-1) = NULCHAR;		/* change it to null */
    
    if (strcmp(name,typecp) == 0) {	/* this our entry? */
      fclose(typedb);			/* yup, shut database */
      return(typelin);			/* and return type name */
    }
  }
  fclose(typedb);			/* not found... close database */
  return("unknown");			/* and give default */
}

/* auxiliary routines to take terminals into and out of raw mode */

#if defined(BSD) || defined(SOLARIS)
static struct sgttyb ttyblk, ttysav;	/* tty parameter blocks */
static struct ltchars ltc,ltcsav;	/* local special chars for new  */
#endif

#if SYSV
static struct termio ttyblk, ttysav;
#endif

/* raw - put the terminal into raw (actually cbreak) mode, turn off
** echoing and output translations, and extract the output speed for
** the termcap library.  On BSD unix systems, literal-next processing
** is also disabled.
**/

static
raw(fd)
int fd;
{
#if defined(SYSV)
  ioctl(fd, TCGETA, &ttysav);
  ttyblk = ttysav;
  ttyblk.c_lflag &= ~(ICANON|ECHO);
  ttyblk.c_cc[0] = 003;			/* interrupt char is control-c */
  ttyblk.c_cc[4] = 1;
  ttyblk.c_cc[5] = 1;
  ioctl(fd,TCSETAW,&ttyblk);		/* set new modes . */
#endif /* SYSV */

#if BSD || defined(SOLARIS)
  extern short ospeed;			/* declared in termlib */

  ioctl(fd,TIOCGETP,&ttysav);		/* get original parameters */
  ttyblk = ttysav;			/* copy into new parameter block */
  ospeed = ttysav.sg_ospeed;		/* save output speed for termlib */
  ttyblk.sg_flags &= ~(RAW | ECHO | LCASE); /* no echoing or xlates */
  ttyblk.sg_flags |= CBREAK;		/* single character reads */
  ioctl(fd,TIOCSETN,&ttyblk);		/* set params, leave typeahead */
  ioctl(fd,TIOCGLTC,&ltc);		/* get current local special chars */
  ltcsav = ltc;				/* copy it for later restore */
  ltc.t_lnextc = -1;			/* disable literal-next */
  ioctl(fd,TIOCSLTC,&ltc);		/* set the new chars in place */
#endif
}

/* unraw - restore the tty modes in effect before raw was performed. */

static
unraw(fd)
int fd;
{
#if SYSV
  ioctl(fd,TCSETAW, &ttysav);		/* put back saved params */
#endif
#if defined(BSD) || defined(SOLARIS)
  ioctl(fd,TIOCSETN,&ttysav);		/* put back saved params */
  ioctl(fd,TIOCSLTC,&ltcsav);		/* restore local special chars */
#endif
}

/* outc - aux routine to be passed to termlib routines - output one char */

PASSEDSTATIC
outc(c)
char c;
{
  FILE *fd = cmcsb._cmoj;		/* get output filedesc */

  if (fd != NULL)
    putc(c,fd);				/* do the write */
}

/* intson - Install our interrupt handlers for INT and STOP, so
** any terminal settings we have installed will be undone before
** the program exits.  Any handlers that are already installed
** are left in place.  Those handlers should call cmdone if
** they expect to exit with the terminal set correctly.
**/

#ifdef V7				/* used improve calls in version 7 */
#define signal sigsys
#endif
#define mask(sig)   (1 << ((sig)-1))

static
intson()
{
  SIG sighand();			/* forward decl of our handler */
  int oldmask;				/* signal mask before we play */
  sigval oldhand;			/* old handler */

#ifdef SIGTSTP
#ifdef SOLARIS
  sighold(SIGINT);
  sighold(SIGTSTP);
#else
  oldmask = sigblock(mask(SIGINT) | mask(SIGTSTP));	/* hold these */
#endif
#endif
  oldhand = signal(SIGINT,sighand);	/* install our handler, get prior */
  if (oldhand != SIG_DFL)		/* did they have something? */
    signal(SIGINT,oldhand);		/* yup, leave it there */
#ifdef SIGTSTP
  oldhand = signal(SIGTSTP,sighand);	/* install ours for TSTP too */
  if (oldhand != SIG_DFL)
    signal(SIGTSTP,oldhand);		/* but leave theirs intact */
#ifdef SOLARIS
  sigrelse(SIGINT);
  sigrelse(SIGTSTP);
#else
  sigsetmask(oldmask);			/* now unblock the signals */
#endif
#endif /* SIGTSTP */
#ifdef SIGWINCH
  oldhand = signal(SIGWINCH,sighand);	/* install ours for TSTP too */
  if (oldhand != SIG_DFL)
    signal(SIGWINCH,oldhand);		/* but leave theirs intact */
#endif /* SIGWINCH */
}


/* intsoff - Remove our interrupt handlers.  If we remove something
** that isn't ours, put it back.
**/

static
intsoff()
{
  SIG sighand();			/* forward decl of our handler */
  int oldmask;				/* sig mask prior to our diddling */
  sigval oldhand;			/* prior handler for a signal */

#ifdef SIGTSTP
#ifdef SOLARIS
  sighold(SIGINT);
  sighold(SIGTSTP);
#else
  oldmask = sigblock(mask(SIGINT) | mask(SIGTSTP));	/* block our sigs */
#endif
#endif
  oldhand = signal(SIGINT,SIG_DFL);	/* remove INT handler */
  if (oldhand != sighand)
    signal(SIGINT,oldhand);		/* replace if not ours */
#ifdef SIGTSTP
  oldhand = signal(SIGTSTP,SIG_DFL);	/* remove TSTP handler */
  if (oldhand != sighand)
    signal(SIGTSTP,oldhand);		/* replace if not ours */
#ifdef SOLARIS
  sigrelse(SIGINT);
  sigrelse(SIGTSTP);
#else
  sigsetmask(oldmask);			/* replace old signal mask */
#endif
#endif
#ifdef SIGWINCH
  oldhand = signal(SIGWINCH,SIG_DFL);	/* remove TSTP handler */
  if (oldhand != sighand)
    signal(SIGWINCH,oldhand);		/* replace if not ours */
#endif
}


/* sighand - Handler for INT and TSTP signals.  We first fix
** the terminal to its normal settings, then remove our handler
** and generate whichever signal invoked us to get the default
** action.  If the program is continued, the terminal is adjusted
** again, and our handler is reinstalled.  (This should only happen
** with TSTP signals).
**/

static SIG
sighand(sig,code,scp)
int sig,code;				/* sig is all we care about */
struct sigcontext *scp;
{
  int oldmask;				/* prior interrupt mask */
  long getpid();			/* pids are long */

  switch (sig) {
#ifdef SIGWINCH
  case SIGWINCH:
    cmtsize();
    break;
#endif
  default:
    cmtend();				/* fix the terminal */
#ifdef BSD
    oldmask = sigsetmask(0);		/* let our signal get through */
#endif
#ifdef SIGTSTP
    if (sig == SIGTSTP)
      cmnl(stdout);			/* move to new line for looks */
#endif SIGTSTP
    kill(getpid(),sig);			/* get the default action */
    cmtset();				/* redo the terminal if continued */
#ifdef BSD
    sigsetmask(oldmask);		/* set mask back to before */
#endif
  }
}



/* ctty, cttycls - Ctty opens the controlling terminal and returns a
** file descriptor for it.  After the first call, it just returns the
** file descriptor opened previously.  Cttycls closes the file
** descriptor opened by ctty, after which another call to ctty will
** open another one.
**/

static int ttyfd = -1;

static int
ctty()
{
  if (ttyfd == -1)
    ttyfd = open("/dev/tty",O_RDWR,0);
  return(ttyfd);
}

static
cttycls()
{
  if (ttyfd != -1)
    close(ttyfd);
  ttyfd = -1;
}

crmod(val)
int val;
{
#ifdef TIOCSETN
  struct sgttyb ttyblk;
  int oldval;
  int fd;

  if (cmcsb._cmoj == NULL) 
      return(-1);
  fd = fileno(cmcsb._cmoj);

  ioctl(fd,TIOCGETP,&ttyblk);		/* get original parameters */

  oldval = ttyblk.sg_flags & CRMOD;
  if (val)
      ttyblk.sg_flags |= CRMOD;		/* turn on CRMOD */
  else
      ttyblk.sg_flags &= ~CRMOD;	/* turn off CRMOD */
  ioctl(fd,TIOCSETN,&ttyblk);		/* set params, leave typeahead */
  return(oldval);
#else
  return (0);
#endif
}

cmselect(fd)
int fd;
{
#ifdef FD_SETSIZE
  fd_set rfds, efds;
  int r;
  
  while(1) {
    FD_ZERO(&rfds);
    FD_ZERO(&efds);
    FD_SET(fd, &rfds);
    FD_SET(fd, &efds);
    r = select(FD_SETSIZE, &rfds, NULL, &efds, NULL);
    if (r >= 0 || errno != EINTR)
	return;
  }
#endif
}
