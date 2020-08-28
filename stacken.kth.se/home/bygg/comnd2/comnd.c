/* put copyright statement here */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "comnd.h"

static jmp_buf cm_pfbuf;	/* Parse Failed.  Used to abort parsers. */
static jmp_buf cm_rdbuf;	/* -- redo current item. */
jmp_buf cm_erbuf;		/* -- for seterror(); */
jmp_buf cm_rpbuf;		/* -- for setreparse(); */

cmparseval pval;		/* Holds return values etc. */

char* atombuffer = NULL;	/* Points to current atom buffer. */
static int atomsize = 0;	/* Size of allocated atom buffer. */
static int atomleft = 0;	/* Bytes left in atom buffer. */
static int atomptr = 0;		/* Index of next writable byte in A.B. */

#if 0
static char* inputbuffer = NULL; /* Points to current input buffer. */
static int inputsize = 0;	/* Size of allocated input buffer. */
static int inputleft = 0;	/* Items left in input buffer. */
#else
static char inputbuffer[100];
static int inputsize = 100;
static int inputleft = 100;
#endif

static int (*extreader)(void) = NULL; /* External char reader. */

enum {
  act_normal,
  act_complete,
  act_help,
  act_done,
};

static int action;		/* Type of action here. */
static int actpos;		/* Pos of first char of action. */
static int actlen;		/* Length of action. */

static int curraction;		/* Action for curent char. */

static int parsepoint = 0;	/* Next pos to parse from. */
static int inputptr = 0;	/* Next pos to put new data. */
static int inputend = 0;	/* Last pos in input. */

static bool incmd = false;	/* true if tty is inited. */

static bool helpflag;
static bool hlp2flag;
static bool cpeflag;
static bool recogflag;
static bool bolflag;

static int lwpos = 0;		/* Last written pos. on screen. */
static int hpos = 0;		/* Horisontal pos. on screen. */

static char* prompter;

static cmfdb* this;		/* The FDB we are trying now. */

static char* errmsg;

typedef struct dyn_fdb {
  struct dyn_fdb* next;
  cmfdb f;
} dyn_fdb;

typedef struct dyn_keytab {
  struct dyn_keytab* next;
  cmkeytab k;
} dyn_keytab;

/*
** handle dynamic fdb's etc:
*/

static dyn_fdb* gc_fdb = NULL;
static dyn_fdb* free_fdb = NULL;

static dyn_keytab* gc_keytab = NULL;
static dyn_keytab* free_keytab = NULL;

static bool gcflag = false;

/*
** Make atom buffer about 100 chars larger.
*/

static void atomgrow(void)
{
  char* newbuffer;
  int newsize;

  newsize = atomsize + 100;
  newbuffer = realloc(atombuffer, newsize);

  if (newbuffer == NULL) {
    /* bug! figure out a way to handle. */
  }

  atombuffer = newbuffer;
  atomsize = newsize;
  atomleft += 100;
}

/* store char in atom buffer. */

static void atomstore(char c)
{
  while (atomleft <= 0) {
    atomgrow();
  }
  atombuffer[atomptr++] = c;
  atomleft--;
}

/* init atom buffer for collecting atom. */

static void atominit(void)
{
  atomleft = atomsize;
  atomptr = 0;
}

/* terminate atom buffer with a null. */

static void atomdone(void)
{
  atomstore((char) 0);
  atomptr -= 1;
}

/* match a string against the atom buffer */

static bool atommatch(char* str)
{
  if (strncmp(str, atombuffer, atomptr) == 0) {
    return (true);
  }
  return (false);
}

/* Check if input is a break character. */

static bool breakchar(int c, breakset* brk)
{
  int bit, byte;

  if (brk != NULL) {
    if (c < brk->bitlen) {
      bit = c & 7;
      byte = c >> 3;
      if (brk->bitmask[byte] & (0x80 >> bit)) {
	return (true);
      }
    } else if (brk->testfunc != NULL) {
      return ((*brk->testfunc)(c));
    }
  }
  return (false);
}

/*
** reparse the current command.
*/

static void reparse(void)
{
  parsepoint = 0;
  longjmp(cm_rpbuf, 1);
}

/************************************************************************/

/*
** Functions to implement command history:
*/

typedef struct histent {
  struct histent* prev;
  struct histent* next;
  time_t when;
  char* line;
  int len;
} histent;
  
static histent* histptr = NULL;

static histent* currline = NULL;

static histent* history;
static histent* histend;
static int histsize = 0;
static int maxhistory = 5;

histent* histinput(void)
{
  int len;
  histent* h;
  
  len = inputend;
  if (len == 0) {
    return (NULL);
  }
  if (inputbuffer[inputend-1] == '\n') {
    len--;
  }
  h = malloc(sizeof(histent));
  if (h == NULL) {
    return (NULL);
  }
  h->line = malloc(len);
  if (h->line == NULL) {
    free(h);
    return (NULL);
  }

  h->prev = h->next = NULL;

  h->len = len;
  while (len-- > 0) {
    h->line[len] = inputbuffer[len];
  }

  h->when = time(NULL);

  return (h);
}

void freehist(histent* h)
{
  if (h != NULL) {
    free(h->line);
    free(h);
  }
}

void histsave(void)
{
  histent* h;

  histptr = NULL;
  freehist(currline);
  currline = NULL;

  h = histinput();

  if (h == NULL) {
    return;
  }

  if (histsize == 0) {
    history = h;
    histend = h;
    histsize = 1;
    return;
  }

  histsize++;
  history->next = h;
  h->prev = history;
  h->next = NULL;
  history = h;

  if (histsize > maxhistory) {
    histsize--;
    h = histend;
    histend = histend->next;
    histend->prev = NULL;
    freehist(h);
  }
}

/* (temp) debugging routine */

void phist(void)
{
  histent* h;
  int i;
  char* t;

  for (h = histend; h != NULL; h = h->next) {
    t = ctime(&h->when);
    t[19] = 0;
    printf("%s: ", &t[11]);
    putchar('"');
    for (i = 0; i < h->len; i += 1) {
      putchar(h->line[i]);
    }
    putchar('"');
    putchar('\n');
  }
}

