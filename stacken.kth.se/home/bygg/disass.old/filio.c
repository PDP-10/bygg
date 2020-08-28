/*
** This module implements file i/o for save/restore handling.
*/

#include "disass.h"

/*
** Our local variables:
*/

static FILE* workfile = nil;
static char* openname = nil;
static char openmode;

static char lastchar;
static bool pushback = false;
static bool eofflag = false;
static bool eolnflag = false;

/*
** variables for text collecting:
*/

#define txtsize 1000

static char* txtptr;
static char txtbuf[txtsize];
static int txtcount;

/*
** text_setup() sets up for collecting strings, char by char.
*/

static void text_setup(void)
{
  txtptr = txtbuf;
  txtcount = 0;
}

static void text_save(char c)
{
  txtcount += 1;
  if (txtcount < txtsize) {
    *txtptr++ = c;
  }
}

static char* text_reap(void)
{
  *txtptr = (char) 0;
  return(txtbuf);
}

static int c2hex(char c)
{
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'a') && (c <= 'f')) return(c + 10 - 'a');
  if ((c >= 'A') && (c <= 'F')) return(c + 10 - 'A');
  return(-1);
}

/*
** savename() saves the file name and mode we are going for.
*/

static void savename(char* filename, char mode)
{
  openname = copystring(filename, openname);
  openmode = mode;
}

/*
** iocheck() is a routine that prints out an error message (why the last
** file operation failed) if its boolean argument is false.  The argument
** is passed on as the return value.
*/

bool iocheck(bool flag)
{
  if (!flag) {
    bufstring("?Can't open \"");
    bufstring(openname);
    bufstring("\" for ");
    switch (openmode) {
    case 'b': 
      bufstring("binary input.");
      break;
    case 'r':
      bufstring("input.");
      break;
    case 'w':
      bufstring("output.");
      break;
    }
    bufnewline();
  }
  return(flag);
}

/*
** wf_ropen() opens the i/o stream for reading.
*/

bool wf_ropen(char* filename)
{
  savename(filename, 'r');
  if ((workfile = fopen(filename, "r")) == nil) {
    return(false);
  }
  pushback = false;
  eofflag = false;
  eolnflag = false;
  return(true);
}

/*
** wf_bopen() opens the i/o stream for binary reading.
*/

bool wf_bopen(char* filename)
{
  savename(filename, 'b');
  if ((workfile = fopen(filename, "rb")) == nil) {
    return(false);
  }
  return(true);
}

/*
** wf_wopen() opens the i/o channel for writing.
*/

bool wf_wopen(char* filename)
{
  savename(filename, 'w');
  if ((workfile = fopen(filename, "w")) == nil) {
    return(false);
  }
  return(true);
}

/*
** wf_close() closes the i/o channel.
*/

bool wf_close(void)
{
  fclose(workfile);
  workfile = nil;
  return(true);
}

/*
** wf_wchar() writes one character to the i/o stream.
*/

void wf_wchar(char c)
{
  putc(c, workfile);
}

/*
** wf_newline() writes a newline to the i/o stream.
*/

void wf_newline(void)
{
  wf_wchar('\n');
}

/*
** wf_whex() writes a hexadecimal number to the i/o stream.
*/

void wf_whex(longword l)
{
  fprintf(workfile, "%lx", l);
}

/*
** wf_waddr() writes an address to the i/o stream.  The address is
** written in hexadecimal, with as many digits as are necessary, and
** terminated with a colon.
*/

void wf_waddr(address* a)
{
  a_print(a, true);
  wf_wchar(':');
}

/*
** simplechar() checks if a char is printable, excluding '=' and '"'.
*/

static bool simplechar(char c)
{
  if (c < ' ')   return(false);
  if (c >= 0177) return(false);
  if (c == '=')  return(false);
/*  if (c == ':')  return(false); */
  if (c == '"')  return(false);
  return(true);
}

/*
** simplestring() checks that a string is shorter than 64 chars, and
** that it contains only simple chars.
*/

static bool simplestring(char* s)
{
  char c;
  int len = 0;
  char lastchar = (char) 0;
  
  while ((c = *s++) != (char) 0) {
    len += 1;
    if (len > 64) return(false);
    if (!simplechar(c)) return(false);
    lastchar = c;
  }
  if (lastchar == ' ') return(false);
  return(true);
}

/*
** wf_wstr() writes a string to the i/o stream.
*/

void wf_wstr(char* s)
{
  char c;
  int pos;
  
  if (simplestring(s)) {
    while((c = *s++) != (char) 0) {
      wf_wchar(c);
    }
  } else {
    pos = 0;
    wf_wchar('"');
    while((c = *s++) != (char) 0) {
      if (simplechar(c)) {
	wf_wchar(c);
	pos += 1;
      } else {
	wf_wchar('=');
	wf_w2hex(c);
	pos += 3;
      }
      if ((pos > 64) || (c == '\n')) {
	if (*s != (char) 0) {
	  wf_wchar('\n');
	  wf_wchar(' ');
	  pos = 0;
	}
	if (*s == ' ') {
	  wf_wchar('=');
	  wf_w2hex(' ');
	  s++;
	}
      }
    }
    wf_wchar('"');
  }
}

/*
** wf_wname() writes a name (token) to the i/o stream.
*/

