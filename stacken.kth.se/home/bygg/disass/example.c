/*
** This module implements driver code for a processor, "foo42", that does
** not exist in the real world.  It is meant as a framework on which to
** build real drivers, and also as documentation.
*/

/*
** This gets all (hopefully) symbols that we need.
*/

#include "disass.h"

/*
** Forward declaration of the routines pointed to by the init vector.
*/

evf_init foo42_init;
evf_help foo42_help;

struct entryvector foo42_vector = {
  "foo42",			/* Name. */
  "example processor",		/* One-liner. */
  foo42_init,			/* Init routine. */
  foo42_help,			/* Help routine. */
};

/************************************************************************/

/*
** hex() prints its argument in hexadecimal.  There are two main formats
** for hex numbers used in FOO42 assemblers; one of them uses "0x...", in
** that case we prepend "0x" if the number is larger than 9, the other
** uses "....h", with the first character numeric.  For the latter case
** we make use of a feature in bufhex(): if the size argument is 0, it
** will make sure that the first character is numeric, and behave as if
** the size was 1.
**
** This would make sense if we cared about different assemblers, but
** right now we don't.  Most of the text above is therefore garbage.
*/

static void hex(longword l)
{
  if (l > 9) {
    bufstring("0x");
  }
  bufhex(l, 1);
}

/*
** octal() is the same as hex(), only radixally challenged.
*/

static void octal(longword l)
{
  if (l > 7) {
    bufchar('0');
  }
  bufoctal(l, 1);
}

/*
** number() prints out a number, according to the selected radix.  If the
** selected radix is something we don't support, use the default, i.e. hex.
*/

static void number(longword l)
{
  switch (radix) {
  case 8:
    octal(l);
    break;
  case 10:
    bufdecimal(l, 1);
    break;
  case 16:
  default:
    hex(l);
    break;
  }
}

/*
** This routine will generate a label on the specified address, unless
** one already exist.
*/

static void genlabel(address* addr)
{
  char work[10];

  if (!l_exist(addr)) {
    sprintf(work, "L%05" PRIxl, (a_a2l(addr) & 0xfffff));
    while(l_lookup(work) != NULL) {
      sprintf(work, "L%" PRIxl, uniq());
    }
    l_insert(addr, work);
  }
}

/*
** dobyte() will output the current item as a byte of data.
*/

static void foo42_dobyte(void)
{
  byte b;

  b = getbyte();
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

/*
** dochar() will try to output its argument as a character.  If it can't,
** it will default to dobyte().
*/

static void foo42_dochar(void)
{
  byte b;

  b = getbyte();
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();

  if (printable(b)) {
    bufchar('"');
    if (b == '"') {
      bufchar('"');
    }
    bufchar((char) b);
    bufchar('"');
  } else {
    number(b);
  }

  checkblank();
}

/*
** doword() will output the current item as a word (16 bits) of data.
*/

static void foo42_doword(void)
{
  word w;

  w = getword();
  pb_length = 2;
  startline(true);
  casestring(".word");
  tabdelim();
  number(w);
  checkblank();
}

/*
** dolong() will output the current item as a longword (32 bits) of data.
*/

static void foo42_dolong(void)
{
  longword l;

  l = getlong();

  pb_length = 4;
  startline(true);
  casestring(".long");
  tabdelim();
  number(l);
  checkblank();
}

/*
** doptr() tries to output its argument as a pointer, i.e. if there is
** a label that corresponds to the argument, it will be used, otherwise
** we will setter for a numeric value.
*/

static void foo42_doptr(void)
{
  longword l;
  address* a;
  
  pb_length = 4;		/* Pointers are four bytes here. */
  l = getlong();		/* Get bits. */
  a = a_l2a(l);			/* Get corresponding address. */
  startline(true);		/* Start off line. */
  casestring(".long");		/* Suitable pseudo-op. */
  tabdelim();			/* Tab (or space). */
  reference(a);			/* Note the reference, for highlighting etc. */
  if (l_exist(a)) {		/*   If we have a label, - */
    bufstring(l_find(a));	/*      use it. */
  } else {			/*   If not, - */
    number(l);			/*      just print the numeric value. */
  }
  endref();			/* Stop reference. */
  checkblank();			/* Check for possible blank line here. */
}

/*
** dotext() will try to decode a text constant.
*/

static void foo42_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    setpc(istart);
    foo42_dochar();
  } else {
    startline(true);
    casestring(".ascii");
    tabdelim();
    bufchar('\'');
    for (pos = 0; pos < pb_length; pos += 1) {
      c = line[pos];
      if (c == '\'') {
	bufchar(c);
      }
      bufchar(c);
    }
    bufchar('\'');
  }
}

/*
** doinstr() is the main workhorse.  This is the place you will
** have to write some serious code.
*/

static void foo42_doinstr(void)
{
  pb_length = 1;
  startline(true);
  casestring(".instr");
  tabdelim();
  bufhex(getbyte(), 1);
}

/*
** This routine will be called once for each address where there is any
** data like labels, comments, ... defined.  This takes place when we
** generate the beginning of the program, and we use it to output all
** symbols (labels) that corresponds to unmapped memory.
*/

static void checkunmap(address* a)
{
  if (!mapped(a) && l_exist(a)) {
    bufstring(l_find(a));
    bufstring("==");
    number(a_a2l(a));
    if (c_exist(a)) {
      tabto(32);
      bufchar(';');
      bufstring(c_find(a));
    }
    bufnewline();
  }
}

