/*
** This module contains variables common to all processor-specific
** modules, as well as common helper routines.
*/

#include "disass.h"

/*
** local data types:
*/

typedef struct watchblock {
  struct watchblock* prev;
  struct watchblock* next;
  address* addr;
  int handle;
  wbcallback* callback;
  bool wantsignoff;
  bool signedon;
} watchblock;

struct entryvector* processor = nil;

/* Forward declaration of default handlers: */

evf_exit dflt_exit;
evf_a2s  dflt_a2s;
evf_s2a  dflt_s2a; 
evf_lcan dflt_lcan;
evf_lchk dflt_lchk;
evf_lgen dflt_lgen;
evf_rcre dflt_rcre;
evf_rdel dflt_rdel;
evf_scre dflt_scre;
evf_sdel dflt_sdel;
evf_sset dflt_sset;
evf_auto dflt_auto; 
evf_cchk dflt_cchk;

/* Processor functions: */

evf_exit* pf_exit = dflt_exit;		/* Exit handler. */
evf_peek* pf_peek = (evf_peek*) 0;	/* Main workhorse. */
evf_spec* pf_spec = (evf_spec*) 0;	/* Special functions. */

evf_a2s*  pf_a2s  = dflt_a2s;		/* Address-to-string translator. */
evf_s2a*  pf_s2a  = dflt_s2a;		/* String-to-address translator. */

evf_lcan* pf_lcan = dflt_lcan;		/* Canonicalize label. */
evf_lchk* pf_lchk = dflt_lchk;		/* Check label for valid syntax. */
evf_lgen* pf_lgen = dflt_lgen;		/* Make up a label at spec. addr. */

evf_rcre* pf_rcre = dflt_rcre;		/* Create register hook. */
evf_rdel* pf_rdel = dflt_rdel;		/* Delete register hook. */

evf_scre* pf_scre = dflt_scre;		/* Create symbol hook. */
evf_sdel* pf_sdel = dflt_sdel;		/* Delete symbol hook. */
evf_sset* pf_sset = dflt_sset;		/* Assign symbol hook. */

evf_auto* pf_auto = dflt_auto;		/* Return auto points. */
evf_cchk* pf_cchk = dflt_cchk;		/* Check if char is printable. */

/* Processor variables: */

int       pv_bpa = 1;		/* Bytes per Address unit. */
int       pv_bpl = 4;		/* Bytes per line in hex data blocks. */
int       pv_abits = 16;	/* Number of address bits. */
bool      pv_bigendian;		/* Controls getword/getlong. */
char*     pv_cstart = ";";	/* Controls commenting. */

/* Former peekblock: */

address*  pb_touch;		/* Where we touch. */
int       pb_length;		/* Computed length of object, in addr units. */
stcode    pb_prefer;		/* Prefered status. */
stcode    pb_actual;		/* Actual status. */
stcode    pb_status;		/* ... */
address*  pb_detour;		/* Where we detour, if we do. */
bool      pb_deadend;		/* Instruction stream ends here. */

/* other variables: */

address*  pc = nil;		/* Virtual PC. */
address*  savepc = nil;		/* Saved copy. */
address*  istart;		/* Where current object starts. */

address* datapos = nil;		/* Used for data expansion. */
char statuschar;		/* Char describing the status at istart. */
int residue;			/* Number of data bytes not yet output. */

bool delayblank;		/* Want object terminated with a blank line? */

bool unixflag;			/* Unix format wanted. */
bool updateflag;		/* Updating, creating labels and so on? */
bool listformat;		/* Listing format, or just assembly? */

int peekserial = 1;		/* Serial number of the call to peek(). */

int argindex;			/* Item number on line. */
stcode argstatus;		/* Override status for this item. */
int argradix;			/* Override radix for this item. */

static int flagindex;		/* Number of elements in flag vector here. */
static byte* flagstatus;	/* Array of status codes for here. */
static byte* flagradix;		/* Array of radixes for here. */

/*
** Configurable variables:
*/

static char* vof_opdelim = nil;	/* Value of OPDELIM. */
static char* vof_argdelim = nil; /* Value of ARGDELIM. */
static char* vof_syntax = nil;	/* Value of SYNTAX */

/*
** old (non-string) ones:
*/

int casing = casing_default;
int radix = 0;

stcode defobjstatus = st_none;

/**********************************************************************/

/*
** partmatch() does string matching.
*/

