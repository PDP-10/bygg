/*
** This module implements output (text) buffering.
*/

#include "disass.h"
#include "signal.h"
#include "setjmp.h"

extern int casing;

#define maxbuf 4000

static char buffer[maxbuf];
static char* bufp;
static int room;
static int used = 0;

static int hpos;
static int vpos;
static int mark;
static int lastll;

static bool silentflag;
static bool quitflag;
static bool bufxflag;

static FILE* outfile = NULL;
static bool pipeflag;

/*
** buffer holding:
*/

#define maxhold 256

static char holdbuf[maxhold];
static int holdctr;
static bool holding = false;

/*
** Routines local to this module:
*/

static void pipehandler(int sig)
{
  UNUSED(sig);

  quitflag = true;
  silentflag = true;
}

static bool openpipe(char* name)
{
  if ((outfile = popen(name, "w")) != NULL) {
    pipeflag = true;
    signal(SIGPIPE, pipehandler);
    return true;
  }
  return false;
}

static void openpager(void)
{
  char* pager;

  pager = sy_getenv("PAGER");

  if ((pager == NULL) || (*pager == (char) 0)) {
    pager = "more";
  }

  if (openpipe(pager)) {
    *bufp++ = (char) 0;
    fprintf(outfile, "%s", buffer);
    used = 0;
  } else {
    silentflag = true;
  }
}

/*
** This routine tells us that our output (if any) should be sent to a
** specified file instead of to the terminal.
*/

/* Declare these routines, or make us give an error code back. */

void buffile(char* filename)
{
  if ((outfile = fopen(filename, "w")) != NULL) {
    pipeflag = false;
    return;
  }

  printf("?Can't open %s for output.\n", filename);
  { extern jmp_buf cm_erbuf; longjmp(cm_erbuf, 1); } /* XXXXXX */
}

/*
** This routine tells us that our output (if any) should be sent to a
** pipe instead of to the terminal.
*/

void bufpipe(char* pipecmd)
{
  if (!openpipe(pipecmd)) {
    printf("Pipe open failed.\n");
    { extern jmp_buf cm_erbuf; longjmp(cm_erbuf, 1); } /* XXXXXX */
  }
}

/*
** This routine will close the open file or pipe, if any.
*/

void bufclose(void)
{
  if (outfile != NULL) {
    if (pipeflag) {
      pclose(outfile);
      signal(SIGPIPE, SIG_DFL);
    } else {
      fclose(outfile);
    }
    outfile = NULL;
  }
}

/*
** bufctx() starts a new output context.
*/

void bufctx(void)
{
  silentflag = false;
  quitflag = false;
  lastll = 1;
  mark = 0;
  hpos = 0;
  vpos = 0;
  bufp = &buffer[0];
  room = maxbuf;
  used = 0;
}

/*
** This routine will be called by the parser between every command.
*/

void buftop(void)
{
  if (used > 0) {
    *bufp++ = (char) 0;
    printf("%s", buffer);
  }
  bufxflag = false;

  bufclose();

  bufctx();
}

/*
** This routine turns the silence flag on.
*/

void bufshutup(bool silent)
{
  silentflag = silent;
}

/*
** This routine sets the "output-to-X" flag.
*/

void bufxset(bool bufx)
{
  bufxflag = bufx;
  bufctx();
}

/*
** bufquit() is true if the output stream is broken in some way, this
** allows us to stop long commands.
*/

bool bufquit(void)
{
  return quitflag;
}

/*
** bufbyte() is an internal routine to put one byte onto the current output
** stream, whatever it is at the moment.  We don't care about hpos and
** the like, that's not our job.
*/

static void bufbyte(char c)
{
  if (!silentflag) {

    if (holding) {
      if (holdctr < 256) {
	holdbuf[holdctr++] = c;
      }
      return;
    }
    
    if (bufxflag) {
      w_putc(c);
      return;
    }

    if (outfile != NULL) {
      putc(c, outfile);
      return;
    }

    room -= 1;
    if (room > 0) {
      used += 1;
      *(bufp++) = c;
      return;
    }

    openpager();
    bufbyte(c);
  }
}

/*
** Implement buffer holding areas. (save output for a rainy day)
*/

void bufhold(bool startflag)
{
  if (startflag) {
    holdctr = 0;
  }
  holding = startflag;
}

/*
** Impment that rainy day. (empty save buffer)
*/

void bufunhold(void)
{
  int i;

  holding = false;

  for (i = 0; i < holdctr; i += 1) {
    bufbyte(holdbuf[i]);
  }
  holdctr = 0;
}

/*
** Check if we had rain:
*/

bool bufheld(void)
{
  return holdctr > 0;
}

/*
** bufnewline() moves onto the next line.
*/