void wf_wname(char* n)
{
  wf_wstr(n);			/* This is wrong. */
}

/*
** wf_rblock() reads a block from the currently open input stream.
*/

int wf_rblock(byte* buffer, int size)
{
  return(fread(buffer, 1, size, workfile));
}

/*
** wf_rskip() skips a given number of bytes from the input stream.
*/

void wf_rskip(int count)
{
  (void) fseek(workfile, count, SEEK_CUR);
}

/*
** wf_rchar() reads a character from the i/o stream.  On end of file, it
** sets the eof flag (can be checked with wf_ateof()) and returns a new-
** line character.  When reaching end of line it STAYS there, returning
** newlines, until wf_nextline() gets called.  This last trick simplifies
** parse routines...
*/

char wf_rchar(void)
{
  int c;

  if (pushback) {
    pushback = false;
    return(lastchar);
  }

  if (eolnflag) {
    return('\n');
  }

  c = fgetc(workfile);

  if (c == EOF) {
    eofflag = true;
    c = '\n';
  }
  if (c == '\n') {
    eolnflag = true;
  }

  lastchar = (char) c;
  return(lastchar);
}

/*
** wf_pushback() causes the last character to be read again.
*/

void wf_pushback(void)
{
  pushback = true;
}

/*
** wf_ateof() checks if the input stream is at end-of-file.
*/
bool wf_ateof(void)
{
  return(eofflag);
}

/*
** wf_nextline() eats input until we are past the end of the current line.
*/

void wf_nextline(void)
{
  char c;

  c = wf_rchar();
  while (c != '\n') {
    c = wf_rchar();
  }
  eolnflag = false;
  c = wf_rchar();		/* Make sure that the EOF flag gets set. */
  wf_pushback();		/* ... but don't skip chars. */
}

/*
** wf_raddr() reads an address from the i/o stream.
**
** [BIGADDR]
*/

address* wf_raddr(void)
{
  longword l;
  int count;
  char c;

  count = 0;
  l = 0;

  while (true) {
    c = wf_rchar();
    switch (c) {
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      c += ('a' - 'A');
      /* fall into next (lower) case */
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
      c += ('9' + 1 - 'a');
      /* fall into next case */
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      l = (l << 4) + (c - '0');
      count += 1;
      break;
    case ':':
      if (count > 0) {
	return(a_l2a(l));
      }
    default:
      return(nil);
    }
  }
}

/*
** wf_rhex() reads a hexadecimal number from the i/o stream.
*/

longword wf_rhex(void)
{
  longword l;
  int count;
  char c;

  count = 0;
  l = 0;

  while (true) {
    c = wf_rchar();
    switch (c) {
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      c += ('a' - 'A');
      /* fall into next (lower) case */
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
      c += ('9' + 1 - 'a');
      /* fall into next case */
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      l = (l << 4) + (c - '0');
      count += 1;
      break;
    default:
      pushback = true;
      return(l);
    }
  }
}

/*
** wf_rname() reads a name from the i/o stream.
*/

char* wf_rname(void)
{
  char c;

  text_setup();

  while (true) {
    c = wf_rchar();
    if (c == ':') {
      return(text_reap());
    }
    if (   ((c >= '0') && (c <= '9'))
	|| ((c >= 'A') && (c <= 'Z'))
	|| ((c >= 'a') && (c <= 'z'))) {
      text_save(c);
    } else {
      return(nil);
    }
  }
}

/*
** wf_rstr() reads a string from the i/o stream.
*/

char* wf_rstr(void)
{
  char c;

  text_setup();

  c = wf_rchar();
  if (c != '"') {		/* Extended format? */
    pushback = true;		/* No. */
    while (true) {
      c = wf_rchar();
      if (c == '\n') {
	return(text_reap());
      }
      text_save(c);
    }
  } else {			/* Extended format! */
    while (true) {
      c = wf_rchar();
      switch (c) {
      case '"':
	while (wf_rchar() != '\n'); /* skip to EOL/EOF */
	return(text_reap());
      case '=':
	if (wf_is2hex()) {
	  text_save(wf_rchar());
	}
	break;
      case '\n':
	wf_nextline();		/* Goto next line. */
	if (eofflag) {		/* EOF not expected, but... */
	  return(text_reap());
	}
	while (wf_rchar() == ' '); /* skip blanks. */
	wf_pushback();
	break;
      default:
	text_save(c);
	break;
      }
    }
  }
}

/*
** wf_is2hex() checks if the next two bytes are legal hex digits.  If
** they are, we set up so that the next wf_rchar() returns the byte,
** and return true.  If they are not, we return false.
*/

bool wf_is2hex(void)
{
  byte b;
  int i;

  i = c2hex(wf_rchar());
  if (i >= 0) {
    b = i << 4;
    i = c2hex(wf_rchar());
    if (i >= 0) {
      b += i;
      lastchar = b;
      pushback = true;
      return(true);
    }
  }
  return(false);
}

/*
** wf_w2hex() writes its byte argument to the i/o stream as two hex digits.
*/

void wf_w2hex(byte b)
{
  static char digits[16] = "0123456789abcdef";

  wf_wchar(digits[(b & 0xf0) >> 4]);
  wf_wchar(digits[(b & 0x0f)]);
}