/************************************************************************/
/*
** Functions to perform video editing.  Right now we assume an ANSI com-
** patible terminal.
**
**   cursor up = $[A
**   cursor down = linefeed. ($[B)
**   cursor left = backspace. ($[D)
**   cursor right = $[C
**   cursor home = $[H
**   erase to eol = $[K
**   erase to eos = $[J
**   insert char = $[@
**   delete char = $[P
*/

/*
** Output 'n' spaces:
*/

static void spaces(int n)
{
  hpos += n;

  while (n-- > 0) {
    putchar(' ');
  }
}

/*
** Output 'n' backspaces:
*/

static void backspaces(int n)
{
  hpos -= n;

  while (n-- > 0) {
    putchar(8);
  }
}

/*
** clear screen
*/

static void cls(void)
{
  putchar(033);
  putchar('[');
  putchar('H');
  putchar(033);
  putchar('[');
  putchar('J');
}

/*
** erase to end of line
*/

static void ereol(void)
{
  putchar(033);
  putchar('[');
  putchar('K');
}

/*
** beep at user
*/

static void beep(void)
{
  putchar(007);
}

/************************************************************************/

static void lmarg(void)
{
  putchar('\r');
  hpos = 0;
}

static void echo(char c)
{
  hpos += 1;			/* Bluntly assume that all chars are equal. */
  putchar(c);
  if (c != ' ') {
    if (hpos > lwpos) {
      lwpos = hpos;
    }
  }
}

/*
** (re)issue prompter.
*/

static void reprompt(void)
{
  char c;
  char* p;

  p = prompter;
  while ((c = *p++) != (char) 0) {
    echo(c);
  }
}

/*
** reissue current line.
*/

static void retype(void)
{
  int i;

  lwpos = 0;
  reprompt();
  for (i = 0; i < inputend; i += 1) {
    echo(inputbuffer[i]);
  }  
  ereol();
  backspaces(inputend - inputptr);
}

/* get next input char. */

static char xxxchar(void)
{
  if (extreader != NULL) {
    if (fflush(stdout) != 0) {
      printf("fflush failed!\n");
      exit(0);
    }
    return ((*extreader)());
  }
  return (getchar());
}

/* store one char at inputpos, shifting rest of line. */

static void store(char c)
{
  int i;

  if (inputend < inputsize) {
    for (i = inputend; i >= inputptr; i -= 1) {
      inputbuffer[i+1] = inputbuffer[i];
    }
    inputend++;
    inputbuffer[inputptr++] = c;
  }
}

/* delete one char at input pos, shifting rest of line. */

static void delete(void)
{
  int i;

  for (i = inputptr + 1; i < inputend; i += 1) {
    inputbuffer[i-1] = inputbuffer[i];
  }
  inputend--;
}

/* print rest of line, from ptr to end. */

static void prrest(void)
{
  int i;

  for (i = inputptr; i < inputend; i += 1) {
    echo(inputbuffer[i]);
  }
}

/* clear out the input buffer completely. */

static void clearinput(void)
{
  parsepoint = 0;
  inputptr = 0;
  inputend = 0;
}

static bool needreparse = false;

static void loadline(histent* h)
{
  int i;

  if (parsepoint > 0) {
    needreparse = true;
  }

  if (h == NULL) {
    clearinput();
  } else {
    for (i = 0; i < h->len; i += 1) {
      inputbuffer[i] = h->line[i];
    }
    inputptr = inputend = h->len;
    parsepoint = 0;
  }
  
  lmarg();
  retype();
}

static void uphist(void)
{
  if (histptr != NULL) {
    if (histptr->prev != NULL) {
      histptr = histptr->prev;
      loadline(histptr);
      return;
    }
  } else {
    histptr = history;
    if (histptr != NULL) {
      freehist(currline);
      currline = histinput();
      loadline(histptr);
      return;
    }
  }
  beep();
}

static void downhist(void)
{
  if (histptr != NULL) {
    if (histptr->next != NULL) {
      histptr = histptr->next;
      loadline(histptr);
    } else {			/* Back to reality. */
      loadline(currline);
      freehist(currline);
      currline = NULL;
      histptr = NULL;
    }
    return;
  }
  beep();
}

static void hist_end(void)
{
  beep();			/* Missing. */
}

static void hist_begin(void)
{
  beep();			/* Missing. */
}

/*
** This routine fills the input buffer, doing editing, until an action
** character is found.  Currently the action chars are Newline, Tab and "?".
*/

static void setaction(int newaction)
{
  action = newaction;
  actpos = inputptr;
  actlen = 1;			/* For now... */
}

static bool dibflag = false;