void bufnewline(void)
{
  lastll = hpos;
  hpos = 0;
  mark = 0;
  if (!silentflag) {
    vpos += 1;			/* vpos += 1 + (hpos / cmcsb._cmcmx); ??? */
    if (vpos > 24) {		/* GROSS KLUDGE AT THE MOMENT! */
      room = 0;
    }
  }
  bufbyte('\n');
}

/*
** bufmark() sets up a virtual "column zero" for tabto().
*/

void bufmark(void)
{
  mark = hpos;
}

/*
** bufhighlight() will do highlighting on/off.
*/

void bufhighlight(int handle, address* addr, bool enterflag)
{
  if (bufxflag)
    w_highlight(handle, addr, enterflag);
}

/*
** tabto(pos) will space over to column "pos", counted from the last call
** to bufmark().  If we are already at or past that pos, nothing happens.
*/

void tabto(int pos)
{
  pos += mark;

  if (!bufxflag) {
    while ((hpos | 7) < pos) {
      bufbyte('\t');
      hpos |= 7;
      hpos += 1;
    }
  }
  while (hpos < pos) {
    bufbyte(' ');
    hpos += 1;
  }
}

/*
** tabspace() will do a tabto() with at least some whitespace output.
*/

void tabspace(int pos)
{
  if (pos <= (hpos - mark)) {
    pos = hpos - mark + 1;
  }
  tabto(pos);
}

/*
** bufchar(c) will put the character c onto the current output stream.
*/

void bufchar(char c)
{
  if (c == '\t') {
    tabto(((hpos - mark) | 7) + 1);
  }
  if (c == '\n') {
    bufnewline();
  }
  if ((c >= ' ') && (c < (char) 0177)) {
    bufbyte(c);
    hpos += 1;
  }

  /*
  ** handle highlight on/off values?
  */
}

/*
** casechar(c) is like bufchar, except that it cares about casing.
*/

void casechar(char c)
{
  /* handle casing_initial too. */

  if ((casing == casing_lower) && (c >= 'A') && (c <= 'Z')) {
    c = c - 'A' + 'a';
  }
  if ((casing == casing_upper) && (c >= 'a') && (c <= 'z')) {
    c = c - 'a' + 'A';
  }
  bufchar(c);
}

/*
** bufstring() should be obvious.
*/

void bufstring(char* s)
{
  char c;

  while((c = *s++) != (char) 0) {
    bufchar(c);
  }
}

/*
** bufctlstring() does like bufstring, but prints control characters
** as ^<char>.
*/

void bufctlstring(char* s)
{
  char c;

  while ((c = *s++) != (char) 0) {
    if (c < ' ') {
      bufchar('^');
      bufchar(c + 64);
    } else if (c == 127) {
      bufchar('^');
      bufchar('?');
    } else {
      bufchar(c);
    }
  }
}

/*
** casestring() is the casing-aware version of bufstring.
*/

void casestring(char* s)
{
  char c;

  while((c = *s++) != (char) 0) {
    casechar(c);
  }
}

/*
** altstr() will take a string of the form "move;load" and skip past the
** semicolon, returning the alternative opcode from the string.  If there
** is no semicolon, return the original string.  This is used together
** with bufaltstr() and casealtstr().
*/

char* altstr(char* s)
{
  char* p;
  char c;

  p = s;
  while ((c = *p++) != (char) 0) {
    if (c == ';') {
      return p;
    }
  }
  return s;
}

/*
** bufaltstr() is a version of bufstring() that thinks ";" is a string
** terminator.  This enables us to use a string constant looking like
** "move;load" to hold two opcodes at once, and select the second one
** by skipping to the ";" if there is one...
*/

void bufaltstr(char* s)
{
  char c;

  while((c = *s++) != (char) 0) {
    if (c == ';') {
      break;
    }
    bufchar(c);
  }
}

/*
** casealtstr() is the casing-aware version of bufaltstr.
*/

void casealtstr(char* s)
{
  char c;

  while((c = *s++) != (char) 0) {
    if (c == ';') {
      break;
    }
    casechar(c);
  }
}

/*
** bufblankline() makes sure that there is a blank line here.
*/

void bufblankline(void)
{
  if (hpos > 0) {
    bufnewline();
  }
  if (!silentflag) {
    if (lastll > 0) {
      bufnewline();
    }
  }
}

/*
** bufdone() is a (temporary) routine that is to be called whenever we
** are done with the current item.  Called from the wrappers around
** peek() and spec() machine dependent routines.
*/

void bufdone(void)
{
  if (hpos > 0) {
    bufnewline();
  }
}

/*
** bufdescription() takes an address and a comment start string, and outputs
** the description that we already know exist at that address, with each line
** preceded by the comment string.
*/