bool partmatch(char* string, char* pattern)
{
  char sc, pc;

  do {
    sc = *string++;
    pc = *pattern++;
    if (sc == (char) 0) {
      return(true);
    }
  } while (sc == pc);
  return(false);
}

/**********************************************************************/

static void csy_argdelim(void)
{
  vof_argdelim = s_read(s_index("argdelim"));
}

static void csy_casing(void)
{
  char* value;

  value = s_read(s_index("casing"));
  casing = casing_default;
  if (value != nil) {
    if (strcmp(value, "lower") == 0) casing = casing_lower;
    if (strcmp(value, "upper") == 0) casing = casing_upper;
    if (strcmp(value, "initial") == 0) casing = casing_initial;
  }
}

static void csy_opdelim(void)
{
  vof_opdelim = s_read(s_index("opdelim"));
}

static void csy_radix(void)
{
  char* r;

  r = s_read(s_index("radix"));

  radix = 0;

  if (r != nil) {
    if (strcmp(r, "2") == 0) radix = 2;
    if (strcmp(r, "8") == 0) radix = 8;
    if (strcmp(r, "10") == 0) radix = 10;
    if (strcmp(r, "16") == 0) radix = 16;
  }
}

static void csy_syntax(void)
{
  vof_syntax = s_read(s_index("syntax"));

  unixflag = false;
  if (vof_syntax != nil) {
    if (strcmp(vof_syntax, "unix") == 0) {
      unixflag = true;
    }
  }
}

static void checksyms(void)
{
  csy_argdelim();
  csy_casing();
  csy_opdelim();
  csy_radix();
  csy_syntax();

  wc_total();			/* Windows might need update. */
				/* (at least wc_symbols must be called.) */
}

/*
** com_scre() gets called from memory.c whenever a symbol gets created.
*/

void com_scre(symindex index)
{
  (*pf_scre)(index);		/* Propagate to processor driver. */
}

/*
** com_sdel() gets called from memory.c whenever a symbol gets deleted.
*/

void com_sdel(symindex index)
{
  if (index == 0) {		/* We only need to handle the post-call. */
    checksyms();
  }
  (*pf_sdel)(index);		/* Propagate to processor driver. */
}

/*
** com_sset() gets called from memory.c whenever a symbol gets a new
** value assigned to it.
*/

void com_sset(symindex index)
{
  checksyms();
  (*pf_sset)(index);		/* Propagate to processor driver. */
}

/*
** com_rcre() gets called from memory.c whenever a register gets created.
*/

void com_rcre(regindex index)
{
  wc_register(index);		/* Tell window driver. */
  (*pf_rcre)(index);		/* Propagate. */
}

/*
** com_rdel gets called from memory.c whenever a register gets deleted.
*/

void com_rdel(regindex index)
{
  wc_register(index);		/* Tell window driver. */
  (*pf_rdel)(index);		/* Propagate. */
}

/*
** setproc() sets the processor entryvector pointer.
*/

void setproc(struct entryvector* p)
{
  if (p == nil) {		/* May be garbage in save file. */
    return;
  }

  (*pf_exit)();			/* Say goodbye to the old processor. */

  pv_bpa = 1;			/* Reset variables to sensible values. */
  pv_bpl = 4;
  pv_abits = 32;
  pv_bigendian = true;
  pv_cstart = ";";

  pf_exit = &dflt_exit;		/* Set up default handlers: */
  pf_a2s  = &dflt_a2s;
  pf_s2a  = &dflt_s2a;
  pf_lcan = &dflt_lcan;
  pf_lchk = &dflt_lchk;
  pf_lgen = &dflt_lgen;
  pf_rcre = &dflt_rcre;
  pf_rdel = &dflt_rdel;
  pf_scre = &dflt_scre;
  pf_sdel = &dflt_sdel;
  pf_sset = &dflt_sset;
  pf_auto = &dflt_auto;
  pf_cchk = &dflt_cchk;

  processor = p;		/* Set up the new processor. */
  (*processor->init)();		/* Say hello, and let it do the rest. */
  wc_total();			/* We might need a window update... */
}

/*
** pcheck() checks that the processor entryvector is set up.  It will
** return true if everything is OK, otherwise it will print an error
** message and return false.
*/