static void getmore(void)
{
  unsigned char c;

  if (!incmd) {			/* Make sure we are inited. */
    cm_init();
    incmd = true;
  }

  if (dibflag) {
    dibflag = false;
    if (inputend > inputptr) {
      prrest();
      backspaces(inputend - inputptr);
    }
  }

  for (;;) {
    c = xxxchar();

    if (c == 0177) {		/* Rubout? */
      c = 'H' - 0100;		/* Turn into backspace. */
    }

    if (c == ('A' - 0100)) {	/* C-A? */
      if (inputptr > 0) {
	printf("\r%s", prompter);
	if (parsepoint > 0) {
	  needreparse = true;
	}
	inputptr = 0;
      }
      continue;
    }

    if (c == ('B' - 0100)) {	/* C-B? */
      if (inputptr == 0) {
	beep();
	continue;
      }
      inputptr--;
      backspaces(1);
      continue;
    }

    if (c == ('D' - 0100)) {	/* C-D? */
      if (inputptr >= inputend) {
	beep();
	continue;
      }
      delete();
      if (inputptr < parsepoint) {
	needreparse = true;
      }
      prrest();
      echo(' ');
      backspaces(inputend + 1 - inputptr);
      continue;
    }

    if (c == ('E' - 0100)) {	/* C-E? */
      while (inputptr < inputend) {
	echo(inputbuffer[inputptr++]);
      }
      continue;
    }

    if (c == ('F' - 0100)) {	/* C-F? */
      if (inputptr >= inputend) {
	beep();
	continue;
      }
      echo(inputbuffer[inputptr++]);
      continue;
    }

    if (c == ('H' - 0100)) {
      if (inputptr == 0) {
	beep();
	continue;
      }
      inputptr--;
      backspaces(1);
      delete();
      prrest();
      echo(' ');
      backspaces(inputend + 1 - inputptr);
      if (inputptr < parsepoint) {
	needreparse = true;
      }
      continue;
    }

    if (c == '\t') {		/* Tab? */
      store(c);
      if (inputptr < parsepoint) {
	needreparse = true;
      }
      setaction(act_complete);
      break;
    }

    if (c == '\n') {		/* Newline -- good. */
      inputptr = inputend;
      store(c);
      echo(c);
      hpos = 0;
      setaction(act_done);
      break;
    }

    if (c == ('K' - 0100)) {	/* C-K? */
      if (inputend > inputptr) {
	inputend = inputptr;
	ereol();
      }
      if (inputptr < parsepoint) {
	needreparse = true;
      }
      continue;
    }

    if (c == ('L' - 0100)) {	/* C-L? */
      cls();
      retype();
      continue;
    }

    if (c == ('N' - 0100)) {	/* C-N? */
      downhist();
      continue;
    }

    if (c == ('P' - 0100)) {	/* C-P? */
      uphist();
      continue;
    }

    if (c == ('R' - 0100)) {	/* C-R? */
      lmarg();
      retype();
      continue;
    }

    if (c == ('U' - 0100)) {	/* C-U? */
      if (inputptr > 0) {
	if (parsepoint > 0) {
	  needreparse = true;
	}
	clearinput();
	lmarg();
	reprompt();
	ereol();
	continue;
      }
    }

    if (c == ('V' - 0100)) {	/* C-V */
      /* should store next char */
      beep();
      continue;
    }

    if (c == ('W' - 0100)) {	/* C-W */
      beep();
      continue;
    }

    if (c == 033) {		/* Esc? */
      c = xxxchar();
      switch (c) {
      case '[':
	c = xxxchar();
	switch (c) {
	case 'A': /* up */
	  uphist();
	  continue;
	case 'B': /* down */
	  downhist();
	  continue;
	case 'C': /* right */
	  if (inputptr >= inputend) {
	    beep();
	    continue;
	  }
	  echo(inputbuffer[inputptr++]);
	  continue;
	case 'D': /* left */
	  if (inputptr == 0) {
	    beep();
	    continue;
	  }
	  inputptr--;
	  backspaces(1);
	  continue;
	default:
	  beep();
	  continue;
	}
      case '<':
	hist_begin();
	continue;
      case '>':
	hist_end();
	continue;
      case 'b':
      case 'B':
	/* backward word */
      case 'd':
      case 'D':
	/* forward delete word */
      case 'f':
      case 'F':
	/* forward word */
      case 0177:
	/* backward delete word */
      default:
	beep();
	continue;
      }
    }

    if (c < ' ') {		/* Unknown control char -- ignore. */
      beep();
      continue;
    }

    if ((c >= 0200) && (c < 0240)) { /* No C1 controls, please. */
      beep();
      continue;
    }

    if (c == '?') {
      store(c);
      echo(c);
      if (inputend > inputptr) {
	prrest();
	backspaces(inputend - inputptr);
      }
      if (inputptr < parsepoint) {
	needreparse = true;
      }
      setaction(act_help);
      break;
    }

    if (c == '*') {		/* DEBUG: */
      int i;

      printf("\npp=%d, pos=%d, end=%d, rpf=%d, s='",
	     parsepoint, inputptr, inputend, needreparse);
      for (i = 0; i < inputend; i += 1) {
	putchar(inputbuffer[i]);
      }
      printf("'\nhpos=%d, lwp=%d\n", hpos, lwpos);
      lmarg();
      retype();
      continue;
    }

    /* here for a printing character -- store in buffer. */

    store(c);
    echo(c);
    if (inputend > inputptr) {
      prrest();
      backspaces(inputend - inputptr);
    }
    if (inputptr < parsepoint) {
      needreparse = true;
    }
  }

  if (needreparse) {
    reparse();
  }
}

/*
** nextch() reads one character from the input buffer, filling it
** if needed.
*/

static char nextch(void)
{
  char c;

  while (inputptr <= parsepoint) {
    getmore();
  }
  if (parsepoint == actpos) {
    curraction = action;
  }
  c = inputbuffer[parsepoint++];
  if (c == '\t') {
    recogflag = true;
  }
  return (c);
}

/* fake user input */

static void cmdib(char c)
{
  store(c);
  echo(c);
  dibflag = true;
  parsepoint++;
}

/* move input back one char. */

static void cmdip(void)
{
  curraction = act_normal;
  if (parsepoint > 0) {
    parsepoint -= 1;
  }
}

/* remove one input item (e.g. "?") */

static void uninput(void)
{
  inputptr--;
  delete();
  cmdip();
}

/* skip spaces input. */

static void skipblanks(void)
{
  while (nextch() == ' ');
  cmdip();
}

/* handle "no parse" situation: */

static void noparse(char* errormessage)
{
  if (errmsg == NULL) {
    errmsg = errormessage;
  }
  longjmp(cm_pfbuf, 1);		/* Go back and check for alternatives. */
}

static void starthelp(char* helpmessage)
{
  if (!(this->flags & CM_SDH)) {
    if (hlp2flag) {
      printf("\n  or");
    }
    hlp2flag = true;
    helpflag = true;
    if (this->help != NULL) {
      helpmessage = this->help;
    }
    printf(" %s", helpmessage);
  }
}

static void dohelp(char* helpmessage)
{
  starthelp(helpmessage);
  longjmp(cm_pfbuf, 1);		/* Go back and check the other alternatives. */
}

/*
** Read a general field.
*/

static void readfield(breakset* brk)
{
  char c;

  skipblanks();
  for (;;) {
    c = nextch();
    if (!breakchar(c, brk)) {
      atomstore(c);
      continue;
    }
    if (c == '\t') {
      uninput();
      recogflag = true;
      break;
    }
    if (c == '?') {
      helpflag = true;
    }
    cmdip();
    break;
  }
  atomdone();
}

