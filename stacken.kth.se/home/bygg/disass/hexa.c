/*
** This module implements driver code for the proposed HEXA processor.
*/

#include "disass.h"

evf_init hexa_init;
evf_help hexa_help;

struct entryvector hexa_vector = {
  "hexa",			/* Name. */
  "hexa",		/* One-liner. */
  hexa_init,			/* Init routine. */
  hexa_help,			/* Help routine. */
};

/************************************************************************/

/* itype values: */

#define unused      0
#define instr       1

/* flags: */

#define IM          0x01	/* Immediate instruction. */

static dispblock hexadisp[256] = {
  { 0x00, 0, unused,  0,   0 },
  { 0x01, 0, instr,   0,   "luuo" },
  { 0x02, 0, instr,   0,   "luuo" },
  { 0x03, 0, instr,   0,   "luuo" },
  { 0x04, 0, instr,   0,   "luuo" },
  { 0x05, 0, instr,   0,   "luuo" },
  { 0x06, 0, instr,   0,   "luuo" },
  { 0x07, 0, instr,   0,   "luuo" },
  { 0x08, 0, instr,   IM,  "arithi" },
  { 0x09, 0, unused,  0,   0 },
  { 0x0A, 0, instr,   IM,  "floati" },
  { 0x0B, 0, unused,  0,   0 },
  { 0x0C, 0, unused,  0,   0 },
  { 0x0D, 0, unused,  0,   0 },
  { 0x0E, 0, unused,  0,   0 },
  { 0x0F, 0, unused,  0,   0 },

  { 0x10, 0, instr,   0,   "jsp" },
  { 0x11, 0, instr,   0,   "jsr" },
  { 0x12, 0, instr,   IM,  "jsys" },
  { 0x13, 0, instr,   0,   "xct" },
  { 0x14, 0, unused,  0,   0 },
  { 0x15, 0, unused,  0,   0 },
  { 0x16, 0, unused,  0,   0 },
  { 0x17, 0, unused,  0,   0 },
  { 0x18, 0, instr,   IM,  "hrli" },
  { 0x19, 0, instr,   0,   "hrl" },
  { 0x1A, 0, instr,   IM,  "hrri" },
  { 0x1B, 0, instr,   0,   "hrr" },
  { 0x1C, 0, instr,   IM,  "hrlzi" },
  { 0x1D, 0, instr,   0,   "hrlz" },
  { 0x1E, 0, instr,   IM,  "hrrzi" },
  { 0x1F, 0, instr,   0,   "hrrz" },

  { 0x20, 0, instr,   IM,  "movei" },
  { 0x21, 0, instr,   0,   "movea" },
  { 0x22, 0, instr,   IM,  "movni" },
  { 0x23, 0, instr,   0,   "exch" },
  { 0x24, 0, instr,   0,   "move" },
  { 0x25, 0, instr,   0,   "move32" },
  { 0x26, 0, instr,   0,   "move16" },
  { 0x27, 0, instr,   0,   "move8" },
  { 0x28, 0, unused,  0,   0 },
  { 0x29, 0, unused,  0,   0 },
  { 0x2A, 0, unused,  0,   0 },
  { 0x2B, 0, unused,  0,   0 },
  { 0x2C, 0, instr,   0,   "movem" },
  { 0x2D, 0, instr,   0,   "mov32m" },
  { 0x2E, 0, instr,   0,   "mov16m" },
  { 0x2F, 0, instr,   0,   "mov8m" },

  { 0x30, 0, instr,   IM,  "pushi" },
  { 0x31, 0, instr,   0,   "pusha" },
  { 0x32, 0, instr,   IM,  "pushm" },
  { 0x33, 0, instr,   0,   "pushj" },
  { 0x34, 0, instr,   0,   "push" },
  { 0x35, 0, instr,   0,   "push32" },
  { 0x36, 0, instr,   0,   "push16" },
  { 0x37, 0, instr,   0,   "push8" },
  { 0x38, 0, unused,  0,   0 },
  { 0x39, 0, unused,  0,   0 },
  { 0x3A, 0, instr,   0,   "popm" },
  { 0x3B, 0, instr,   0,   "popj" },
  { 0x3C, 0, instr,   0,   "pop" },
  { 0x3D, 0, instr,   0,   "pop32" },
  { 0x3E, 0, instr,   0,   "pop16" },
  { 0x3F, 0, instr,   0,   "pop8" },

  { 0x40, 0, instr,   IM,  "cai" },
  { 0x41, 0, instr,   IM,  "cail" },
  { 0x42, 0, instr,   IM,  "caie" },
  { 0x43, 0, instr,   IM,  "caile" },
  { 0x44, 0, instr,   IM,  "caia" },
  { 0x45, 0, instr,   IM,  "caige" },
  { 0x46, 0, instr,   IM,  "cain" },
  { 0x47, 0, instr,   IM,  "caig" },
  { 0x48, 0, instr,   0,   "cam" },
  { 0x49, 0, instr,   0,   "caml" },
  { 0x4A, 0, instr,   0,   "came" },
  { 0x4B, 0, instr,   0,   "camle" },
  { 0x4C, 0, instr,   0,   "cama" },
  { 0x4D, 0, instr,   0,   "camge" },
  { 0x4E, 0, instr,   0,   "camn" },
  { 0x4F, 0, instr,   0,   "camg" },

  { 0x50, 0, instr,   0,   "jump" },
  { 0x51, 0, instr,   0,   "jumpl" },
  { 0x52, 0, instr,   0,   "jumpe" },
  { 0x53, 0, instr,   0,   "jumple" },
  { 0x54, 0, instr,   0,   "jumpa" },
  { 0x55, 0, instr,   0,   "jumpge" },
  { 0x56, 0, instr,   0,   "jumpn" },
  { 0x57, 0, instr,   0,   "jumpg" },
  { 0x58, 0, instr,   0,   "skip" },
  { 0x59, 0, instr,   0,   "skipl" },
  { 0x5A, 0, instr,   0,   "skipe" },
  { 0x5B, 0, instr,   0,   "skiple" },
  { 0x5C, 0, instr,   0,   "skipa" },
  { 0x5D, 0, instr,   0,   "skipge" },
  { 0x5E, 0, instr,   0,   "skipn" },
  { 0x5F, 0, instr,   0,   "skipg" },

  { 0x60, 0, instr,   0,   "aoj" },
  { 0x61, 0, instr,   0,   "aojl" },
  { 0x62, 0, instr,   0,   "aoje" },
  { 0x63, 0, instr,   0,   "aojle" },
  { 0x64, 0, instr,   0,   "aoja" },
  { 0x65, 0, instr,   0,   "aojge" },
  { 0x66, 0, instr,   0,   "aojn" },
  { 0x67, 0, instr,   0,   "aojg" },
  { 0x68, 0, instr,   0,   "soj" },
  { 0x69, 0, instr,   0,   "sojl" },
  { 0x6A, 0, instr,   0,   "soje" },
  { 0x6B, 0, instr,   0,   "sojle" },
  { 0x6C, 0, instr,   0,   "soja" },
  { 0x6D, 0, instr,   0,   "sojge" },
  { 0x6E, 0, instr,   0,   "sojn" },
  { 0x6F, 0, instr,   0,   "sojg" },

  { 0x70, 0, instr,   0,   "aos" },
  { 0x71, 0, instr,   0,   "aosl" },
  { 0x72, 0, instr,   0,   "aose" },
  { 0x73, 0, instr,   0,   "aosle" },
  { 0x74, 0, instr,   0,   "aosa" },
  { 0x75, 0, instr,   0,   "aosge" },
  { 0x76, 0, instr,   0,   "aosn" },
  { 0x77, 0, instr,   0,   "aosg" },
  { 0x78, 0, instr,   0,   "sos" },
  { 0x79, 0, instr,   0,   "sosl" },
  { 0x7A, 0, instr,   0,   "sose" },
  { 0x7B, 0, instr,   0,   "sosle" },
  { 0x7C, 0, instr,   0,   "sosa" },
  { 0x7D, 0, instr,   0,   "sosge" },
  { 0x7E, 0, instr,   0,   "sosn" },
  { 0x7F, 0, instr,   0,   "sosg" },

  { 0x80, 0, instr,   IM,  "addi" },
  { 0x81, 0, instr,   0,   "add" },
  { 0x82, 0, instr,   IM,  "subi" },
  { 0x83, 0, instr,   0,   "sub" },
  { 0x84, 0, instr,   IM,  "addci" },
  { 0x85, 0, instr,   0,   "addc" },
  { 0x86, 0, instr,   IM,  "subci" },
  { 0x87, 0, instr,   0,   "subc" },
  { 0x88, 0, instr,   IM,  "xori" },
  { 0x89, 0, instr,   0,   "xor" },
  { 0x8A, 0, instr,   IM,  "andi" },
  { 0x8B, 0, instr,   0,   "and" },
  { 0x8C, 0, instr,   IM,  "iori" },
  { 0x8D, 0, instr,   0,   "ior" },
  { 0x8E, 0, instr,   IM,  "eqvi" },
  { 0x8F, 0, instr,   0,   "eqv" },

  { 0x90, 0, instr,   IM,  "imuli" },
  { 0x91, 0, instr,   0,   "imul" },
  { 0x92, 0, instr,   IM,  "muli" },
  { 0x93, 0, instr,   0,   "mul" },
  { 0x94, 0, instr,   IM,  "uimuli" },
  { 0x95, 0, instr,   0,   "uimul" },
  { 0x96, 0, instr,   IM,  "umuli" },
  { 0x97, 0, instr,   0,   "umul" },
  { 0x98, 0, instr,   IM,  "idivi" },
  { 0x99, 0, instr,   0,   "idiv" },
  { 0x9A, 0, instr,   IM,  "divi" },
  { 0x9B, 0, instr,   0,   "div" },
  { 0x9C, 0, instr,   IM,  "uidivi" },
  { 0x9D, 0, instr,   0,   "uidiv" },
  { 0x9E, 0, instr,   IM,  "udivi" },
  { 0x9F, 0, instr,   0,   "udiv" },

  { 0xA0, 0, instr,   IM,  "maddi" },
  { 0xA1, 0, instr,   0,   "madd" },
  { 0xA2, 0, instr,   IM,  "msubi" },
  { 0xA3, 0, instr,   0,   "msub" },
  { 0xA4, 0, instr,   IM,  "umaddi" },
  { 0xA5, 0, instr,   0,   "umadd" },
  { 0xA6, 0, instr,   IM,  "umsubi" },
  { 0xA7, 0, instr,   0,   "umsub" },
  { 0xA8, 0, instr,   IM,  "lsh" },
  { 0xA9, 0, instr,   IM,  "lshc" },
  { 0xAA, 0, instr,   IM,  "ash" },
  { 0xAB, 0, instr,   IM,  "ashc" },
  { 0xAC, 0, instr,   IM,  "rot" },
  { 0xAD, 0, instr,   IM,  "rotc" },
  { 0xAE, 0, unused,  0,   0 },
  { 0xAF, 0, unused,  0,   0 },


  { 0xB0, 0, instr,   IM,  "faddi" },
  { 0xB1, 0, instr,   0,   "fadd" },
  { 0xB2, 0, instr,   IM,  "fsubi" },
  { 0xB3, 0, instr,   0,   "fsub" },
  { 0xB4, 0, instr,   IM,  "fmuli" },
  { 0xB5, 0, instr,   0,   "fmul" },
  { 0xB6, 0, instr,   IM,  "fdivi" },
  { 0xB7, 0, instr,   0,   "fdiv" },
  { 0xB8, 0, instr,   IM,  "fmaddi" },
  { 0xB9, 0, instr,   0,   "fmadd" },
  { 0xBA, 0, instr,   IM,  "fmsubi" },
  { 0xBB, 0, instr,   0,   "fmsub" },
  { 0xBC, 0, unused,  0,   0 },
  { 0xBD, 0, unused,  0,   0 },
  { 0xBE, 0, unused,  0,   0 },
  { 0xBF, 0, unused,  0,   0 },

  { 0xC0, 0, instr,   IM,  "trn" },
  { 0xC1, 0, instr,   IM,  "trne" },
  { 0xC2, 0, instr,   IM,  "trna" },
  { 0xC3, 0, instr,   IM,  "trnn" },
  { 0xC4, 0, instr,   IM,  "trz" },
  { 0xC5, 0, instr,   IM,  "trze" },
  { 0xC6, 0, instr,   IM,  "trza" },
  { 0xC7, 0, instr,   IM,  "trzn" },
  { 0xC8, 0, instr,   IM,  "trc" },
  { 0xC9, 0, instr,   IM,  "trce" },
  { 0xCA, 0, instr,   IM,  "trca" },
  { 0xCB, 0, instr,   IM,  "trcn" },
  { 0xCC, 0, instr,   IM,  "tro" },
  { 0xCD, 0, instr,   IM,  "troe" },
  { 0xCE, 0, instr,   IM,  "troa" },
  { 0xCF, 0, instr,   IM,  "tron" },

  { 0xD0, 0, instr,   IM,  "tln" },
  { 0xD1, 0, instr,   IM,  "tlne" },
  { 0xD2, 0, instr,   IM,  "tlna" },
  { 0xD3, 0, instr,   IM,  "tlnn" },
  { 0xD4, 0, instr,   IM,  "tlz" },
  { 0xD5, 0, instr,   IM,  "tlze" },
  { 0xD6, 0, instr,   IM,  "tlza" },
  { 0xD7, 0, instr,   IM,  "tlzn" },
  { 0xD8, 0, instr,   IM,  "tlc" },
  { 0xD9, 0, instr,   IM,  "tlce" },
  { 0xDA, 0, instr,   IM,  "tlca" },
  { 0xDB, 0, instr,   IM,  "tlcn" },
  { 0xDC, 0, instr,   IM,  "tlo" },
  { 0xDD, 0, instr,   IM,  "tloe" },
  { 0xDE, 0, instr,   IM,  "tloa" },
  { 0xDF, 0, instr,   IM,  "tlon" },

  { 0xE0, 0, unused,  0,   0 },
  { 0xE1, 0, unused,  0,   0 },
  { 0xE2, 0, unused,  0,   0 },
  { 0xE3, 0, unused,  0,   0 },
  { 0xE4, 0, unused,  0,   0 },
  { 0xE5, 0, unused,  0,   0 },
  { 0xE6, 0, unused,  0,   0 },
  { 0xE7, 0, unused,  0,   0 },
  { 0xE8, 0, unused,  0,   0 },
  { 0xE9, 0, unused,  0,   0 },
  { 0xEA, 0, unused,  0,   0 },
  { 0xEB, 0, unused,  0,   0 },
  { 0xEC, 0, unused,  0,   0 },
  { 0xED, 0, unused,  0,   0 },
  { 0xEE, 0, unused,  0,   0 },
  { 0xEF, 0, unused,  0,   0 },
  { 0xF0, 0, unused,  0,   0 },
  { 0xF1, 0, unused,  0,   0 },
  { 0xF2, 0, unused,  0,   0 },
  { 0xF3, 0, unused,  0,   0 },
  { 0xF4, 0, unused,  0,   0 },
  { 0xF5, 0, unused,  0,   0 },
  { 0xF6, 0, unused,  0,   0 },
  { 0xF7, 0, unused,  0,   0 },
  { 0xF8, 0, unused,  0,   0 },
  { 0xF9, 0, unused,  0,   0 },
  { 0xFA, 0, unused,  0,   0 },
  { 0xFB, 0, unused,  0,   0 },
  { 0xFC, 0, unused,  0,   0 },
  { 0xFD, 0, unused,  0,   0 },
  { 0xFE, 0, unused,  0,   0 },
  { 0xFF, 0, unused,  0,   0 },
};