bool pcheck(void)
{
  static bool virgin = true;

  if (processor == nil) {
    if (virgin) {
      bufstring("%There is no processor set up.\n");
    } else {
      bufstring("%There is still no processor set up.\n");
    }
    virgin = false;
    return(false);
  }
  return(true);
}

/*
** getbyte is a routine that gets the next byte to look at, incrementing
** the virtual PC as it goes along.  This is the basis for all looking
** around in the code.
*/

byte getbyte(void)
{
  return(getnext());
}

/*
** getword returns the next 16-bit word from the input stream, updating
** the virtual PC.  Note that this routine depends on the byte ordering
** of the host processor.
*/

word getword(void)
{
  word w;

  if (pv_bigendian) {
    w = getbyte() << 8;
    w |= getbyte();
  } else {
    w = getbyte();
    w |= (getbyte() << 8);
  }
  return(w);
}

/*
** getlong is the longword (32-bit) version of getword.
*/

longword getlong(void)
{
  longword l;

  if (pv_bigendian) {
    l = getbyte() << 24;
    l |= (getbyte() << 16);
    l |= (getbyte() << 8);
    l |= getbyte();
  } else {
    l = getbyte();
    l |= (getbyte() << 8);
    l |= (getbyte() << 16);
    l |= (getbyte() << 24);
  }
  return(l);
}

/*
** save/restore pc.
*/

void pushpc(void)
{
  savepc = a_copy(pc, savepc);
}

void poppc(void)
{
  pc = a_copy(savepc, pc);
  setnext(pc);
}

void setpc(address* addr)
{
  pc = a_copy(addr, pc);
  setnext(pc);
}

/* implement ".", ".." and "..." */

static address* dot = nil;
static address* dotdot = nil;
static address* dotdotdot = nil;

void setdot(address* addr)
{
  a_free(dotdotdot);
  dotdotdot = dotdot;
  dotdot = dot;
  dot = a_copy(addr, nil);
  wc_dots();
}

address* getdot(int numdots)
{
  switch (numdots) {
    case 1: return(dot);
    case 2: return(dotdot);
    case 3: return(dotdotdot);
    default: return(nil);
  }
}

/**********************************************************************/

/*
** ls_dots() lists the values of various pseudo-addresses.
*/

void ls_dots(void)
{
  bufstring("^   = "); bufaddress(w_getaddr(0)); bufnewline();
  bufstring(".   = "); bufaddress(getdot(1)); bufnewline();
  bufstring("..  = "); bufaddress(getdot(2)); bufnewline();
  bufstring("... = "); bufaddress(getdot(3)); bufnewline();

  bufstring(".x  = "); bufaddress(a_l2a(rpn_rstack(0) * pv_bpa));
		       bufnewline();
  bufstring(".y  = "); bufaddress(a_l2a(rpn_rstack(1) * pv_bpa));
		       bufnewline();
  bufstring(".z  = "); bufaddress(a_l2a(rpn_rstack(2) * pv_bpa));
		       bufnewline();
  bufstring(".t  = "); bufaddress(a_l2a(rpn_rstack(3) * pv_bpa));
		       bufnewline();
}

/*
** ls_highlights() does the job of listing highlight points, both for
** the command and for the window update mode.
*/

void ls_highlights(void)
{
  objindex index;
  address* a;

  index = hl_next(0);
  if (index == 0) {
    bufstring("%There are no highlight points.\n");
  }
  while (index != 0) {
    a = (hl_read(index));

    bufnumber(index);
    bufchar(':');
    tabto(6);
    bufaddress(a);
#if 0
    bufstring(a_a2str(a));
    if (l_exist(a)) {
      bufstring(" (");
      bufstring(l_find(a));
      bufstring(")");
    }
#endif
    bufnewline();
    index = hl_next(index);
  }
}

/*
** ls_notes() does the job of listing notes, both for the command and
** for the window updater.
*/

void ls_notes(void)
{
  objindex index;
  char* note;

  index = n_next(0);
  if (index == 0) {
    bufstring("%There are no defined notes.\n");
  }
  while (index != 0) {
    note = n_read(index);
    if (note != nil) {
      bufnumber(index);
      tabto(8);
      bufstring(note);
      bufnewline();
    }
    index = n_next(index);
  }
}

/*
** ls_patterns() does the job of listing patterns, both for the command
** and the window updater.
*/

void ls_patterns(void)
{
  objindex index;

  index = p_next(0);
  if (index == 0) {
    bufstring("%There are no defined patterns.\n");
  }
  while (index != 0) {
    bufpattern(index);
    index = p_next(index);
  }
}