void bufdescription(address* addr, char* cstart)
{
  int startpos;
  char c;
  char* descr;
  int pos = 0;

  startpos = hpos;
  mark = 0;
  descr = d_find(addr);
  
  while((c = *descr++) != (char) 0) {
    if (c == '\n') {
      bufnewline();
      pos = 0;
    } else {
      if (pos == 0) {
	tabto(startpos);
	bufstring(cstart);
      }
      bufchar(c);
      pos += 1;
    }
  }

  if (pos > 0) {
    bufnewline();
  }
}

/*
** bufhex() outputs the longword argument in hex, with at least "n" digits.
** If n == 0 we make sure that the leading hex digit is numeric.
*/

void bufhex(longword l, int n)
{
  char work[20];
  char fmt[10];

  sprintf(fmt, "%%0%ulx", n);
  sprintf(work, fmt, (unsigned long) l);
  if ((n == 0) && (work[0] >= 'a')) {
    bufchar('0');
  }
  casestring(work);
}

/*
** bufdecimal() outputs the longword argument in decimal, with at least
** "n" digits.
*/

void bufdecimal(longword l, int n)
{
  char work[20];
  char fmt[10];

  sprintf(fmt, "%%0%uld", n);
  sprintf(work, fmt, (unsigned long) l);
  bufstring(work);
}

/*
** bufoctal() outputs the longword argument in octal, with at least
** "n" digits.
*/

void bufoctal(longword l, int n)
{
  char work[20];
  char fmt[10];

  sprintf(fmt, "%%0%ulo", n);
  sprintf(work, fmt, (unsigned long) l);
  bufstring(work);
}

/*
** bufbinary() outputs the longword argument in binary, with at least
** "n" digits.
*/

void bufbinary(longword l, int n)
{
  char work[33];
  longword b;
  int i;
  int pos;

  if (n > 32)
    n = 32;

  pos = 31;
  work[32] = 0;

  for (b = 1L, i = 31; i >= 0; b <<= 1, i -= 1) {
    if (l & b) {
      work[i] = '1';
      pos = i;
    } else {
      work[i] = '0';
    }
  }

  /* should worry about "n" here. */

  bufstring(&work[pos]);
}

/*
** bufnumber() outputs the longword argument in decimal, with as many
** digits that are neccesary.
*/

void bufnumber(longword number)
{
  bufdecimal(number, 1);
}

/*
** bufvalue() outputs the contents of a value block.
*/

void bufvalue(value* v)
{
  switch (v_type(v)) {
    case vty_long: bufnumber(v_v2l(v)); break;
    case vty_addr: bufaddress(v_v2a(v)); break;
    default: bufstring("???"); break;
  }
}

/*
** bufsize() outputs the argument as a size, in all three useful radixes.
*/

void bufsize(longword size)
{
  bufhex(size, 1);     bufstring(" (hex) ");
  bufoctal(size, 1);   bufstring(" (oct) ");
  bufdecimal(size, 1); bufstring(" (dec)\n");
}

/*
** bufaddress() prints an address.
*/

void bufaddress(address* a)
{
  a_print(a, false);
}

/*
** bufsymbol() prints the definition of a symbol.
*/

void bufsymbol(symindex index)
{
  char* symdef;

  bufstring("Symbol ");
  bufnumber(index);
  bufstring(", name=");
  bufstring(s_name(index));
  bufstring(", def=");
  symdef = s_read(index);
  if (symdef == NULL) {
    bufstring("<null>");
  } else {
    bufchar('"');
    bufctlstring(symdef);
    bufchar('"');
  }
  bufnewline();
}

/*
** bufpattern() prints the definition of a pattern.
*/

void bufpattern(patindex index)
{
  pattern* p;

  p = p_read(index);

  bufstring("Pattern ");
  bufnumber(index);
  bufstring(", name=");
  bufstring(p_name(index));
  bufstring(", def=");
  while (p != NULL) {
    switch (p->status) {
      case st_none:   bufstring("none");   break;
      case st_cont:   bufstring("cont");   break;
      case st_byte:   bufstring("byte");   break;
      case st_char:   bufstring("char");   break;
      case st_double: bufstring("double"); break;
      case st_float:  bufstring("float");  break;
      case st_inst:   bufstring("inst");   break;
      case st_long:   bufstring("long");   break;
      case st_mask:   bufstring("mask");   break;
      case st_octa:   bufstring("octa");   break;
      case st_ptr:    bufstring("ptr");    break;
      case st_quad:   bufstring("quad");   break;
      case st_text:   bufstring("text");   break;
      case st_word:   bufstring("word");   break;
      default:        bufstring("unknown"); break;
    }
    if (p->length != 0) {
      bufchar(':');
      bufnumber(p->length);
    }
    bufchar(' ');
    p = p->next;
  }
  bufnewline();
}