/*
** Subroutine to try to parse a keyword.
*/

static void help_keyword(cmkeytab* kt)
{
  cmkeyword* kw;		/* current keyword. */
  int maxlen;			/* length of longest keyword. */
  int maxwpl;			/* max number of kw's per line. */
  int wpl;			/* kw's on this line. */
  int matchcount;
  int len;
  int i;

  if (this->flags & CM_SDH) {
    if (this->help != NULL) {
      dohelp(this->help);
    }
    longjmp(cm_pfbuf, 1);
  }

  starthelp("keyword, ");

  maxlen = 0;
  for (i = 0; i < kt->count; i += 1) {
    kw = &kt->keys[i];
    if (atommatch(kw->key)) {
      if (kw->flags & KEY_INV) {
	continue;
      }
      len = strlen(kw->key);
      if (len > maxlen) maxlen = len;
    }
  }

  maxwpl = (79 + 2) / (maxlen + 3);
  wpl = 0;

  matchcount = 0;
  for (i = 0; i < kt->count; i += 1) {
    kw = &kt->keys[i];
    if (atommatch(kw->key)) {
      if (kw->flags & KEY_INV) {
	continue;
      }
      if (matchcount == 0) {
	printf("one of the following:");
      }
      if (kt->flags & KT_MWL) {
	if (wpl == 0) {
	  printf("\n ");
	}
	printf("%s", kw->key);
	wpl += 1;
	if (wpl == maxwpl) {
	  wpl = 0;
	} else {
	  spaces(3 + maxlen - strlen(kw->key));
	}
      } else {
	printf("\n%s", kw->key);
	if (kw->descr != NULL) {
	  spaces(2 + maxlen - strlen(kw->key));
	  printf("- %s", kw->descr);
	}
      }
      matchcount += 1;
    }
  }
  if (matchcount == 0) {
    printf("(no keyword matches current input)");
  }
}

void cm_fillchar(char c)
{
  cmdib(c);
  atomstore(c);
}

void cm_fillstr(char* p, int n)
{
  while (n-- > 0) {
    cm_fillchar(*p++);
  }
}

static void try_keyword(void)
{
  static unsigned char bitmap[] = {
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xfb, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x1f,
    0x80, 0x00, 0x00, 0x1f,
  };
  static breakset keybrk = { bitmap, 128, NULL, 0 };

  cmkeytab* kt = (cmkeytab*) this->data;

  readfield(&keybrk);

  if (helpflag) {
    help_keyword(kt);
    longjmp(cm_pfbuf, 1);
  }

  if (!cm_tbluk(kt, atombuffer)) {  
    noparse("no such keyword");
  }

  if (recogflag) {
    if (pval.tbl.exact) {
      if (pval.kw->flags & KEY_ABR) {
	cm_fillstr(&(((char*) pval.kw->data)[atomptr]),
		   strlen((char*) pval.kw->data) - atomptr);
	if (!cm_tbluk(kt, (char*) pval.kw->data) || !pval.tbl.exact) {
	  noparse("abbrev fsckup");
	}
      }
    } else {
      if (pval.tbl.count > 1) {
	if (pval.tbl.mlen == 0) {
	  beep();
	  longjmp(cm_rdbuf, 1);
	}
      }
      if (pval.tbl.count == 1) {
	if (pval.kw->flags & KEY_NOC) {
	  beep();
	  longjmp(cm_rdbuf, 1);
	}
      }
      cm_fillstr(pval.tbl.mptr, pval.tbl.mlen);
      if (pval.tbl.count > 1) {
	longjmp(cm_rdbuf, 1);
      }
      atomdone();
    }
    cmdib(' ');
    /* XXX recog done, clean up flags */
  } else {
    if (pval.tbl.exact) {
      if (pval.kw->flags & KEY_ABR) {
	if (!cm_tbluk(kt, (char*) pval.kw->data) || !pval.tbl.exact) {
	  noparse("abbrev fsckup");
	}
      }
    }
    if ((pval.tbl.count == 1)
	&& (pval.kw->flags & KEY_EMO)
	&& !pval.tbl.exact) {
      noparse("no such keyword");
    }
    if (pval.tbl.count > 1) {
      noparse("ambigous keyword");
    }
  }
}

/*
** Subroutine(s) to try to parse a number.
*/

static void help_number(int radix)
{
  static char msgbuf[100];

  switch (radix) {
  case 2:
    dohelp("binary number");
  case 8:
    dohelp("octal number");
  case 10:
    dohelp("decimal number");
  case 16:
    dohelp("hexadecimal number");
  }
  sprintf(msgbuf, "number in radix %d", radix);
  dohelp(msgbuf);
}

static int try_number(void)
{
  unsigned long long number;
  int radix;
  int sign;
  int digits;
  char c;
  char d;
  bool more;

  radix = (int) this->data;
  if ((radix < 2) || (radix > 16)) {
    radix = 10;
  }
  number = 0;
  sign = +1;
  digits = 0;			/* No digits seen yet. */

  skipblanks();
  c = nextch();

  /* handle leading sign: */

  if (!(this->flags & NUM_US)) {
    switch (c) {
    case '+':
      sign = +1;
      atomstore(c);
      c = nextch();
      break;
    case '-':
      sign = -1;
      atomstore(c);
      c = nextch();
      break;
    }
  }

  /* handle radix modifiers: */

  if (this->flags & NUM_UNIX) {
    if (c == '0') {
      radix = 8;
      digits = 1;		/* Plain zero is a digit. */
      atomstore(c);
      c = nextch();
      switch (c) {
      case 'b':
	radix = 2;
	digits = 0;		/* ... "0b" is not a digit! */
	atomstore(c);
	c = nextch();
	break;
      case 'x':
	radix = 16;
	digits = 0;		/* ... "0x" is not a digit! */
	atomstore(c);
	c = nextch();
	break;
      }
    }
  }

  cmdip();

  /* now parse the actual number: */

  more = true;

  while (more) {
    c = d = nextch();
    switch (c) {
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      d += ('a' - 'A');
      /* fall into next (lower) case */
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
      d += ('9' + 1 - 'a');
      /* fall into next case */
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      d -= '0';
      if (d >= radix) {
	/* check for anybreak here */

	/* delay error? */

	noparse("Invalid character in number");
      }
      switch (radix) {
      case 2:
	number <<= 1; break;
      case 4:
	number <<= 2; break;
      case 8:
	number <<= 3; break;
      case 16:
	number <<= 4; break;
      default:
	number *= radix; break;
      }
      atomstore(c);
      number += d;
      digits += 1;		/* Seen one more digit now. */
      break;
    case '?':			/* Use a subroutine for help. */
      help_number(radix);
    case '\t':			/* Recog? */
      uninput();
      if (digits == 0) {
	/* XXX what if num_unix is on, user types "0x", and there is
	 * a token later in the fdb chain, like "0xabc"?
	 */
	beep();
	break;
      } else {
	cmdib(' ');
	more = false;
      }
      break;
    default:
      cmdip();			/* Back over terminator. */
      more = false;
    }    
  }
  pval.num.sign = sign;
  pval.num.magnitude = number;
  pval.num.number = sign * number;
  atomdone();
}