/*
** ls_register() does the job of listing the contents of a specified
** register, both for the command and the window updater.
*/

void ls_register(regindex index)
{
  address* subrange;
  int type;

  type = r_type(index);

  bufstring("Register ");
  bufnumber(index);
  bufstring(", name=");
  bufstring(r_name(index));
  bufstring(", type=");
  switch (type) {
    case vty_long: bufstring("long"); break;
    case vty_addr: bufstring("addr"); break;
    default: bufnumber(type); break;
  }
  if (r_isdef(index, nil)) {
    bufstring(", default value=");
    bufvalue(r_read(index, nil));
  }
  bufnewline();

  subrange = r_subrange(index, nil);
  while (subrange != nil) {
    bufstring("   at address ");
    bufaddress(subrange);
    bufstring(" value=");
    bufvalue(r_read(index, subrange));
    subrange = r_subrange(index, subrange);
    bufnewline();
  }
}

/*
** ls_symbols() does the job of listing symbols, both for the command
** and for the window updater.
*/

void ls_symbols(void)
{
  objindex index;

  index = s_next(0);
  if (index == 0) {
    bufstring("%There are no defined symbols.\n");
  }
  while (index != 0) {
    bufsymbol(index);
    index = s_next(index);
  }
}

/**********************************************************************/

/*
** These routines handle refererencing.
*/

static watchblock* watchlist = nil; /* List of suspects. */
static bool watching = false;	/* One or more wants exit call. */
static address* suspect = nil;	/* Last match when watching. */
static int nextwatchhandle = 0;	/* Next handle to assign. */

/*
** reference() tells us that we think that a memory ref. is being done
** here.  If we are watching out for refs to a specified address, this
** is the place to raise our hand.
*/

void reference(address* addr)
{
  watchblock* wb;

  pb_touch = addr;

  for (wb = watchlist; wb != nil; wb = wb->next) {
    if (a_inside(addr, wb->addr)) {
      if (wb->wantsignoff) {
	wb->signedon = true;
	if (!watching) {
	  watching = true;
	  suspect = a_copy(addr, suspect);
	}
      }
      (*wb->callback)(wb->handle, addr, true);
    }
  }
}

/*
** endref() tells us that a possible reference has ended.  Do what-
** ever cleanup is needed.
*/

void endref(void)
{
  watchblock* wb;

  if (watching) {
    for (wb = watchlist; wb != nil; wb = wb->next) {
      if (wb->signedon) {
	(*wb->callback)(wb->handle, suspect, false);
	wb->signedon = false;
      }
    }
    watching = false;
  }
}

/*
** setwatch() sets a watchout for references to a certain address.
*/

int setwatch(address* addr, wbcallback* callback, bool exitflag)
{
  watchblock* wb;

  wb = malloc(sizeof(watchblock));
  if (wb != nil) {
    nextwatchhandle += 1;
    wb->prev = nil;
    wb->next = watchlist;
    if (watchlist != nil) {
      watchlist->prev = wb;
    }
    watchlist = wb;
    wb->addr = a_copy(addr, nil);
    wb->handle = nextwatchhandle;
    wb->callback = callback;
    wb->wantsignoff = exitflag;
    wb->signedon = false;
    return(wb->handle);
  }
  return(0);
}

/*
** clearwatch() clears a previously set watchpoint.
*/

void clearwatch(int handle)
{
  watchblock* wb;

  for (wb = watchlist; wb != nil; wb = wb->next) {
    if (wb->handle == handle) {
      if (wb->prev != nil) {
	wb->prev->next = wb->next;
      } else {
	watchlist = wb->next;
      }
      if (wb->next != nil) {
	wb->next->prev = wb->prev;
      }
      a_free(wb->addr);
      free(wb);
      return;
    }
  }
}

/*
** scanref() scans code from "." and forward, looking for references.
*/

static int scanhandle;
static bool scandone;

static void matchref(int handle, address* addr, bool flag)
{
  if (handle == scanhandle) {
    scandone = true;
  }
}