/************************************************************************/

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

static void defb(byte b)
{
  pb_length = 1;
  startline(true);
  casestring(".byte");
  tabdelim();
  number(b);
  checkblank();
}

/*
** dobyte() will output the current item as a byte of data.
*/

static void hexa_dobyte(void)
{
  defb(getbyte());
}

/*
** dochar() will try to output its argument as a character.  If it can't,
** it will default to dobyte().
*/

static void hexa_dochar(void)
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

static void hexa_doword(void)
{
  word w;

  w = getword();
  pb_length = 2;
  startline(true);
  casestring(".short");
  tabdelim();
  number(w);
  checkblank();
}

/*
** dolong() will output the current item as a longword (32 bits) of data.
*/

static void hexa_dolong(void)
{
  longword l;

  l = getlong();

  pb_length = 4;
  startline(true);
  casestring(".half");
  tabdelim();
  number(l);
  checkblank();
}

/*
** doptr() tries to output its argument as a pointer, i.e. if there is
** a label that corresponds to the argument, it will be used, otherwise
** we will setter for a numeric value.
*/

static void hexa_doptr(void)
{
  longword l;
  address* a;
  
  pb_length = 4;		/* Pointers are four bytes here. */
  l = getlong();		/* Get first 32, and ignore them. */
  l = getlong();		/* Get bits. */
  a = a_l2a(l);			/* Get corresponding address. */
  startline(true);		/* Start off line. */
  casestring(".word");		/* Suitable pseudo-op. */
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

static void hexa_dotext(void)
{
  int pos;
  char c;
  char* line;

  line = scantext(60);

  if (pb_length <= 1) {
    setpc(istart);
    hexa_dochar();
  } else {
    startline(true);
    casestring(".ascii");
    tabdelim();
    bufchar('"');
    for (pos = 0; pos < pb_length; pos += 1) {
      c = line[pos];
      if (c == '"') {
	bufchar(c);
      }
      bufchar(c);
    }
    bufchar('"');
  }
}

static void hexa_doinstr(void)
{
  int opcode, lbit, num, a, ibit, xsf, x;
  longword y;
  longword l;
  dispblock* disp;

  if (a_a2w(istart) & 3) {	/* Check alignment. */
    defb(getbyte());
    return;
  }

  pb_length = 4;
  l = getlong();

  opcode = (l & 0xff000000) >> 24;
  a =      (l & 0x00f00000) >> 20;
  lbit =    l & 0x00080000;
  num =    (l & 0x0007ff00) >> 8;
  ibit =    l & 0x00000080;
  xsf =    (l & 0x00000070) >> 4;
  x =       l & 0x0000000f;

  disp = &hexadisp[opcode];

  if (disp->itype == unused) {
    defb(opcode);
    return;
  }

  if (lbit) {
    pb_length = 8;
    y = getlong();
  } else {
    y = num;
    num = 0;
  }
  
  startline(true);
  casestring(disp->expand);
  if (num != 0) {
    bufchar(' ');
    bufchar('{');
    number(num);
    bufchar('}');
  }
  tabdelim();
  if (a != 0) {
    number(a);
    argdelim(",");
  }
  if (ibit)
    bufchar('@');

  /*
   * check the IM flag here.
   */
  number(y);

  if (x != 0) {
    bufchar('(');
    if (xsf != 0) {
      bufchar('*');
      bufdecimal(1 << xsf, 1);
      bufchar(' ');
    }
    number(x);
    bufchar(')');
  }
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

void hexa_begin(void)
{
  bufstring(";Beginning of hexa program");
  bufblankline();
  foreach(checkunmap);
  bufblankline();
}

/*
** This routine will be called every time we "move" to a different
** address, i.e. once at the start of each memory segment.  Again,
** this is when generating source.
*/

void hexa_org(address* a)
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

void hexa_end(void)
{
  bufblankline();
  bufstring(";End of hexa program");
}

/************************************************************************/

char* hexa_lcan(char* name)
{
  static char work[10];

  return canonicalize(name, work, 8);
}

bool hexa_lchk(char* name)
{
  return checkstring(name, ".", "0123456789.");
}

void hexa_lgen(address* addr)
{
  genlabel(addr);
}

/**********************************************************************/

/*
** This routine will be called when we select this processor.  It is then
** our job to set up whatever we need in our environment, like telling
** the support routines if we are big- or little-endian.
*/

void hexa_init(void)
{
  /* Set up our functions: */
  
  spf_lcan(hexa_lcan);
  spf_lchk(hexa_lchk);
  spf_lgen(hexa_lgen);

  /* set up our object handlers: */
  
  spf_dodef(hexa_dobyte);

  spf_doobj(st_inst, hexa_doinstr);
  spf_doobj(st_ptr,  hexa_doptr);
  spf_doobj(st_long, hexa_dolong);
  spf_doobj(st_word, hexa_doword);
  spf_doobj(st_char, hexa_dochar);
  spf_doobj(st_text, hexa_dotext);
  spf_doobj(st_byte, hexa_dobyte);

  spf_begin(hexa_begin);
  spf_end(hexa_end);
  spf_org(hexa_org);

  /* set up our variables: */

  pv_bpa = 1;			/* Bytes per Address unit. */
  pv_bpl = 4;			/* Bytes per line. */
  pv_abits = 32;		/* Number of address bits. */
  pv_bigendian = true;		/* (We are little-endian) */
  pv_cstart = ";";		/* Comment start string. */
}

bool hexa_help(int helptype)
{
  if (helptype == hty_general) {
    bufstring("\
Help string for HEXA.\n\
");
    return true;
  }
  return false;
}