/*
** subroutine to parse a noise word (string).
*/

static int try_noise(void)
{
  char* n = (char*) this->data;
  char c;

  if (cpeflag) {
    recogflag = true;
    cmdib('(');
    while ((c = *n++) != (char) 0) {
      cmdib(c);
    }
    cmdib(')');
    cmdib(' ');
  } else {
    c = nextch();
    while (c == ' ') {
      c = nextch();
    }
    if (c == '(') {
      for (;;) {
	c = nextch();
	if (c == '\t') {
	  uninput();
	  while ((c = *n++) != (char) 0) {
	    cmdib(c);
	  }
	  cmdib(')');
	  cmdib(' ');
	  return;
	}
	if (c == ')') {
	  if (*n == (char) 0) {
	    return;
	  }
	}
	if (c == *n++) {
	  continue;
	}
	noparse("invalid noise word");
      }
    } else {
      cmdip();
    }
  }
}

/*
** subroutine to parse a file name.
*/

static int try_filename(void)
{
  static unsigned char filmap[] = {
    0xff, 0xff, 0xff, 0xff,	/* Allow all printing chars. */
    0x80, 0x00, 0x00, 0x01,	/* ... but not '?'. */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
  };
  static breakset filbrk = { filmap, 128, NULL, 0 };

  readfield(&filbrk);

  if (helpflag) {
    dohelp("file name");
  }

  if (recogflag) {
    cmdib(' ');
    /* XXX fixup recog flags. */
  }
}

/*
** subroutine to parse a field.
*/

static int try_field(void)
{
  static unsigned char fldmap[] = {
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xfb, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x1f,
    0x80, 0x00, 0x00, 0x1f,
  };
  static breakset fldbrk = { fldmap, 128, NULL, 0 };

  readfield(&fldbrk);

  if (helpflag) {
    dohelp("field");
  }

  if (recogflag) {
    cmdib(' ');
    /* XXX fixup recog flags. */
  }
}

/*
** subroutine to parse a confirm.
*/

static int try_cfm(void)
{
  char c;

  c = nextch();
  while (c == ' ') {
    c = nextch();
  }
  if (c == '?') dohelp("confirm");
  if (c == '\n') return;
  if (c == '\t') {
    beep();
    uninput();
    reparse();
    /* XXX fixup code above. */
  }
  noparse("not confirmed");
}

/*
** subroutine to parse a line of text.
*/

static int try_text(void)
{
  noparse("Text parsing not yet supported");
}

/*
** subroutine to parse a date/time string.
*/

static int try_date_time(void)
{
  noparse("Date/time parsing not yet supported");
}

/*
** subroutine to parse a quoted string.
*/

static int try_qst(void)
{
  char c;
  char quote;

again:
  c = nextch();
  while (c == ' ') {
    c = nextch();
  }
  if (c == '?') dohelp("quoted string");
  if (c == '\t') {
    uninput();
    beep();
    goto again;			/* Maybe call reparse()? */
  }
  if ((c == '"') || (c == '\'')) {
    quote = c;
    for (;;) {
      c = nextch();
      if (c == '\t') {
	uninput();
	beep();
	continue;
      }
      if (c == quote) {
	c = nextch();
	if (c != quote) {
	  if (c == '\t') {
	    uninput();
	    cmdib(' ');
	    atomdone();
	    return;
	  }
	  cmdip();
	  atomdone();
	  return;
	}
      }
      if (c == '\n') {
	break;
      }
      atomstore(c);
    }
  }
  noparse("invalid quoted string");
}

/*
** subroutine to parse a token.
*/

static int try_token(void)
{
  char* t = (char*) this->data;
  char c;

  skipblanks();
  for (;;) {
    c = nextch();
    if (c == '\t') {
      uninput();
      while ((c = *t++) != (char) 0) {
	cmdib(c);
      }
      cmdib(' ');
      return;
    }
    if (c == *t) {
      t++;
      continue;
    }
    if (c == '?') {
      starthelp("matching token: ");
      if (this->help == NULL) {
	printf((char*) this->data);
      }
      longjmp(cm_pfbuf, 1);
    }
    if (*t == (char) 0) {
      cmdip();
      return;
    }
    noparse("does not match token");
  }
}

/*
** subroutine to parse a user name.
*/

static int try_username(void)
{
  static unsigned char fyfan[] = {

    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    0x80, 0x00, 0x00, 0x1f,
    0x80, 0x00, 0x00, 0x1f,
  };
  static breakset usrbrk = { fyfan, 128, NULL, 0 };

  readfield(&usrbrk);

  if (helpflag) {
    dohelp("user name");
  }

  if (recogflag) {
    cmdib(' ');
    /* XXX fixup recog flags. */
  }
}

/*
** subroutine to parse a group name.
*/

static int try_groupname(void)
{
  static unsigned char bitmap[] = {
    0xff, 0xff, 0xff, 0xff,
    0x80, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
  };
  static breakset grpbrk = { bitmap, 128, NULL, 0 };

  readfield(&grpbrk);

  if (helpflag) {
    dohelp("group name");
  }

  if (recogflag) {
    cmdib(' ');
    /* XXX fixup recog flags. */
  }
}