int scanref(address* addr)
{
  address* start;
  static address* pos = nil;
  int count = 0;

  start = getdot(1);
  if (start == nil) {
    start = getsfirst(1);
  }

  if (start == nil) {
    return(0);
  }

  pos = a_copy(start, pos);
  scandone = false;
  scanhandle = setwatch(addr, matchref, false);
  bufshutup(true);

  while (mapped(pos) && !scandone) {
    peek(pos, EX_ASM, st_inst);
    setdot(pos);
    a_inc(pos, pb_length);
    count += 1;
  }
  bufshutup(false);
  clearwatch(scanhandle);
  if (scandone) {
    return(count);
  }
  return(-1);
}

/*
** argstep() is to be called once before each argument to the current
** opcode, to distinguish between the different arguments.  It will
** handle things like parsing/stepping the flag vector etc.
*/

void argstep(void)
{
  /* call refend() to signify end of possible reference. */

  argindex += 1;
  argstatus = st_none;

  if (flagindex >= argindex) {
    argstatus = flagstatus[argindex];
  }
}

/*
** sextb, sextw and sextl does sign-extend on a byte, word and longword
** respectively.
*/

long sextb(byte b)
{
  if (b > 0x7f) {
    return(b - 0x100);
  }
  return(b);
}

long sextw(word w)
{
  if (w > 0x7fff) {
    return(w - 0x10000);
  }
  return(w);
}

long sextl(longword l)
{
  return((long) l);
}

/*
** st2char() returns the char corresponding to the given status code.
*/

char st2char(stcode s)
{
  switch (s) {
    case st_none:   return('-');
    case st_cont:   return('.');
    case st_byte:   return('B');
    case st_char:   return('C');
    case st_double: return('D');
    case st_float:  return('F');
    case st_inst:   return('I');
    case st_long:   return('L');
    case st_mask:   return('M');
    case st_octa:   return('O');
    case st_ptr:    return('P');
    case st_quad:   return('Q');
    case st_text:   return('T');
    case st_word:   return('W');
  }
  return('?');
}

/*
** char2st() returns the status code corresponding to the given char.
*/

stcode char2st(char c)
{
  switch (c) {
    case '-': return(st_none);
    case '.': return(st_cont);
    case 'B': return(st_byte);
    case 'C': return(st_char);
    case 'D': return(st_double);
    case 'F': return(st_float);
    case 'I': return(st_inst);
    case 'L': return(st_long);
    case 'M': return(st_mask);
    case 'O': return(st_octa);
    case 'P': return(st_ptr);
    case 'Q': return(st_quad);
    case 'T': return(st_text);
    case 'W': return(st_word);
  }
  return(st_none);
}

/*
** starthex is a routine that does the common part of what startline()
** or restline() does for a processor with hex addresses.  The number of
** address bits is taken from the entry vector.
*/

static void startaddr(int count)
{
  bufchar(statuschar);
  bufchar(' ');
  switch (pv_abits) {
    case 16: bufhex(a_a2w(datapos), 4); break;
    case 32: bufhex(a_a2l(datapos), 8); break;
    default: bug("startaddr", "unsupported pv_abits");
  }
  bufstring(": ");
  while ((count-- > 0) && (residue-- > 0)) {
    bufhex(getmemory(datapos), 2);
    a_inc(datapos, 1);
  }
}

void starthex(int count)
{
  datapos = a_copy(istart, datapos);
  residue = pb_length;
  startaddr(count);
  statuschar = ' ';
}

void stdlabel(void)
{
  if (l_exist(istart)) {
    l_def(istart);
    reference(istart);
    bufstring(l_find(istart));
    endref();
    bufchar(':');
  }
  tabto(8);
}

/*
** Format of the "data" portion of the lines:
**
** "F aaaaaaaa: dddddddd  ", length = 2 + len(addr) + 2 + len(data) + 2;
*/

void startline(bool nonempty)
{
  if (listformat) {
    if (nonempty) {
      starthex(pv_bpl);
    }
    switch (pv_abits) {
      case 16: tabto(2 + 4 + 2 + (pv_bpl * 2) + 2); break;
      case 32: tabto(2 + 8 + 2 + (pv_bpl * 2) + 2); break;
      default: bug("startline", "unsupported pv_abits");
    }
    bufmark();
  }

  if (nonempty) {
    stdlabel();
  }
}

/*
** stdcomment() ...
*/

void stdcomment(int column, char* cstart)
{
  if (c_exist(istart)) {
    tabto(column);
    bufstring(cstart);
    bufstring(c_find(istart));
  }
}

/*
** stddescription() ...
*/

