/*
** Power PC?
*/

#include "disass.h"

/*
** Forward declaration of the routines pointed to by the init vector.
*/

evf_init ppc_init;
evf_help ppc_help;

struct entryvector ppc_vector = {
  "ppc",			/* Name. */
  "PowerPC",			/* One-liner. */
  ppc_init,			/* Init routine. */
  ppc_help,			/* Help routine. */
};

/************************************************************************/

static void number(longword l)
{
  switch (radix) {
  case 8:
    if (l > 7)
      bufchar('0');
    bufoctal(l, 1);
    break;
  case 10:
    bufdecimal(l, 1);
    break;
  case 16:
  default:
    if (l > 9) {
      bufstring("0x");
    }
    bufhex(l, 1);
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
** byte data output:
*/

static void ppc_dobyte(void)
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
** char data output:
*/

static void ppc_dochar(void)
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
** word (16-bit) data output:
*/

static void ppc_doword(void)
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
** longword (32-bit) data output:
*/

static void ppc_dolong(void)
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
** pointer output (32-bit assumed):
*/

static void ppc_doptr(void)
{
  longword l;
  address* a;
  
  pb_length = 4;
  l = getlong();
  a = a_l2a(l);
  startline(true);
  casestring(".long");
  tabdelim();
  reference(a);
  if (l_exist(a)) {
    bufstring(l_find(a));
  } else {
    number(l);
  }
  endref();
  checkblank();
}

/*
** dotext() will try to decode a text constant.
*/

static void ppc_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    setpc(istart);
    ppc_dochar();
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

static void ppc_doinstr(void)
{
  ppc_dobyte();			/* FIX THIS */
}

/*
** needs rethinking.
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

void ppc_begin(void)
{
  bufstring(";Beginning of ppc program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

void ppc_org(address* a)
{
  bufblankline();
  casestring("org");
  spacedelim();
  bufstring(a_a2str(a));
  bufblankline();
}

void ppc_end(void)
{
  bufblankline();
  bufstring(";End of ppc program");
}

/************************************************************************/

/*
** This routine is used to check that a user-specified label string con-
** forms to the syntax for labels for the selected assembler.  Labels can
** have letters and "." in the first position, and letters, "." and digits
** in all but the first.  Check this with a standard helper routine.
*/

bool ppc_lchk(char* name)
{
  return checkstring(name, ".", "0123456789.");
}

void ppc_lgen(address* addr)
{
  genlabel(addr);
}

void ppc_init(void)
{
  spf_lchk(ppc_lchk);
  spf_lgen(ppc_lgen);

  /* set up our object handlers: */
  
  spf_dodef(ppc_dobyte);

  spf_doobj(st_inst, ppc_doinstr);
  spf_doobj(st_ptr,  ppc_doptr);
  spf_doobj(st_long, ppc_dolong);
  spf_doobj(st_word, ppc_doword);
  spf_doobj(st_char, ppc_dochar);
  spf_doobj(st_text, ppc_dotext);
  spf_doobj(st_byte, ppc_dobyte);

  spf_begin(ppc_begin);
  spf_end(ppc_end);
  spf_org(ppc_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = true;		/* Is this true? */
  pv_cstart = ";";		/* Comment start string. */
}

/*
** This routine prints a help string for this processor, so that the
** user can ask us what the **** we are.  We should give information on
** what the different assemblers mean and all that jazz.
*/

bool ppc_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Not yet written help text.\n\
");
    return true;
  }
  return false;
}