/*
** subroutine to parse a MAC address.
*/

static int try_macaddr(void)
{
  static unsigned char bitmap[] = {
    0xff, 0xff, 0xff, 0xff,
    0x80, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
  };
  static breakset macbrk = { bitmap, 128, NULL, 0 };

  int b0, b1, b2, b3, b4, b5;

  readfield(&macbrk);

  if (helpflag) {
    dohelp("mac address");
  }
  
  if (recogflag) {
    beep();
    longjmp(cm_rpbuf, 1);
  }

  if (sscanf(atombuffer, "%2x:%2x:%2x:%2x:%2x:%2x",
	     &b0, &b1, &b2, &b3, &b4, &b5) == 6) {
    pval.mac.addr[0] = b0;
    pval.mac.addr[1] = b1;
    pval.mac.addr[2] = b2;
    pval.mac.addr[3] = b3;
    pval.mac.addr[4] = b4;
    pval.mac.addr[5] = b5;
    return;
  }

  if (sscanf(atombuffer, "%2x-%2x-%2x-%2x-%2x-%2x",
	     &b0, &b1, &b2, &b3, &b4, &b5) == 6) {
    pval.mac.addr[0] = b0;
    pval.mac.addr[1] = b1;
    pval.mac.addr[2] = b2;
    pval.mac.addr[3] = b3;
    pval.mac.addr[4] = b4;
    pval.mac.addr[5] = b5;
    return;
  }

  /* parse a mac addr.  Formats allowed:
  **   xx-xx-xx-xx-xx-xx
  **   xx:xx:xx:xx:xx:xx
  **   xxxx.xxxx.xxxx
  */

  noparse("invalid mac address");
}

/*
** subroutine to parse an IPv4 address.
*/

static int try_ip4addr(void)
{
  static unsigned char fyfan[] = {
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xfc, 0x00, 0x3f,
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
  };
  static breakset ip4brk = { fyfan, 128, NULL, 0 };

  readfield(&ip4brk);

  if (helpflag) {
    dohelp("IP address");
  }

  if (recogflag) {
    beep();
    longjmp(cm_rpbuf, 1);
  }

  if (atomptr > 0) {
    int ipaddr;
    int ccount;
    int dcount;
    int pos;
    int b;
    char c;

    pos = 0;
    ipaddr = 0;
    ccount = 0;
    dcount = 0;
    b = 0;
    for (;;) {
      c = atombuffer[pos++];
      if ((c >= '0') && (c <= '9')) {
	b = b * 10 + c - '0';
	ccount += 1;
      } else if (c == '.') {
	if (ccount == 0) break;
	if (dcount >= 3) break;
	pval.ip4.addr[dcount] = b;
	dcount += 1;
	b = 0;
	ccount = 0;
      } else if (c == (char) 0) {
	if (dcount != 3) break;
	if (ccount == 0) break;
	pval.ip4.addr[3] = b;
	return;
      } else {
	break;
      }
    }    
  }
  noparse("invalid IPv4 address");
}

/*
** subroutine to parse an IPv6 address.
*/

static int try_ip6addr(void)
{
  static unsigned char fyfan[] = {
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xfd, 0x00, 0x1f,	/*  !"# $%&' ()*+ ,-./ 0123 4567 89:; <=>? */
    0x80, 0x00, 0x00, 0x1f,	/* @ABC DEFG HIJK LMNO PQRS TUVW XYZ[ \]^_ */
    0x80, 0x00, 0x00, 0x1f,	/* `abc defg hijk lmno pqrs tuvw xyz{ |}~  */
  };
  static breakset ip6brk = { fyfan, 128, NULL, 0 };

  readfield(&ip6brk);

  if (helpflag) {
    dohelp("IPv6 address");
  }

  if (recogflag) {
    beep();
    longjmp(cm_rpbuf, 1);
  }

  if (atomptr > 0) {
    if (strchr(atombuffer, ':') != NULL) {
      return;
    }
  }
  noparse("invalid IPv6 address");
}

/************************************************************************/

/* User callable functions below: */

/*
** read a password, no echoing.
*/

char* cm_password(char* prompt)
{
  /* prompt for a password, and read it without echo.
  **
  ** The following editing is available:
  **
  ** Newline -- returns the (possibly empty) string collected.
  ** Rubout/Backspace -- delete one char backwards.
  ** C-U -- delete the whole line.
  ** C-C or C-G -- abort, and return NULL.
  */

  char c;

  if (!incmd) {			/* Make sure we are inited. */
    cm_init();
    incmd = true;
  }

  printf("%s", prompt);

  atominit();			/* Use the atom buffer for collecting. */
  for (;;) {
    c = xxxchar();
    switch (c) {
    case ('U' - 0100):
      atominit();
      continue;
    case ('H' - 0100):
    case 0177:
      if (atomptr > 0) {
	atomptr--;
      }
      continue;
    case ('C' - 0100):
    case ('G' - 0100):
      putchar('\n');
      return (NULL);
    case '\n':
      atomdone();
      putchar('\n');
      return (atombuffer);
    default:
      atomstore(c);
      echo((char) 0);		/* Feed the dog. */
      continue;
    }
  }
}

/*
** helper routine to make sure a keytab is dynamic, of proper size:
*/

static char* copystring(char* s)
{
  char* newstr;

  newstr = malloc(strlen(s) + 1);
  if (newstr != NULL) {
    (void) strcpy(newstr, s);
  }
  return (newstr);
}
  
static bool makedyn(cmkeytab* kt, bool grow)
{
  cmkeyword* newtab;
  int newsize;
  int i;

  newsize = kt->count;
  if (grow) {
    newsize += 4;
    newsize &= ~3;
  }

  if (kt->size >= newsize) {
    if (kt->flags & KT_DYN) {
      return (true);
    }
  }

  newtab = malloc(sizeof(cmkeyword) * newsize);
  if (newtab == NULL) return (false);

  for (i = 0; i < newsize; i += 1) {
    if (i < kt->count) {
      newtab[i] = kt->keys[i];
    }
  }

  if (kt->flags & KT_DYN) {
    free(kt->keys);
  }

  kt->keys = newtab;
  kt->flags |= KT_DYN;
  kt->size = newsize;

  return (true);
}