void stddescription(void)
{
  if (d_exist(istart)) {
    bufblankline();
    startline(false);
    bufdescription(istart, pv_cstart);
    bufblankline();
  }
}

/*
** restline() does the bytes startine() did not.
*/

void restline(void)
{
  while (residue > 0) {
    delayblank = false;
    bufnewline();
    startaddr(pv_bpl);
  }
}

/*
** checkblank sets the delayblank flag if the next thing is
** an instruction or an unknown object.
*/

void checkblank(void)
{
  stcode s;

  s = getstatus(pc);
  if (s == st_inst) {
    delayblank = true;
  } else if (s == st_none) {
    if (getstatus(istart) != st_none) {
      delayblank = true;
    }
  }
}

/*
** finddisp() looks up a byte in a dispblock table.
*/

dispblock* finddisp(byte b, dispblock tbl[])
{
  int i;
  
  for (i = 0; tbl[i].itype != arnold; i += 1) {
    if (b == tbl[i].opcode) {
      return(&tbl[i]);
    }
  }
  return(nil);
}

/*
** scantext does a common pick-up of text constants.
*/

#define maxtext 255

char* scantext(int maxlength)
{
  byte c;
  bool textflag;
  int pos;
  static char line[maxtext];

  if (maxlength > maxtext) {
    maxlength = maxtext;
  }

  pos = 0;
  textflag = (getstatus(istart) == st_text);
  while (pos < maxlength) {
    c = getbyte();
    if (!(*pf_cchk)(c)) {
      break;
    }
    line[pos] = c;
    pos += 1;
    if (textflag && (getstatus(pc) != st_cont)) {
      break;
    }
  }
  pb_length = pos;
  return(line);
}

/*
** printable() checks if a char is printable.
*/

bool printable(byte b)
{
  return((*pf_cchk)(b));
}

/*
** argdelim() prints out a delimiting string, either the default given
** as a string argument, or the value of "argdelim" if set.
*/

void argdelim(char* delim)
{
  argstep();
  if (vof_argdelim != nil) {
    delim = vof_argdelim;
  }
  bufstring(delim);
}

/*
** opdelim() is line argdelim(), except that it checks "opdelim".
*/

void opdelim(char* delim)
{
  argstep();
  if (vof_opdelim != nil) {
    delim = vof_opdelim;
  }
  bufstring(delim);
}

/*
** spacedelim() calls opdelim with a default string of a single space.
*/

void spacedelim(void)
{
  opdelim(" ");
}

/*
** tabdelim() calls opdelim with a default string of a tab character.
*/

void tabdelim(void)
{
  opdelim("\t");
}

/*
** overrun() checks if a possible expansion (determined by istart and
** pb_length) overruns an existing object.
*/

bool overrun(void)
{
  int i;

  if (pb_status == st_none) {
    for (i = 1; i < pb_length; i += 1) {
      if (getstatus(a_offset(istart, i)) != st_none) {
	return(true);		/* Overun! */
      }
    }
  }
  return(false);		/* No overrun found. */
}

/*
** peek() is a wrapper routine that does some common initializing, then
** calls the processor-specific module for the main job, and finally
** does some cleanup.
*/

void peek(address* a, int flags, stcode prefer)
{
  updateflag = flags & EX_UPD;	/* Updating? */
  listformat = flags & EX_data;	/* List format? */

  peekserial += 1;		/* Keep track. */

  residue = 0;			/* No residue yet. */

  istart = a;

  setpc(a);			/* Set up our virtual PC. */

  argindex = 0;			/* Currently at opcode. */
  argstatus = st_none;		/* None specified. */
  argradix = 0;			/* None specified. */

  {				/* Read the real flags here: */
    flagindex = 0;
    flagstatus = f_read(istart);
    if (flagstatus != nil) {
      flagindex = flagstatus[0];
    }
    flagradix = nil;
  }

  delayblank = false;		/* Normally no extra blank line. */

  pb_length = 1;
  pb_deadend = false;
  pb_detour = nil;
  pb_status = prefer;
  pb_prefer = prefer;

  pb_actual = getstatus(istart);

  if (prefer == st_none) {
    pb_status = pb_actual;
    prefer = defobjstatus;
  }

  statuschar = st2char(pb_actual);

  (*pf_peek)(prefer);

  /* if reference-in-progress, call refend() to stop now. */

  if (delayblank) {
    bufblankline();
  }

  bufdone();
}