/*
** This routine will be called at the very start of disassembly, when
** generating source.
*/

void foo42_begin(void)
{
  bufstring(";Beginning of foo42 program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

/*
** This routine will be called every time we "move" to a different
** address, i.e. once at the start of each memory segment.  Again,
** this is when generating source.
*/

void foo42_org(address* a)
{
  bufblankline();
  casestring("org");
  spacedelim();
  bufstring(a_a2str(a));
  bufblankline();
}

/*
** This routine will be called at the very end of generating source.
*/

void foo42_end(void)
{
  bufblankline();
  bufstring(";End of foo42 program");
}

/************************************************************************/

/*
** This routine will be called when we deselect this processor.  This is
** a good place to put cleanup code.
*/

void foo42_exit(void)
{
  /*
  ** No special exit handling.  This means that this routine (and the
  ** setup, via spf_exit) could have been left out.
  */
}

/*
** This routine returns a canonical representation of a label, used for
** looking them up by name in the database.  (Labels are stored and used
** exactly the same way as you type them in).  Labels in all known foo42
** assemblers have eight significant characters, and upper/lower case is
** considered equal.  We have a standard helper routine that copies (and
** unifies (to lower) the case of) a label to a work buffer, cutting off
** after a given number of chars.  If you change this, just make sure
** that the buffer is large enough for what you will put into it, and
** don't forget to count the terminating null byte!
**
** If there are different rules for different assemblers, you will have
** to deal with that here.
**
** If there is no _lcan handler registered, the default (dflt_lcan) will
** be used.
*/

char* foo42_lcan(char* name)
{
  static char work[10];

  return canonicalize(name, work, 8);
}

/*
** This routine is used to check that a user-specified label string con-
** forms to the syntax for labels for the selected assembler.  Labels can
** have (in all known assemblers) letters and "." in the first position,
** and letters, "." and digits in all but the first.  Check this with a 
** standard helper routine.
*/

bool foo42_lchk(char* name)
{
  return checkstring(name, ".", "0123456789.");
}

/*
** This routine is used to generate labels at specified addresses, from
** the command "SET LABEL <address>", if there is no label given.  In that
** case we make one up.
*/

void foo42_lgen(address* addr)
{
  genlabel(addr);
}

/*
** This routines will be called when a register is created.
*/

void foo42_rcre(regindex index)
{
  UNUSED(index);
}

/*
** This routines will be called when a register is deleted.
*/

void foo42_rdel(regindex index)
{
  UNUSED(index);
}

/*
** This routines will be called when a symbol is created.
*/

void foo42_scre(symindex index)
{
  UNUSED(index);
}

/*
** This routines will be called when a symbol is deleted.  Actually,
** it will be called twice, the first time with symindex == index of
** the symbol we are deleting, just before the actual delete.  The
** second time after the delete is done, this time symindex == 0.
*/

void foo42_sdel(symindex index)
{
  UNUSED(index);
}

/*
** This routines will be called when a symbol is assigned a new value.
*/

void foo42_sset(symindex index)
{
  UNUSED(index);
}

/*
** The _auto handler returns a list of addresses that programs for this
** processor use to start executing.
**
** The return value from this routine is used by the command "do auto".
**
** The foo42 processor always starts executing at address 0, therefore
** we return that address.  We don't have to check for a mapped address,
** since our caller handles this.
*/

address* foo42_auto(void)
{
  return a_zero();		/* Return an address with the value "0". */
}

/*
** The _cchk handler checks if a given character is "printable", i.e. if
** the character can be part of a text constant.
**
** The default handler will accept the range 32-126, inclusive.  If this
** is what you want, just leave out this routine.
*/

bool foo42_cchk(byte b)
{
  if (b < 32) return false;
  if (b >= 127) return false;
  return true;
}

/**********************************************************************/

/*
** This routine will be called when we select this processor.  It is then
** our job to set up whatever we need in our environment, like telling
** the support routines if we are big- or little-endian.
*/

void foo42_init(void)
{
  /* Set up our functions: */
  
  spf_exit(foo42_exit);
  spf_lcan(foo42_lcan);
  spf_lchk(foo42_lchk);
  spf_lgen(foo42_lgen);
  spf_rcre(foo42_rcre);
  spf_rdel(foo42_rdel);
  spf_scre(foo42_scre);
  spf_sdel(foo42_sdel);
  spf_sset(foo42_sset);
  spf_auto(foo42_auto);
  spf_cchk(foo42_cchk);

  /* set up our object handlers: */
  
  spf_dodef(foo42_dobyte);

  spf_doobj(st_inst, foo42_doinstr);
  spf_doobj(st_ptr,  foo42_doptr);
  spf_doobj(st_long, foo42_dolong);
  spf_doobj(st_word, foo42_doword);
  spf_doobj(st_char, foo42_dochar);
  spf_doobj(st_text, foo42_dotext);
  spf_doobj(st_byte, foo42_dobyte);

  spf_begin(foo42_begin);
  spf_end(foo42_end);
  spf_org(foo42_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = false;		/* (We are little-endian) */
  pv_cstart = ";";		/* Comment start string. */
}

/*
** This routine prints a help string for this processor, so that the
** user can ask us what the **** we are.  We should give information on
** what the different assemblers mean and all that jazz.
*/

bool foo42_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
There is no FOO42 processor, this is just an example.\n\
");
    return true;
  }
  return false;
}