/*
** insert an item into a keyword table:
**   offset = place wanted for new item, or -1 if we should just do it.
*/

bool cm_tbadd(cmkeytab* kt, char* name, int offset)
{
  int i;
  cmkeyword* kw;

  if (cm_tbluk(kt, name)) {
    if (pval.tbl.exact) {
      /* set fail reason */
      return (false);
    }
  }

  if (offset == -1) {
    /* XXX should find suitable pos */
    offset = 0;
  }

  if ((offset < 0) || (offset > kt->count)) {
    return (false);
  }
  
  if (!makedyn(kt, true)) {
    return (false);
  }

  for (i = kt->count; i > offset; i--) {
     kt->keys[i] = kt->keys[i - 1];
  }

  kt->count += 1;

  kw = &kt->keys[offset];
  kw->key = copystring(name);
  kw->flags = KEY_DYN;
  kw->data = NULL;
  kw->func = NULL;
  kw->descr = NULL;

  return (true);
}

/*
** delete an item from a keyword table:
*/

bool cm_tbdel(cmkeytab* kt, char* name)
{
  int i;

  if (!cm_tbluk(kt, name)) {
    /* does not match, complain. */
    return (false);
  }

  if (!pval.tbl.exact) {
    return (false);
  }

  if (!(kt->flags & KT_DYN)) {
    if (!makedyn(kt, false)) {
      return (false);
    }
  }

  kt->count -= 1;

  /* if the keyword we are removing is dynamic, free the key. */

  for (i = pval.tbl.offset; i < kt->count; i += 1) {
    kt->keys[i] = kt->keys[i + 1];
  }

  return (true);
}

/*
** lookup an item in a keyword table:
*/

bool cm_tbluk(cmkeytab* kt, char* name)
{
  int i;
  int namelen;
  cmkeyword* kw;

  namelen = strlen(name);

  pval.tbl.exact = false;
  pval.tbl.ambig = false;
  pval.tbl.count = 0;
  pval.tbl.mlen = 0;
  pval.tbl.mptr = "";

  for (i = 0; i < kt->count; i += 1) {
    kw = &kt->keys[i];
    if (kw->flags & KEY_NOR) {
      continue;
    }
    if (strncmp(kw->key, name, namelen) == 0) {
      if (kw->flags & KEY_ABR) {
	if (kw->key[namelen] != 0) {
	  continue;
	}
      }
      if (kw->key[namelen] == (char) 0) {
	pval.tbl.count = 1;
	pval.tbl.mlen = 0;
	pval.tbl.exact = true;
        pval.tbl.offset = i;
        pval.kw = kw;
	break;
      }
      if (pval.tbl.count == 0) {
	pval.tbl.offset = i;
	pval.kw = kw;
	pval.tbl.mptr = &kw->key[namelen];
	pval.tbl.mlen = strlen(pval.tbl.mptr);
      } else {
	char* p = pval.tbl.mptr;
	char* q = &kw->key[namelen];
	int i = 0;

	while ((*p != (char) 0) && (*p++ == *q++)) {
	  i += 1;
	}
	if (i < pval.tbl.mlen) {
	  pval.tbl.mlen = i;
	}
      }
      pval.tbl.count += 1;
    }
  }

  if (pval.tbl.count == 0) {
    return (false);
  }
  if (pval.tbl.count > 1) {
    pval.tbl.ambig = true;
  }
  return (true);
}

/*
** parse something.
*/

void cm_parse(cmfdb* f)
{
  int saveptr;
  char c;
  
  helpflag = false;
  hlp2flag = false;
  cpeflag = recogflag;
  recogflag = false;

  gcflag = true;		/* fdb list can be pruned. */

  errmsg = NULL;

  saveptr = parsepoint;
  this = f;

  if (parsepoint == 0) {
    bolflag = true;
  }

  /* XXX before ignoring empty lines, we should check for:
  ** 1. an FDB list containing a confirm.
  ** 2. a provided default answer.
  */

  if (bolflag) {
    for (;;) {
      c = nextch();
      while (c == ' ') {
	c = nextch();
      }
      if (c != '\n') {
	cmdip();
	break;
      }
      histptr = NULL;		/* Blank lines reset history entry point. */
      clearinput();
      hpos = lwpos = 0;
      reprompt();
    }
  }
    
  /* Restart here for one of the following reasons:
  ** 1. do a partial reparse, say after a partial complete.
  ** 2. as part of giving help, step to next.
  ** 3. after an error occured, step to next.
  */

  if (setjmp(cm_rdbuf) != 0) {
    /* someone wants a partial reparse. */

    this = f;
    parsepoint = saveptr;
    /* XXX clean up regog flag(s) */

  } else if (setjmp(cm_pfbuf) != 0) {
    /* Some prev. try_XXX failed.  Step to next block and try again. */

    parsepoint = saveptr;
    if (this != NULL) {
      this = this->next;
    }
  } else if (f == NULL) {
    noparse("empty fdb chain");
  }

  if (this == NULL) {
    /* end of fdb chain. */

    /* XXX clean up after ev. recog/help situation. */

    if (helpflag) {
      printf("\n");
      if (inputptr > 0) {
	uninput();		/* Eat '?'. */
      }
      hpos = lwpos = 0;
      retype();
      reparse();
    }

    printf("error: %s\n", errmsg);
    longjmp(cm_erbuf, 1);
  }

  atominit();			/* Clear out the atom buffer. */

  switch (this->function) {
    case _CMKEY: try_keyword(); break;
    case _CMNUM: try_number(); break;
    case _CMNOI: try_noise(); break;
    case _CMFIL: try_filename(); break;
    case _CMFLD: try_field(); break;
    case _CMCFM: try_cfm(); break;
    case _CMTXT: try_text(); break;
    case _CMTAD: try_date_time(); break;
    case _CMQST: try_qst(); break;
    case _CMTOK: try_token(); break;
    case _CMUSR: try_username(); break;
    case _CMGRP: try_groupname(); break;
    case _CMMAC: try_macaddr(); break;
    case _CMIP4: try_ip4addr(); break;
    case _CMIP6: try_ip6addr(); break;
    default: noparse("illegal function code");
  }

  /* XXX clean up after ev. recog/help situation. */

  pval.used = this;		/* Remember what succeded. */
  bolflag = false;		/* Not at beginning of line. */
}