/*
** pe_size() returns the size of a pattern element, at a given address.
*/

int pe_size(address* addr, pattern* pe)
{
  if (pe->length > 0) {
    return(pe->length);
  }
  if (pe->status == st_none) {
    return(1);
  }
  if (processor != nil) {
    bufshutup(true);
    peek(addr, 0, pe->status);
    return(pb_length);
  }
  switch (pe->status) {
    case st_word: return(2);
    case st_long: return(4);
    case st_quad: return(8);
    case st_octa: return(16);
    case st_mask: return(2);
    case st_float: return(4);
    case st_double: return(8);
  }
  return(1);
}

/*
** follow_code() does that.
*/

int follow_code(address* a)
{
  bool done;
  int count;
  address* pos;
  pattern* ia;
  int size;

  pos = a_copy(a, nil);
  count = 0;

  done = false;
  while ((getstatus(pos) == st_none) && mapped(pos) && !done) {
    peek(pos, EX_UPD, st_inst);
    if (pb_status == st_inst) {
      setstatus(istart, st_inst, pb_length);
      done = pb_deadend;
      a_inc(pos, pb_length);
      count += pb_length;
      if (pb_detour != nil) {
	ia = ia_read(pb_detour);
	count += follow_code(pb_detour);
	for (/* ia set up above */; ia != nil; ia = ia->next) {
	  updateflag = true;
	  size = pe_size(pos, ia);
	  suggest(pos, ia->status, size);
	  a_inc(pos, size);
	}
      }
    } else {
      done = true;
    }
  }
  a_free(pos);
  return(count);
}

/*
** spec is a wrapper routine that calls the processor-specific routine
** to do the intended job, with common enter/exit code.
*/

void spec(address* a, int func)
{
  (*pf_spec)(a, func);

  bufdone();
}

/*
** autolist() returns a list of auto starting points.
*/

address* autolist(void)
{
  return((*pf_auto)());
}

/*
** a_a2str converts an address to a string.
*/

char* a_a2str(address* a)
{
  if (a == nil) {
    return(nil);
  }
  return((*pf_a2s)(a));
}

/*
** a_str2a converts a string to an address.
*/

address* a_str2a(char* p)
{
  return((*pf_s2a)(p));
}

/*
** l_canonical() returns the canonical version of a label.  This depends
** on the processor type selected.  Default is to do nothing.
*/

char* l_canonical(char* name)
{
  return((*pf_lcan)(name));
}

/*
** l_check() checks out a label for valid syntax.
*/

bool l_check(char* name)
{
  if (*name == (char) 0) {
    return(false);		/* Don't allow empty labels. */
  }
  return((*pf_lchk)(name));
}

/*
** l_generate() generates a label at a specified address.
*/

void l_generate(address* addr)
{
  (*pf_lgen)(addr);
}

/*
** canonicalize converts a string to lower case, and terminates it after
** a fixed number of characters, unless it's shorter.  The destination
** string is returned as a convinience.
*/

char* canonicalize(char* src, char* dst, int len)
{
  char c;
  char* ret;

  ret = dst;
  while(len-- > 0) {
    if ((c = *src++) == (char) 0) {
      break;
    }
    if ((c >= 'A') && (c <= 'Z')) {
      c += 'a' - 'A';
    }
    *dst++ = c;
  }
  *dst++ = (char) 0;
  return(ret);
}

/*
** checkstring checks that a given string only contains allowable char-
** acters.  Used for checking label syntax.
*/

static bool checkchar(char c, char* s)
{
  char cc;

  if ((c >= 'a') && (c <= 'z')) {
    return(true);
  }
  if ((c >= 'A') && (c <= 'Z')) {
    return(true);
  }
  while ((cc = *s++) != (char) 0) {
    if (cc == c) {
      return(true);
    }
  }
  return(false);
}

bool checkstring(char* string, char* first, char* rest)
{
  char c;

  c = *string++;
  if (!checkchar(c, first)) {
    return(false);
  }
  while ((c = *string++) != (char) 0) {
    if (!checkchar(c, rest)) {
      return(false);
    }
  }
  return(true);
}

/*
** mkccomment() will conditionally insert a comment.  If the argument
** is a character with a string representation, and there is no previous
** comment (at istart) and EX_UPD is set, then we insert the comment.
*/