void cm_setprompt(char* message)
{
  histsave();
  prompter = message;
  printf("%s", message);
  clearinput();
}

void cm_confirm(void)
{
  static cmfdb cfmfdb = {
    _CMCFM, 0, 0, 0, 0, 0, 0
  };

  cm_parse(&cfmfdb);
}

void cm_noise(char* text)
{
  static cmfdb noisefdb = {
    _CMNOI, 0, 0, 0, 0, 0, 0
  };

  noisefdb.data = text;
  cm_parse(&noisefdb);
}

void cm_init(void)
{
  if (!incmd) {
    sys_init();
  }
  incmd = true;
}

void cm_exit(void)
{
  /* placeholder */
}

/*
** fix these:
*/

static void gccheck(void)
{
  if (gcflag) {
    dyn_fdb* f;
    dyn_keytab* k;

    while (gc_fdb != NULL) {
      f = gc_fdb;
      gc_fdb = gc_fdb->next;
      f->next = free_fdb;
      free_fdb = f;
    }
    while (gc_keytab != NULL) {
      k = gc_keytab;
      gc_keytab = gc_keytab->next;
      k->next = free_keytab;
      free_keytab = k;
    }
  }
  gcflag = false;
}

cmkeytab* cm_ktab(cmkeyword* keys, int flags)
{
  dyn_keytab* dk;
  cmkeytab* k;
  int i;

  gccheck();
  
  if (free_keytab != NULL) {
    dk = free_keytab;
    free_keytab = dk->next;
  } else {
    dk = (dyn_keytab*) malloc(sizeof(dyn_keytab));
  }

  if (dk != NULL) {
    dk->next = gc_keytab;
    gc_keytab = dk;

    k = &dk->k;

    for (i = 0; keys[i].key != NULL; i += 1);
    k->count = i;
    k->size = i;
    k->flags = flags;
    k->keys = keys;
    return (k);
  }
  return (NULL);
}

cmfdb* cm_fdb(int type, char* help, int flags, void* data)
{
  dyn_fdb* df;
  cmfdb* f;

  gccheck();			/* Check for giving back old junk. */

  if (free_fdb != NULL) {
    df = free_fdb;
    free_fdb = df->next;
  } else {
    df = (dyn_fdb*) malloc(sizeof(dyn_fdb));
  }
  if (df != NULL) {
    df->next = gc_fdb;
    gc_fdb = df;

    f = &df->f;
    f->function = type;
    f->flags = flags;
    f->next = NULL;
    f->data = data;
    f->help = help;
    f->defanswer = NULL;
    f->brk = NULL;
    return (f);
  }
  return (NULL);
}

cmfdb* cm_chain(cmfdb* head, ...)
{
  va_list ap;
  cmfdb* this;
  cmfdb* next;

  va_start(ap, head);
  
  this = head;

  while ((next = va_arg(ap, cmfdb*)) != NULL) {
    this->next = next;
    this = next;
    this->next = NULL;
  }
  va_end(ap);

  return (head);
}

void cm_setinput(int (*reader)(void))
{
  extreader = reader;
}

/* simplifiers: */

static void simpleparse(int function, char* help, int flags, void* data)
{
  cmfdb f = { 0 };

  f.function = function;
  f.help = help;
  f.flags = flags;
  f.data = data;
  cm_parse(&f);
}

void cm_dispatch(void)
{
  (*pval.kw->func)();
}

void cm_pkey(char* help, int flags, cmkeyword* keys, int kflags)
{
  simpleparse(_CMKEY, help, flags, cm_ktab(keys, kflags));
}

void cm_pcmd(char* help, int flags, cmkeyword* keys, int kflags)
{
  cm_pkey(help, flags, keys, kflags);
  cm_dispatch();
}

void cm_pkey_kt(char* help, int flags, cmkeytab* kt)
{
  simpleparse(_CMKEY, help, flags, kt);
}

void cm_pcmd_kt(char* help, int flags, cmkeytab* kt)
{
  simpleparse(_CMKEY, help, flags, kt);
  cm_dispatch();
}

void cm_pnum(char* help, int flags, int radix){

  simpleparse(_CMNUM, help, flags, (void*) radix);
}

void cm_pnoi(char* help, int flags, char* noise)
{
  simpleparse(_CMNOI, help, flags, noise);
}

void cm_pfil(char* help, int flags)
{
  simpleparse(_CMFIL, help, flags, NULL);
}

void cm_pwrd(char* help, int flags)
{
  simpleparse(_CMFLD, help, flags, NULL);
}

void cm_pfld(char* help, int flags, void* foo)
{
  simpleparse(_CMFLD, help, flags, foo);
}

void cm_pcfm(char* help, int flags)
{
  simpleparse(_CMCFM, help, flags, NULL);
}

void cm_ptxt(char* help, int flags)
{
  simpleparse(_CMTXT, help, flags, NULL);
}

void cm_ptad(char* help, int flags)
{
  simpleparse(_CMTAD, help, flags, NULL);
}

void cm_pqst(char* help, int flags)
{
  simpleparse(_CMQST, help, flags, NULL);
}

void cm_ptok(char* help, int flags, char* token)
{
  simpleparse(_CMTOK, help, flags, token);
}

void cm_pusr(char* help, int flags)
{
  simpleparse(_CMUSR, help, flags, NULL);
}

void cm_pgrp(char* help, int flags)
{
  simpleparse(_CMGRP, help, flags, NULL);
}

void cm_pmac(char* help, int flags)
{
  simpleparse(_CMMAC, help, flags, NULL);
}

void cm_pip4(char* help, int flags)
{
  simpleparse(_CMIP4, help, flags, NULL);
}

void cm_pip6(char* help, int flags)
{
  simpleparse(_CMIP6, help, flags, NULL);
}