void mkccomment(byte b)
{
  char* cstring;
  
  if (updateflag && !c_exist(istart)) {
    cstring = charname(b);
    if (cstring != nil) {
      c_insert(istart, cstring);
    }
  }
}

/*
** charname() returns a string "name" for its character argument.
*/

static char* cname[32] = {
  "null",       "^a",         "^b",         "^c",
  "^d",         "^e",         "^f",         "bell",
  "backspace",  "tab",        "linefeed",   "^k",
  "formfeed",   "return",     "^n",         "^o",
  "^p",         "^q",         "^r",         "^s",
  "^t",         "^u",         "^v",         "^w",
  "^x",         "^y",         "^z",         "escape",
  "^\\",        "^]",         "^^",         "^_" };

char* charname(byte b)
{
  static char cn[2];

  if (b < 32) return(cname[b]);
  if (b == 0177) return("rubout");
  cn[0] = b;
  cn[1] = (char) 0;
  return(cn);
}

/*
** Default entry-vector routines.
*/

void dflt_exit(void)
{
  /* The default exit handler does nothing. */
}

/*
** Default address-to-string translator, hex:
*/

char* dflt_a2s(address* pos)
{
  static char work[20];

  if (pos == nil) {
    return(nil);
  }
  sprintf(work, "%lx", a_a2l(pos));
  return(work);
}

/*
** Default string-to-address translator, hex:
*/

address* dflt_s2a(char* p)
{
  longword n;
  char c;

  n = 0;
  while ((c = *p++) != (char) 0) {
    if ((c >= 'A') && (c <= 'F')) {
      c += ('a' - 'A');
    }
    if ((c < '0') || (c > '9')) {
      if ((c < 'a') || (c > 'f')) {
	return(nil);
      } else {
	c += ('9' + 1 - 'a');
      }
    }
    n = (n << 4) + (c - '0');
  }
  return(a_l2a(n));
}

/*
** Default label canonicalizer:
*/

char* dflt_lcan(char* name)
{
  return(name);
}

/*
** Default label checker:
*/

bool dflt_lchk(char* name)
{
  return(true);
}

/*
** Default label generator:
*/

void dflt_lgen(address* addr)
{
  char work[10];

  if (!l_exist(addr)) {
    do {
      sprintf(work, "L%lx", uniq());
    } while(l_lookup(work) != nil);
    l_insert(addr, work);
  }
}

/*
** Default register creator:
*/

void dflt_rcre(regindex index)
{
  UNUSED(index);
}

/*
** Default register deleter:
*/

void dflt_rdel(regindex index)
{
  UNUSED(index);
}

/*
** Default symbol creator:
*/

void dflt_scre(symindex index)
{
  UNUSED(index);
}

/*
** Default symbol deleter:
*/

void dflt_sdel(symindex index)
{
  UNUSED(index);
}

/*
** Default symbol setter:
*/

void dflt_sset(symindex index)
{
  UNUSED(index);
}

/*
** Default auto start points:
*/

address* dflt_auto(void)
{
  return(nil);
}

/*
** Default "is this char printable" checker:
*/

bool dflt_cchk(byte b)
{
  if (b < 32) return(false);
  if (b >= 127) return(false);
  return(true);
}

/************************************************************************/

/*
** Simple routines to set up the various handlers:
*/

void spf_exit(evf_exit* handler) { pf_exit = handler; }

void spf_peek(evf_peek* handler) { pf_peek = handler; }

void spf_spec(evf_spec* handler) { pf_spec = handler; }

void spf_a2s (evf_a2s*  handler) { pf_a2s  = handler; }

void spf_s2a (evf_s2a*  handler) { pf_s2a  = handler; }

void spf_lcan(evf_lcan* handler) { pf_lcan = handler; }

void spf_lchk(evf_lchk* handler) { pf_lchk = handler; }

void spf_lgen(evf_lgen* handler) { pf_lgen = handler; }

void spf_rcre(evf_rcre* handler) { pf_rcre = handler; }

void spf_rdel(evf_rdel* handler) { pf_rdel = handler; }

void spf_scre(evf_scre* handler) { pf_scre = handler; }

void spf_sdel(evf_sdel* handler) { pf_sdel = handler; }

void spf_sset(evf_sset* handler) { pf_sset = handler; }

void spf_auto(evf_auto* handler) { pf_auto = handler; }

void spf_cchk(evf_cchk* handler) { pf_cchk = handler; }
